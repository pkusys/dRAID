#include "bdev_raid.h"
#include "spdk/config.h"
#include "spdk/env.h"
#include "spdk/thread.h"
#include "spdk/likely.h"
#include "spdk/string.h"
#include "spdk/util.h"
#include "spdk/log.h"

#include "../shared/common.h"

#include <rte_hash.h>
#include <rte_memory.h>
#include <raid.h>

/* The type of chunk request */
enum chunk_request_type {
    CHUNK_READ,
    CHUNK_WRITE,
    CHUNK_WRITE_FORWARD,
    CHUNK_WRITE_FORWARD_DIFF,
    CHUNK_PARITY,
    CHUNK_PARITY_DIFF,
    CHUNK_FORWARD,
    CHUNK_RECON, // if dest chuck is null, 1 return message; if dest chunk exists, 0 return message
    CHUNK_READ_RECON, // if dest chunk is null, 2 return messages; if dest chunk exists, 1 return message
    CHUNK_MIX_DIFF,
    CHUNK_PARITY_MIX
};

struct chunk {
    /* Corresponds to base_bdev index */
    uint8_t index;

    /* Request offset from chunk start */
    uint64_t req_offset;

    /* Request blocks count */
    uint64_t req_blocks;

    /* The iovecs associated with the chunk request */
    struct iovec *iovs;

    /* The number of iovecs */
    int iovcnt;

    /* A single iovec for non-SG buffer request cases */
    struct iovec iov;

    /* The type of chunk request */
    enum chunk_request_type request_type;

    /* number of partial parity to wait for */
    uint8_t fwd_num;

    /* next data chunk to forward to */
    struct chunk *dst_chunk;

    /* target data chunk to reconstruct/generate */
    struct chunk *tgt_chunk;

    /* For retrying base bdev IOs in case submit fails with -ENOMEM */
    struct spdk_bdev_io_wait_entry waitq_entry;
};

struct stripe_request {
    /* The associated raid_bdev_io */
    struct raid_bdev_io *raid_io;

    /* The target stripe */
    struct stripe *stripe;

    /* Counter for remaining chunk requests */
    int remaining;

    /* Status of the request */
    enum spdk_bdev_io_status status;

    /* Function to call when all remaining chunk requests have completed */
    void (*chunk_requests_complete_cb)(struct stripe_request *);

    /* Offset into the parent bdev_io iovecs */
    uint64_t iov_offset;

    /* Initial iov_offset */
    uint64_t init_iov_offset;

    /* First data chunk applicable to this request */
    struct chunk *first_data_chunk;

    /* Last data chunk applicable to this request */
    struct chunk *last_data_chunk;

    /* The stripe's parity chunk */
    struct chunk *parity_chunk;

    /* Degraded chunk */
    struct chunk *degraded_chunk;

    /* Link for the stripe's requests list */
    TAILQ_ENTRY(stripe_request) link;

    /* Array of chunks corresponding to base_bdevs */
    struct chunk chunks[0];
};

struct stripe {
    /* The stripe's index in the raid array. Also a key for the hash table. */
    uint64_t index;

    /* Hashed key value */
    hash_sig_t hash;

    /* List of requests queued for this stripe */
    TAILQ_HEAD(requests_head, stripe_request) requests;

    /* Protects the requests list */
    pthread_spinlock_t requests_lock; // Note: acquire lock when accessing request queue

    /* Stripe can be reclaimed if this reaches 0 */
    unsigned int refs;

    /* Link for the active/free stripes lists */
    TAILQ_ENTRY(stripe) link;

    /* Array of buffers for chunk parity/preread data */
    void **chunk_buffers;
};

TAILQ_HEAD(active_stripes_head, stripe);

struct raid5_info {
    /* The parent raid bdev */
    struct raid_bdev *raid_bdev;

    /* Number of data blocks in a stripe (without parity) */
    uint64_t stripe_blocks;

    /* Number of stripes on this array */
    uint64_t total_stripes;

    /* Mempool for stripe_requests */
    struct spdk_mempool *stripe_request_mempool;

    /* Pointer to an array of all available stripes */
    struct stripe *stripes;

    /* Hash table containing currently active stripes */
    struct rte_hash *active_stripes_hash; // Note: DPDK's implementation of hash table

    /* List of active stripes (in hash table) */
    struct active_stripes_head active_stripes;

    /* List of free stripes (not in hash table) */
    TAILQ_HEAD(, stripe) free_stripes;

    /* Lock protecting the stripes hash and lists */
    pthread_spinlock_t active_stripes_lock;
};

struct raid5_io_channel {
    TAILQ_HEAD(, spdk_bdev_io_wait_entry) retry_queue;
};

#define FOR_EACH_CHUNK(req, c) \
	for (c = req->chunks; \
	     c < req->chunks + req->raid_io->raid_bdev->num_base_rpcs; c++)

#define __NEXT_DATA_CHUNK(req, c) \
	c+1 == req->parity_chunk ? c+2 : c+1 // Note: only works for raid5

#define FOR_EACH_DATA_CHUNK(req, c) \
	for (c = __NEXT_DATA_CHUNK(req, req->chunks-1); \
	     c < req->chunks + req->raid_io->raid_bdev->num_base_rpcs; \
	     c = __NEXT_DATA_CHUNK(req, c))

static inline struct stripe_request *
raid5_chunk_stripe_req(struct chunk *chunk)
{
    return SPDK_CONTAINEROF((chunk - chunk->index), struct stripe_request, chunks);
}

static inline uint8_t
raid5_chunk_data_index(struct chunk *chunk)
{
    // Note: only works for raid5
    return chunk < raid5_chunk_stripe_req(chunk)->parity_chunk ? chunk->index : chunk->index - 1;
}

static inline struct chunk *
raid5_get_data_chunk(struct stripe_request *stripe_req, uint8_t chunk_data_idx)
{
    // Note: only works for raid5
    uint8_t p_chunk_idx = stripe_req->parity_chunk - stripe_req->chunks;

    return &stripe_req->chunks[chunk_data_idx < p_chunk_idx ? chunk_data_idx : chunk_data_idx + 1];
}

static inline uint8_t
raid5_stripe_data_chunks_num(const struct raid_bdev *raid_bdev)
{
    // Note: this also works for raid6
    return raid_bdev->num_base_rpcs - raid_bdev->module->base_rpcs_max_degraded;
}

static bool
raid5_is_full_stripe_write(struct stripe_request *stripe_req)
{
    struct raid_bdev *raid_bdev = stripe_req->raid_io->raid_bdev;
    uint32_t stripe_size = raid_bdev->strip_size;

    struct chunk *chunk;

    FOR_EACH_CHUNK(stripe_req, chunk) {
        if (chunk->req_offset != 0 || chunk->req_blocks != stripe_size) {
            return false;
        }
    }

    return true;
}

// Note: also need q for raid6, also this might not be the most efficient implementation
static void
raid5_xor_buf(void *to, void *from, size_t size)
{
    int ret;
    void *vects[3] = { from, to, to };

    ret = xor_gen(3, size, vects);
    if (ret) {
        SPDK_ERRLOG("xor_gen failed\n");
    }
}

// Note: need q for raid6
static void
raid5_xor_iovs(struct iovec *iovs_dest, int iovs_dest_cnt, size_t iovs_dest_offset,
               const struct iovec *iovs_src, int iovs_src_cnt, size_t iovs_src_offset,
               size_t size)
{
    struct iovec *v1;
    const struct iovec *v2;
    size_t off1, off2;
    size_t n;

    v1 = iovs_dest;
    v2 = iovs_src;

    n = 0;
    off1 = 0;
    while (v1 < iovs_dest + iovs_dest_cnt) {
        n += v1->iov_len;
        if (n > iovs_dest_offset) {
            off1 = v1->iov_len - (n - iovs_dest_offset);
            break;
        }
        v1++;
    }

    n = 0;
    off2 = 0;
    while (v2 < iovs_src + iovs_src_cnt) {
        n += v2->iov_len;
        if (n > iovs_src_offset) {
            off2 = v2->iov_len - (n - iovs_src_offset);
            break;
        }
        v2++;
    }

    while (v1 < iovs_dest + iovs_dest_cnt &&
           v2 < iovs_src + iovs_src_cnt &&
           size > 0) {
        n = spdk_min(v1->iov_len - off1, v2->iov_len - off2);

        if (n > size) {
            n = size;
        }

        size -= n;

        raid5_xor_buf((char*)v1->iov_base + off1, (char*)v2->iov_base + off2, n);

        off1 += n;
        off2 += n;

        if (off1 == v1->iov_len) {
            off1 = 0;
            v1++;
        }

        if (off2 == v2->iov_len) {
            off2 = 0;
            v2++;
        }
    }
}

static int
raid5_chunk_map_iov(struct chunk *chunk, const struct iovec *iov, int iovcnt,
                    uint64_t offset, uint64_t len)
{
    int i;
    size_t off = 0;
    int start_v = -1;
    size_t start_v_off;
    int new_iovcnt = 0;

    // Note: find the start iov
    for (i = 0; i < iovcnt; i++) {
        if (off + iov[i].iov_len > offset) {
            start_v = i;
            break;
        }
        off += iov[i].iov_len;
    }

    if (start_v == -1) {
        return -EINVAL;
    }

    start_v_off = off;

    // Note: find the end iov
    for (i = start_v; i < iovcnt; i++) {
        new_iovcnt++;

        if (off + iov[i].iov_len >= offset + len) {
            break;
        }
        off += iov[i].iov_len;
    }

    assert(start_v + new_iovcnt <= iovcnt);

    // Note: dynamically change size of iovs
    if (new_iovcnt > chunk->iovcnt) {
        void *tmp;

        if (chunk->iovs == &chunk->iov) {
            chunk->iovs = NULL;
        }
        tmp = realloc(chunk->iovs, new_iovcnt * sizeof(struct iovec));
        if (!tmp) {
            return -ENOMEM;
        }
        chunk->iovs = (struct iovec*) tmp;
    }
    chunk->iovcnt = new_iovcnt;

    off = start_v_off;
    iov += start_v;

    // Note: set up iovs from iovs from bdev_io
    for (i = 0; i < new_iovcnt; i++) {
        chunk->iovs[i].iov_base = (char*)iov->iov_base + (offset - off);
        chunk->iovs[i].iov_len = spdk_min(len, iov->iov_len - (offset - off));

        off += iov->iov_len;
        iov++;
        offset += chunk->iovs[i].iov_len;
        len -= chunk->iovs[i].iov_len;
    }

    if (len > 0) {
        return -EINVAL;
    }

    return 0;
}

static int
raid5_chunk_map_req_data(struct chunk *chunk)
{
    struct stripe_request *stripe_req = raid5_chunk_stripe_req(chunk);
    struct spdk_bdev_io *bdev_io = spdk_bdev_io_from_ctx(stripe_req->raid_io);
    uint64_t len = chunk->req_blocks << stripe_req->raid_io->raid_bdev->blocklen_shift;
    int ret;

    ret = raid5_chunk_map_iov(chunk, bdev_io->u.bdev.iovs, bdev_io->u.bdev.iovcnt,
                              stripe_req->iov_offset, len);
    if (ret == 0) {
        stripe_req->iov_offset += len;
    }

    return ret;
}

static void
raid5_io_channel_retry_request(struct raid5_io_channel *r5ch)
{
    struct spdk_bdev_io_wait_entry *waitq_entry;

    waitq_entry = TAILQ_FIRST(&r5ch->retry_queue);
    assert(waitq_entry != NULL);
    TAILQ_REMOVE(&r5ch->retry_queue, waitq_entry, link);
    waitq_entry->cb_fn(waitq_entry->cb_arg);
}

static void
raid5_submit_stripe_request(struct stripe_request *stripe_req);

static void
_raid5_submit_stripe_request(void *_stripe_req)
{
    struct stripe_request *stripe_req = (struct stripe_request*) _stripe_req;

    raid5_submit_stripe_request(stripe_req);
}

static void
raid5_stripe_request_put(struct stripe_request *stripe_req)
{
    struct raid5_info *r5info = (struct raid5_info*) stripe_req->raid_io->raid_bdev->module_private;
    struct chunk *chunk;

    FOR_EACH_CHUNK(stripe_req, chunk) {
        if (chunk->iovs != &chunk->iov) {
            free(chunk->iovs);
        }
    }

    spdk_mempool_put(r5info->stripe_request_mempool, stripe_req);
}

static void
raid5_complete_stripe_request(struct stripe_request *stripe_req)
{
    struct stripe *stripe = stripe_req->stripe;
    struct raid_bdev_io *raid_io = stripe_req->raid_io;
    enum spdk_bdev_io_status status = stripe_req->status;
    struct raid5_io_channel *r5ch = (struct raid5_io_channel*) raid_bdev_io_channel_get_resource(raid_io->raid_ch);
    struct stripe_request *next_req;
    struct chunk *chunk;
    uint64_t req_blocks;

    // Note: if next request available, submit it
    pthread_spin_lock(&stripe->requests_lock);
    next_req = TAILQ_NEXT(stripe_req, link);
    TAILQ_REMOVE(&stripe->requests, stripe_req, link);
    pthread_spin_unlock(&stripe->requests_lock);
    if (next_req) {
        spdk_thread_send_msg(spdk_io_channel_get_thread(spdk_io_channel_from_ctx(
                                     next_req->raid_io->raid_ch)),
                             _raid5_submit_stripe_request, next_req);
    }

    req_blocks = 0;
    FOR_EACH_DATA_CHUNK(stripe_req, chunk) {
        req_blocks += chunk->req_blocks;
    }

    raid5_stripe_request_put(stripe_req);

    if (raid_bdev_io_complete_part(raid_io, req_blocks, status)) {
        __atomic_fetch_sub(&stripe->refs, 1, __ATOMIC_SEQ_CST);

        if (!TAILQ_EMPTY(&r5ch->retry_queue)) {
            raid5_io_channel_retry_request(r5ch);
        }
    }
}

static inline enum spdk_bdev_io_status errno_to_status(int err)
{
    err = abs(err);
    switch (err) {
        case 0:
            return SPDK_BDEV_IO_STATUS_SUCCESS;
        case ENOMEM:
            return SPDK_BDEV_IO_STATUS_NOMEM;
        default:
            return SPDK_BDEV_IO_STATUS_FAILED;
    }
}

static void
raid5_abort_stripe_request(struct stripe_request *stripe_req, enum spdk_bdev_io_status status)
{
    stripe_req->remaining = 0;
    stripe_req->status = status;
    raid5_complete_stripe_request(stripe_req);
}

void
raid5_complete_chunk_request(void *ctx)
{
    struct send_wr_wrapper *send_wrapper = static_cast<struct send_wr_wrapper*>(ctx);
    struct chunk *chunk = static_cast<struct chunk*>(send_wrapper->ctx);
    struct stripe_request *stripe_req = raid5_chunk_stripe_req(chunk);
    struct raid_bdev_io *raid_io = stripe_req->raid_io;
    struct raid_bdev *raid_bdev = raid_io->raid_bdev;
    struct raid5_io_channel *r5ch = (struct raid5_io_channel*) raid_bdev_io_channel_get_resource(raid_io->raid_ch);
    uint32_t blocklen_shift = raid_bdev->blocklen_shift;

    if (--stripe_req->remaining == 0) {
        if (stripe_req->status == SPDK_BDEV_IO_STATUS_SUCCESS) {
            stripe_req->chunk_requests_complete_cb(stripe_req);
        } else {
            raid5_complete_stripe_request(stripe_req);
        }
    }
}

static int
raid5_dispatch_to_rpc(struct raid5_info *r5info, uint8_t chunk_idx, uint64_t base_offset_blocks,
                      uint64_t num_blocks, uint32_t blocklen_shift, struct chunk *ck, uint8_t cs_type)
{
    struct stripe_request *stripe_req = raid5_chunk_stripe_req(ck);
    struct raid_bdev_io *raid_io = stripe_req->raid_io;
    struct raid5_io_channel *r5ch = (struct raid5_io_channel*) raid_bdev_io_channel_get_resource(raid_io->raid_ch);
    struct raid_bdev *raid_bdev = raid_io->raid_bdev;
    struct raid_bdev_io_channel *raid_ch = raid_io->raid_ch;
    struct spdk_mem_map *mem_map = raid_ch->qp_group->qps[chunk_idx]->mem_map;

    struct send_wr_wrapper *send_wrapper = raid_get_send_wrapper(raid_io, chunk_idx);
    struct ibv_send_wr *send_wr = &send_wrapper->send_wr;
    struct ibv_sge *sge = send_wr->sg_list;
    struct cs_message_t *cs_msg = (struct cs_message_t *) sge->addr;
    cs_msg->type = cs_type;
    cs_msg->offset = base_offset_blocks;
    cs_msg->length = num_blocks;
    cs_msg->last_index = raid_bdev->num_base_rpcs;
    cs_msg->num_sge[0] = (uint8_t) ck->iovcnt;
    cs_msg->num_sge[1] = 0;

    struct ibv_mr *mr;
    for (uint8_t i = 0; i < cs_msg->num_sge[0]; i++) {
        cs_msg->sgl[i].addr = (uint64_t) ck->iovs[i].iov_base;
        cs_msg->sgl[i].len = (uint32_t) ck->iovs[i].iov_len;
        mr = (struct ibv_mr *) spdk_mem_map_translate(mem_map, cs_msg->sgl[i].addr, NULL);
        cs_msg->sgl[i].rkey = mr->rkey;
    }
    sge->length = sizeof(struct cs_message_t) - sizeof(struct sg_entry) * (32 - cs_msg->num_sge[0]);
    send_wr->imm_data = kReqTypeRW;
    send_wrapper->rtn_cnt = return_count(cs_msg);
    send_wrapper->cb = raid5_complete_chunk_request;
    send_wrapper->ctx = (void *) ck;

    int ret = raid_dispatch_to_rpc(send_wrapper);

    return ret;
}

static void
_raid5_submit_chunk_request(void *_chunk)
{
    struct chunk *chunk = (struct chunk*) _chunk;
    struct stripe_request *stripe_req = raid5_chunk_stripe_req(chunk);
    struct raid_bdev_io *raid_io = stripe_req->raid_io;
    struct raid_bdev *raid_bdev = raid_io->raid_bdev;
    struct raid5_info *r5info = (struct raid5_info*) raid_bdev->module_private;
    uint32_t blocklen_shift = raid_bdev->blocklen_shift;
    uint64_t offset_blocks = chunk->req_offset;
    uint64_t num_blocks = chunk->req_blocks;
    enum chunk_request_type request_type = chunk->request_type;
    uint64_t base_offset_blocks;
    int ret;

    base_offset_blocks = (stripe_req->stripe->index << raid_bdev->strip_size_shift) + offset_blocks;

    if (request_type == CHUNK_READ) {
        ret = raid5_dispatch_to_rpc(r5info, chunk->index, base_offset_blocks, num_blocks,
                                    blocklen_shift, chunk, CS_RD);
    } else {
        ret = raid5_dispatch_to_rpc(r5info, chunk->index, base_offset_blocks, num_blocks,
                                    blocklen_shift, chunk, CS_WT);
    }

    if (spdk_unlikely(ret != 0)) {
        SPDK_ERRLOG("bdev io submit error not due to ENOMEM, it should not happen\n");
        assert(false);
    }
}

static void
raid5_submit_chunk_request(struct chunk *chunk, enum chunk_request_type type)
{
    struct stripe_request *stripe_req = raid5_chunk_stripe_req(chunk);

    stripe_req->remaining++;

    chunk->request_type = type;

    _raid5_submit_chunk_request(chunk);
}


static void
raid5_stripe_write_submit(struct stripe_request *stripe_req)
{
    struct chunk *chunk;
    struct chunk *d_chunk = stripe_req->degraded_chunk;

    stripe_req->chunk_requests_complete_cb = raid5_complete_stripe_request;

    FOR_EACH_CHUNK(stripe_req, chunk) {
        if (chunk != d_chunk && chunk->req_blocks > 0) {
            raid5_submit_chunk_request(chunk, CHUNK_WRITE);
        }
    }
}

// Note: full stripe write
static void
raid5_stripe_write_full_stripe(struct stripe_request *stripe_req)
{
    struct chunk *chunk;
    struct chunk *p_chunk = stripe_req->parity_chunk;
    uint32_t blocklen_shift = stripe_req->raid_io->raid_bdev->blocklen_shift;

    raid_memset_iovs(p_chunk->iovs, p_chunk->iovcnt, 0);

    FOR_EACH_DATA_CHUNK(stripe_req, chunk) {
        raid5_xor_iovs(p_chunk->iovs, p_chunk->iovcnt,
                       (chunk->req_offset - p_chunk->req_offset) << blocklen_shift,
                       chunk->iovs, chunk->iovcnt, 0,
                       chunk->req_blocks << blocklen_shift);
    }

    raid5_stripe_write_submit(stripe_req);
}

void
raid5_complete_chunk_request_d_raid(void *ctx)
{
    struct send_wr_wrapper *send_wrapper = static_cast<struct send_wr_wrapper*>(ctx);
    struct chunk *chunk = static_cast<struct chunk*>(send_wrapper->ctx);
    struct stripe_request *stripe_req = raid5_chunk_stripe_req(chunk);
    struct raid_bdev_io *raid_io = stripe_req->raid_io;
    struct raid_bdev *raid_bdev = raid_io->raid_bdev;
    struct raid5_io_channel *r5ch = (struct raid5_io_channel*) raid_bdev_io_channel_get_resource(raid_io->raid_ch);
    uint32_t blocklen_shift = raid_bdev->blocklen_shift;
    struct chunk *t_chunk = chunk->tgt_chunk;

    if (--stripe_req->remaining == 0) {
        if (stripe_req->status == SPDK_BDEV_IO_STATUS_SUCCESS) {
            stripe_req->chunk_requests_complete_cb(stripe_req);
        } else {
            raid5_complete_stripe_request(stripe_req);
        }
    }
}

static int
raid5_dispatch_to_rpc_d_raid(struct raid5_info *r5info, uint8_t chunk_idx,
                             uint64_t base_offset_blocks, uint64_t num_blocks, uint32_t blocklen_shift, struct chunk *ck,
                             uint64_t t_base_offset_blocks, uint64_t t_num_blocks, int8_t dst_index,
                             uint8_t fwd_num, uint8_t msg_type, uint8_t cs_type)
{
    struct stripe_request *stripe_req = raid5_chunk_stripe_req(ck);
    struct raid_bdev_io *raid_io = stripe_req->raid_io;
    struct raid5_io_channel *r5ch = (struct raid5_io_channel*) raid_bdev_io_channel_get_resource(raid_io->raid_ch);
    struct raid_bdev *raid_bdev = raid_io->raid_bdev;
    struct raid_bdev_io_channel *raid_ch = raid_io->raid_ch;
    struct spdk_mem_map *mem_map = raid_ch->qp_group->qps[chunk_idx]->mem_map;

    struct send_wr_wrapper *send_wrapper = raid_get_send_wrapper(raid_io, chunk_idx);
    struct ibv_send_wr *send_wr = &send_wrapper->send_wr;
    struct ibv_sge *sge = send_wr->sg_list;
    struct cs_message_t *cs_msg = (struct cs_message_t *) sge->addr;
    cs_msg->type = cs_type;
    cs_msg->offset = base_offset_blocks;
    cs_msg->length = num_blocks;
    cs_msg->last_index = raid_bdev->num_base_rpcs;
    cs_msg->fwd_offset = t_base_offset_blocks;
    cs_msg->fwd_length = t_num_blocks;
    cs_msg->next_index = dst_index;
    cs_msg->fwd_num = fwd_num;

    struct ibv_mr *mr;
    if (cs_type == FWD_RW || cs_type == FWD_RW_DIFF || cs_type == FWD_MIX || cs_type == PR_MIX) {
        cs_msg->num_sge[0] = (uint8_t) ck->iovcnt;
        cs_msg->num_sge[1] = 0;
    } else if (cs_type == FWD_RO || cs_type == PR_NO_DIFF || cs_type == PR_DIFF) {
        cs_msg->num_sge[0] = 0;
        cs_msg->num_sge[1] = 0;
    } else if (cs_type == RECON_RT) {
        cs_msg->num_sge[0] = (uint8_t) ck->iovcnt;
        if (dst_index == -1) {
            cs_msg->num_sge[1] = ck->tgt_chunk->iovcnt;
        } else {
            cs_msg->num_sge[1] = 0;
        }
    } else if (cs_type == RECON_NRT) {
        cs_msg->num_sge[0] = 0;
        if (dst_index == -1) {
            cs_msg->num_sge[1] = ck->tgt_chunk->iovcnt;
        } else {
            cs_msg->num_sge[1] = 0;
        }
    }
    for (uint8_t i = 0; i < cs_msg->num_sge[0]; i++) {
        cs_msg->sgl[i].addr = (uint64_t) ck->iovs[i].iov_base;
        cs_msg->sgl[i].len = (uint32_t) ck->iovs[i].iov_len;
        mr = (struct ibv_mr *) spdk_mem_map_translate(mem_map, cs_msg->sgl[i].addr, NULL);
        cs_msg->sgl[i].rkey = mr->rkey;
    }
    for (uint8_t i = 0; i < cs_msg->num_sge[1]; i++) {
        cs_msg->sgl[cs_msg->num_sge[0]+i].addr = (uint64_t) ck->tgt_chunk->iovs[i].iov_base;
        cs_msg->sgl[cs_msg->num_sge[0]+i].len = (uint32_t) ck->tgt_chunk->iovs[i].iov_len;
        mr = (struct ibv_mr *) spdk_mem_map_translate(mem_map, cs_msg->sgl[cs_msg->num_sge[0]+i].addr, NULL);
        cs_msg->sgl[cs_msg->num_sge[0]+i].rkey = mr->rkey;
    }

    sge->length = sizeof(struct cs_message_t) - sizeof(struct sg_entry) * (32 - cs_msg->num_sge[0] - cs_msg->num_sge[1]);
    send_wr->imm_data = msg_type;
    send_wrapper->rtn_cnt = return_count(cs_msg);
    send_wrapper->cb = raid5_complete_chunk_request_d_raid;
    send_wrapper->ctx = (void *) ck;

    int ret = raid_dispatch_to_rpc(send_wrapper);

    return ret;
}

static void
_raid5_submit_chunk_request_d_raid(void *_chunk)
{
    struct chunk *chunk = (struct chunk*) _chunk;
    struct chunk *dst_chunk = chunk->dst_chunk;
    struct chunk *tgt_chunk = chunk->tgt_chunk;
    struct stripe_request *stripe_req = raid5_chunk_stripe_req(chunk);
    struct raid_bdev_io *raid_io = stripe_req->raid_io;
    struct raid_bdev *raid_bdev = raid_io->raid_bdev;
    struct raid5_info *r5info = (struct raid5_info*) raid_bdev->module_private;
    uint32_t blocklen_shift = raid_bdev->blocklen_shift;
    uint64_t offset_blocks = chunk->req_offset;
    uint64_t num_blocks = chunk->req_blocks;
    uint64_t t_offset_blocks = tgt_chunk->req_offset;
    uint64_t t_num_blocks = tgt_chunk->req_blocks;
    uint64_t base_offset_blocks, t_base_offset_blocks;
    enum chunk_request_type request_type = chunk->request_type;
    int ret;
    uint8_t msg_type, cs_type;

    t_base_offset_blocks = (stripe_req->stripe->index << raid_bdev->strip_size_shift) + t_offset_blocks;
    base_offset_blocks = 0; // calculate when necessary

    if (request_type == CHUNK_WRITE_FORWARD) {
        base_offset_blocks = (stripe_req->stripe->index << raid_bdev->strip_size_shift) + offset_blocks;
        msg_type = kReqTypePartialWrite;
        cs_type = FWD_RW;
    } else if (request_type == CHUNK_WRITE_FORWARD_DIFF) {
        base_offset_blocks = (stripe_req->stripe->index << raid_bdev->strip_size_shift) + offset_blocks;
        msg_type = kReqTypePartialWrite;
        cs_type = FWD_RW_DIFF;
    } else if (request_type == CHUNK_FORWARD) {
        offset_blocks = dst_chunk->req_offset;
        num_blocks = dst_chunk->req_blocks;
        base_offset_blocks = (stripe_req->stripe->index << raid_bdev->strip_size_shift) + offset_blocks;
        msg_type = kReqTypePartialWrite;
        cs_type = FWD_RO;
    } else if (request_type == CHUNK_PARITY) {
        base_offset_blocks = t_base_offset_blocks;
        msg_type = kReqTypeParity;
        cs_type = PR_NO_DIFF;
    } else if (request_type == CHUNK_PARITY_DIFF) {
        base_offset_blocks = t_base_offset_blocks;
        msg_type = kReqTypeParity;
        cs_type = PR_DIFF;
    } else if (request_type == CHUNK_READ_RECON) {
        base_offset_blocks = (stripe_req->stripe->index << raid_bdev->strip_size_shift) + offset_blocks;
        msg_type = kReqTypeReconRead;
        cs_type = RECON_RT;
    } else if (request_type == CHUNK_RECON) {
        msg_type = kReqTypeReconRead;
        cs_type = RECON_NRT;
    } else if (request_type == CHUNK_MIX_DIFF) {
        base_offset_blocks = (stripe_req->stripe->index << raid_bdev->strip_size_shift) + offset_blocks;
        msg_type = kReqTypePartialWrite;
        cs_type = FWD_MIX;
    } else if (request_type == CHUNK_PARITY_MIX) {
        base_offset_blocks = (stripe_req->stripe->index << raid_bdev->strip_size_shift) + offset_blocks;
        msg_type = kReqTypeParity;
        cs_type = PR_MIX;
    } else {
        assert(false);
    }

    ret = raid5_dispatch_to_rpc_d_raid(r5info, chunk->index, base_offset_blocks, num_blocks, blocklen_shift, chunk,
                                       t_base_offset_blocks, t_num_blocks, dst_chunk == NULL? -1 : (int8_t) dst_chunk->index,
                                       chunk->fwd_num, msg_type, cs_type);

    if (spdk_unlikely(ret != 0)) {
        SPDK_ERRLOG("bdev io submit error not due to ENOMEM, it should not happen\n");
        assert(false);
    }
}

static void
raid5_submit_chunk_request_d_raid(struct chunk *chunk, enum chunk_request_type type) {
    struct stripe_request *stripe_req = raid5_chunk_stripe_req(chunk);

    stripe_req->remaining++;

    chunk->request_type = type;

    _raid5_submit_chunk_request_d_raid(chunk);
}

static void
raid5_stripe_write_d_rmw(struct stripe_request *stripe_req)
{
    struct chunk *chunk;
    struct chunk *p_chunk = stripe_req->parity_chunk;
    uint8_t fwd_num = 0;

    stripe_req->chunk_requests_complete_cb = raid5_complete_stripe_request;

    FOR_EACH_DATA_CHUNK(stripe_req, chunk) {
        chunk->dst_chunk = p_chunk;
        chunk->tgt_chunk = p_chunk;
        if (chunk->req_blocks > 0) {
            fwd_num++;
            raid5_submit_chunk_request_d_raid(chunk, CHUNK_WRITE_FORWARD_DIFF);
        }
    }

    p_chunk->fwd_num = fwd_num;
    p_chunk->dst_chunk = NULL;
    p_chunk->tgt_chunk = p_chunk;
    raid5_submit_chunk_request_d_raid(p_chunk, CHUNK_PARITY_DIFF);
}

static void
raid5_stripe_write_d_rcrmw(struct stripe_request *stripe_req)
{
    struct chunk *chunk;
    struct chunk *p_chunk = stripe_req->parity_chunk;
    uint8_t fwd_num = 0;
    stripe_req->chunk_requests_complete_cb = raid5_complete_stripe_request;
    struct chunk *d_chunk = stripe_req->degraded_chunk;

    std::swap(p_chunk->iov, d_chunk->iov);
    std::swap(p_chunk->iovs, d_chunk->iovs);
    if (p_chunk->iovs == &d_chunk->iov) {
        p_chunk->iovs = &p_chunk->iov;
    }
    if (d_chunk->iovs == &p_chunk->iov) {
        d_chunk->iovs = &d_chunk->iov;
    }
    std::swap(p_chunk->iovcnt, d_chunk->iovcnt);

    FOR_EACH_DATA_CHUNK(stripe_req, chunk) {
        if (chunk == stripe_req->degraded_chunk) {
            continue;
        }
        chunk->dst_chunk = p_chunk;
        chunk->tgt_chunk = d_chunk;
        if (chunk->req_blocks > 0) {
            fwd_num++;
            raid5_submit_chunk_request_d_raid(chunk, CHUNK_MIX_DIFF);
        } else {
            fwd_num++;
            raid5_submit_chunk_request_d_raid(chunk, CHUNK_FORWARD);
        }
    }

    p_chunk->fwd_num = fwd_num;
    p_chunk->dst_chunk = NULL;
    p_chunk->tgt_chunk = d_chunk;
    raid5_submit_chunk_request_d_raid(p_chunk, CHUNK_PARITY_MIX);
}

static void
raid5_stripe_write_d_reconstructed_write(struct stripe_request *stripe_req)
{
    struct chunk *chunk;
    struct chunk *p_chunk = stripe_req->parity_chunk;
    uint8_t fwd_num = raid5_stripe_data_chunks_num(stripe_req->raid_io->raid_bdev);

    stripe_req->chunk_requests_complete_cb = raid5_complete_stripe_request;

    FOR_EACH_DATA_CHUNK(stripe_req, chunk) {
        chunk->dst_chunk = p_chunk;
        chunk->tgt_chunk = p_chunk;
        if (chunk->req_blocks > 0) {
            raid5_submit_chunk_request_d_raid(chunk, CHUNK_WRITE_FORWARD);
        } else {
            raid5_submit_chunk_request_d_raid(chunk, CHUNK_FORWARD);
        }
    }

    p_chunk->fwd_num = fwd_num;
    p_chunk->dst_chunk = NULL;
    p_chunk->tgt_chunk = p_chunk;
    raid5_submit_chunk_request_d_raid(p_chunk, CHUNK_PARITY);
}

static void
raid5_stripe_write(struct stripe_request *stripe_req)
{
    struct raid_bdev *raid_bdev = stripe_req->raid_io->raid_bdev;
    struct chunk *p_chunk = stripe_req->parity_chunk;
    struct chunk *chunk;
    int preread_balance = 0;
    struct chunk *d_chunk = stripe_req->degraded_chunk = NULL;
    struct raid_base_rpc_info *base_info;
    uint8_t total_degraded = 0;

    // Note: Count degraded device and record last visited d_chunk.
    FOR_EACH_CHUNK(stripe_req, chunk) {
        base_info = &raid_bdev->base_rpc_info[chunk->index];
        if (base_info->degraded) {
            total_degraded++;
            d_chunk = stripe_req->degraded_chunk = chunk;
        }
    }

    if (d_chunk && total_degraded > raid_bdev->module->base_rpcs_max_degraded) {
        raid5_abort_stripe_request(stripe_req, SPDK_BDEV_IO_STATUS_FAILED);
        return;
    }

    // Note: single chunk -> partial parity update; > 1 chunk -> full parity update
    if (stripe_req->first_data_chunk == stripe_req->last_data_chunk) {
        p_chunk->req_offset = stripe_req->first_data_chunk->req_offset;
        p_chunk->req_blocks = stripe_req->first_data_chunk->req_blocks;
    } else {
        p_chunk->req_offset = 0;
        p_chunk->req_blocks = raid_bdev->strip_size;
    }

    p_chunk->iov.iov_base = stripe_req->stripe->chunk_buffers[0]; // for full stripe write
    p_chunk->iov.iov_len = p_chunk->req_blocks << raid_bdev->blocklen_shift;

    // Note: This is the vote process
    // 1. req_blocks == 0: no update, favors rmw
    // 2. 0 < req_blocks < stripe_size: partial update, no preference
    // 3. req_blocks == stripe_size: full update, favors reconstruction write
    FOR_EACH_DATA_CHUNK(stripe_req, chunk) {
        if (chunk->req_blocks < p_chunk->req_blocks) {
            preread_balance++;
        }

        if (chunk->req_blocks > 0) {
            preread_balance--;
        }
    }

    if (raid5_is_full_stripe_write(stripe_req) || d_chunk == p_chunk) {
        // Full write or p_chunk down, just skip the offline device.
        raid5_stripe_write_full_stripe(stripe_req);
        return;
    }

    if (d_chunk == NULL && preread_balance <= 0) {
        // reconstruct if no chunk degraded.
        raid5_stripe_write_d_reconstructed_write(stripe_req);
        return;
    }

    if (d_chunk == NULL || d_chunk->req_blocks == 0) {
        // rmw without d_chunk.
        raid5_stripe_write_d_rmw(stripe_req);
        return;
    }

    // write on d_chunk.
    raid5_stripe_write_d_rcrmw(stripe_req);
}

static void
raid5_stripe_read_normal(struct stripe_request *stripe_req)
{
    struct chunk *chunk;

    FOR_EACH_DATA_CHUNK(stripe_req, chunk) {
        if (chunk->req_blocks > 0) {
            raid5_submit_chunk_request(chunk, CHUNK_READ);
        }
    }
}

static void
raid5_compute_reconstruction_route(struct stripe_request *stripe_req)
{
    struct raid_bdev_io *raid_io = stripe_req->raid_io;
    struct raid_bdev *raid_bdev = raid_io->raid_bdev;
    struct chunk *chunk;
    struct chunk *parent_chunk = NULL;
    struct chunk * d_chunk = stripe_req->degraded_chunk;
    uint8_t avail_devs = raid_bdev->num_base_rpcs;
    uint8_t leaves = avail_devs - 2;
    uint8_t i, parent_idx;

    FOR_EACH_CHUNK(stripe_req, chunk) {
        chunk->tgt_chunk = d_chunk;
        chunk->dst_chunk = NULL;
        chunk->fwd_num = 0;
    }

    uint8_t root_idx = rte_rand_max(avail_devs);
    if (d_chunk->index == root_idx) {
        root_idx = (root_idx + avail_devs - 1) % avail_devs;
    }
//    uint8_t root_idx;
//    if (rte_rand_max(8) < 7) {
//        root_idx = rte_rand_max(3);
//    } else {
//        root_idx = rte_rand_max(7);
//    }
    uint8_t stop_idx = root_idx + raid_bdev->num_base_rpcs;
    for (i = root_idx; i < stop_idx; i++) {
        chunk = &stripe_req->chunks[i % avail_devs];
        if (chunk == d_chunk) {
            continue;
        }
        if (parent_chunk == NULL) {
            parent_chunk = chunk;
            parent_idx = i % avail_devs;
        } else {
            chunk->dst_chunk = parent_chunk;
            parent_chunk->fwd_num++;
        }
        if (parent_chunk->fwd_num == leaves) {
            parent_idx++;
            parent_chunk = &stripe_req->chunks[parent_idx % avail_devs];
            if (parent_chunk == d_chunk) {
                parent_idx++;
                parent_chunk = &stripe_req->chunks[parent_idx % avail_devs];
            }
        }
    }

}

static void
raid5_stripe_read_d_reconstructed_read(struct stripe_request *stripe_req)
{
    struct chunk *chunk;
    struct chunk * d_chunk = stripe_req->degraded_chunk;
    int ret;

    raid5_compute_reconstruction_route(stripe_req);

    FOR_EACH_CHUNK(stripe_req, chunk) {
        if (chunk != d_chunk) {
            if (chunk->req_blocks > 0) {
                raid5_submit_chunk_request_d_raid(chunk, CHUNK_READ_RECON);
            } else {
                raid5_submit_chunk_request_d_raid(chunk, CHUNK_RECON);
            }
        }
    }
}

static void
raid5_stripe_read(struct stripe_request *stripe_req)
{
    struct chunk *chunk;
    struct raid_bdev_io *raid_io = stripe_req->raid_io;
    struct raid_bdev *raid_bdev = raid_io->raid_bdev;
    struct raid_base_rpc_info *base_info;
    struct chunk *d_chunk = stripe_req->degraded_chunk = NULL;
    uint8_t total_degraded = 0;

    // Note: check if any degraded device is involved
    FOR_EACH_CHUNK(stripe_req, chunk) {
        base_info = &raid_bdev->base_rpc_info[chunk->index];
        if (base_info->degraded) {
            total_degraded++;
            if (chunk->req_blocks > 0) {
                d_chunk = stripe_req->degraded_chunk = chunk;
            }
        }
    }

    if (d_chunk && total_degraded > raid_bdev->module->base_rpcs_max_degraded) {
        raid5_abort_stripe_request(stripe_req, SPDK_BDEV_IO_STATUS_FAILED);
        return;
    }

    stripe_req->chunk_requests_complete_cb = raid5_complete_stripe_request;

    if (d_chunk) {
        raid5_stripe_read_d_reconstructed_read(stripe_req);
    } else {
        raid5_stripe_read_normal(stripe_req);
    }
}

static void
raid5_submit_stripe_request(struct stripe_request *stripe_req)
{
    struct spdk_bdev_io *bdev_io = spdk_bdev_io_from_ctx(stripe_req->raid_io);
    struct chunk *chunk;
    int ret;

    FOR_EACH_DATA_CHUNK(stripe_req, chunk) {
        if (chunk->req_blocks > 0) {
            ret = raid5_chunk_map_req_data(chunk);
            if (ret) {
                raid5_abort_stripe_request(stripe_req, errno_to_status(ret));
                return;
            }
        }
    }

    if (bdev_io->type == SPDK_BDEV_IO_TYPE_READ) {
        raid5_stripe_read(stripe_req);
    } else if (bdev_io->type == SPDK_BDEV_IO_TYPE_WRITE) {
        raid5_stripe_write(stripe_req);
    } else {
        assert(false);
    }
}

static void
raid5_handle_stripe(struct raid_bdev_io *raid_io, struct stripe *stripe,
                    uint64_t stripe_offset, uint64_t blocks, uint64_t iov_offset)
{
    struct spdk_bdev_io *bdev_io = spdk_bdev_io_from_ctx(raid_io);
    struct raid_bdev *raid_bdev = raid_io->raid_bdev;
    struct raid5_info *r5info = (struct raid5_info*) raid_bdev->module_private;
    struct stripe_request *stripe_req;
    struct chunk *chunk;
    uint64_t stripe_offset_from, stripe_offset_to;
    uint8_t first_chunk_data_idx, last_chunk_data_idx;
    bool do_submit;

    if (raid_io->base_bdev_io_remaining == blocks &&
        bdev_io->type == SPDK_BDEV_IO_TYPE_WRITE &&
        blocks < raid_bdev->strip_size) {
        /*
         * Split in 2 smaller requests if this request would require
         * a non-contiguous parity chunk update
         */
        uint64_t blocks_limit = raid_bdev->strip_size -
                                (stripe_offset % raid_bdev->strip_size);
        if (blocks > blocks_limit) {
            raid5_handle_stripe(raid_io, stripe, stripe_offset,
                                blocks_limit, iov_offset);
            blocks -= blocks_limit;
            stripe_offset += blocks_limit;
            iov_offset += blocks_limit << raid_bdev->blocklen_shift;
        }
    }

    stripe_req = (struct stripe_request*) spdk_mempool_get(r5info->stripe_request_mempool);
    if (spdk_unlikely(!stripe_req)) {
        raid_bdev_io_complete_part(raid_io, blocks, SPDK_BDEV_IO_STATUS_NOMEM);
        return;
    }

    stripe_req->raid_io = raid_io;
    stripe_req->iov_offset = iov_offset;
    stripe_req->init_iov_offset = iov_offset;
    stripe_req->status = SPDK_BDEV_IO_STATUS_SUCCESS;
    stripe_req->remaining = 0;

    stripe_req->stripe = stripe;
    stripe_req->parity_chunk = &stripe_req->chunks[raid5_stripe_data_chunks_num(
            raid_bdev) - stripe->index % raid_bdev->num_base_rpcs];

    stripe_offset_from = stripe_offset;
    stripe_offset_to = stripe_offset_from + blocks;
    first_chunk_data_idx = stripe_offset_from >> raid_bdev->strip_size_shift;
    last_chunk_data_idx = (stripe_offset_to - 1) >> raid_bdev->strip_size_shift;

    stripe_req->first_data_chunk = raid5_get_data_chunk(stripe_req, first_chunk_data_idx);
    stripe_req->last_data_chunk = raid5_get_data_chunk(stripe_req, last_chunk_data_idx);

    FOR_EACH_CHUNK(stripe_req, chunk) {
        chunk->index = chunk - stripe_req->chunks;
        chunk->iovs = &chunk->iov;
        chunk->iovcnt = 1;

        if (chunk == stripe_req->parity_chunk ||
            chunk < stripe_req->first_data_chunk ||
            chunk > stripe_req->last_data_chunk) {
            chunk->req_offset = 0;
            chunk->req_blocks = 0;
        } else {
            uint64_t chunk_offset_from = raid5_chunk_data_index(chunk) << raid_bdev->strip_size_shift;
            uint64_t chunk_offset_to = chunk_offset_from + raid_bdev->strip_size;

            if (stripe_offset_from > chunk_offset_from) {
                chunk->req_offset = stripe_offset_from - chunk_offset_from;
            } else {
                chunk->req_offset = 0;
            }

            if (stripe_offset_to < chunk_offset_to) {
                chunk->req_blocks = stripe_offset_to - chunk_offset_from;
            } else {
                chunk->req_blocks = raid_bdev->strip_size;
            }

            chunk->req_blocks -= chunk->req_offset;
        }
    }

    pthread_spin_lock(&stripe->requests_lock);
    do_submit = TAILQ_EMPTY(&stripe->requests);
    TAILQ_INSERT_TAIL(&stripe->requests, stripe_req, link);
    pthread_spin_unlock(&stripe->requests_lock);

    if (do_submit) {
        raid5_submit_stripe_request(stripe_req);
    }
}

static int
raid5_reclaim_stripes(struct raid5_info *r5info)
{
    struct stripe *stripe, *tmp;
    int reclaimed = 0;
    int ret;
    int to_reclaim = (RAID_MAX_STRIPES >> 3) - RAID_MAX_STRIPES +
                     rte_hash_count(r5info->active_stripes_hash);

    TAILQ_FOREACH_REVERSE_SAFE(stripe, &r5info->active_stripes, active_stripes_head, link, tmp) {
        if (__atomic_load_n(&stripe->refs, __ATOMIC_SEQ_CST) > 0) {
            continue;
        }

        TAILQ_REMOVE(&r5info->active_stripes, stripe, link);
        TAILQ_INSERT_TAIL(&r5info->free_stripes, stripe, link);

        ret = rte_hash_del_key_with_hash(r5info->active_stripes_hash,
                                         &stripe->index, stripe->hash);
        if (spdk_unlikely(ret < 0)) {
            assert(false);
        }

        if (++reclaimed > to_reclaim) {
            break;
        }
    }

    return reclaimed;
}

static struct stripe *
raid5_get_stripe(struct raid5_info *r5info, uint64_t stripe_index)
{
    struct stripe *stripe;
    hash_sig_t hash;
    int ret;

    hash = rte_hash_hash(r5info->active_stripes_hash, &stripe_index);

    pthread_spin_lock(&r5info->active_stripes_lock);
    ret = rte_hash_lookup_with_hash_data(r5info->active_stripes_hash,
                                         &stripe_index, hash, (void **)&stripe);
    if (ret == -ENOENT) {
        stripe = TAILQ_FIRST(&r5info->free_stripes);
        if (!stripe) {
            if (raid5_reclaim_stripes(r5info) > 0) {
                stripe = TAILQ_FIRST(&r5info->free_stripes);
                assert(stripe != NULL);
            } else {
                pthread_spin_unlock(&r5info->active_stripes_lock);
                return NULL;
            }
        }
        TAILQ_REMOVE(&r5info->free_stripes, stripe, link);

        stripe->index = stripe_index;
        stripe->hash = hash;

        ret = rte_hash_add_key_with_hash_data(r5info->active_stripes_hash,
                                              &stripe_index, hash, stripe);
        if (spdk_unlikely(ret < 0)) {
            assert(false);
        }
    } else {
        TAILQ_REMOVE(&r5info->active_stripes, stripe, link);
    }
    TAILQ_INSERT_HEAD(&r5info->active_stripes, stripe, link);

    __atomic_fetch_add(&stripe->refs, 1, __ATOMIC_SEQ_CST);

    pthread_spin_unlock(&r5info->active_stripes_lock);

    return stripe;
}

static void
raid5_submit_rw_request(struct raid_bdev_io *raid_io);

static void
_raid5_submit_rw_request(void *_raid_io)
{
    struct raid_bdev_io *raid_io = (struct raid_bdev_io*) _raid_io;

    raid5_submit_rw_request(raid_io);
}

void
raid5_complete_chunk_request_read(void *ctx)
{
    struct send_wr_wrapper *send_wrapper = static_cast<struct send_wr_wrapper*>(ctx);
    struct raid_bdev_io *raid_io = (struct raid_bdev_io *) send_wrapper->ctx;

    raid_bdev_io_complete_part(raid_io, send_wrapper->num_blocks, SPDK_BDEV_IO_STATUS_SUCCESS);
}

static int
raid5_map_iov(struct sg_entry *sgl, const struct iovec *iov, int iovcnt,
              uint64_t offset, uint64_t len)
{
    int i;
    size_t off = 0;
    int start_v = -1;
    size_t start_v_off;
    int new_iovcnt = 0;
    int ret;

    // Note: find the start iov
    for (i = 0; i < iovcnt; i++) {
        if (off + iov[i].iov_len > offset) {
            start_v = i;
            break;
        }
        off += iov[i].iov_len;
    }

    if (start_v == -1) {
        return -1;
    }

    start_v_off = off;

    // Note: find the end iov
    for (i = start_v; i < iovcnt; i++) {
        new_iovcnt++;

        if (off + iov[i].iov_len >= offset + len) {
            break;
        }
        off += iov[i].iov_len;
    }

    assert(start_v + new_iovcnt <= iovcnt);

    ret = new_iovcnt;

    off = start_v_off;
    iov += start_v;

    // Note: set up iovs from iovs from bdev_io
    for (i = 0; i < new_iovcnt; i++) {
        sgl[i].addr = (uint64_t) iov->iov_base + (offset - off);
        sgl[i].len = spdk_min(len, iov->iov_len - (offset - off));

        off += iov->iov_len;
        iov++;
        offset += sgl[i].len;
        len -= sgl[i].len;
    }

    if (len > 0) {
        return -1;
    }

    return ret;
}

static int
raid5_dispatch_to_rpc_read(struct raid_bdev_io *raid_io, uint8_t chunk_idx, uint64_t base_offset_blocks,
                           uint64_t num_blocks, uint64_t chunk_req_blocks, struct iovec *iov,
                           int total_iovcnt, uint64_t iov_offset, uint64_t chunk_len)
{
    struct raid5_io_channel *r5ch = (struct raid5_io_channel*) raid_bdev_io_channel_get_resource(raid_io->raid_ch);
    struct raid_bdev *raid_bdev = raid_io->raid_bdev;
    struct raid_bdev_io_channel *raid_ch = raid_io->raid_ch;
    struct spdk_mem_map *mem_map = raid_ch->qp_group->qps[chunk_idx]->mem_map;

    struct send_wr_wrapper *send_wrapper = raid_get_send_wrapper(raid_io, chunk_idx);
    struct ibv_send_wr *send_wr = &send_wrapper->send_wr;
    struct ibv_sge *sge = send_wr->sg_list;
    struct cs_message_t *cs_msg = (struct cs_message_t *) sge->addr;

    int chunk_iovcnt = raid5_map_iov(cs_msg->sgl, iov, total_iovcnt, iov_offset, chunk_len);

    cs_msg->type = CS_RD;
    cs_msg->offset = base_offset_blocks;
    cs_msg->length = num_blocks;
    cs_msg->last_index = raid_bdev->num_base_rpcs;
    cs_msg->num_sge[0] = (uint8_t) chunk_iovcnt;
    cs_msg->num_sge[1] = 0;

    struct ibv_mr *mr;
    for (uint8_t i = 0; i < cs_msg->num_sge[0]; i++) {
        mr = (struct ibv_mr *) spdk_mem_map_translate(mem_map, cs_msg->sgl[i].addr, NULL);
        cs_msg->sgl[i].rkey = mr->rkey;
    }
    sge->length = sizeof(struct cs_message_t) - sizeof(struct sg_entry) * (32 - cs_msg->num_sge[0]);
    send_wr->imm_data = kReqTypeRW;
    send_wrapper->rtn_cnt = return_count(cs_msg);
    send_wrapper->cb = raid5_complete_chunk_request_read;
    send_wrapper->ctx = (void *) raid_io;
    send_wrapper->num_blocks = chunk_req_blocks;

    int ret = raid_dispatch_to_rpc(send_wrapper);

    return ret;
}

static void
raid5_handle_read(struct raid_bdev_io *raid_io, uint64_t stripe_index, uint64_t stripe_offset, uint64_t blocks)
{
    struct raid_bdev *raid_bdev = raid_io->raid_bdev;
    struct raid5_io_channel *r5ch = (struct raid5_io_channel*) raid_bdev_io_channel_get_resource(raid_io->raid_ch);
    struct spdk_bdev_io *bdev_io = spdk_bdev_io_from_ctx(raid_io);
    struct iovec *iov = bdev_io->u.bdev.iovs;
    int iovcnt = bdev_io->u.bdev.iovcnt;
    uint64_t iov_offset = 0;

    uint64_t chunk_offset_from, chunk_offset_to, chunk_req_offset, chunk_req_blocks, chunk_len;
    int chunk_iovcnt;
    uint64_t base_offset_blocks;
    int ret;

    uint64_t stripe_offset_from = stripe_offset;
    uint64_t stripe_offset_to = stripe_offset_from + blocks;
    uint8_t first_chunk_idx = stripe_offset_from >> raid_bdev->strip_size_shift;
    uint8_t last_chunk_idx = (stripe_offset_to - 1) >> raid_bdev->strip_size_shift;

    uint8_t p_chunk_idx = raid5_stripe_data_chunks_num(raid_bdev) - stripe_index % raid_bdev->num_base_rpcs; // different for raid6
    if (first_chunk_idx >= p_chunk_idx) {
        first_chunk_idx++;
    }
    if (last_chunk_idx >= p_chunk_idx) {
        last_chunk_idx++;
    }
    for (uint8_t i = 0; i < raid_bdev->num_base_rpcs; i++) {
        if (i == p_chunk_idx || i < first_chunk_idx || i > last_chunk_idx) { // add q for raid6
            continue;
        } else {
            chunk_offset_from = (i < p_chunk_idx ? i : i - 1) << raid_bdev->strip_size_shift; // different for raid6
            chunk_offset_to = chunk_offset_from + raid_bdev->strip_size;

            if (stripe_offset_from > chunk_offset_from) {
                chunk_req_offset = stripe_offset_from - chunk_offset_from;
            } else {
                chunk_req_offset = 0;
            }

            if (stripe_offset_to < chunk_offset_to) {
                chunk_req_blocks = stripe_offset_to - chunk_offset_from;
            } else {
                chunk_req_blocks = raid_bdev->strip_size;
            }

            chunk_req_blocks -= chunk_req_offset;
            chunk_len = chunk_req_blocks << raid_bdev->blocklen_shift;
            base_offset_blocks = (stripe_index << raid_bdev->strip_size_shift) + chunk_req_offset;

            ret = raid5_dispatch_to_rpc_read(raid_io, i, base_offset_blocks, chunk_req_blocks, chunk_req_blocks, iov, iovcnt, iov_offset, chunk_len);
            iov_offset += chunk_len;

            if (spdk_unlikely(ret != 0)) {
                SPDK_ERRLOG("bdev io submit error not due to ENOMEM, it should not happen\n");
                assert(false);
            }
        }
    }

}

static void
raid5_submit_rw_request(struct raid_bdev_io *raid_io)
{
    struct raid_bdev *raid_bdev = raid_io->raid_bdev;
    struct spdk_bdev_io *bdev_io = spdk_bdev_io_from_ctx(raid_io);
    struct raid5_info *r5info = (struct raid5_info*) raid_io->raid_bdev->module_private;
    uint64_t offset_blocks = bdev_io->u.bdev.offset_blocks;
    uint64_t num_blocks = bdev_io->u.bdev.num_blocks;
    uint64_t stripe_index = offset_blocks / r5info->stripe_blocks;
    uint64_t stripe_offset = offset_blocks % r5info->stripe_blocks;
    struct stripe *stripe;

    // TODO: for testing
//    if (raid5_stripe_data_chunks_num(raid_bdev) - stripe_index % raid_bdev->num_base_rpcs == 0) {
//        stripe_index += (stripe_index + 1) % r5info->total_stripes;
//    }
//    stripe_offset = (stripe_offset % raid_bdev->strip_size) + (raid_bdev->strip_size * 0);

    if (!raid_bdev->degraded && bdev_io->type == SPDK_BDEV_IO_TYPE_READ) {
        raid_io->base_bdev_io_remaining = num_blocks;
        raid5_handle_read(raid_io, stripe_index, stripe_offset, num_blocks);
        return;
    }

    stripe = raid5_get_stripe(r5info, stripe_index);
    if (spdk_unlikely(stripe == NULL)) {
        struct raid5_io_channel *r5ch = (struct raid5_io_channel*) raid_bdev_io_channel_get_resource(raid_io->raid_ch);
        struct spdk_bdev_io_wait_entry *wqe = &raid_io->waitq_entry;

        wqe->cb_fn = _raid5_submit_rw_request;
        wqe->cb_arg = raid_io;
        TAILQ_INSERT_TAIL(&r5ch->retry_queue, wqe, link);
        SPDK_ERRLOG("run out of stripes");
        return;
    }

    raid_io->base_bdev_io_remaining = num_blocks;

    raid5_handle_stripe(raid_io, stripe, stripe_offset, num_blocks, 0);
}

static int
raid5_stripe_init(struct stripe *stripe, struct raid_bdev *raid_bdev)
{
    int i;

    stripe->chunk_buffers = (void**) calloc(1, sizeof(void *));
    if (!stripe->chunk_buffers) {
        SPDK_ERRLOG("Failed to allocate chunk buffers array\n");
        return -ENOMEM;
    }

    for (i = 0; i < 1; i++) {
        void *buf;

        buf = spdk_dma_malloc(raid_bdev->strip_size << raid_bdev->blocklen_shift, 32, NULL);
        if (!buf) {
            SPDK_ERRLOG("Failed to allocate chunk buffer\n");
            for (i = i - 1; i >= 0; i--) {
                spdk_dma_free(stripe->chunk_buffers[i]);
            }
            free(stripe->chunk_buffers);
            return -ENOMEM;
        }
        stripe->chunk_buffers[i] = buf;
    }

    TAILQ_INIT(&stripe->requests);
    pthread_spin_init(&stripe->requests_lock, PTHREAD_PROCESS_PRIVATE);

    return 0;
}

static void
raid5_stripe_deinit(struct stripe *stripe, struct raid_bdev *raid_bdev)
{
    uint8_t i;

    for (i = 0; i < 1; i++) {
        spdk_dma_free(stripe->chunk_buffers[i]);
    }
    free(stripe->chunk_buffers);

    pthread_spin_destroy(&stripe->requests_lock);
}

static void
raid5_free(struct raid5_info *r5info)
{
    unsigned int i;

    pthread_spin_destroy(&r5info->active_stripes_lock);

    if (r5info->active_stripes_hash) {
        rte_hash_free(r5info->active_stripes_hash);
    }

    if (r5info->stripe_request_mempool) {
        spdk_mempool_free(r5info->stripe_request_mempool);
    }

    if (r5info->stripes) {
        for (i = 0; i < RAID_MAX_STRIPES; i++) {
            raid5_stripe_deinit(&r5info->stripes[i], r5info->raid_bdev);
        }
        free(r5info->stripes);
    }

    free(r5info);
}

static int
raid5_start(struct raid_bdev *raid_bdev)
{
    uint64_t min_blockcnt = UINT64_MAX;
    struct raid_base_rpc_info *base_info;
    struct raid5_info *r5info;
    char name_buf[32];
    struct rte_hash_parameters hash_params = { 0 };
    unsigned int i;
    int ret = 0;

    r5info = (struct raid5_info*) calloc(1, sizeof(*r5info));
    if (!r5info) {
        SPDK_ERRLOG("Failed to allocate r5info\n");
        return -ENOMEM;
    }
    r5info->raid_bdev = raid_bdev;

    pthread_spin_init(&r5info->active_stripes_lock, PTHREAD_PROCESS_PRIVATE);

    RAID_FOR_EACH_BASE_RPC(raid_bdev, base_info) {
        min_blockcnt = spdk_min(min_blockcnt, base_info->blockcnt);
    }

    r5info->total_stripes = min_blockcnt / raid_bdev->strip_size;
    r5info->stripe_blocks = raid_bdev->strip_size * raid5_stripe_data_chunks_num(raid_bdev);

    raid_bdev->bdev.blockcnt = r5info->stripe_blocks * r5info->total_stripes;
    SPDK_NOTICELOG("raid blockcnt %" PRIu64 "\n", raid_bdev->bdev.blockcnt);
    raid_bdev->bdev.optimal_io_boundary = r5info->stripe_blocks;
    raid_bdev->bdev.split_on_optimal_io_boundary = true;

    r5info->stripes = (struct stripe*) calloc(RAID_MAX_STRIPES, sizeof(*r5info->stripes));
    if (!r5info->stripes) {
        SPDK_ERRLOG("Failed to allocate stripes array\n");
        ret = -ENOMEM;
        goto out;
    }

    TAILQ_INIT(&r5info->free_stripes);

    for (i = 0; i < RAID_MAX_STRIPES; i++) {
        struct stripe *stripe = &r5info->stripes[i];

        ret = raid5_stripe_init(stripe, raid_bdev);
        if (ret) {
            for (; i > 0; --i) {
                raid5_stripe_deinit(&r5info->stripes[i], raid_bdev);
            }
            free(r5info->stripes);
            r5info->stripes = NULL;
            goto out;
        }

        TAILQ_INSERT_TAIL(&r5info->free_stripes, stripe, link);
    }

    snprintf(name_buf, sizeof(name_buf), "raid5_sreq_%p", raid_bdev);

    r5info->stripe_request_mempool = spdk_mempool_create(name_buf,
                                                         RAID_MAX_STRIPES * 4,
                                                         sizeof(struct stripe_request) + sizeof(struct chunk) * raid_bdev->num_base_rpcs,
                                                         SPDK_MEMPOOL_DEFAULT_CACHE_SIZE,
                                                         SPDK_ENV_SOCKET_ID_ANY);
    if (!r5info->stripe_request_mempool) {
        SPDK_ERRLOG("Failed to allocate stripe_request_mempool\n");
        ret = -ENOMEM;
        goto out;
    }

    snprintf(name_buf, sizeof(name_buf), "raid5_hash_%p", raid_bdev);

    hash_params.name = name_buf;
    hash_params.entries = RAID_MAX_STRIPES * 2;
    hash_params.key_len = sizeof(uint64_t);

    r5info->active_stripes_hash = rte_hash_create(&hash_params);
    if (!r5info->active_stripes_hash) {
        SPDK_ERRLOG("Failed to allocate active_stripes_hash\n");
        ret = -ENOMEM;
        goto out;
    }

    TAILQ_INIT(&r5info->active_stripes);

    raid_bdev->module_private = r5info;
    out:
    if (ret) {
        raid5_free(r5info);
    }
    return ret;
}

static void
raid5_stop(struct raid_bdev *raid_bdev)
{
    struct raid5_info *r5info = (struct raid5_info*) raid_bdev->module_private;

    raid5_free(r5info);
}

static int
raid5_io_channel_resource_init(struct raid_bdev *raid_bdev, void *resource)
{
    struct raid5_io_channel *r5ch = (struct raid5_io_channel*) resource;

    TAILQ_INIT(&r5ch->retry_queue);

    return 0;
}

static void
raid5_io_channel_resource_deinit(void *resource)
{
    struct raid5_io_channel *r5ch = (struct raid5_io_channel*) resource;

    assert(TAILQ_EMPTY(&r5ch->retry_queue));
}

static struct raid_bdev_module g_raid5_module = {
        .level = RAID5,
        .base_rpcs_min = 3,
        .base_rpcs_max_degraded = 1,
        .io_channel_resource_size = sizeof(struct raid5_io_channel),
        .start = raid5_start,
        .stop = raid5_stop,
        .submit_rw_request = raid5_submit_rw_request,
        .io_channel_resource_init = raid5_io_channel_resource_init,
        .io_channel_resource_deinit = raid5_io_channel_resource_deinit,
};
RAID_MODULE_REGISTER(&g_raid5_module)

SPDK_LOG_REGISTER_COMPONENT(bdev_raid5)
