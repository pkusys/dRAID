#include "bdev_raid.h"
#include "spdk/config.h"
#include "spdk/env.h"
#include "spdk/thread.h"
#include "spdk/likely.h"
#include "spdk/string.h"
#include "spdk/util.h"
#include "spdk/log.h"

#include "../shared/common.h"

#include <raid.h>
#include <gf_vect_mul.h>
#include <erasure_code.h>

#include <rte_hash.h>
#include <rte_memory.h>

/* The type of chunk request */
enum chunk_request_type {
    CHUNK_READ,
    CHUNK_WRITE,
    CHUNK_WRITE_FORWARD,
    CHUNK_WRITE_FORWARD_DIFF,
    CHUNK_PARITY,
    CHUNK_PARITY_DIFF,
    CHUNK_FORWARD,
    CHUNK_WRITE_FORWARD_MIX,
    CHUNK_WRITE_FORWARD_PRO,
    CHUNK_PARITY_MIX,
    CHUNK_PARITY_PRO,
    CHUNK_PARITY_QRO,
    CHUNK_FORWARD_PRO,
    CHUNK_RECON_D, // if dest chuck is null, 1 return message; if dest chunk exists, 0 return message
    CHUNK_READ_RECON_D, // if dest chuck is null, 2 return messages; if dest chunk exists, 1 return message
    CHUNK_RECON_DP, // if dest chuck is null, 1 return message; if dest chunk exists, 0 return message
    CHUNK_RECON_DP_Q, // if dest chuck is null, 1 return message; if dest chunk exists, 0 return message
    CHUNK_READ_RECON_DP, // if dest chuck is null, 2 return messages; if dest chunk exists, 1 return message
    CHUNK_RECON_DD, // if dest chuck is null, 1 return message (double size); if dest chunk exists, 0 return message
    CHUNK_RECON_DD_P, // if dest chuck is null, 1 return message (double size); if dest chunk exists, 0 return message
    CHUNK_RECON_DD_Q, // if dest chuck is null, 1 return message (double size); if dest chunk exists, 0 return message
    CHUNK_READ_RECON_DD, // if dest chuck is null, 2 return messages (read is chunk size, recon is double size); if dest chunk exists, 0 return message
    NO_OP
};

enum stripe_degraded_type {
    DEGRADED_D,
    DEGRADED_DP,
    DEGRADED_DD,
    NORMAL,
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

    bool is_degraded;

    /* The type of chunk request */
    enum chunk_request_type request_type;

    /* number of partial parity to wait for */
    uint8_t fwd_num;

    /* next data chunk to forward to */
    struct chunk *dst_chunks[2];

    /* target offset */
    uint64_t tgt_offset_blocks;

    /* target number of blocks */
    uint64_t tgt_num_blocks;

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

    /* The stripe's q chunk */
    struct chunk *q_chunk;

    /* degrade mode*/
    enum stripe_degraded_type degraded_type;

    /* Degraded chunk */
    struct chunk *degraded_chunks[2];

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

struct raid6_info {
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

    unsigned char *gf_const_tbl_arr[256];

    unsigned char *gf_const_tbl_arr_a[256];

    unsigned char *gf_const_tbl_arr_b[256][256];

    /* List of active stripes (in hash table) */
    struct active_stripes_head active_stripes;

    /* List of free stripes (not in hash table) */
    TAILQ_HEAD(, stripe) free_stripes;

    /* Lock protecting the stripes hash and lists */
    pthread_spinlock_t active_stripes_lock;
};

struct raid6_io_channel {
    TAILQ_HEAD(, spdk_bdev_io_wait_entry) retry_queue;
};

#define FOR_EACH_CHUNK(req, c) \
	for (c = req->chunks; \
	     c < req->chunks + req->raid_io->raid_bdev->num_base_rpcs; c++)

#define __NEXT_DATA_CHUNK(req, c) \
	c+1 == req->parity_chunk ? c+3 : (c+1 == req->q_chunk ? c+2 :c+1)

#define FOR_EACH_DATA_CHUNK(req, c) \
	for (c = __NEXT_DATA_CHUNK(req, req->chunks-1); \
	     c < req->chunks + req->raid_io->raid_bdev->num_base_rpcs; \
	     c = __NEXT_DATA_CHUNK(req, c))

static inline struct stripe_request *
raid6_chunk_stripe_req(struct chunk *chunk)
{
    return SPDK_CONTAINEROF((chunk - chunk->index), struct stripe_request, chunks);
}

static inline uint8_t
raid6_chunk_data_index(struct chunk *chunk)
{
    uint8_t diff = 0;
    if (chunk > raid6_chunk_stripe_req(chunk)->parity_chunk) {
        diff++;
    }
    if (chunk > raid6_chunk_stripe_req(chunk)->q_chunk) {
        diff++;
    }
    return chunk->index - diff;
}

static inline struct chunk *
raid6_get_data_chunk(struct stripe_request *stripe_req, uint8_t chunk_data_idx)
{
    uint8_t p_chunk_idx = stripe_req->parity_chunk - stripe_req->chunks;
    uint8_t q_chunk_idx = stripe_req->q_chunk - stripe_req->chunks;
    if (q_chunk_idx < p_chunk_idx) {
        return &stripe_req->chunks[chunk_data_idx + 1];
    } else {
        if (chunk_data_idx < p_chunk_idx) {
            return &stripe_req->chunks[chunk_data_idx];
        } else {
            return &stripe_req->chunks[chunk_data_idx + 2];
        }
    }
}

static inline uint8_t
raid6_stripe_data_chunks_num(const struct raid_bdev *raid_bdev)
{
    return raid_bdev->num_base_rpcs - raid_bdev->module->base_rpcs_max_degraded;
}

static bool
raid6_is_full_stripe_write(struct stripe_request *stripe_req)
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

static void
raid6_xor_buf(void *to, void *from, size_t size)
{
    int ret;
    void *vects[3] = { from, to, to };

    ret = xor_gen(3, size, vects);
    if (ret) {
        SPDK_ERRLOG("xor_gen failed\n");
    }
}

static void
raid6_gf_mul(unsigned char *gf_const_tbl, void *to, void *from, size_t size) {
    gf_vect_mul(size, gf_const_tbl, from, to);
}

static void
raid6_q_gen(struct raid6_info* r6info, uint8_t index, void *to, void *from, size_t size) {
    unsigned char *gf_const_tbl = r6info->gf_const_tbl_arr[index];
    gf_vect_mul(size, gf_const_tbl, from, to);
}


static void
raid6_gf_mul_iovs(struct iovec *iovs_dest, int iovs_dest_cnt, size_t iovs_dest_offset,
                  const struct iovec *iovs_src, int iovs_src_cnt, size_t iovs_src_offset,
                  size_t size, unsigned char *gf_const_tbl)
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

        raid6_gf_mul(gf_const_tbl, (char*)v1->iov_base + off1, (char*)v2->iov_base + off2, n);

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

static void
raid6_q_gen_iovs(struct iovec *iovs_dest, int iovs_dest_cnt, size_t iovs_dest_offset,
                 const struct iovec *iovs_src, int iovs_src_cnt, size_t iovs_src_offset,
                 size_t size, struct raid6_info* r6info, uint8_t index)
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

        raid6_q_gen(r6info, index, (char*)v1->iov_base + off1, (char*)v2->iov_base + off2, n);

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

static void
raid6_xor_iovs(struct iovec *iovs_dest, int iovs_dest_cnt, size_t iovs_dest_offset,
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

        raid6_xor_buf((char*)v1->iov_base + off1, (char*)v2->iov_base + off2, n);

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
raid6_chunk_map_iov(struct chunk *chunk, const struct iovec *iov, int iovcnt,
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
raid6_chunk_map_req_data(struct chunk *chunk)
{
    struct stripe_request *stripe_req = raid6_chunk_stripe_req(chunk);
    struct spdk_bdev_io *bdev_io = spdk_bdev_io_from_ctx(stripe_req->raid_io);
    uint64_t len = chunk->req_blocks << stripe_req->raid_io->raid_bdev->blocklen_shift;
    int ret;

    if (len == 0) {
        return 0;
    }

    ret = raid6_chunk_map_iov(chunk, bdev_io->u.bdev.iovs, bdev_io->u.bdev.iovcnt,
                              stripe_req->iov_offset, len);
    if (ret == 0) {
        stripe_req->iov_offset += len;
    }

    return ret;
}

static void
raid6_io_channel_retry_request(struct raid6_io_channel *r6ch)
{
    struct spdk_bdev_io_wait_entry *waitq_entry;

    waitq_entry = TAILQ_FIRST(&r6ch->retry_queue);
    assert(waitq_entry != NULL);
    TAILQ_REMOVE(&r6ch->retry_queue, waitq_entry, link);
    waitq_entry->cb_fn(waitq_entry->cb_arg);
}

static void
raid6_submit_stripe_request(struct stripe_request *stripe_req);

static void
_raid6_submit_stripe_request(void *_stripe_req)
{
    struct stripe_request *stripe_req = (struct stripe_request*) _stripe_req;

    raid6_submit_stripe_request(stripe_req);
}

static void
raid6_stripe_request_put(struct stripe_request *stripe_req)
{
    struct raid6_info *r6info = (struct raid6_info*) stripe_req->raid_io->raid_bdev->module_private;
    struct chunk *chunk;

    FOR_EACH_CHUNK(stripe_req, chunk) {
        if (chunk->iovs != &chunk->iov) {
            free(chunk->iovs);
        }
    }

    spdk_mempool_put(r6info->stripe_request_mempool, stripe_req);
}

static void
raid6_complete_stripe_request(struct stripe_request *stripe_req)
{
    struct stripe *stripe = stripe_req->stripe;
    struct raid_bdev_io *raid_io = stripe_req->raid_io;
    enum spdk_bdev_io_status status = stripe_req->status;
    struct raid6_io_channel *r6ch = (struct raid6_io_channel*) raid_bdev_io_channel_get_resource(raid_io->raid_ch);
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
                             _raid6_submit_stripe_request, next_req);
    }

    req_blocks = 0;
    FOR_EACH_DATA_CHUNK(stripe_req, chunk) {
        req_blocks += chunk->req_blocks;
    }

    raid6_stripe_request_put(stripe_req);

    if (raid_bdev_io_complete_part(raid_io, req_blocks, status)) {
        __atomic_fetch_sub(&stripe->refs, 1, __ATOMIC_SEQ_CST);

        if (!TAILQ_EMPTY(&r6ch->retry_queue)) {
            raid6_io_channel_retry_request(r6ch);
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
raid6_abort_stripe_request(struct stripe_request *stripe_req, enum spdk_bdev_io_status status)
{
    stripe_req->remaining = 0;
    stripe_req->status = status;
    raid6_complete_stripe_request(stripe_req);
}

void
raid6_complete_chunk_request(void *ctx)
{
    struct send_wr_wrapper *send_wrapper = static_cast<struct send_wr_wrapper*>(ctx);
    struct chunk *chunk = static_cast<struct chunk*>(send_wrapper->ctx);
    struct stripe_request *stripe_req = raid6_chunk_stripe_req(chunk);
    struct raid_bdev_io *raid_io = stripe_req->raid_io;
    struct raid_bdev *raid_bdev = raid_io->raid_bdev;
    struct raid6_io_channel *r6ch = (struct raid6_io_channel*) raid_bdev_io_channel_get_resource(raid_io->raid_ch);
    uint32_t blocklen_shift = raid_bdev->blocklen_shift;

    if (--stripe_req->remaining == 0) {
        if (stripe_req->status == SPDK_BDEV_IO_STATUS_SUCCESS) {
            stripe_req->chunk_requests_complete_cb(stripe_req);
        } else {
            raid6_complete_stripe_request(stripe_req);
        }
    }
}

static int
raid6_dispatch_to_rpc(struct raid6_info *r6info, uint8_t chunk_idx, uint64_t base_offset_blocks,
                      uint64_t num_blocks, uint32_t blocklen_shift, struct chunk *ck, uint8_t cs_type)
{
    struct stripe_request *stripe_req = raid6_chunk_stripe_req(ck);
    struct raid_bdev_io *raid_io = stripe_req->raid_io;
    struct raid6_io_channel *r6ch = (struct raid6_io_channel*) raid_bdev_io_channel_get_resource(raid_io->raid_ch);
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
    send_wrapper->cb = raid6_complete_chunk_request;
    send_wrapper->ctx = (void *) ck;

    int ret = raid_dispatch_to_rpc(send_wrapper);

    return ret;
}

static void
_raid6_submit_chunk_request(void *_chunk)
{
    struct chunk *chunk = (struct chunk*) _chunk;
    struct stripe_request *stripe_req = raid6_chunk_stripe_req(chunk);
    struct raid_bdev_io *raid_io = stripe_req->raid_io;
    struct raid_bdev *raid_bdev = raid_io->raid_bdev;
    struct raid6_info *r6info = (struct raid6_info*) raid_bdev->module_private;
    uint32_t blocklen_shift = raid_bdev->blocklen_shift;
    uint64_t offset_blocks = chunk->req_offset;
    uint64_t num_blocks = chunk->req_blocks;
    enum chunk_request_type request_type = chunk->request_type;
    uint64_t base_offset_blocks;
    int ret;

    base_offset_blocks = (stripe_req->stripe->index << raid_bdev->strip_size_shift) + offset_blocks;

    if (request_type == CHUNK_READ) {
        ret = raid6_dispatch_to_rpc(r6info, chunk->index, base_offset_blocks, num_blocks, blocklen_shift, chunk, CS_RD);
    } else {
        ret = raid6_dispatch_to_rpc(r6info, chunk->index, base_offset_blocks, num_blocks, blocklen_shift, chunk, CS_WT);
    }

    if (spdk_unlikely(ret != 0)) {
        SPDK_ERRLOG("bdev io submit error not due to ENOMEM, it should not happen\n");
        assert(false);
    }
}

static void
raid6_submit_chunk_request(struct chunk *chunk, enum chunk_request_type type)
{
    struct stripe_request *stripe_req = raid6_chunk_stripe_req(chunk);

    stripe_req->remaining++;

    chunk->request_type = type;

    _raid6_submit_chunk_request(chunk);
}


static void
raid6_stripe_write_submit(struct stripe_request *stripe_req)
{
    struct chunk *chunk;

    stripe_req->chunk_requests_complete_cb = raid6_complete_stripe_request;

    FOR_EACH_CHUNK(stripe_req, chunk) {
        if (!chunk->is_degraded && chunk->req_blocks > 0) {
            raid6_submit_chunk_request(chunk, CHUNK_WRITE);
        }
    }
}

// Note: full stripe write
static void
raid6_stripe_write_full_stripe(struct stripe_request *stripe_req)
{
    struct chunk *chunk;
    struct chunk *p_chunk = stripe_req->parity_chunk;
    struct chunk *q_chunk = stripe_req->q_chunk;
    struct raid_bdev *raid_bdev = stripe_req->raid_io->raid_bdev;
    struct raid6_info *r6info = (struct raid6_info *) raid_bdev->module_private;
    uint32_t blocklen_shift = raid_bdev->blocklen_shift;
    void *tmp_buf = stripe_req->stripe->chunk_buffers[2];
    struct iovec iov = {
            .iov_base = tmp_buf,
            .iov_len = raid_bdev->strip_size << blocklen_shift,
    };

    raid_memset_iovs(p_chunk->iovs, p_chunk->iovcnt, 0);
    raid_memset_iovs(q_chunk->iovs, q_chunk->iovcnt, 0);

    FOR_EACH_DATA_CHUNK(stripe_req, chunk) {
        raid6_xor_iovs(p_chunk->iovs, p_chunk->iovcnt,
                       (chunk->req_offset - p_chunk->req_offset) << blocklen_shift,
                       chunk->iovs, chunk->iovcnt, 0,
                       chunk->req_blocks << blocklen_shift);
        raid6_q_gen_iovs(&iov, 1, 0,
                         chunk->iovs, chunk->iovcnt, 0,
                         chunk->req_blocks << blocklen_shift, r6info, raid6_chunk_data_index(chunk));
        raid6_xor_iovs(q_chunk->iovs, q_chunk->iovcnt,
                       (chunk->req_offset - q_chunk->req_offset) << blocklen_shift,
                       &iov, 1, 0,
                       chunk->req_blocks << blocklen_shift);
    }

    raid6_stripe_write_submit(stripe_req);
}

void
raid6_complete_chunk_request_d_raid(void *ctx)
{
    struct send_wr_wrapper *send_wrapper = static_cast<struct send_wr_wrapper*>(ctx);
    struct chunk *chunk = static_cast<struct chunk*>(send_wrapper->ctx);
    struct stripe_request *stripe_req = raid6_chunk_stripe_req(chunk);
    struct raid_bdev_io *raid_io = stripe_req->raid_io;
    struct raid_bdev *raid_bdev = raid_io->raid_bdev;
    struct raid6_info *r6info = (struct raid6_info *) raid_bdev->module_private;
    struct raid6_io_channel *r6ch = (struct raid6_io_channel*) raid_bdev_io_channel_get_resource(raid_io->raid_ch);
    uint32_t blocklen_shift = raid_bdev->blocklen_shift;
    struct chunk *d_chunk = stripe_req->degraded_chunks[0];
    struct chunk *d_chunk2 = stripe_req->degraded_chunks[1];
    struct chunk *dst_chunk = chunk->dst_chunks[0];
    size_t len;
    int iovcnt_x, iovcnt_y;
    void *buf, *tmp_buf_x, *tmp_buf_y;
    struct iovec iov_buf, iov_buf2, iov_tmp_x, iov_tmp_y;
    struct iovec *iovs_x, *iovs_y;

    struct cs_message_t *cs_msg = (struct cs_message_t *) send_wrapper->send_wr.sg_list->addr;

    if ((chunk->request_type == CHUNK_READ_RECON_DP || chunk->request_type == CHUNK_RECON_DP ||
         chunk->request_type == CHUNK_RECON_DP_Q) && dst_chunk == NULL) {
        len = cs_msg->fwd_length << blocklen_shift;
        raid6_q_gen_iovs(d_chunk->iovs, d_chunk->iovcnt, 0,
                         d_chunk->iovs, d_chunk->iovcnt, 0,
                         len, r6info, 255 - raid6_chunk_data_index(d_chunk));
    } else if ((chunk->request_type == CHUNK_READ_RECON_DD || chunk->request_type == CHUNK_RECON_DD ||
                chunk->request_type == CHUNK_RECON_DD_P || chunk->request_type == CHUNK_RECON_DD_Q)
               && dst_chunk == NULL) {
        buf = send_wrapper->buf;
        struct chunk *d_chunk_x;
        struct chunk *d_chunk_y;
        if (d_chunk->index > d_chunk2->index) {
            d_chunk_y = d_chunk;
            d_chunk_x = d_chunk2;
        } else {
            d_chunk_y = d_chunk2;
            d_chunk_x = d_chunk;
        }

        uint64_t req_offset = chunk->tgt_offset_blocks;
        unsigned char *gf_const_tbl_a = r6info->gf_const_tbl_arr_a
        [raid6_chunk_data_index(d_chunk_y) - raid6_chunk_data_index(d_chunk_x)];
        unsigned char *gf_const_tbl_b = r6info->gf_const_tbl_arr_b
        [raid6_chunk_data_index(d_chunk_y) - raid6_chunk_data_index(d_chunk_x)]
        [255 - raid6_chunk_data_index(d_chunk_x)];

        len = cs_msg->fwd_length << blocklen_shift;

        iov_buf = {
                .iov_base = buf,
                .iov_len = len,
        };
        iov_buf2 = {
                .iov_base = (char*) buf + len,
                .iov_len = len,
        };

        if (d_chunk_x->req_blocks == cs_msg->fwd_length) {
            iovs_x = d_chunk_x->iovs;
            iovcnt_x = d_chunk_x->iovcnt;
        } else {
            tmp_buf_x = stripe_req->stripe->chunk_buffers[2];
            iov_tmp_x = {
                    .iov_base = tmp_buf_x,
                    .iov_len = len,
            };
            iovs_x = &iov_tmp_x;
            iovcnt_x = 1;
        }
        if (d_chunk_y->req_blocks == cs_msg->fwd_length) {
            iovs_y = d_chunk_y->iovs;
            iovcnt_y = d_chunk_y->iovcnt;
            raid_memcpy_iovs(iovs_y, iovcnt_y, 0,
                             &iov_buf, 1, 0,
                             len); //copy p into y
        } else {
            tmp_buf_y = buf;
            iov_tmp_y = {
                    .iov_base = tmp_buf_y,
                    .iov_len = len,
            };
            iovs_y = &iov_tmp_y;
            iovcnt_y = 1;
        }

        // Note: construct d_chunk_x
        raid6_gf_mul_iovs(iovs_x, iovcnt_x, 0,
                          &iov_buf, 1, 0,
                          len, gf_const_tbl_a);
        raid6_gf_mul_iovs(&iov_buf2, 1, 0,
                          &iov_buf2, 1, 0,
                          len, gf_const_tbl_b);
        raid6_xor_iovs(iovs_x, iovcnt_x, 0,
                       &iov_buf2, 1, 0,
                       len);

        // Note: construct d_chunk_y
        raid6_xor_iovs(iovs_y, iovcnt_y, 0,
                       iovs_x, iovcnt_x, 0,
                       len);

        if (d_chunk_x->req_blocks > 0 && d_chunk_x->req_blocks < cs_msg->fwd_length) {
            size_t src_offset = (d_chunk_x->req_offset - req_offset) << blocklen_shift;
            raid_memcpy_iovs(d_chunk_x->iovs, d_chunk_x->iovcnt, 0,
                             iovs_x, iovcnt_x, src_offset,
                             d_chunk_x->req_blocks << blocklen_shift);
        }
        if (d_chunk_y->req_blocks > 0 &&d_chunk_y->req_blocks < cs_msg->fwd_length) {
            size_t src_offset = (d_chunk_y->req_offset - req_offset) << blocklen_shift;
            raid_memcpy_iovs(d_chunk_y->iovs, d_chunk_y->iovcnt, 0,
                             iovs_y, iovcnt_y, src_offset,
                             d_chunk_y->req_blocks << blocklen_shift);
        }
    }

    if (--stripe_req->remaining == 0) {
        if (stripe_req->status == SPDK_BDEV_IO_STATUS_SUCCESS) {
            stripe_req->chunk_requests_complete_cb(stripe_req);
        } else {
            raid6_complete_stripe_request(stripe_req);
        }
    }
}

static int
raid6_dispatch_to_rpc_d_raid(struct raid6_info *r6info, uint8_t chunk_idx,
                             uint64_t base_offset_blocks, uint64_t num_blocks, uint32_t blocklen_shift, struct chunk *ck,
                             uint64_t t_base_offset_blocks, uint64_t t_num_blocks, int8_t dst_index,
                             int8_t dst_index2, uint8_t fwd_num, uint8_t msg_type, uint8_t cs_type, uint8_t data_index)
{
    struct stripe_request *stripe_req = raid6_chunk_stripe_req(ck);
    struct raid_bdev_io *raid_io = stripe_req->raid_io;
    struct raid6_io_channel *r6ch = (struct raid6_io_channel*) raid_bdev_io_channel_get_resource(raid_io->raid_ch);
    struct raid_bdev *raid_bdev = raid_io->raid_bdev;
    struct raid_bdev_io_channel *raid_ch = raid_io->raid_ch;
    struct spdk_mem_map *mem_map = raid_ch->qp_group->qps[chunk_idx]->mem_map;

    struct send_wr_wrapper *send_wrapper = raid_get_send_wrapper(raid_io, chunk_idx);
    struct ibv_send_wr *send_wr = &send_wrapper->send_wr;
    struct ibv_sge *sge = send_wr->sg_list;
    struct cs_message_t *cs_msg = (struct cs_message_t *) sge->addr;
    cs_msg->type = cs_type;
    cs_msg->data_index = data_index;
    cs_msg->offset = base_offset_blocks;
    cs_msg->length = num_blocks;
    cs_msg->last_index = raid_bdev->num_base_rpcs;
    cs_msg->fwd_offset = t_base_offset_blocks;
    cs_msg->fwd_length = t_num_blocks;
    cs_msg->fwd_num = fwd_num;
    cs_msg->next_index = dst_index;
    cs_msg->next_index2 = dst_index2;

    struct ibv_mr *mr;
    if (cs_type == FWD_RW || cs_type == FWD_RW_DIFF || cs_type == FWD_MIX || cs_type == PR_MIX) {
        cs_msg->num_sge[0] = (uint8_t) ck->iovcnt;
        cs_msg->num_sge[1] = 0;
    } else if (cs_type == FWD_RO || cs_type == PR_NO_DIFF || cs_type == PR_DIFF) {
        cs_msg->num_sge[0] = 0;
        cs_msg->num_sge[1] = 0;
    } else if (cs_type == RECON_RT || cs_type == RECON_RT_DP) {
        cs_msg->num_sge[0] = (uint8_t) ck->iovcnt;
        if (dst_index == -1) {
            cs_msg->num_sge[1] = stripe_req->degraded_chunks[0]->iovcnt;
        } else {
            cs_msg->num_sge[1] = 0;
        }
    } else if (cs_type == RECON_NRT || cs_type == RECON_NRT_DP_Q || cs_type == RECON_NRT_DP) {
        cs_msg->num_sge[0] = 0;
        if (dst_index == -1) {
            cs_msg->num_sge[1] = stripe_req->degraded_chunks[0]->iovcnt;
        } else {
            cs_msg->num_sge[1] = 0;
        }
    } else if (cs_type == RECON_RT_DD) {
        cs_msg->num_sge[0] = (uint8_t) ck->iovcnt;
        if (dst_index == -1) {
            cs_msg->num_sge[1] = 1;
        } else {
            cs_msg->num_sge[1] = 0;
        }
    } else if (cs_type == RECON_NRT_DD_P || cs_type == RECON_NRT_DD_Q || cs_type == RECON_NRT_DD) {
        cs_msg->num_sge[0] = 0;
        if (dst_index == -1) {
            cs_msg->num_sge[1] = 1;
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
    if ((cs_type == RECON_RT_DD || cs_type == RECON_NRT_DD_P || cs_type == RECON_NRT_DD_Q || cs_type == RECON_NRT_DD) && dst_index == -1) {
        cs_msg->sgl[cs_msg->num_sge[0]].addr = (uint64_t) send_wrapper->buf;
        cs_msg->sgl[cs_msg->num_sge[0]].len = (uint32_t) t_num_blocks << (blocklen_shift + 1);
        mr = (struct ibv_mr *) spdk_mem_map_translate(mem_map,
                                                      cs_msg->sgl[cs_msg->num_sge[0]].addr,
                                                      NULL);
        cs_msg->sgl[cs_msg->num_sge[0]].rkey = mr->rkey;
    } else {
        for (uint8_t i = 0; i < cs_msg->num_sge[1]; i++) {
            cs_msg->sgl[cs_msg->num_sge[0] + i].addr = (uint64_t) stripe_req->degraded_chunks[0]->iovs[i].iov_base;
            cs_msg->sgl[cs_msg->num_sge[0] + i].len = (uint32_t) stripe_req->degraded_chunks[0]->iovs[i].iov_len;
            mr = (struct ibv_mr *) spdk_mem_map_translate(mem_map,
                                                          cs_msg->sgl[cs_msg->num_sge[0] + i].addr,
                                                          NULL);
            cs_msg->sgl[cs_msg->num_sge[0] + i].rkey = mr->rkey;
        }
    }

    sge->length = sizeof(struct cs_message_t) - sizeof(struct sg_entry) * (32 - cs_msg->num_sge[0] - cs_msg->num_sge[1]);
    send_wr->imm_data = msg_type;
    send_wrapper->rtn_cnt = return_count(cs_msg);
    send_wrapper->cb = raid6_complete_chunk_request_d_raid;
    send_wrapper->ctx = (void *) ck;

    int ret = raid_dispatch_to_rpc(send_wrapper);

    return ret;
}

static void
_raid6_submit_chunk_request_d_raid(void *_chunk)
{
    struct chunk *chunk = (struct chunk*) _chunk;
    struct chunk *dst_chunk = chunk->dst_chunks[0];
    struct chunk *dst_chunk2 = chunk->dst_chunks[1];
    struct stripe_request *stripe_req = raid6_chunk_stripe_req(chunk);
    struct chunk *p_chunk = stripe_req->parity_chunk;
    struct chunk *q_chunk = stripe_req->q_chunk;
    struct raid_bdev_io *raid_io = stripe_req->raid_io;
    struct raid_bdev *raid_bdev = raid_io->raid_bdev;
    struct raid6_info *r6info = (struct raid6_info *) raid_bdev->module_private;
    uint32_t blocklen_shift = raid_bdev->blocklen_shift;
    uint64_t offset_blocks = chunk->req_offset;
    uint64_t num_blocks = chunk->req_blocks;
    uint64_t t_offset_blocks = chunk->tgt_offset_blocks;
    uint64_t t_num_blocks = chunk->tgt_num_blocks;
    uint64_t base_offset_blocks, t_base_offset_blocks;
    enum chunk_request_type request_type = chunk->request_type;
    int ret;
    uint8_t msg_type, cs_type, data_index;

    if (chunk != p_chunk && chunk != q_chunk) {
        data_index = raid6_chunk_data_index(chunk);
    } else {
        data_index = 0;
    }

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
        offset_blocks= dst_chunk != NULL ? dst_chunk->req_offset : dst_chunk2->req_offset;
        base_offset_blocks = (stripe_req->stripe->index << raid_bdev->strip_size_shift) + offset_blocks;
        num_blocks = dst_chunk != NULL ? dst_chunk->req_blocks : dst_chunk2->req_blocks;
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
    } else if (request_type == CHUNK_WRITE_FORWARD_MIX) {
        base_offset_blocks = (stripe_req->stripe->index << raid_bdev->strip_size_shift) + offset_blocks;
        msg_type = kReqTypePartialWrite;
        cs_type = FWD_MIX;
    } else if (request_type == CHUNK_PARITY_MIX) {
        base_offset_blocks = (stripe_req->stripe->index << raid_bdev->strip_size_shift) + offset_blocks;
        msg_type = kReqTypeParity;
        cs_type = PR_MIX;
    } else if (request_type == CHUNK_READ_RECON_D) {
        base_offset_blocks = (stripe_req->stripe->index << raid_bdev->strip_size_shift) + offset_blocks;
        msg_type = kReqTypeReconRead;
        cs_type = RECON_RT;
    } else if (request_type == CHUNK_RECON_D) {
        msg_type = kReqTypeReconRead;
        cs_type = RECON_NRT;
    } else if (request_type == CHUNK_READ_RECON_DP) {
        base_offset_blocks = (stripe_req->stripe->index << raid_bdev->strip_size_shift) + offset_blocks;
        msg_type = kReqTypeReconRead;
        cs_type = RECON_RT_DP;
    } else if (request_type == CHUNK_RECON_DP_Q) {
        msg_type = kReqTypeReconRead;
        cs_type = RECON_NRT_DP_Q;
    } else if (request_type == CHUNK_RECON_DP) {
        msg_type = kReqTypeReconRead;
        cs_type = RECON_NRT_DP;
    } else if (request_type == CHUNK_READ_RECON_DD) {
        base_offset_blocks = (stripe_req->stripe->index << raid_bdev->strip_size_shift) + offset_blocks;
        msg_type = kReqTypeReconRead;
        cs_type = RECON_RT_DD;
    } else if (request_type == CHUNK_RECON_DD_P) {
        msg_type = kReqTypeReconRead;
        cs_type = RECON_NRT_DD_P;
    } else if (request_type == CHUNK_RECON_DD_Q) {
        msg_type = kReqTypeReconRead;
        cs_type = RECON_NRT_DD_Q;
    } else if (request_type == CHUNK_RECON_DD) {
        msg_type = kReqTypeReconRead;
        cs_type = RECON_NRT_DD;
    } else {
        assert(false);
    }

    ret = raid6_dispatch_to_rpc_d_raid(r6info, chunk->index, base_offset_blocks, num_blocks, blocklen_shift, chunk,
                                       t_base_offset_blocks, t_num_blocks, dst_chunk == NULL? -1 : (int8_t) dst_chunk->index,
                                       dst_chunk2 == NULL? -1 : (int8_t) dst_chunk2->index, chunk->fwd_num,
                                       msg_type, cs_type, data_index);

    if (spdk_unlikely(ret != 0)) {
        SPDK_ERRLOG("bdev io submit error not due to ENOMEM, it should not happen\n");
        assert(false);
    }
}

static void
raid6_submit_chunk_request_d_raid(struct chunk *chunk) {
    struct stripe_request *stripe_req = raid6_chunk_stripe_req(chunk);

    stripe_req->remaining++;

    _raid6_submit_chunk_request_d_raid(chunk);
}

static void
raid6_stripe_write_d_rcrmw_pro(struct stripe_request *stripe_req)
{
    struct chunk *chunk;
    struct chunk *p_chunk = stripe_req->parity_chunk;
    struct chunk *q_chunk = stripe_req->q_chunk;
    struct chunk *d_chunk_x = stripe_req->degraded_chunks[0];
    struct chunk *d_chunk_y = stripe_req->degraded_chunks[1];
    struct raid_bdev_io *raid_io = stripe_req->raid_io;
    struct raid_bdev_io_channel *raid_ch = raid_io->raid_ch;
    struct raid_bdev *raid_bdev = raid_io->raid_bdev;
    struct raid6_io_channel *r6ch = (struct raid6_io_channel*) raid_bdev_io_channel_get_resource(raid_io->raid_ch);
    uint32_t blocklen_shift = raid_bdev->blocklen_shift;
    struct raid6_info *r6info = (struct raid6_info *) raid_bdev->module_private;
    struct spdk_mem_map *mem_map;
    struct send_wr_wrapper *send_wrapper;
    struct ibv_send_wr *send_wr;
    struct ibv_sge *sge;
    struct cs_message_t *cs_msg;
    struct ibv_mr *mr;
    int ret;

    void *tmp_buf = stripe_req->stripe->chunk_buffers[2];
    struct iovec iov = {
            .iov_base = tmp_buf,
            .iov_len = raid_bdev->strip_size << blocklen_shift,
    };

    stripe_req->chunk_requests_complete_cb = raid6_complete_stripe_request;
    raid_memset_iovs(p_chunk->iovs, p_chunk->iovcnt, 0);
    raid_memset_iovs(q_chunk->iovs, q_chunk->iovcnt, 0);
    // TODO: avoid memcpy
    if (d_chunk_x->req_blocks) {
        raid6_xor_iovs(p_chunk->iovs, p_chunk->iovcnt,
                       (d_chunk_x->req_offset - p_chunk->req_offset) << blocklen_shift,
                       d_chunk_x->iovs, d_chunk_x->iovcnt, 0,
                       d_chunk_x->req_blocks << blocklen_shift);
        raid6_q_gen_iovs(&iov, 1, 0,
                         d_chunk_x->iovs, d_chunk_x->iovcnt, 0,
                         d_chunk_x->req_blocks << blocklen_shift, r6info, raid6_chunk_data_index(d_chunk_x));
        raid6_xor_iovs(q_chunk->iovs, q_chunk->iovcnt,
                       (d_chunk_x->req_offset - q_chunk->req_offset) << blocklen_shift,
                       &iov, 1, 0,
                       d_chunk_x->req_blocks << blocklen_shift);
    }
    if (d_chunk_y->req_blocks) {
        raid6_xor_iovs(p_chunk->iovs, p_chunk->iovcnt,
                       (d_chunk_y->req_offset - p_chunk->req_offset) << blocklen_shift,
                       d_chunk_y->iovs, d_chunk_y->iovcnt, 0,
                       d_chunk_y->req_blocks << blocklen_shift);
        raid6_q_gen_iovs(&iov, 1, 0,
                         d_chunk_y->iovs, d_chunk_y->iovcnt, 0,
                         d_chunk_y->req_blocks << blocklen_shift, r6info, raid6_chunk_data_index(d_chunk_y));
        raid6_xor_iovs(q_chunk->iovs, q_chunk->iovcnt,
                       (d_chunk_y->req_offset - q_chunk->req_offset) << blocklen_shift,
                       &iov, 1, 0,
                       d_chunk_y->req_blocks << blocklen_shift);
    }

    FOR_EACH_CHUNK(stripe_req, chunk) {
        if (chunk->is_degraded) {
            continue;
        }

        send_wrapper = raid_get_send_wrapper(raid_io, chunk->index);
        send_wr = &send_wrapper->send_wr;
        sge = send_wr->sg_list;
        cs_msg = (struct cs_message_t *) sge->addr;
        mem_map = raid_ch->qp_group->qps[chunk->index]->mem_map;
        stripe_req->remaining++;

        cs_msg->next_index = raid6_chunk_data_index(p_chunk);
        cs_msg->next_index2 = raid6_chunk_data_index(q_chunk);
        cs_msg->last_index = raid_bdev->num_base_rpcs;
        cs_msg->x_index = raid6_chunk_data_index(d_chunk_x);
        cs_msg->y_index = raid6_chunk_data_index(d_chunk_y);
        cs_msg->fwd_offset = (stripe_req->stripe->index << raid_bdev->strip_size_shift) + d_chunk_x->req_offset;
        cs_msg->fwd_length = d_chunk_x->req_blocks;
        cs_msg->y_offset = (stripe_req->stripe->index << raid_bdev->strip_size_shift) + d_chunk_y->req_offset;
        cs_msg->y_length = d_chunk_y->req_blocks;
        send_wrapper->cb = raid6_complete_chunk_request_d_raid;
        send_wrapper->ctx = (void *) chunk;

        if (chunk == p_chunk) {
            cs_msg->type = PR_PRO;
            send_wr->imm_data = kReqTypeParity;
            cs_msg->fwd_num = raid_bdev->num_base_rpcs - 3;
            cs_msg->data_index = 0;
            cs_msg->offset = (stripe_req->stripe->index << raid_bdev->strip_size_shift) + chunk->req_offset;
            cs_msg->length = chunk->req_blocks;
            cs_msg->num_sge[0] = (uint8_t) chunk->iovcnt;
            cs_msg->num_sge[1] = 0;
        } else if (chunk == q_chunk) {
            cs_msg->type = PR_QRO;
            send_wr->imm_data = kReqTypeParity;
            cs_msg->fwd_num = raid_bdev->num_base_rpcs - 3;
            cs_msg->data_index = 0;
            cs_msg->offset = (stripe_req->stripe->index << raid_bdev->strip_size_shift) + chunk->req_offset;
            cs_msg->length = chunk->req_blocks;
            cs_msg->num_sge[0] = (uint8_t) chunk->iovcnt;
            cs_msg->num_sge[1] = 0;
        } else if (chunk->req_blocks > 0) {
            cs_msg->type = FWD_RW_PRO;
            send_wr->imm_data = kReqTypePartialWrite;
            cs_msg->fwd_num = 0;
            cs_msg->data_index = raid6_chunk_data_index(chunk);
            cs_msg->offset = (stripe_req->stripe->index << raid_bdev->strip_size_shift) + chunk->req_offset;
            cs_msg->length = chunk->req_blocks;
            cs_msg->num_sge[0] = (uint8_t) chunk->iovcnt;
            cs_msg->num_sge[1] = 0;
        } else {
            cs_msg->type = FWD_RO_PRO;
            send_wr->imm_data = kReqTypePartialWrite;
            cs_msg->fwd_num = 0;
            cs_msg->data_index = raid6_chunk_data_index(chunk);
            cs_msg->offset = (stripe_req->stripe->index << raid_bdev->strip_size_shift) + p_chunk->req_offset;
            cs_msg->length = p_chunk->req_blocks;
            cs_msg->num_sge[0] = 0;
            cs_msg->num_sge[1] = 0;
        }

        for (uint8_t i = 0; i < cs_msg->num_sge[0]; i++) {
            cs_msg->sgl[i].addr = (uint64_t) chunk->iovs[i].iov_base;
            cs_msg->sgl[i].len = (uint32_t) chunk->iovs[i].iov_len;
            mr = (struct ibv_mr *) spdk_mem_map_translate(mem_map, cs_msg->sgl[i].addr, NULL);
            cs_msg->sgl[i].rkey = mr->rkey;
        }

        sge->length = sizeof(struct cs_message_t) - sizeof(struct sg_entry) * (32 - cs_msg->num_sge[0] - cs_msg->num_sge[1]);
        send_wrapper->rtn_cnt = return_count(cs_msg);

        ret = raid_dispatch_to_rpc(send_wrapper);
        if (spdk_unlikely(ret != 0)) {
            SPDK_ERRLOG("bdev io submit error not due to ENOMEM, it should not happen\n");
            assert(false);
        }
    }
}

static void
raid6_stripe_write_d_rcrmw(struct stripe_request *stripe_req)
{
    struct chunk *chunk;
    struct chunk *p_chunk = stripe_req->parity_chunk;
    struct chunk *q_chunk = stripe_req->q_chunk;
    struct chunk *d_chunk = stripe_req->degraded_chunks[0];
    uint8_t fwd_num = 0;
    struct raid_bdev *raid_bdev = stripe_req->raid_io->raid_bdev;
    uint32_t blocklen_shift = raid_bdev->blocklen_shift;
    struct raid6_info *r6info = (struct raid6_info *) raid_bdev->module_private;
    bool p_degraded = p_chunk->is_degraded;
    bool q_degraded = q_chunk->is_degraded;

    stripe_req->chunk_requests_complete_cb = raid6_complete_stripe_request;

    raid_memset_iovs(p_chunk->iovs, p_chunk->iovcnt, 0);
    raid_memset_iovs(q_chunk->iovs, q_chunk->iovcnt, 0);
    // TODO: not to memcpy?
    raid_memcpy_iovs(p_chunk->iovs, p_chunk->iovcnt,
                     (d_chunk->req_offset - p_chunk->req_offset) << blocklen_shift,
                     d_chunk->iovs, d_chunk->iovcnt, 0,
                     d_chunk->req_blocks << blocklen_shift);
    raid6_q_gen_iovs(q_chunk->iovs, q_chunk->iovcnt,
                     (d_chunk->req_offset - q_chunk->req_offset) << blocklen_shift,
                     d_chunk->iovs, d_chunk->iovcnt, 0,
                     d_chunk->req_blocks << blocklen_shift,
                     r6info, raid6_chunk_data_index(d_chunk));

    FOR_EACH_DATA_CHUNK(stripe_req, chunk) {
        if (chunk->is_degraded) {
            chunk->request_type = NO_OP;
            continue;
        }
        chunk->dst_chunks[0] = p_degraded ? NULL : p_chunk;
        chunk->dst_chunks[1] = q_degraded ? NULL : q_chunk;
        chunk->tgt_offset_blocks = d_chunk->req_offset;
        chunk->tgt_num_blocks = d_chunk->req_blocks;
        if (chunk->req_blocks > 0) {
            fwd_num++;
            chunk->request_type = CHUNK_WRITE_FORWARD_MIX;
        }
        else{
            fwd_num++;
            chunk->request_type = CHUNK_FORWARD;
        }
    }

    p_chunk->fwd_num = fwd_num;
    p_chunk->dst_chunks[0] = p_chunk->dst_chunks[1] = NULL;
    p_chunk->tgt_offset_blocks = d_chunk->req_offset;
    p_chunk->tgt_num_blocks = d_chunk->req_blocks;
    p_chunk->request_type = CHUNK_PARITY_MIX;
    q_chunk->fwd_num = fwd_num;
    q_chunk->dst_chunks[0] = q_chunk->dst_chunks[1] = NULL;
    q_chunk->tgt_offset_blocks = d_chunk->req_offset;
    q_chunk->tgt_num_blocks = d_chunk->req_blocks;;
    q_chunk->request_type = CHUNK_PARITY_MIX;

    if (p_degraded) {
        p_chunk->request_type=NO_OP;
    }
    if (q_degraded) {
        q_chunk->request_type=NO_OP;
    }

    FOR_EACH_CHUNK(stripe_req, chunk) {
        if (chunk->request_type != NO_OP) {
            raid6_submit_chunk_request_d_raid(chunk);
        }
    }

}

static void
raid6_stripe_write_d_rmw(struct stripe_request *stripe_req)
{
    struct chunk *chunk;
    struct chunk *p_chunk = stripe_req->parity_chunk;
    struct chunk *q_chunk = stripe_req->q_chunk;
    uint8_t fwd_num = 0;
    bool p_degraded = p_chunk->is_degraded;
    bool q_degraded = q_chunk->is_degraded;

    stripe_req->chunk_requests_complete_cb = raid6_complete_stripe_request;

    FOR_EACH_DATA_CHUNK(stripe_req, chunk) {
        chunk->dst_chunks[0] = p_degraded ? NULL : p_chunk;
        chunk->dst_chunks[1] = q_degraded ? NULL : q_chunk;
        chunk->tgt_offset_blocks = p_chunk->req_offset;
        chunk->tgt_num_blocks = p_chunk->req_blocks;
        if (chunk->req_blocks > 0) {
            fwd_num++;
            chunk->request_type = CHUNK_WRITE_FORWARD_DIFF;
        } else {
            chunk->request_type = NO_OP;
        }
    }

    p_chunk->fwd_num = fwd_num;
    p_chunk->dst_chunks[0] = NULL;
    p_chunk->dst_chunks[1] = NULL;
    p_chunk->tgt_offset_blocks = p_chunk->req_offset;
    p_chunk->tgt_num_blocks = p_chunk->req_blocks;
    p_chunk->request_type = CHUNK_PARITY_DIFF;
    q_chunk->fwd_num = fwd_num;
    q_chunk->dst_chunks[0] = NULL;
    q_chunk->dst_chunks[1] = NULL;
    q_chunk->tgt_offset_blocks = p_chunk->req_offset; // Note: same as p
    q_chunk->tgt_num_blocks = p_chunk->req_blocks; // Note: same as p
    q_chunk->request_type = CHUNK_PARITY_DIFF;

    FOR_EACH_CHUNK(stripe_req, chunk) {
        if (chunk->request_type != NO_OP && !chunk->is_degraded) {
            raid6_submit_chunk_request_d_raid(chunk);
        }
    }
}

static void
raid6_stripe_write_d_reconstructed_write(struct stripe_request *stripe_req)
{
    struct chunk *chunk;
    struct chunk *p_chunk = stripe_req->parity_chunk;
    struct chunk *q_chunk = stripe_req->q_chunk;
    uint8_t fwd_num = raid6_stripe_data_chunks_num(stripe_req->raid_io->raid_bdev);
    bool p_degraded = p_chunk->is_degraded;
    bool q_degraded = q_chunk->is_degraded;

    stripe_req->chunk_requests_complete_cb = raid6_complete_stripe_request;

    FOR_EACH_DATA_CHUNK(stripe_req, chunk) {
        chunk->dst_chunks[0] = p_degraded ? NULL : p_chunk;
        chunk->dst_chunks[1] = q_degraded ? NULL : q_chunk;
        chunk->tgt_offset_blocks = p_chunk->req_offset;
        chunk->tgt_num_blocks = p_chunk->req_blocks;
        if (chunk->req_blocks > 0) {
            chunk->request_type = CHUNK_WRITE_FORWARD;
        } else {
            chunk->request_type = CHUNK_FORWARD;
        }
    }

    p_chunk->fwd_num = fwd_num;
    p_chunk->dst_chunks[0] = NULL;
    p_chunk->dst_chunks[1] = NULL;
    p_chunk->tgt_offset_blocks = p_chunk->req_offset;
    p_chunk->tgt_num_blocks = p_chunk->req_blocks;
    p_chunk->request_type = CHUNK_PARITY;
    q_chunk->fwd_num = fwd_num;
    q_chunk->dst_chunks[0] = NULL;
    q_chunk->dst_chunks[1] = NULL;
    q_chunk->tgt_offset_blocks = p_chunk->req_offset; // Note: same as p
    q_chunk->tgt_num_blocks = p_chunk->req_blocks; // Note: same as p
    q_chunk->request_type = CHUNK_PARITY;

    FOR_EACH_CHUNK(stripe_req, chunk) {
        if (!chunk->is_degraded) {
            raid6_submit_chunk_request_d_raid(chunk);
        }
    }
}

static void
raid6_stripe_write(struct stripe_request *stripe_req)
{
    struct raid_bdev *raid_bdev = stripe_req->raid_io->raid_bdev;
    struct chunk *p_chunk = stripe_req->parity_chunk;
    struct chunk *q_chunk = stripe_req->q_chunk;
    struct chunk *chunk;
    struct raid_base_rpc_info *base_info;
    int preread_balance = 0;
    uint8_t total_degraded = 0;
    uint8_t data_degraded = 0;
    uint8_t req_degraded = 0;

    FOR_EACH_CHUNK(stripe_req, chunk) {
        chunk->is_degraded = false;
    }
    stripe_req->degraded_chunks[0] = NULL;
    stripe_req->degraded_chunks[1] = NULL;

    FOR_EACH_CHUNK(stripe_req, chunk) {
        base_info = &raid_bdev->base_rpc_info[chunk->index];
        if (base_info->degraded) {
            chunk->is_degraded = true;
            if (total_degraded < 2) {
                stripe_req->degraded_chunks[total_degraded] = chunk;
            }
            total_degraded++;
            if (chunk != p_chunk && chunk != q_chunk) {
                data_degraded++;
                if (chunk->req_blocks) {
                    req_degraded++;
                }
            }
        }
    }

    // keep in order D,P,Q
    if (stripe_req->degraded_chunks[1]) {
        if (stripe_req->degraded_chunks[0] == p_chunk) {
            chunk = stripe_req->degraded_chunks[0];
            stripe_req->degraded_chunks[0] = stripe_req->degraded_chunks[1];
            stripe_req->degraded_chunks[1] = chunk;
        }
        if (stripe_req->degraded_chunks[0] == q_chunk) {
            chunk = stripe_req->degraded_chunks[0];
            stripe_req->degraded_chunks[0] = stripe_req->degraded_chunks[1];
            stripe_req->degraded_chunks[1] = chunk;
        }
    }

    if (total_degraded > raid_bdev->module->base_rpcs_max_degraded) {
        raid6_abort_stripe_request(stripe_req, SPDK_BDEV_IO_STATUS_FAILED);
        return;
    }

    // Note: single chunk -> partial parity update; > 1 chunk -> full parity update
    if (stripe_req->first_data_chunk == stripe_req->last_data_chunk) {
        p_chunk->req_offset = stripe_req->first_data_chunk->req_offset;
        p_chunk->req_blocks = stripe_req->first_data_chunk->req_blocks;
        q_chunk->req_offset = stripe_req->first_data_chunk->req_offset;
        q_chunk->req_blocks = stripe_req->first_data_chunk->req_blocks;
    } else {
        p_chunk->req_offset = 0;
        p_chunk->req_blocks = raid_bdev->strip_size;
        q_chunk->req_offset = 0;
        q_chunk->req_blocks = raid_bdev->strip_size;
    }

    p_chunk->iov.iov_base = stripe_req->stripe->chunk_buffers[0];
    p_chunk->iov.iov_len = p_chunk->req_blocks << raid_bdev->blocklen_shift;
    q_chunk->iov.iov_base = stripe_req->stripe->chunk_buffers[1];
    q_chunk->iov.iov_len = q_chunk->req_blocks << raid_bdev->blocklen_shift;

    if (raid6_is_full_stripe_write(stripe_req)) {
        // full, skip d_cks.
        raid6_stripe_write_full_stripe(stripe_req);
        return;
    }

    if (total_degraded - data_degraded == 2) {
        // direct w/o PQ.
        raid6_stripe_write_submit(stripe_req);
        return;
    }

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

    if (preread_balance <= 0 && data_degraded == 0) {
        // we may reconstruct if no data chunk degraded.
        raid6_stripe_write_d_reconstructed_write(stripe_req);
        return;
    }

    if (req_degraded == 0) {
        // rmw misses d_ck
        raid6_stripe_write_d_rmw(stripe_req);
        return;
    }

    // write on d_chunk. W,WW,WR,WP,WQ
    if (data_degraded == 1) {
        // W,WP,WQ
        raid6_stripe_write_d_rcrmw(stripe_req);
    }
    else {
        //WW,WR
        raid6_stripe_write_d_rcrmw_pro(stripe_req);
    }
}

static void
raid6_stripe_read_normal(struct stripe_request *stripe_req)
{
    struct chunk *chunk;

    FOR_EACH_DATA_CHUNK(stripe_req, chunk) {
        if (chunk->req_blocks > 0) {
            raid6_submit_chunk_request(chunk, CHUNK_READ);
        }
    }
}

static void
raid6_compute_reconstruction_route(struct stripe_request *stripe_req, uint64_t tgt_offset, uint64_t tgt_blocks)
{
    struct raid_bdev_io *raid_io = stripe_req->raid_io;
    struct raid_bdev *raid_bdev = raid_io->raid_bdev;
    struct chunk *chunk;
    struct chunk *parent_chunk = NULL;
    struct chunk *d_chunk = stripe_req->degraded_chunks[0];
    struct chunk *d_chunk2 = stripe_req->degraded_chunks[1];
    enum stripe_degraded_type d_type = stripe_req->degraded_type;
    struct chunk *p_chunk = stripe_req->parity_chunk;
    struct chunk *q_chunk = stripe_req->q_chunk;
    uint8_t avail_devs = raid_bdev->num_base_rpcs;
    uint8_t leaves = avail_devs - 3;
    uint8_t i, parent_idx;
    if (d_type == DEGRADED_DP) {
        d_chunk2 = p_chunk;
    }
    if (d_type == DEGRADED_D) {
        d_chunk2 = q_chunk;
    }

    FOR_EACH_CHUNK(stripe_req, chunk) {
        chunk->tgt_offset_blocks = tgt_offset;
        chunk->tgt_num_blocks = tgt_blocks;
        chunk->dst_chunks[0] = NULL;
        chunk->dst_chunks[1] = NULL;
        chunk->fwd_num = 0;
    }

    uint8_t root_idx = rte_rand_max(avail_devs);
    while (d_chunk->index == root_idx || d_chunk2->index == root_idx) {
        root_idx = (root_idx + 1) % avail_devs;
    }
    uint8_t stop_idx = root_idx + raid_bdev->num_base_rpcs;
    for (i = root_idx; i < stop_idx; i++) {
        chunk = &stripe_req->chunks[i % avail_devs];
        if (chunk == d_chunk || chunk == d_chunk2) {
            continue;
        }
        if (parent_chunk == NULL) {
            parent_chunk = chunk;
            parent_idx = i % avail_devs;
        } else {
            chunk->dst_chunks[0] = parent_chunk;
            parent_chunk->fwd_num++;
        }
        if (parent_chunk->fwd_num == leaves) {
            parent_idx++;
            parent_chunk = &stripe_req->chunks[parent_idx % avail_devs];
            while (parent_chunk == d_chunk || parent_chunk == d_chunk2) {
                parent_idx++;
                parent_chunk = &stripe_req->chunks[parent_idx % avail_devs];
            }
        }
    }
}

static void
raid6_stripe_read_d_reconstructed_read(struct stripe_request *stripe_req, uint64_t offset, uint64_t blocks)
{
    struct chunk *chunk;
    struct chunk * d_chunk = stripe_req->degraded_chunks[0];
    struct chunk * d_chunk2 = stripe_req->degraded_chunks[1];
    struct chunk * p_chunk = stripe_req->parity_chunk;
    struct chunk * q_chunk = stripe_req->q_chunk;
    enum stripe_degraded_type d_type = stripe_req->degraded_type;
    int ret;

    raid6_compute_reconstruction_route(stripe_req, offset, blocks);

    if (stripe_req->degraded_type == DEGRADED_D) {
        FOR_EACH_CHUNK(stripe_req, chunk) {
            if (chunk != d_chunk && chunk != q_chunk) {
                if (chunk->req_blocks > 0) {
                    chunk->request_type = CHUNK_READ_RECON_D;
                } else {
                    chunk->request_type = CHUNK_RECON_D;
                }
            } else {
                chunk->request_type = NO_OP;
            }
        }
    } else if (stripe_req->degraded_type == DEGRADED_DP) {
        FOR_EACH_CHUNK(stripe_req, chunk) {
            if (chunk != d_chunk && chunk != p_chunk) {
                if (chunk->req_blocks > 0) {
                    chunk->request_type = CHUNK_READ_RECON_DP;
                } else {
                    if (chunk == q_chunk) {
                        chunk->request_type = CHUNK_RECON_DP_Q;
                    } else {
                        chunk->request_type = CHUNK_RECON_DP;
                    }
                }
            } else {
                chunk->request_type = NO_OP;
            }
        }
    } else {
        FOR_EACH_CHUNK(stripe_req, chunk) {
            if (chunk != d_chunk && chunk != d_chunk2) {
                if (chunk->req_blocks > 0) {
                    chunk->request_type = CHUNK_READ_RECON_DD;
                } else {
                    if (chunk == p_chunk) {
                        chunk->request_type = CHUNK_RECON_DD_P;
                    } else if (chunk == q_chunk) {
                        chunk->request_type = CHUNK_RECON_DD_Q;
                    } else {
                        chunk->request_type = CHUNK_RECON_DD;
                    }
                }
            } else {
                chunk->request_type = NO_OP;
            }
        }
    }

    FOR_EACH_CHUNK(stripe_req, chunk) {
        if (chunk->request_type != NO_OP) {
            raid6_submit_chunk_request_d_raid(chunk);
        }
    }
}

static void
raid6_stripe_read(struct stripe_request *stripe_req)
{
    struct chunk *chunk;
    int ret;
    struct chunk *p_chunk = stripe_req->parity_chunk;
    struct chunk *q_chunk = stripe_req->q_chunk;
    struct raid_bdev_io *raid_io = stripe_req->raid_io;
    struct raid_bdev *raid_bdev = raid_io->raid_bdev;
    struct spdk_bdev_io *bdev_io = spdk_bdev_io_from_ctx(raid_io);
    struct raid_base_rpc_info *base_info;
    uint8_t total_degraded = 0;
    uint8_t degraded_data_chunks = 0;
    enum stripe_degraded_type degraded_type;
    uint64_t len, req_offset, req_blocks;
    req_offset = 0;
    req_blocks = 0;

    FOR_EACH_CHUNK(stripe_req, chunk) {
        chunk->is_degraded = false;
    }
    stripe_req->degraded_chunks[0] = NULL;
    stripe_req->degraded_chunks[1] = NULL;

    // Note: check how many degraded devices are involved
    FOR_EACH_CHUNK(stripe_req, chunk) {
        base_info = &raid_bdev->base_rpc_info[chunk->index];
        if (base_info->degraded) {
            total_degraded++;
            chunk->is_degraded = true;
            if (chunk->req_blocks > 0) {
                if (degraded_data_chunks < 2) {
                    stripe_req->degraded_chunks[degraded_data_chunks] = chunk;
                }
                degraded_data_chunks++;
            }
        }
    }

    if (degraded_data_chunks > raid_bdev->module->base_rpcs_max_degraded) {
        raid6_abort_stripe_request(stripe_req, SPDK_BDEV_IO_STATUS_FAILED);
        return;
    } else if (degraded_data_chunks == raid_bdev->module->base_rpcs_max_degraded) {
        req_offset = spdk_min(stripe_req->degraded_chunks[0]->req_offset,
                              stripe_req->degraded_chunks[1]->req_offset);
        req_blocks = spdk_max(stripe_req->degraded_chunks[0]->req_offset + stripe_req->degraded_chunks[0]->req_blocks,
                              stripe_req->degraded_chunks[1]->req_offset + stripe_req->degraded_chunks[1]->req_blocks)
                     - req_offset;
        if (total_degraded > raid_bdev->module->base_rpcs_max_degraded) {
            raid6_abort_stripe_request(stripe_req, SPDK_BDEV_IO_STATUS_FAILED);
            return;
        } else {
            stripe_req->degraded_type = degraded_type = DEGRADED_DD;
        }
    } else if (degraded_data_chunks == 1) {
        req_offset = stripe_req->degraded_chunks[0]->req_offset;
        req_blocks = stripe_req->degraded_chunks[0]->req_blocks;
        if (total_degraded > raid_bdev->module->base_rpcs_max_degraded) {
            raid6_abort_stripe_request(stripe_req, SPDK_BDEV_IO_STATUS_FAILED);
            return;
        } else if (total_degraded == raid_bdev->module->base_rpcs_max_degraded) {
            if (p_chunk->is_degraded) {
                stripe_req->degraded_type = degraded_type = DEGRADED_DP;
            } else if (q_chunk->is_degraded) {
                stripe_req->degraded_type = degraded_type = DEGRADED_D;
            } else {
                stripe_req->degraded_type = degraded_type = DEGRADED_DD;
                FOR_EACH_DATA_CHUNK(stripe_req, chunk) {
                    if (chunk->is_degraded && chunk != stripe_req->degraded_chunks[0]) {
                        stripe_req->degraded_chunks[1] = chunk;
                    }
                }
            }
        } else {
            stripe_req->degraded_type = degraded_type = DEGRADED_D;
        }
    } else {
        stripe_req->degraded_type = degraded_type = NORMAL;
    }

    stripe_req->chunk_requests_complete_cb = raid6_complete_stripe_request;

    if (degraded_type == NORMAL) {
        raid6_stripe_read_normal(stripe_req);
    } else {
        raid6_stripe_read_d_reconstructed_read(stripe_req, req_offset, req_blocks);
    }
}

static void
raid6_submit_stripe_request(struct stripe_request *stripe_req)
{
    struct spdk_bdev_io *bdev_io = spdk_bdev_io_from_ctx(stripe_req->raid_io);
    struct chunk *chunk;
    int ret;

    FOR_EACH_DATA_CHUNK(stripe_req, chunk) {
        if (chunk->req_blocks > 0) {
            ret = raid6_chunk_map_req_data(chunk);
            if (ret) {
                raid6_abort_stripe_request(stripe_req, errno_to_status(ret));
                return;
            }
        }
    }

    if (bdev_io->type == SPDK_BDEV_IO_TYPE_READ) {
        raid6_stripe_read(stripe_req);
    } else if (bdev_io->type == SPDK_BDEV_IO_TYPE_WRITE) {
        raid6_stripe_write(stripe_req);
    } else {
        assert(false);
    }
}

static void
raid6_handle_stripe(struct raid_bdev_io *raid_io, struct stripe *stripe,
                    uint64_t stripe_offset, uint64_t blocks, uint64_t iov_offset)
{
    struct spdk_bdev_io *bdev_io = spdk_bdev_io_from_ctx(raid_io);
    struct raid_bdev *raid_bdev = raid_io->raid_bdev;
    struct raid6_info *r6info = (struct raid6_info*) raid_bdev->module_private;
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
            raid6_handle_stripe(raid_io, stripe, stripe_offset,
                                blocks_limit, iov_offset);
            blocks -= blocks_limit;
            stripe_offset += blocks_limit;
            iov_offset += blocks_limit << raid_bdev->blocklen_shift;
        }
    }

    stripe_req = (struct stripe_request*) spdk_mempool_get(r6info->stripe_request_mempool);
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
    uint8_t p_idx = raid_bdev->num_base_rpcs - 1 - (stripe->index % raid_bdev->num_base_rpcs);
    uint8_t q_idx = (p_idx + 1) % raid_bdev->num_base_rpcs;
    stripe_req->parity_chunk = &stripe_req->chunks[p_idx];
    stripe_req->q_chunk = &stripe_req->chunks[q_idx];

    stripe_offset_from = stripe_offset;
    stripe_offset_to = stripe_offset_from + blocks;
    first_chunk_data_idx = stripe_offset_from >> raid_bdev->strip_size_shift;
    last_chunk_data_idx = (stripe_offset_to - 1) >> raid_bdev->strip_size_shift;

    stripe_req->first_data_chunk = raid6_get_data_chunk(stripe_req, first_chunk_data_idx);
    stripe_req->last_data_chunk = raid6_get_data_chunk(stripe_req, last_chunk_data_idx);

    FOR_EACH_CHUNK(stripe_req, chunk) {
        chunk->index = chunk - stripe_req->chunks;
        chunk->iovs = &chunk->iov;
        chunk->iovcnt = 1;

        if (chunk == stripe_req->parity_chunk ||
            chunk == stripe_req->q_chunk ||
            chunk < stripe_req->first_data_chunk ||
            chunk > stripe_req->last_data_chunk) {
            chunk->req_offset = 0;
            chunk->req_blocks = 0;
        } else {
            uint64_t chunk_offset_from = raid6_chunk_data_index(chunk) << raid_bdev->strip_size_shift;
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
        raid6_submit_stripe_request(stripe_req);
    }
}

static int
raid6_reclaim_stripes(struct raid6_info *r6info)
{
    struct stripe *stripe, *tmp;
    int reclaimed = 0;
    int ret;
    int to_reclaim = (RAID_MAX_STRIPES >> 3) - RAID_MAX_STRIPES +
                     rte_hash_count(r6info->active_stripes_hash);

    TAILQ_FOREACH_REVERSE_SAFE(stripe, &r6info->active_stripes, active_stripes_head, link, tmp) {
        if (__atomic_load_n(&stripe->refs, __ATOMIC_SEQ_CST) > 0) {
            continue;
        }

        TAILQ_REMOVE(&r6info->active_stripes, stripe, link);
        TAILQ_INSERT_TAIL(&r6info->free_stripes, stripe, link);

        ret = rte_hash_del_key_with_hash(r6info->active_stripes_hash,
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
raid6_get_stripe(struct raid6_info *r6info, uint64_t stripe_index)
{
    struct stripe *stripe;
    hash_sig_t hash;
    int ret;

    hash = rte_hash_hash(r6info->active_stripes_hash, &stripe_index);

    pthread_spin_lock(&r6info->active_stripes_lock);
    ret = rte_hash_lookup_with_hash_data(r6info->active_stripes_hash,
                                         &stripe_index, hash, (void **)&stripe);
    if (ret == -ENOENT) {
        stripe = TAILQ_FIRST(&r6info->free_stripes);
        if (!stripe) {
            if (raid6_reclaim_stripes(r6info) > 0) {
                stripe = TAILQ_FIRST(&r6info->free_stripes);
                assert(stripe != NULL);
            } else {
                pthread_spin_unlock(&r6info->active_stripes_lock);
                return NULL;
            }
        }
        TAILQ_REMOVE(&r6info->free_stripes, stripe, link);

        stripe->index = stripe_index;
        stripe->hash = hash;

        ret = rte_hash_add_key_with_hash_data(r6info->active_stripes_hash,
                                              &stripe_index, hash, stripe);
        if (spdk_unlikely(ret < 0)) {
            assert(false);
        }
    } else {
        TAILQ_REMOVE(&r6info->active_stripes, stripe, link);
    }
    TAILQ_INSERT_HEAD(&r6info->active_stripes, stripe, link);

    __atomic_fetch_add(&stripe->refs, 1, __ATOMIC_SEQ_CST);

    pthread_spin_unlock(&r6info->active_stripes_lock);

    return stripe;
}

static void
raid6_submit_rw_request(struct raid_bdev_io *raid_io);

static void
_raid6_submit_rw_request(void *_raid_io)
{
    struct raid_bdev_io *raid_io = (struct raid_bdev_io*) _raid_io;

    raid6_submit_rw_request(raid_io);
}

void
raid6_complete_chunk_request_read(void *ctx)
{
    struct send_wr_wrapper *send_wrapper = static_cast<struct send_wr_wrapper*>(ctx);
    struct raid_bdev_io *raid_io = (struct raid_bdev_io *) send_wrapper->ctx;

    raid_bdev_io_complete_part(raid_io, send_wrapper->num_blocks, SPDK_BDEV_IO_STATUS_SUCCESS);
}

static int
raid6_map_iov(struct sg_entry *sgl, const struct iovec *iov, int iovcnt,
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
raid6_dispatch_to_rpc_read(struct raid_bdev_io *raid_io, uint8_t chunk_idx, uint64_t base_offset_blocks,
                           uint64_t num_blocks, uint64_t chunk_req_blocks, struct iovec *iov,
                           int total_iovcnt, uint64_t iov_offset, uint64_t chunk_len)
{
    struct raid6_io_channel *r6ch = (struct raid6_io_channel*) raid_bdev_io_channel_get_resource(raid_io->raid_ch);
    struct raid_bdev *raid_bdev = raid_io->raid_bdev;
    struct raid_bdev_io_channel *raid_ch = raid_io->raid_ch;
    struct spdk_mem_map *mem_map = raid_ch->qp_group->qps[chunk_idx]->mem_map;

    struct send_wr_wrapper *send_wrapper = raid_get_send_wrapper(raid_io, chunk_idx);
    struct ibv_send_wr *send_wr = &send_wrapper->send_wr;
    struct ibv_sge *sge = send_wr->sg_list;
    struct cs_message_t *cs_msg = (struct cs_message_t *) sge->addr;

    int chunk_iovcnt = raid6_map_iov(cs_msg->sgl, iov, total_iovcnt, iov_offset, chunk_len);

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
    send_wrapper->cb = raid6_complete_chunk_request_read;
    send_wrapper->ctx = (void *) raid_io;
    send_wrapper->num_blocks = chunk_req_blocks;

    int ret = raid_dispatch_to_rpc(send_wrapper);

    return ret;
}

static void
raid6_handle_read(struct raid_bdev_io *raid_io, uint64_t stripe_index, uint64_t stripe_offset, uint64_t blocks)
{
    struct raid_bdev *raid_bdev = raid_io->raid_bdev;
    struct raid6_io_channel *r6ch = (struct raid6_io_channel*) raid_bdev_io_channel_get_resource(raid_io->raid_ch);
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

    uint8_t p_chunk_idx = raid_bdev->num_base_rpcs - 1 - (stripe_index % raid_bdev->num_base_rpcs);
    uint8_t q_chunk_idx = (p_chunk_idx + 1) % raid_bdev->num_base_rpcs;

    if (q_chunk_idx < p_chunk_idx) {
        first_chunk_idx++;
        last_chunk_idx++;
    } else {
        if (first_chunk_idx >= p_chunk_idx) {
            first_chunk_idx += 2;
        }
        if (last_chunk_idx >= p_chunk_idx) {
            last_chunk_idx += 2;
        }
    }

    for (uint8_t i = 0; i < raid_bdev->num_base_rpcs; i++) {
        if (i == p_chunk_idx || i == q_chunk_idx || i < first_chunk_idx || i > last_chunk_idx) {
            continue;
        } else {
            chunk_offset_from = i;
            if (i > p_chunk_idx) {
                chunk_offset_from--;
            }
            if (i > q_chunk_idx) {
                chunk_offset_from--;
            }
            chunk_offset_from = chunk_offset_from << raid_bdev->strip_size_shift;
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

            ret = raid6_dispatch_to_rpc_read(raid_io, i, base_offset_blocks, chunk_req_blocks, chunk_req_blocks, iov, iovcnt, iov_offset, chunk_len);
            iov_offset += chunk_len;

            if (spdk_unlikely(ret != 0)) {
                SPDK_ERRLOG("bdev io submit error not due to ENOMEM, it should not happen\n");
                assert(false);
            }
        }
    }

}

static void
raid6_submit_rw_request(struct raid_bdev_io *raid_io)
{
    struct raid_bdev *raid_bdev = raid_io->raid_bdev;
    struct spdk_bdev_io *bdev_io = spdk_bdev_io_from_ctx(raid_io);
    struct raid6_info *r6info = (struct raid6_info*) raid_io->raid_bdev->module_private;
    uint64_t offset_blocks = bdev_io->u.bdev.offset_blocks;
    uint64_t num_blocks = bdev_io->u.bdev.num_blocks;
    uint64_t stripe_index = offset_blocks / r6info->stripe_blocks;
    uint64_t stripe_offset = offset_blocks % r6info->stripe_blocks;
    struct stripe *stripe;

    if (!raid_bdev->degraded && bdev_io->type == SPDK_BDEV_IO_TYPE_READ) {
        raid_io->base_bdev_io_remaining = num_blocks;
        raid6_handle_read(raid_io, stripe_index, stripe_offset, num_blocks);
        return;
    }

    stripe = raid6_get_stripe(r6info, stripe_index);
    if (spdk_unlikely(stripe == NULL)) {
        struct raid6_io_channel *r6ch = (struct raid6_io_channel*) raid_bdev_io_channel_get_resource(raid_io->raid_ch);
        struct spdk_bdev_io_wait_entry *wqe = &raid_io->waitq_entry;

        wqe->cb_fn = _raid6_submit_rw_request;
        wqe->cb_arg = raid_io;
        TAILQ_INSERT_TAIL(&r6ch->retry_queue, wqe, link);
        return;
    }

    raid_io->base_bdev_io_remaining = num_blocks;

    raid6_handle_stripe(raid_io, stripe, stripe_offset, num_blocks, 0);
}

static int
raid6_stripe_init(struct stripe *stripe, struct raid_bdev *raid_bdev)
{
    int i;

    stripe->chunk_buffers = (void**) calloc(3, sizeof(void *));
    if (!stripe->chunk_buffers) {
        SPDK_ERRLOG("Failed to allocate chunk buffers array\n");
        return -ENOMEM;
    }

    for (i = 0; i < 3; i++) {
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
raid6_stripe_deinit(struct stripe *stripe, struct raid_bdev *raid_bdev)
{
    uint8_t i;

    for (i = 0; i < 3; i++) {
        spdk_dma_free(stripe->chunk_buffers[i]);
    }
    free(stripe->chunk_buffers);

    pthread_spin_destroy(&stripe->requests_lock);
}

static void
raid6_free(struct raid6_info *r6info)
{
    unsigned int i;

    pthread_spin_destroy(&r6info->active_stripes_lock);

    if (r6info->active_stripes_hash) {
        rte_hash_free(r6info->active_stripes_hash);
    }

    if (r6info->stripe_request_mempool) {
        spdk_mempool_free(r6info->stripe_request_mempool);
    }

    if (r6info->stripes) {
        for (i = 0; i < RAID_MAX_STRIPES; i++) {
            raid6_stripe_deinit(&r6info->stripes[i], r6info->raid_bdev);
        }
        free(r6info->stripes);
    }

    free(r6info);
}

static int
raid6_start(struct raid_bdev *raid_bdev)
{
    uint64_t min_blockcnt = UINT64_MAX;
    struct raid_base_rpc_info *base_info;
    struct raid6_info *r6info;
    char name_buf[32];
    struct rte_hash_parameters hash_params = { 0 };
    unsigned int i;
    int ret = 0;

    r6info = (struct raid6_info*) calloc(1, sizeof(*r6info));
    if (!r6info) {
        SPDK_ERRLOG("Failed to allocate r6info\n");
        return -ENOMEM;
    }

    for (int i = 0; i < 256; i++) {
        r6info->gf_const_tbl_arr[i] = (unsigned char *) calloc(32, sizeof(unsigned char));
        r6info->gf_const_tbl_arr_a[i] = (unsigned char *) calloc(32, sizeof(unsigned char));
        if (!r6info->gf_const_tbl_arr[i] || !r6info->gf_const_tbl_arr_a[i]) {
            SPDK_ERRLOG("Failed to allocate gf_const_tbl_arr\n");
            return -ENOMEM;
        }
        for (int j = 0; j < 256; j++) {
            r6info->gf_const_tbl_arr_b[i][j] = (unsigned char *) calloc(32, sizeof(unsigned char));
            if (!r6info->gf_const_tbl_arr_b[i][j]) {
                SPDK_ERRLOG("Failed to allocate gf_const_tbl_arr\n");
                return -ENOMEM;
            }
        }
    }

    for (int i = 0; i < 256; i++) {
        unsigned char c = 1;
        unsigned char a;
        for (int j = 0; j < i; j++) {
            c = gf_mul(c, 2);
        }
        a = gf_mul(c, gf_inv(c ^ 1));
        for (int l = 0; l < 256; l++) {
            unsigned char c1 = 1;
            unsigned char b;
            for (int m = 0; m < l; m++) {
                c1 = gf_mul(c1, 2);
            }
            b = gf_mul(c1, gf_inv(c ^ 1));
            gf_vect_mul_init(b, r6info->gf_const_tbl_arr_b[i][l]);
        }
        gf_vect_mul_init(c, r6info->gf_const_tbl_arr[i]);
        gf_vect_mul_init(a, r6info->gf_const_tbl_arr_a[i]);
    }

    r6info->raid_bdev = raid_bdev;

    pthread_spin_init(&r6info->active_stripes_lock, PTHREAD_PROCESS_PRIVATE);

    RAID_FOR_EACH_BASE_RPC(raid_bdev, base_info) {
        min_blockcnt = spdk_min(min_blockcnt, base_info->blockcnt);
    }

    r6info->total_stripes = min_blockcnt / raid_bdev->strip_size;
    r6info->stripe_blocks = raid_bdev->strip_size * raid6_stripe_data_chunks_num(raid_bdev);

    raid_bdev->bdev.blockcnt = r6info->stripe_blocks * r6info->total_stripes;
    SPDK_NOTICELOG("raid blockcnt %" PRIu64 "\n", raid_bdev->bdev.blockcnt);
    raid_bdev->bdev.optimal_io_boundary = r6info->stripe_blocks;
    raid_bdev->bdev.split_on_optimal_io_boundary = true;

    r6info->stripes = (struct stripe*) calloc(RAID_MAX_STRIPES, sizeof(*r6info->stripes));
    if (!r6info->stripes) {
        SPDK_ERRLOG("Failed to allocate stripes array\n");
        ret = -ENOMEM;
        goto out;
    }

    TAILQ_INIT(&r6info->free_stripes);

    for (i = 0; i < RAID_MAX_STRIPES; i++) {
        struct stripe *stripe = &r6info->stripes[i];

        ret = raid6_stripe_init(stripe, raid_bdev);
        if (ret) {
            for (; i > 0; --i) {
                raid6_stripe_deinit(&r6info->stripes[i], raid_bdev);
            }
            free(r6info->stripes);
            r6info->stripes = NULL;
            goto out;
        }

        TAILQ_INSERT_TAIL(&r6info->free_stripes, stripe, link);
    }

    snprintf(name_buf, sizeof(name_buf), "raid6_sreq_%p", raid_bdev);

    r6info->stripe_request_mempool = spdk_mempool_create(name_buf,
                                                         RAID_MAX_STRIPES * 4,
                                                         sizeof(struct stripe_request) + sizeof(struct chunk) * raid_bdev->num_base_rpcs,
                                                         SPDK_MEMPOOL_DEFAULT_CACHE_SIZE,
                                                         SPDK_ENV_SOCKET_ID_ANY);
    if (!r6info->stripe_request_mempool) {
        SPDK_ERRLOG("Failed to allocate stripe_request_mempool\n");
        ret = -ENOMEM;
        goto out;
    }

    snprintf(name_buf, sizeof(name_buf), "raid6_hash_%p", raid_bdev);

    hash_params.name = name_buf;
    hash_params.entries = RAID_MAX_STRIPES * 2;
    hash_params.key_len = sizeof(uint64_t);

    r6info->active_stripes_hash = rte_hash_create(&hash_params);
    if (!r6info->active_stripes_hash) {
        SPDK_ERRLOG("Failed to allocate active_stripes_hash\n");
        ret = -ENOMEM;
        goto out;
    }

    TAILQ_INIT(&r6info->active_stripes);

    raid_bdev->module_private = r6info;
    out:
    if (ret) {
        raid6_free(r6info);
    }
    return ret;
}

static void
raid6_stop(struct raid_bdev *raid_bdev)
{
    struct raid6_info *r6info = (struct raid6_info*) raid_bdev->module_private;

    raid6_free(r6info);
}

static int
raid6_io_channel_resource_init(struct raid_bdev *raid_bdev, void *resource)
{
    struct raid6_io_channel *r6ch = (struct raid6_io_channel*) resource;

    TAILQ_INIT(&r6ch->retry_queue);

    return 0;
}

static void
raid6_io_channel_resource_deinit(void *resource)
{
    struct raid6_io_channel *r6ch = (struct raid6_io_channel*) resource;

    assert(TAILQ_EMPTY(&r6ch->retry_queue));
}

static struct raid_bdev_module g_raid6_module = {
        .level = RAID6,
        .base_rpcs_min = 4,
        .base_rpcs_max_degraded = 2,
        .io_channel_resource_size = sizeof(struct raid6_io_channel),
        .start = raid6_start,
        .stop = raid6_stop,
        .submit_rw_request = raid6_submit_rw_request,
        .io_channel_resource_init = raid6_io_channel_resource_init,
        .io_channel_resource_deinit = raid6_io_channel_resource_deinit,
};
RAID_MODULE_REGISTER(&g_raid6_module)

SPDK_LOG_REGISTER_COMPONENT(bdev_raid6)
