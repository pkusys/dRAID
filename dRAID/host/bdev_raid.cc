#include "bdev_raid.h"
#include "spdk/env.h"
#include "spdk/thread.h"
#include "spdk/log.h"
#include "spdk/string.h"
#include "spdk/likely.h"
#include "spdk/util.h"
#include "spdk/json.h"
#include "../shared/common.h"

/* raid bdev config as read from config file */
struct raid_config	g_raid_config = {
        .raid_bdev_config_head = TAILQ_HEAD_INITIALIZER(g_raid_config.raid_bdev_config_head),
};

/* List of all raid bdevs */
struct raid_all_tailq		g_raid_bdev_list = TAILQ_HEAD_INITIALIZER(g_raid_bdev_list);

static TAILQ_HEAD(, raid_bdev_module) g_raid_modules = TAILQ_HEAD_INITIALIZER(g_raid_modules);

void
raid_memset_iovs(struct iovec *iovs, int iovcnt, char c)
{
    struct iovec *iov;

    for (iov = iovs; iov < iovs + iovcnt; iov++) {
        memset(iov->iov_base, c, iov->iov_len);
    }
}

void
raid_memcpy_iovs(struct iovec *iovs_dest, int iovs_dest_cnt, size_t iovs_dest_offset,
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

        memcpy((char*)v1->iov_base + off1, (char*)v2->iov_base + off2, n); // TODO: replace memcpy

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

static struct raid_bdev_module *raid_bdev_module_find(enum raid_level level)
{
    struct raid_bdev_module *raid_module;

    TAILQ_FOREACH(raid_module, &g_raid_modules, link) {
        if (raid_module->level == level) {
            return raid_module;
        }
    }

    return NULL;
}

void raid_bdev_module_list_add(struct raid_bdev_module *raid_module)
{
    if (raid_bdev_module_find(raid_module->level) != NULL) {
        SPDK_ERRLOG("module for raid level '%s' already registered.\n",
                    raid_bdev_level_to_str(raid_module->level));
        assert(false);
    } else {
        TAILQ_INSERT_TAIL(&g_raid_modules, raid_module, link);
    }
}

/* Function declarations */
static void	raid_bdev_examine(struct spdk_bdev *bdev);
static int	raid_bdev_init(void);

static struct {
    const char *name;
    enum raid_level value;
} g_raid_level_names[] = {
        { "raid5", RAID5 },
        { "5", RAID5 },
        { "raid6", RAID6 },
        { "6", RAID6 },
        { }
};

static void
req_handler_rdma(struct cs_resp_t *cs_resp) {
    struct send_wr_wrapper *send_wrapper = (struct send_wr_wrapper *) cs_resp->req_id;

    if (--send_wrapper->rtn_cnt == 0) {
        send_wrapper->cb((void *) send_wrapper);
        TAILQ_INSERT_TAIL(&send_wrapper->rqp->free_send, send_wrapper, link);
    }
}

static int
raid_poll_qp(void *arg)
{
    struct ibv_recv_wr *recv_wr;
    struct ibv_recv_wr *bad_recv_wr;
    struct ibv_send_wr *bad_send_wr;
    struct ibv_wc wc[batch_size];
    struct cs_resp_t *cs_resp;
    struct rdma_qp *rqp;
    uint64_t id;
    int num_cqe;
    uint8_t i;
    struct send_wr_wrapper *send_wrapper;
    struct recv_wr_wrapper *recv_wrapper;

    struct raid_bdev_io_channel *raid_ch = (struct raid_bdev_io_channel*) arg;
    uint8_t num_rpcs = raid_ch->num_rpcs;
    int total_cqe = 0;

    do {
        num_cqe = ibv_poll_cq(raid_ch->qp_group->cq, batch_size, wc);
        if (num_cqe == 0) {
            break;
        }
        total_cqe += num_cqe;
        for (i = 0; i < num_cqe; i++) {
            if (spdk_unlikely(wc[i].status != IBV_WC_SUCCESS)) {
                SPDK_ERRLOG("wc status error status=%u, opcode=%u\n", wc[i].status, wc[i].opcode);
                assert(false);
            }
            id = wc[i].wr_id;
            if (wc[i].opcode == IBV_WC_SEND) {
                send_wrapper = (struct send_wr_wrapper *) id;
                void_cont_func_rdma(send_wrapper);
                continue;
            }
            if (spdk_unlikely(wc[i].opcode != IBV_WC_RECV)) {
                SPDK_ERRLOG("wc opcode error\n");
                assert(false);
            }
            recv_wrapper = (struct recv_wr_wrapper *) id;
            recv_wr = &recv_wrapper->recv_wr;
            rqp = recv_wrapper->rqp;
            cs_resp = (struct cs_resp_t *) ((uint64_t) recv_wr->sg_list->addr);
            req_handler_rdma(cs_resp);
            recv_wr->next = NULL;
            if (rqp->recv_wrs.first == NULL) {
                rqp->recv_wrs.first = recv_wr;
                rqp->recv_wrs.last = recv_wr;
            } else {
                rqp->recv_wrs.last->next = recv_wr;
                rqp->recv_wrs.last = recv_wr;
            }
        }
    } while (total_cqe < batch_size);

    for (i = 0; i < num_rpcs; i++) {

        rqp = raid_ch->qp_group->qps[i];

        if (spdk_unlikely(ibv_post_send(rqp->cmd_cm_id->qp, rqp->send_wrs.first, &bad_send_wr))) {
            SPDK_ERRLOG("post send failed\n");
            assert(false);
        }
        rqp->send_wrs.first = NULL;
        if (ibv_post_recv(rqp->cmd_cm_id->qp, rqp->recv_wrs.first, &bad_recv_wr)) {
            SPDK_ERRLOG("post recv failed\n");
            assert(false);
        }
        rqp->recv_wrs.first = NULL;
    }

    return total_cqe > 0 ? SPDK_POLLER_BUSY : SPDK_POLLER_IDLE;
}

/*
 * brief:
 * raid_bdev_create_cb function is a cb function for raid bdev which creates the
 * hierarchy from raid bdev to base bdev io channels. It will be called per core
 * params:
 * io_device - pointer to raid bdev io device represented by raid_bdev
 * ctx_buf - pointer to context buffer for raid bdev io channel
 * returns:
 * 0 - success
 * non zero - failure
 */
static int
raid_bdev_create_cb(void *io_device, void *ctx_buf)
{
    struct raid_bdev            *raid_bdev = (struct raid_bdev*) io_device;
    struct raid_bdev_io_channel *raid_ch = (struct raid_bdev_io_channel*) ctx_buf;
    uint8_t i;
    int ret = 0;

    SPDK_DEBUGLOG(bdev_raid, "raid_bdev_create_cb, %p\n", raid_ch);

    assert(raid_bdev != NULL);

    raid_ch->num_rpcs = raid_bdev->num_base_rpcs;

    pthread_spin_lock(&raid_bdev->qp_group_lock);
    raid_ch->qp_group = TAILQ_FIRST(&raid_bdev->qp_groups);
    assert(raid_ch->qp_group != NULL);
    TAILQ_REMOVE(&raid_bdev->qp_groups, raid_ch->qp_group, link);
    pthread_spin_unlock(&raid_bdev->qp_group_lock);

    if (!ret && raid_bdev->module->io_channel_resource_init) {
        ret = raid_bdev->module->io_channel_resource_init(raid_bdev,
                                                          raid_bdev_io_channel_get_resource(raid_ch));
    }

    raid_ch->poller = SPDK_POLLER_REGISTER(raid_poll_qp, raid_ch, 0);

    return ret;
}

/*
 * brief:
 * raid_bdev_destroy_cb function is a cb function for raid bdev which deletes the
 * hierarchy from raid bdev to base bdev io channels. It will be called per core
 * params:
 * io_device - pointer to raid bdev io device represented by raid_bdev
 * ctx_buf - pointer to context buffer for raid bdev io channel
 * returns:
 * none
 */
static void
raid_bdev_destroy_cb(void *io_device, void *ctx_buf)
{
    struct raid_bdev            *raid_bdev = (struct raid_bdev*) io_device;
    struct raid_bdev_io_channel *raid_ch = (struct raid_bdev_io_channel*) ctx_buf;
    uint8_t i;

    SPDK_DEBUGLOG(bdev_raid, "raid_bdev_destroy_cb\n");

    assert(raid_ch != NULL);

    spdk_poller_unregister(&raid_ch->poller);

    if (raid_bdev->module->io_channel_resource_deinit) {
        raid_bdev->module->io_channel_resource_deinit(raid_bdev_io_channel_get_resource(raid_ch));
    }

    assert(raid_ch->qp_group != NULL);
    pthread_spin_lock(&raid_bdev->qp_group_lock);
    TAILQ_INSERT_TAIL(&raid_bdev->qp_groups, raid_ch->qp_group, link);
    pthread_spin_unlock(&raid_bdev->qp_group_lock);

    raid_ch->qp_group = NULL;
}

/*
 * brief:
 * raid_bdev_destruct is the destruct function table pointer for raid bdev
 * params:
 * ctxt - pointer to raid_bdev
 * returns:
 * 0 - success
 * non zero - failure
 */
static int
raid_bdev_destruct(void *ctxt)
{
    SPDK_DEBUGLOG(bdev_raid, "raid_bdev_destruct\n");
    return 0;
}

void
raid_bdev_io_complete(struct raid_bdev_io *raid_io, enum spdk_bdev_io_status status)
{
    struct spdk_bdev_io *bdev_io = spdk_bdev_io_from_ctx(raid_io);

    spdk_bdev_io_complete(bdev_io, status);
}

/*
 * brief:
 * raid_bdev_io_complete_part - signal the completion of a part of the expected
 * base bdev IOs and complete the raid_io if this is the final expected IO.
 * The caller should first set raid_io->base_bdev_io_remaining. This function
 * will decrement this counter by the value of the 'completed' parameter and
 * complete the raid_io if the counter reaches 0. The caller is free to
 * interpret the 'base_bdev_io_remaining' and 'completed' values as needed,
 * it can represent e.g. blocks or IOs.
 * params:
 * raid_io - pointer to raid_bdev_io
 * completed - the part of the raid_io that has been completed
 * status - status of the base IO
 * returns:
 * true - if the raid_io is completed
 * false - otherwise
 */
bool
raid_bdev_io_complete_part(struct raid_bdev_io *raid_io, uint64_t completed,
                           enum spdk_bdev_io_status status)
{
    assert(raid_io->base_bdev_io_remaining >= completed);
    raid_io->base_bdev_io_remaining -= completed;

    if (status != SPDK_BDEV_IO_STATUS_SUCCESS) {
        raid_io->base_bdev_io_status = status;
    }

    if (raid_io->base_bdev_io_remaining == 0) {
        raid_bdev_io_complete(raid_io, raid_io->base_bdev_io_status);
        return true;
    } else {
        return false;
    }
}

/*
 * brief:
 * Callback function to spdk_bdev_io_get_buf.
 * params:
 * ch - pointer to raid bdev io channel
 * bdev_io - pointer to parent bdev_io on raid bdev device
 * success - True if buffer is allocated or false otherwise.
 * returns:
 * none
 */
static void
raid_bdev_get_buf_cb(struct spdk_io_channel *ch, struct spdk_bdev_io *bdev_io,
                     bool success)
{
    struct raid_bdev_io *raid_io = (struct raid_bdev_io *)bdev_io->driver_ctx;

    if (!success) {
        raid_bdev_io_complete(raid_io, SPDK_BDEV_IO_STATUS_FAILED);
        return;
    }

    raid_io->raid_bdev->module->submit_rw_request(raid_io);
}

/*
 * brief:
 * raid_bdev_submit_request function is the submit_request function pointer of
 * raid bdev function table. This is used to submit the io on raid_bdev to below
 * layers.
 * params:
 * ch - pointer to raid bdev io channel
 * bdev_io - pointer to parent bdev_io on raid bdev device
 * returns:
 * none
 */
static void
raid_bdev_submit_request(struct spdk_io_channel *ch, struct spdk_bdev_io *bdev_io)
{
    struct raid_bdev_io *raid_io = (struct raid_bdev_io *)bdev_io->driver_ctx;

    raid_io->raid_bdev = (struct raid_bdev*) bdev_io->bdev->ctxt;
    raid_io->raid_ch = (struct raid_bdev_io_channel*) spdk_io_channel_get_ctx(ch);
    raid_io->base_bdev_io_remaining = 0;
    raid_io->base_bdev_io_submitted = 0;
    raid_io->base_bdev_io_status = SPDK_BDEV_IO_STATUS_SUCCESS;

    switch (bdev_io->type) {
        case SPDK_BDEV_IO_TYPE_READ:
            spdk_bdev_io_get_buf(bdev_io, raid_bdev_get_buf_cb,
                                 bdev_io->u.bdev.num_blocks << raid_io->raid_bdev->blocklen_shift);
            break;
        case SPDK_BDEV_IO_TYPE_WRITE:
            raid_io->raid_bdev->module->submit_rw_request(raid_io);
            break;

        case SPDK_BDEV_IO_TYPE_RESET:
        case SPDK_BDEV_IO_TYPE_FLUSH:
        case SPDK_BDEV_IO_TYPE_UNMAP:
            raid_io->raid_bdev->module->submit_null_payload_request(raid_io);
            break;

        default:
            SPDK_ERRLOG("submit request, invalid io type %u\n", bdev_io->type);
            raid_bdev_io_complete(raid_io, SPDK_BDEV_IO_STATUS_FAILED);
            break;
    }
}

/*
 * brief:
 * raid_bdev_io_type_supported is the io_supported function for bdev function
 * table which returns whether the particular io type is supported or not by
 * raid bdev module
 * params:
 * ctx - pointer to raid bdev context
 * type - io type
 * returns:
 * true - io_type is supported
 * false - io_type is not supported
 */
static bool
raid_bdev_io_type_supported(void *ctx, enum spdk_bdev_io_type io_type)
{
    switch (io_type) {
        case SPDK_BDEV_IO_TYPE_READ:
        case SPDK_BDEV_IO_TYPE_WRITE:
            return true;

        case SPDK_BDEV_IO_TYPE_FLUSH:
        case SPDK_BDEV_IO_TYPE_RESET:
        case SPDK_BDEV_IO_TYPE_UNMAP:
            return false;

        default:
            return false;
    }

    return false;
}

/*
 * brief:
 * raid_bdev_get_io_channel is the get_io_channel function table pointer for
 * raid bdev. This is used to return the io channel for this raid bdev
 * params:
 * ctxt - pointer to raid_bdev
 * returns:
 * pointer to io channel for raid bdev
 */
static struct spdk_io_channel *
raid_bdev_get_io_channel(void *ctxt)
{
    struct raid_bdev *raid_bdev = (struct raid_bdev*) ctxt;

    return spdk_get_io_channel(raid_bdev);
}

/*
 * brief:
 * raid_bdev_dump_info_json is the function table pointer for raid bdev
 * params:
 * ctx - pointer to raid_bdev
 * w - pointer to json context
 * returns:
 * 0 - success
 * non zero - failure
 */
static int
raid_bdev_dump_info_json(void *ctx, struct spdk_json_write_ctx *w)
{
    struct raid_bdev *raid_bdev = (struct raid_bdev*) ctx;
    struct raid_base_bdev_info *base_info;

    SPDK_DEBUGLOG(bdev_raid, "raid_bdev_dump_config_json\n");
    assert(raid_bdev != NULL);

    /* Dump the raid bdev configuration related information */
    spdk_json_write_named_object_begin(w, "raid");
    spdk_json_write_named_uint32(w, "strip_size_kb", raid_bdev->strip_size_kb);
    spdk_json_write_named_string(w, "raid_level", raid_bdev_level_to_str(raid_bdev->level));
    spdk_json_write_named_uint32(w, "num_base_rpcs", raid_bdev->num_base_rpcs);
    spdk_json_write_named_uint32(w, "num_base_rpcs_discovered", raid_bdev->num_base_rpcs_discovered);
    spdk_json_write_object_end(w);

    return 0;
}

/*
 * brief:
 * raid_bdev_write_config_json is the function table pointer for raid bdev
 * params:
 * bdev - pointer to spdk_bdev
 * w - pointer to json context
 * returns:
 * none
 */
static void
raid_bdev_write_config_json(struct spdk_bdev *bdev, struct spdk_json_write_ctx *w)
{
    struct raid_bdev *raid_bdev = (struct raid_bdev*) bdev->ctxt;
    struct raid_base_rpc_info *base_info;

    spdk_json_write_object_begin(w);

    spdk_json_write_named_string(w, "method", "bdev_raid_create");

    spdk_json_write_named_object_begin(w, "params");
    spdk_json_write_named_string(w, "name", bdev->name);
    spdk_json_write_named_uint32(w, "strip_size_kb", raid_bdev->strip_size_kb);
    spdk_json_write_named_string(w, "raid_level", raid_bdev_level_to_str(raid_bdev->level));

    spdk_json_write_named_array_begin(w, "base_rpcs");
    RAID_FOR_EACH_BASE_RPC(raid_bdev, base_info) {
        spdk_json_write_object_begin(w);
        spdk_json_write_named_string(w, "uri", base_info->uri);
        spdk_json_write_object_end(w);
    }
    spdk_json_write_array_end(w);
    spdk_json_write_object_end(w);

    spdk_json_write_object_end(w);
}

/* g_raid_bdev_fn_table is the function table for raid bdev */
static const struct spdk_bdev_fn_table g_raid_bdev_fn_table = {
        .destruct		= raid_bdev_destruct,
        .submit_request		= raid_bdev_submit_request,
        .io_type_supported	= raid_bdev_io_type_supported,
        .get_io_channel		= raid_bdev_get_io_channel,
        .dump_info_json		= raid_bdev_dump_info_json,
        .write_config_json	= raid_bdev_write_config_json,
};

/* brief
 * raid_bdev_config_find_by_name is a helper function to find raid bdev config
 * by name as key.
 *
 * params:
 * raid_name - name for raid bdev.
 */
struct raid_bdev_config *
raid_bdev_config_find_by_name(const char *raid_name)
{
    struct raid_bdev_config *raid_cfg;

    TAILQ_FOREACH(raid_cfg, &g_raid_config.raid_bdev_config_head, link) {
        if (!strcmp(raid_cfg->name, raid_name)) {
            return raid_cfg;
        }
    }

    return raid_cfg;
}

/*
 * brief
 * raid_bdev_config_add function adds config for newly created raid bdev.
 *
 * params:
 * raid_name - name for raid bdev.
 * strip_size - strip size in KB
 * num_base_rpcs - number of base rpcs.
 * level - raid level.
 * _raid_cfg - Pointer to newly added configuration
 */
int
raid_bdev_config_add(const char *raid_name, uint32_t strip_size, uint8_t num_qp, uint8_t num_base_rpcs,
                     enum raid_level level, struct raid_bdev_config **_raid_cfg)
{
    struct raid_bdev_config *raid_cfg;

    raid_cfg = raid_bdev_config_find_by_name(raid_name);
    if (raid_cfg != NULL) {
        SPDK_ERRLOG("Duplicate raid bdev name found in config file %s\n",
                    raid_name);
        return -EEXIST;
    }

    if (spdk_u32_is_pow2(strip_size) == false) {
        SPDK_ERRLOG("Invalid strip size %" PRIu32 "\n", strip_size);
        return -EINVAL;
    }

    if (num_qp == 0) {
        SPDK_ERRLOG("Invalid num_qp %u\n", num_qp);
        return -EINVAL;
    }

    if (num_base_rpcs == 0) {
        SPDK_ERRLOG("Invalid base rpc count %u\n", num_base_rpcs);
        return -EINVAL;
    }

    raid_cfg = (struct raid_bdev_config*) calloc(1, sizeof(*raid_cfg));
    if (raid_cfg == NULL) {
        SPDK_ERRLOG("unable to allocate memory\n");
        return -ENOMEM;
    }

    raid_cfg->name = strdup(raid_name);
    if (!raid_cfg->name) {
        free(raid_cfg);
        SPDK_ERRLOG("unable to allocate memory\n");
        return -ENOMEM;
    }
    raid_cfg->strip_size = strip_size;
    raid_cfg->num_qp = num_qp;
    raid_cfg->num_base_rpcs = num_base_rpcs;
    raid_cfg->level = level;

    raid_cfg->base_rpcs = (struct raid_base_rpc_config*) calloc(num_base_rpcs, sizeof(*raid_cfg->base_rpcs));
    if (raid_cfg->base_rpcs == NULL) {
        free(raid_cfg->name);
        free(raid_cfg);
        SPDK_ERRLOG("unable to allocate memory\n");
        return -ENOMEM;
    }

    TAILQ_INSERT_TAIL(&g_raid_config.raid_bdev_config_head, raid_cfg, link);
    g_raid_config.total_raid_bdev++;

    *_raid_cfg = raid_cfg;
    return 0;
}

/*
 * brief:
 * raid_bdev_config_add_base_rpc function add base rpc to raid bdev config.
 *
 * params:
 * raid_cfg - pointer to raid bdev configuration
 * slot - Position to add base rpc
 */
int
raid_bdev_config_add_base_rpc(struct raid_bdev_config *raid_cfg, const char *base_rpc_uri, bool degraded,
                              uint8_t slot)
{
    uint8_t i;
    struct raid_bdev_config *tmp;

    if (slot >= raid_cfg->num_base_rpcs) {
        return -EINVAL;
    }

    raid_cfg->base_rpcs[slot].uri = strdup(base_rpc_uri);
    if (raid_cfg->base_rpcs[slot].uri == NULL) {
        SPDK_ERRLOG("unable to allocate memory\n");
        return -ENOMEM;
    }
    raid_cfg->base_rpcs[slot].degraded = degraded;

    return 0;
}

enum raid_level raid_bdev_parse_raid_level(const char *str)
{
    unsigned int i;

    assert(str != NULL);

    for (i = 0; g_raid_level_names[i].name != NULL; i++) {
        if (strcasecmp(g_raid_level_names[i].name, str) == 0) {
            return g_raid_level_names[i].value;
        }
    }

    return INVALID_RAID_LEVEL;
}

const char *
raid_bdev_level_to_str(enum raid_level level)
{
    unsigned int i;

    for (i = 0; g_raid_level_names[i].name != NULL; i++) {
        if (g_raid_level_names[i].value == level) {
            return g_raid_level_names[i].name;
        }
    }

    return "";
}

/*
 * brief:
 * raid_bdev_fini_start is called when bdev layer is starting the
 * shutdown process
 * params:
 * none
 * returns:
 * none
 */
static void
raid_bdev_fini_start(void)
{
    SPDK_DEBUGLOG(bdev_raid, "raid_bdev_fini_start\n");
}

/*
 * brief:
 * raid_bdev_exit is called on raid bdev module exit time by bdev layer
 * params:
 * none
 * returns:
 * none
 */
static void
raid_bdev_exit(void)
{
    SPDK_DEBUGLOG(bdev_raid, "raid_bdev_exit\n");
}

/*
 * brief:
 * raid_bdev_get_ctx_size is used to return the context size of bdev_io for raid
 * module
 * params:
 * none
 * returns:
 * size of spdk_bdev_io context for raid
 */
static int
raid_bdev_get_ctx_size(void)
{
    SPDK_DEBUGLOG(bdev_raid, "raid_bdev_get_ctx_size\n");
    return sizeof(struct raid_bdev_io);
}

/*
 * brief:
 * raid_bdev_init is the initialization function for raid bdev module
 * params:
 * none
 * returns:
 * 0 - success
 * non zero - failure
 */
static int
raid_bdev_init(void)
{
    return 0;
}

static struct spdk_bdev_module g_raid_if = {
        .module_init = raid_bdev_init,
        .fini_start = raid_bdev_fini_start,
        .module_fini = raid_bdev_exit,
        .name = "raid",
        .get_ctx_size = raid_bdev_get_ctx_size,
        .examine_config = raid_bdev_examine,
        .async_init = false,
        .async_fini = false
};
SPDK_BDEV_MODULE_REGISTER(raid, &g_raid_if)

/*
 * brief:
 * raid_bdev_examine function is the examine function call by the below layers
 * like bdev_nvme layer. This function will check if this base bdev can be
 * claimed by this raid bdev or not.
 * params:
 * bdev - pointer to base bdev
 * returns:
 * none
 */
static void
raid_bdev_examine(struct spdk_bdev *bdev)
{
    spdk_bdev_module_examine_done(&g_raid_if);
}

static int
setup_rdma(struct raid_bdev *raid_bdev, struct raid_bdev_config *raid_cfg)
{
    int ret;
    uint8_t num_qp = raid_bdev->num_qp;
    struct addrinfo	hints = {
            .ai_family    = AF_INET, // ipv4
            .ai_socktype  = SOCK_STREAM,
            .ai_protocol  = 0
    };
    struct addrinfo* dst[raid_bdev->num_base_rpcs];
    struct addrinfo* cmd_dst[raid_bdev->num_base_rpcs][num_qp];
    struct addrinfo* src[raid_bdev->num_base_rpcs];
    struct addrinfo* cmd_src[raid_bdev->num_base_rpcs][num_qp];
    struct rdma_cm_event* event;
    struct rdma_conn_param conn_param = {};
    conn_param.responder_resources = 16;
    conn_param.initiator_depth = 16;
    conn_param.retry_count = 4;
    conn_param.rnr_retry_count = 7;
    struct ibv_qp_init_attr	qp_attr = {};
    struct ibv_cq *cq;
    struct ibv_recv_wr *bad_recv_wr;

    struct ibv_context **contexts = rdma_get_devices(NULL);

    TAILQ_INIT(&raid_bdev->qp_groups);
    pthread_spin_init(&raid_bdev->qp_group_lock, PTHREAD_PROCESS_PRIVATE);

    // setup CM channels
    struct rdma_event_channel **cmd_cm_channel_list[raid_bdev->num_base_rpcs];
    for (uint8_t i = 0; i < raid_bdev->num_base_rpcs; i++) {
        cmd_cm_channel_list[i] = (struct rdma_event_channel **) calloc(num_qp, sizeof(struct rdma_event_channel *));
        if (!cmd_cm_channel_list[i]) {
            SPDK_ERRLOG("Failed to allocate cmd_cm_channel_list[i]\n");
            assert(false);
        }
    }
    for (uint8_t i = 0; i < raid_bdev->num_base_rpcs; i++) {
        for (uint8_t j = 0; j < num_qp; j++) {
            cmd_cm_channel_list[i][j] = rdma_create_event_channel();
            if (!cmd_cm_channel_list[i][j]) {
                SPDK_ERRLOG("Creating event channel failed, errno: %d \n", -errno);
                return -1;
            }
        }
    }

    // connect RDMA QPs
    for (uint8_t j = 0; j < num_qp; j++) {
        struct rdma_qp_group *r_qp_group = (struct rdma_qp_group *) calloc(1, sizeof(struct rdma_qp_group));
        if (!r_qp_group) {
            SPDK_ERRLOG("Failed to allocate r_qp_group\n");
            assert(false);
        }
        r_qp_group->qps = (struct rdma_qp **) calloc(raid_bdev->num_base_rpcs, sizeof(struct rdma_qp *));
        if (!r_qp_group->qps) {
            SPDK_ERRLOG("Failed to allocate r_qp_group->qps\n");
            assert(false);
        }
        r_qp_group->cq = ibv_create_cq(contexts[get_phy_port()], 4096, NULL, NULL, 0);

        for (uint8_t i = 0; i < raid_bdev->num_base_rpcs; i++) {

            struct rdma_qp *rqp = (struct rdma_qp *) calloc(1, sizeof(struct rdma_qp));
            if (!rqp) {
                SPDK_ERRLOG("Failed to allocate rqp\n");
                assert(false);
            }
            r_qp_group->qps[i] = rqp;

            ret = getaddrinfo(get_host_ip_addr().c_str(), std::to_string(kStartPort+i*num_qp+j).c_str(), &hints, &cmd_src[i][j]);
            if (ret) {
                SPDK_ERRLOG("getaddrinfo for src failed\n");
                return -1;
            }
            ret = rdma_create_id(cmd_cm_channel_list[i][j], &rqp->cmd_cm_id, NULL, RDMA_PS_TCP);
            if (ret) {
                SPDK_ERRLOG("Creating cm id failed with errno: %d \n", -errno);
                return -1;
            }
            ret = getaddrinfo(raid_cfg->base_rpcs[i].uri, std::to_string(kStartPort-1-j).c_str(), &hints, &cmd_dst[i][j]);
            if (ret) {
                SPDK_ERRLOG("getaddrinfo for dst failed\n");
                return -1;
            }
            ret = rdma_resolve_addr(rqp->cmd_cm_id, cmd_src[i][j]->ai_addr, cmd_dst[i][j]->ai_addr, 2000);
            if (ret) {
                SPDK_ERRLOG("resolve_addr failed\n");
                return -1;
            }
            ret = rdma_get_cm_event(cmd_cm_channel_list[i][j], &event);
            if(ret) {
                SPDK_ERRLOG("rdma_get_cm_event failed\n");
                return -1;
            }
            if (event->event != RDMA_CM_EVENT_ADDR_RESOLVED) {
                SPDK_ERRLOG("event is not RDMA_CM_EVENT_ADDR_RESOLVED\n");
                return -1;
            }
            rdma_ack_cm_event(event);
            ret = rdma_resolve_route(rqp->cmd_cm_id, 2000);
            if (ret) {
                SPDK_ERRLOG("resolve_route failed\n");
                return -1;
            }
            ret = rdma_get_cm_event(cmd_cm_channel_list[i][j], &event);
            if(ret) {
                SPDK_ERRLOG("rdma_get_cm_event failed\n");
                return -1;
            }
            if (event->event != RDMA_CM_EVENT_ROUTE_RESOLVED) {
                SPDK_ERRLOG("event is not RDMA_CM_EVENT_ROUTE_RESOLVED\n");
                return -1;
            }
            rdma_ack_cm_event(event);

            rqp->mem_map = spdk_rdma_create_mem_map_external(rqp->cmd_cm_id->pd);

            qp_attr.cap.max_send_wr = paraSend;
            qp_attr.cap.max_send_sge = 1;
            qp_attr.cap.max_recv_wr = paraSend;
            qp_attr.cap.max_recv_sge = 1;
            qp_attr.send_cq = r_qp_group->cq;
            qp_attr.recv_cq = r_qp_group->cq;
            qp_attr.qp_type = IBV_QPT_RC;
            ret = rdma_create_qp(rqp->cmd_cm_id, rqp->cmd_cm_id->pd, &qp_attr);
            if(ret) {
                SPDK_ERRLOG("rdma_create_qp failed\n");
                return -1;
            }

            // allocate buffer and pre-post recv
            rqp->cmd_sendbuf = (uint8_t *) spdk_dma_zmalloc(paraSend * 2 * bufferSize, 32, NULL);
            if (!rqp->cmd_sendbuf) {
                SPDK_ERRLOG("allocate buffer failed\n");
                return -1;
            }
            rqp->cmd_recvbuf = rqp->cmd_sendbuf + paraSend * bufferSize;
            rqp->cmd_mr = ibv_reg_mr(rqp->cmd_cm_id->pd,
                                     rqp->cmd_sendbuf,
                                     paraSend * bufferSize * 2,
                                     IBV_ACCESS_LOCAL_WRITE);
            if (!rqp->cmd_mr) {
                SPDK_ERRLOG("register memory failed\n");
                return -1;
            }

            uint8_t *buf = (uint8_t *)spdk_dma_zmalloc(paraSend * kMsgSize, 32, NULL);
            if (!buf) {
                SPDK_ERRLOG("Failed to allocate buf\n");
                assert(false);
            }
            TAILQ_INIT(&rqp->free_send);
            for (int k = 0; k < paraSend; k++) {
                struct ibv_sge *send_sge = (struct ibv_sge *) calloc(1, sizeof(struct ibv_sge));
                if (!send_sge) {
                    SPDK_ERRLOG("Failed to allocate send_sge\n");
                    assert(false);
                }
                struct send_wr_wrapper *send_wrapper = (struct send_wr_wrapper *) calloc(1, sizeof(struct send_wr_wrapper));
                if (!send_wrapper) {
                    SPDK_ERRLOG("Failed to allocate send_wrapper\n");
                    assert(false);
                }
                struct ibv_send_wr *send_wr = &send_wrapper->send_wr;
                send_wrapper->rqp = rqp;
                send_wrapper->buf = (void *) (buf + k * kMsgSize);
                send_wr->opcode = IBV_WR_SEND_WITH_IMM;
                send_wr->sg_list = send_sge;
                send_wr->num_sge = 1;
                send_sge->addr = (uint64_t) rqp->cmd_sendbuf + k * bufferSize;
                send_sge->length = bufferSize;
                send_sge->lkey = rqp->cmd_mr->lkey;
                send_wr->send_flags = IBV_SEND_SIGNALED;
                send_wr->wr_id = (uint64_t) send_wrapper;
                struct cs_message_t *cs_msg = (struct cs_message_t *) send_sge->addr;
                cs_msg->req_id = (uint64_t) send_wrapper;
                TAILQ_INSERT_TAIL(&rqp->free_send, send_wrapper, link);

                struct ibv_sge *recv_sge = (struct ibv_sge *) calloc(1, sizeof(struct ibv_sge));
                if (!recv_sge) {
                    SPDK_ERRLOG("Failed to allocate recv_sge\n");
                    assert(false);
                }
                struct recv_wr_wrapper *recv_wrapper = (struct recv_wr_wrapper *) calloc(1, sizeof(struct recv_wr_wrapper));
                if (!recv_wrapper) {
                    SPDK_ERRLOG("Failed to allocate recv_wrapper\n");
                    assert(false);
                }
                struct ibv_recv_wr *recv_wr = &recv_wrapper->recv_wr;
                recv_wrapper->rqp = rqp;
                recv_sge->addr = (uint64_t) rqp->cmd_recvbuf + k * bufferSize;
                recv_sge->length = bufferSize;
                recv_sge->lkey =  rqp->cmd_mr->lkey;
                recv_wr->wr_id = (uint64_t) recv_wrapper;
                recv_wr->sg_list = recv_sge;
                recv_wr->num_sge = 1;

                if (rqp->recv_wrs.first == NULL) {
                    rqp->recv_wrs.first = recv_wr;
                    rqp->recv_wrs.last = recv_wr;
                } else {
                    rqp->recv_wrs.last->next = recv_wr;
                    rqp->recv_wrs.last = recv_wr;
                }
            }

            if (ibv_post_recv(rqp->cmd_cm_id->qp, rqp->recv_wrs.first, &bad_recv_wr)) {
                SPDK_ERRLOG("post send failed\n");
                assert(false);
            }
            rqp->recv_wrs.first = NULL;

            ret = rdma_connect(rqp->cmd_cm_id, &conn_param);
            if(ret) {
                SPDK_ERRLOG("rdma_connect failed\n");
                return -1;
            }
        }
        pthread_spin_lock(&raid_bdev->qp_group_lock);
        TAILQ_INSERT_TAIL(&raid_bdev->qp_groups, r_qp_group, link);
        pthread_spin_unlock(&raid_bdev->qp_group_lock);
    }

    //check connection
    for (uint8_t i = 0; i < raid_bdev->num_base_rpcs; i++) {
        for (uint8_t j = 0; j < num_qp; j++) {
            ret = rdma_get_cm_event(cmd_cm_channel_list[i][j], &event);
            if(ret) {
                SPDK_ERRLOG("rdma_get_cm_event failed\n");
                return -1;
            }
            if (event->event != RDMA_CM_EVENT_ESTABLISHED) {
                SPDK_ERRLOG("event is not RDMA_CM_EVENT_ESTABLISHED, but %d; i = %d, j = %d\n", event->event, i, j);
                return -1;
            }
            rdma_ack_cm_event(event);
        }
    }

    return 0;
}

/*
 * brief:
 * raid_bdev_create allocates raid bdev based on passed configuration
 * params:
 * raid_cfg - configuration of raid bdev
 * returns:
 * 0 - success
 * non zero - failure
 */
int
raid_bdev_create(struct raid_bdev_config *raid_cfg)
{
    struct raid_bdev *raid_bdev;
    struct spdk_bdev *raid_bdev_gen;
    struct raid_bdev_module *module;

    module = raid_bdev_module_find(raid_cfg->level);
    if (module == NULL) {
        SPDK_ERRLOG("Unsupported raid level '%d'\n", raid_cfg->level);
        return -EINVAL;
    }

    assert(module->base_rpcs_min != 0);
    if (raid_cfg->num_base_rpcs < module->base_rpcs_min) {
        SPDK_ERRLOG("At least %u base devices required for %s\n",
                    module->base_rpcs_min,
                    raid_bdev_level_to_str(raid_cfg->level));
        return -EINVAL;
    }

    raid_bdev = (struct raid_bdev*) calloc(1, sizeof(*raid_bdev));
    if (!raid_bdev) {
        SPDK_ERRLOG("Unable to allocate memory for raid bdev\n");
        return -ENOMEM;
    }

    raid_bdev->module = module;
    raid_bdev->num_base_rpcs = raid_cfg->num_base_rpcs;
    raid_bdev->base_rpc_info = (struct raid_base_rpc_info*) calloc(raid_bdev->num_base_rpcs,
                                                                   sizeof(struct raid_base_rpc_info));
    if (!raid_bdev->base_rpc_info) {
        SPDK_ERRLOG("Unable able to allocate base rpc info\n");
        free(raid_bdev);
        return -ENOMEM;
    }

    /* strip_size_kb is from the rpc param.  strip_size is in blocks and used
     * internally and set later.
     */
    raid_bdev->strip_size = 0;
    raid_bdev->strip_size_kb = raid_cfg->strip_size;
    raid_bdev->num_qp = raid_cfg->num_qp;
    raid_bdev->config = raid_cfg;
    raid_bdev->level = raid_cfg->level;
    raid_bdev->degraded = false;

    raid_bdev_gen = &raid_bdev->bdev;

    raid_bdev_gen->name = strdup(raid_cfg->name);
    if (!raid_bdev_gen->name) {
        SPDK_ERRLOG("Unable to allocate name for raid\n");
        free(raid_bdev->base_rpc_info);
        free(raid_bdev);
        return -ENOMEM;
    }

    raid_bdev_gen->product_name = (char*) "Raid Volume";
    raid_bdev_gen->ctxt = raid_bdev;
    raid_bdev_gen->fn_table = &g_raid_bdev_fn_table;
    raid_bdev_gen->module = &g_raid_if;
    raid_bdev_gen->write_cache = 0;

    setup_rdma(raid_bdev, raid_cfg);

    TAILQ_INSERT_TAIL(&g_raid_bdev_list, raid_bdev, global_link);

    raid_cfg->raid_bdev = raid_bdev;

    return 0;
}

int
raid_dispatch_to_rpc(struct send_wr_wrapper *send_wrapper)
{
    struct ibv_send_wr *send_wr = &send_wrapper->send_wr;
    struct rdma_qp *rqp  = send_wrapper->rqp;

    send_wr->next = NULL;
    if (rqp->send_wrs.first == NULL) {
        rqp->send_wrs.first = send_wr;
        rqp->send_wrs.last = send_wr;
    } else {
        rqp->send_wrs.last->next = send_wr;
        rqp->send_wrs.last = send_wr;
    }

    return 0;
}

/*
 * brief
 * raid_bdev_alloc_base_bdev_resource allocates resource of base bdev.
 * params:
 * raid_bdev - pointer to raid bdev
 * returns:
 * 0 - success
 * non zero - failure
 */
static int
raid_bdev_alloc_base_rpc_resource(struct raid_bdev *raid_bdev, const char *rpc_uri,
                                  uint8_t rpc_slot, bool degraded)
{
    int rc;

    SPDK_DEBUGLOG(bdev_raid, "rpc %s is claimed\n", server_uri);
    assert(rpc_slot < raid_bdev->num_base_rpcs);

    raid_bdev->base_rpc_info[rpc_slot].uri = (char *) malloc((strlen(rpc_uri)+1)*sizeof(char));
    std::strcpy(raid_bdev->base_rpc_info[rpc_slot].uri, rpc_uri);
    raid_bdev->base_rpc_info[rpc_slot].degraded = degraded;
    if (raid_bdev->base_rpc_info[rpc_slot].degraded) {
        raid_bdev->degraded = true;
    }
    raid_bdev->base_rpc_info[rpc_slot].buf_align = 32;
    raid_bdev->base_rpc_info[rpc_slot].blockcnt = kMaxBlockcnt;
    raid_bdev->base_rpc_info[rpc_slot].up = true;
    // TODO: add this when testing with degraded disk
//    if (raid_bdev->level == RAID5) {
//        if (rpc_slot == 0) {
//            raid_bdev->base_rpc_info[rpc_slot].degraded = true;
//            raid_bdev->degraded = true;
//        }
//    }
//    if (raid_bdev->level == RAID6) {
//        if (rpc_slot == 0 || rpc_slot == 1) {
//            raid_bdev->base_rpc_info[rpc_slot].degraded = true;
//            raid_bdev->degraded = true;
//        }
//    }
    raid_bdev->base_rpc_info[rpc_slot].throughput = 100;
    raid_bdev->num_base_rpcs_discovered++;
    assert(raid_bdev->num_base_rpcs_discovered <= raid_bdev->num_base_rpcs);

    return 0;
}

/*
 * brief:
 * If raid bdev config is complete, then only register the raid bdev to
 * bdev layer and remove this raid bdev from configuring list and
 * insert the raid bdev to configured list
 * params:
 * raid_bdev - pointer to raid bdev
 * returns:
 * 0 - success
 * non zero - failure
 */
static int
raid_bdev_configure(struct raid_bdev *raid_bdev)
{
    uint32_t blocklen = 0;
    struct spdk_bdev *raid_bdev_gen;
    struct raid_base_rpc_info *base_info;
    int rc = 0;

    assert(raid_bdev->num_base_rpcs_discovered == raid_bdev->num_base_rpcs);

    blocklen = kBlkSize;
    assert(blocklen > 0);

    /* The strip_size_kb is read in from user in KB. Convert to blocks here for
     * internal use.
     */
    raid_bdev->strip_size = (raid_bdev->strip_size_kb * 1024) / blocklen;

    raid_bdev->strip_size_shift = spdk_u32log2(raid_bdev->strip_size);
    raid_bdev->blocklen_shift = spdk_u32log2(blocklen);
    raid_bdev_gen = &raid_bdev->bdev;
    raid_bdev_gen->blocklen = blocklen;

    rc = raid_bdev->module->start(raid_bdev);
    if (rc != 0) {
        SPDK_ERRLOG("raid module startup callback failed\n");
        return rc;
    }
    SPDK_DEBUGLOG(bdev_raid, "io device register %p\n", raid_bdev);
    SPDK_DEBUGLOG(bdev_raid, "blockcnt %" PRIu64 ", blocklen %u\n",
            raid_bdev_gen->blockcnt, raid_bdev_gen->blocklen);
    spdk_io_device_register(raid_bdev, raid_bdev_create_cb, raid_bdev_destroy_cb,
                            sizeof(struct raid_bdev_io_channel) + raid_bdev->module->io_channel_resource_size,
                            raid_bdev->bdev.name);
    rc = spdk_bdev_register(raid_bdev_gen);
    if (rc != 0) {
        SPDK_ERRLOG("Unable to register raid bdev and stay at configuring state\n");
        if (raid_bdev->module->stop != NULL) {
            raid_bdev->module->stop(raid_bdev);
        }
        spdk_io_device_unregister(raid_bdev, NULL);
        return rc;
    }
    SPDK_DEBUGLOG(bdev_raid, "raid bdev generic %p\n", raid_bdev_gen);
    SPDK_DEBUGLOG(bdev_raid, "raid bdev is created with name %s, raid_bdev %p\n",
                  raid_bdev_gen->name, raid_bdev);

    return 0;
}

/*
 * brief:
 * raid_bdev_add_base_device function is the actual function which either adds
 * the nvme base device to existing raid bdev or create a new raid bdev. It also claims
 * the base device and keep the open descriptor.
 * params:
 * raid_cfg - pointer to raid bdev config
 * returns:
 * 0 - success
 * non zero - failure
 */
static int
raid_bdev_add_base_rpc(struct raid_bdev_config *raid_cfg, const char *rpc_uri,
                       uint8_t rpc_slot, bool degraded)
{
    struct raid_bdev	*raid_bdev;
    int			rc;

    raid_bdev = raid_cfg->raid_bdev;
    if (!raid_bdev) {
        SPDK_ERRLOG("Raid bdev '%s' is not created yet\n", raid_cfg->name);
        return -ENODEV;
    }

    rc = raid_bdev_alloc_base_rpc_resource(raid_bdev, rpc_uri, rpc_slot, degraded);
    if (rc != 0) {
        if (rc != -ENODEV) {
            SPDK_ERRLOG("Failed to allocate resource for rpc '%s'\n", rpc_uri);
        }
        return rc;
    }

    assert(raid_bdev->num_base_rpcs_discovered <= raid_bdev->num_base_rpcs);

    if (raid_bdev->num_base_rpcs_discovered == raid_bdev->num_base_rpcs) {
        rc = raid_bdev_configure(raid_bdev);
        if (rc != 0) {
            SPDK_ERRLOG("Failed to configure raid bdev\n");
            return rc;
        }
    }

    return 0;
}

/*
 * brief:
 *  Add base bdevs to the raid bdev one by one. Skip any base bdev which doesn't
 *  exist or fails to add. If all base bdevs are successfully added, the raid bdev
 *  moves to the configured state and becomes available. Otherwise, the raid bdev
 *  stays at the configuring state with added base bdevs.
 * params:
 * raid_cfg - pointer to raid bdev config
 * returns:
 * 0 - The raid bdev moves to the configured state or stays at the configuring
 *     state with added base bdevs due to any nonexistent base bdev.
 * non zero - Failed to add any base bdev and stays at the configuring state with
 *            added base bdevs.
 */
int
raid_bdev_add_base_rpcs(struct raid_bdev_config *raid_cfg)
{
    uint8_t	i;
    int	rc;

    for (i = 0; i < raid_cfg->num_base_rpcs; i++) {
        rc = raid_bdev_add_base_rpc(raid_cfg, raid_cfg->base_rpcs[i].uri, i, raid_cfg->base_rpcs[i].degraded);
        if (rc != 0) {
            SPDK_ERRLOG("Failed to add base bdev %s to RAID bdev %s: %s\n",
                        raid_cfg->base_rpcs[i].uri, raid_cfg->name, spdk_strerror(-rc));
            return rc;
        }
    }

    return rc;
}

/* Log component for bdev raid bdev module */
SPDK_LOG_REGISTER_COMPONENT(bdev_raid)
