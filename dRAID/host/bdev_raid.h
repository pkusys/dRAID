#ifndef SPDK_BDEV_RAID_INTERNAL_H
#define SPDK_BDEV_RAID_INTERNAL_H

#include "spdk/bdev_module.h"
#include "../shared/common.h"

#define RAID_MAX_STRIPES 4096

enum raid_level {
    INVALID_RAID_LEVEL	= -1,
    RAID5			= 5,
    RAID6           = 6
};

struct send_wr_wrapper;
struct recv_wr_wrapper;

struct rdma_qp {
    struct rdma_cm_id *cmd_cm_id;
    struct ibv_mr *cmd_mr;
    struct spdk_mem_map *mem_map;
    uint8_t *cmd_sendbuf;
    uint8_t *cmd_recvbuf;
    struct rdma_send_wr_list send_wrs;
    struct rdma_recv_wr_list recv_wrs;
    TAILQ_HEAD(, send_wr_wrapper) free_send;
};

struct send_wr_wrapper {
    struct rdma_qp *rqp;
    struct ibv_send_wr send_wr;
    void *ctx;
    spdk_msg_fn cb;
    uint64_t num_blocks;
    uint8_t rtn_cnt;
    void *buf;
    TAILQ_ENTRY(send_wr_wrapper) link;
};

struct recv_wr_wrapper {
    struct rdma_qp *rqp;
    struct ibv_recv_wr recv_wr;
};

struct rdma_qp_group {
    struct rdma_qp **qps;
    struct ibv_cq *cq;
    TAILQ_ENTRY(rdma_qp_group) link;
};

/*
 * raid_base_bdev_info contains information for the base bdevs which are part of some
 * raid. This structure contains the per base bdev information. Whatever is
 * required per base device for raid bdev will be kept here
 */
struct raid_base_rpc_info {
    /* rpc uri */
    char *uri;

    size_t buf_align;

    uint64_t blockcnt;

    /* is this block device degraded */
    bool degraded;

    /* is this block device up */
    bool up;

    size_t throughput;
};

/*
 * raid_bdev_io is the context part of bdev_io. It contains the information
 * related to bdev_io for a raid bdev
 */
struct raid_bdev_io {
    /* The raid bdev associated with this IO */
    struct raid_bdev *raid_bdev;

    /* Context of the original channel for this IO */
    struct raid_bdev_io_channel	*raid_ch;

    /* WaitQ entry, used only in waitq logic */
    struct spdk_bdev_io_wait_entry	waitq_entry;

    /* Used for tracking progress on io requests sent to member disks. */
    uint64_t			     base_bdev_io_remaining;
    uint8_t				     base_bdev_io_submitted;
    enum spdk_bdev_io_status base_bdev_io_status;

    uint64_t timestamp;
};

/*
 * raid_bdev is the single entity structure which contains SPDK block device
 * and the information related to any raid bdev either configured or
 * in configuring list. io device is created on this.
 */
struct raid_bdev {
    /* raid bdev device, this will get registered in bdev layer */
    struct spdk_bdev		bdev;

    /* link of raid bdev to link it to global raid bdev list */
    TAILQ_ENTRY(raid_bdev)		global_link;

    /* pointer to config file entry */
    struct raid_bdev_config		*config;

    /* array of base rpc info */
    struct raid_base_rpc_info	*base_rpc_info;

    /* strip size of raid bdev in blocks */
    uint32_t			strip_size;

    /* strip size of raid bdev in KB */
    uint32_t			strip_size_kb;

    uint8_t             num_qp;

    /* strip size bit shift for optimized calculation */
    uint32_t			strip_size_shift;

    /* block length bit shift for optimized calculation */
    uint32_t			blocklen_shift;

    /* number of base rpcs comprising raid bdev  */
    uint8_t				num_base_rpcs;

    /* number of base rpcs discovered */
    uint8_t				num_base_rpcs_discovered;

    /* Raid Level of this raid bdev */
    enum raid_level			level;

    bool degraded;

    struct ibv_cq *cq;

    pthread_spinlock_t qp_group_lock;

    TAILQ_HEAD(, rdma_qp_group) qp_groups;

    /* Module for RAID-level specific operations */
    struct raid_bdev_module		*module;

    /* Private data for the raid module */
    void				*module_private;
};

#define RAID_FOR_EACH_BASE_RPC(r, i) \
	for (i = r->base_rpc_info; i < r->base_rpc_info + r->num_base_rpcs; i++)

/*
 * raid_base_rpc_config is the per base rpc data structure which contains
 * information w.r.t to per base rpc during parsing config
 */
struct raid_base_rpc_config {
    /* rpc uri */
    char *uri;
    /* degraded */
    bool degraded;
};

/*
 * raid_bdev_config contains the raid bdev config related information after
 * parsing the config file
 */
struct raid_bdev_config {
    /* base bdev config per underlying bdev */
    struct raid_base_rpc_config	*base_rpcs;

    /* Points to already created raid bdev  */
    struct raid_bdev		*raid_bdev;

    char				*name;

    /* strip size of this raid bdev in KB */
    uint32_t			strip_size;

    uint8_t			    num_qp;

    /* number of base bdevs */
    uint8_t				num_base_rpcs;

    /* raid level */
    enum raid_level			level;

    TAILQ_ENTRY(raid_bdev_config)	link;
};

/*
 * raid_config is the top level structure representing the raid bdev config as read
 * from config file for all raids
 */
struct raid_config {
    /* raid bdev  context from config file */
    TAILQ_HEAD(, raid_bdev_config) raid_bdev_config_head;

    /* total raid bdev  from config file */
    uint8_t total_raid_bdev;
};

/*
 * raid_bdev_io_channel is the context of spdk_io_channel for raid bdev device.
 */
struct raid_bdev_io_channel {
    /* Number of RPCs */
    uint8_t			num_rpcs;

    struct spdk_poller *poller;

    struct rdma_qp_group *qp_group;

};

/*
 * Get the resource allocated with the raid bdev IO channel.
 */
static inline void *
raid_bdev_io_channel_get_resource(struct raid_bdev_io_channel *raid_ch)
{
    return (uint8_t *)raid_ch + sizeof(*raid_ch);
}

/* TAIL heads for various raid bdev lists */
TAILQ_HEAD(raid_all_tailq, raid_bdev);

extern struct raid_all_tailq		g_raid_bdev_list;
extern struct raid_config		g_raid_config;

typedef void (*raid_bdev_destruct_cb)(void *cb_ctx, int rc);

int raid_bdev_create(struct raid_bdev_config *raid_cfg);
int raid_bdev_add_base_rpcs(struct raid_bdev_config *raid_cfg);
int raid_bdev_config_add(const char *raid_name, uint32_t strip_size, uint8_t num_qp, uint8_t num_base_rpcs,
                         enum raid_level level, struct raid_bdev_config **_raid_cfg);
int raid_bdev_config_add_base_rpc(struct raid_bdev_config *raid_cfg, const char *base_rpc_uri, bool degraded,
                                  uint8_t slot);
struct raid_bdev_config *raid_bdev_config_find_by_name(const char *raid_name);
enum raid_level raid_bdev_parse_raid_level(const char *str);
const char *raid_bdev_level_to_str(enum raid_level level);
void raid_memset_iovs(struct iovec *iovs, int iovcnt, char c);
void raid_memcpy_iovs(struct iovec *iovs_dest, int iovs_dest_cnt, size_t iovs_dest_offset,
                      const struct iovec *iovs_src, int iovs_src_cnt, size_t iovs_src_offset,
                      size_t size);

int raid_dispatch_to_rpc(struct send_wr_wrapper *send_wrapper);
static inline struct send_wr_wrapper * raid_get_send_wrapper(struct raid_bdev_io *raid_io, uint8_t rpc_id)
{
    struct rdma_qp *rqp = raid_io->raid_ch->qp_group->qps[rpc_id];
    struct send_wr_wrapper *send_wrapper = NULL;
    if (!TAILQ_EMPTY(&rqp->free_send)) {
        send_wrapper = TAILQ_FIRST(&rqp->free_send);
        TAILQ_REMOVE(&rqp->free_send, send_wrapper, link);
    }
    return send_wrapper;
}
static inline void void_cont_func_rdma(struct send_wr_wrapper *send_wrapper) {
    if (send_wrapper->rtn_cnt == 0) {
        send_wrapper->cb((void *) send_wrapper);
        TAILQ_INSERT_TAIL(&send_wrapper->rqp->free_send, send_wrapper, link);
    }
}
static inline uint8_t return_count(struct cs_message_t *csm) {
    uint8_t type = csm->type;
    int8_t next_index = csm->next_index;
    if ((type == RECON_NRT || type == RECON_NRT_DP || type == RECON_NRT_DP_Q ||
         type == RECON_NRT_DD || type == RECON_NRT_DD_P || type == RECON_NRT_DD_Q) &&
        next_index != -1) {
        return 0;
    } else if ((type == RECON_RT || type == RECON_RT_DP || type == RECON_RT_DD) &&
               next_index == -1) {
        return 2;
    } else if (type == FWD_RO || type == FWD_RO_PRO) {
        return 0;
    } else {
        return 1;
    }
}

/*
 * RAID module descriptor
 */
struct raid_bdev_module {
    /* RAID level implemented by this module */
    enum raid_level level;

    /* Minimum required number of base bdevs. Must be > 0. */
    uint8_t base_rpcs_min;

    /*
     * Maximum number of base bdevs that can be removed without failing
     * the array.
     */
    uint8_t base_rpcs_max_degraded;

    /* Size of the additional resource allocated per IO channel context. */
    size_t io_channel_resource_size;

    /*
	 * Called when the raid is starting, right before changing the state to
	 * online and registering the bdev. Parameters of the bdev like blockcnt
	 * should be set here.
	 *
	 * Non-zero return value will abort the startup process.
	 */
    int (*start)(struct raid_bdev *raid_bdev);

    /*
     * Called when the raid is stopping, right before changing the state to
     * offline and unregistering the bdev. Optional.
     */
    void (*stop)(struct raid_bdev *raid_bdev);

    /* Handler for R/W requests */
    void (*submit_rw_request)(struct raid_bdev_io *raid_io);

    /* Handler for requests without payload (flush, unmap). Optional. */
    void (*submit_null_payload_request)(struct raid_bdev_io *raid_io);

    /*
     * Callback to initialize the per IO channel resource.
     * Called when the bdev's IO channel is created. Optional.
     */
    int (*io_channel_resource_init)(struct raid_bdev *raid_bdev, void *resource);

    /*
     * Callback to deinitialize the per IO channel resource.
     * Called when the bdev's IO channel is destroyed. Optional.
     */
    void (*io_channel_resource_deinit)(void *resource);

    TAILQ_ENTRY(raid_bdev_module) link;
};

void raid_bdev_module_list_add(struct raid_bdev_module *raid_module);

#define __RAID_MODULE_REGISTER(line) __RAID_MODULE_REGISTER_(line)
#define __RAID_MODULE_REGISTER_(line) raid_module_register_##line

#define RAID_MODULE_REGISTER(_module)					\
__attribute__((constructor)) static void				\
__RAID_MODULE_REGISTER(__LINE__)(void)					\
{									\
    raid_bdev_module_list_add(_module);					\
}

bool
raid_bdev_io_complete_part(struct raid_bdev_io *raid_io, uint64_t completed,
                           enum spdk_bdev_io_status status);
void
raid_bdev_io_complete(struct raid_bdev_io *raid_io, enum spdk_bdev_io_status status);
#endif /* SPDK_BDEV_RAID_INTERNAL_H */