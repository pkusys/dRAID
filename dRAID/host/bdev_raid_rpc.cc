#include "spdk/rpc.h"
#include "spdk/bdev.h"
#include "bdev_raid.h"
#include "spdk/util.h"
#include "spdk/string.h"
#include "spdk/log.h"
#include "spdk/env.h"

#define RPC_MAX_BASE_RPCS 255

struct rpc_bdev_base_rpc {
    /* rpc uri */
    char *uri;
    /* degraded */
    bool degraded;
};

/*
 * Base bdevs in RPC bdev_raid_create
 */
struct rpc_bdev_raid_create_base_rpcs {
    /* Number of base rpcs */
    size_t           num_base_rpcs;

    /* List of base rpcs */
    struct rpc_bdev_base_rpc base_rpcs[RPC_MAX_BASE_RPCS];
};

/*
 * Input structure for RPC rpc_bdev_raid_create
 */
struct rpc_bdev_raid_create {
    /* Raid bdev name */
    char                                 *name;

    /* RAID strip size in KB */
    uint32_t                             strip_size_kb;

    /* RAID raid level */
    enum raid_level                      level; // 5 or 6

    uint8_t                              num_qp;

    /* Base bdevs information */
    struct rpc_bdev_raid_create_base_rpcs base_rpcs;
};

/*
 * brief:
 * free_rpc_bdev_raid_create function is to free RPC bdev_raid_create related parameters
 * params:
 * req - pointer to RPC request
 * returns:
 * none
 */
static void
free_rpc_bdev_raid_create(struct rpc_bdev_raid_create *req)
{
    size_t i;

    free(req->name);
    for (i = 0; i < req->base_rpcs.num_base_rpcs; i++) {
        free(req->base_rpcs.base_rpcs[i].uri);
    }
}

/*
 * Decoder function for RPC bdev_raid_create to decode raid level
 */
static int
decode_raid_level(const struct spdk_json_val *val, void *out)
{
    int ret;
    char *str = NULL;
    enum raid_level level;

    ret = spdk_json_decode_string(val, &str);
    if (ret == 0 && str != NULL) {
        level = raid_bdev_parse_raid_level(str);
        if (level == INVALID_RAID_LEVEL) {
            ret = -EINVAL;
        } else {
            *(enum raid_level *)out = level;
        }
    }

    free(str);
    return ret;
}

/*
 * Decoder object for rpc_bdev_base_rpc struct
 */
static const struct spdk_json_object_decoder rpc_bdev_base_rpc_decoders[] = {
        {"uri", offsetof(struct rpc_bdev_base_rpc, uri), spdk_json_decode_string},
        {"degraded", offsetof(struct rpc_bdev_base_rpc, degraded), spdk_json_decode_bool}
};

/*
 * Decoder function for rpc_bdev_base_rpc struct
 */
static int
rpc_decode_base_rpc_object(const struct spdk_json_val *val, void *out)
{
    int rc = 0;
    struct rpc_bdev_base_rpc *base_rpc = (struct rpc_bdev_base_rpc*) out;

    rc = spdk_json_decode_object(val, rpc_bdev_base_rpc_decoders,
                                   SPDK_COUNTOF(rpc_bdev_base_rpc_decoders), base_rpc);
    if (rc) {
        printf("rpc_decode_base_rpc_object failed\n");
    }
    return rc;
}

/*
 * Decoder function for RPC bdev_raid_create to decode base rpcs list
 */
static int
decode_base_rpcs(const struct spdk_json_val *val, void *out)
{
    int rc = 0;
    struct rpc_bdev_raid_create_base_rpcs *base_rpcs = (struct rpc_bdev_raid_create_base_rpcs*) out;
    rc = spdk_json_decode_array(val, rpc_decode_base_rpc_object, base_rpcs->base_rpcs,
                                  RPC_MAX_BASE_RPCS, &base_rpcs->num_base_rpcs, sizeof(struct rpc_bdev_base_rpc));
    if (rc) {
        printf("decode_base_rpcs failed\n");
    }
    return rc;
}

/*
 * Decoder object for RPC bdev_raid_create
 */
static const struct spdk_json_object_decoder rpc_bdev_raid_create_decoders[] = {
        {"name", offsetof(struct rpc_bdev_raid_create, name), spdk_json_decode_string},
        {"strip_size_kb", offsetof(struct rpc_bdev_raid_create, strip_size_kb), spdk_json_decode_uint32, true},
        {"raid_level", offsetof(struct rpc_bdev_raid_create, level), decode_raid_level},
        {"num_qp", offsetof(struct rpc_bdev_raid_create, num_qp), spdk_json_decode_uint8, true},
        {"base_rpcs", offsetof(struct rpc_bdev_raid_create, base_rpcs), decode_base_rpcs}
};

/*
 * brief:
 * rpc_bdev_raid_create function is the RPC for creating RAID bdevs. It takes
 * input as raid bdev name, raid level, strip size in KB and list of base bdev names.
 * params:
 * request - pointer to json rpc request
 * params - pointer to request parameters
 * returns:
 * none
 */
static void
rpc_bdev_raid_create(struct spdk_jsonrpc_request *request,
                     const struct spdk_json_val *params)
{
    struct rpc_bdev_raid_create	req = {};
    struct raid_bdev_config		*raid_cfg;
    int				rc;
    size_t				i;

    if (spdk_json_decode_object(params, rpc_bdev_raid_create_decoders,
                                SPDK_COUNTOF(rpc_bdev_raid_create_decoders),
                                &req)) {
        spdk_jsonrpc_send_error_response(request, SPDK_JSONRPC_ERROR_INTERNAL_ERROR,
                                         "spdk_json_decode_object failed");
        goto cleanup;
    }

    if (req.strip_size_kb == 0) {
        spdk_jsonrpc_send_error_response(request, EINVAL, "strip size not specified");
        goto cleanup;
    }

    if (req.num_qp == 0) {
        spdk_jsonrpc_send_error_response(request, EINVAL, "num_qp not specified");
        goto cleanup;
    }

    rc = raid_bdev_config_add(req.name, req.strip_size_kb, req.num_qp, req.base_rpcs.num_base_rpcs,
                              req.level,
                              &raid_cfg);
    if (rc != 0) {
        spdk_jsonrpc_send_error_response_fmt(request, rc,
                                             "Failed to add RAID bdev config %s: %s",
                                             req.name, spdk_strerror(-rc));
        goto cleanup;
    }

    for (i = 0; i < req.base_rpcs.num_base_rpcs; i++) {
        rc = raid_bdev_config_add_base_rpc(raid_cfg, req.base_rpcs.base_rpcs[i].uri, req.base_rpcs.base_rpcs[i].degraded, i);
        if (rc != 0) {
            spdk_jsonrpc_send_error_response_fmt(request, rc,
                                                 "Failed to add base rpc %s to RAID bdev config %s: %s",
                                                 req.base_rpcs.base_rpcs[i].uri, req.name, spdk_strerror(-rc));
            goto cleanup;
        }
    }

    rc = raid_bdev_create(raid_cfg);
    if (rc != 0) {
        spdk_jsonrpc_send_error_response_fmt(request, rc,
                                             "Failed to create RAID bdev %s: %s",
                                             req.name, spdk_strerror(-rc));
        goto cleanup;
    }

    rc = raid_bdev_add_base_rpcs(raid_cfg);
    if (rc != 0) {
        spdk_jsonrpc_send_error_response_fmt(request, rc,
                                             "Failed to add any base bdev to RAID bdev %s: %s",
                                             req.name, spdk_strerror(-rc));
        goto cleanup;
    }

    spdk_jsonrpc_send_bool_response(request, true);

    cleanup:
    free_rpc_bdev_raid_create(&req);
}
SPDK_RPC_REGISTER("bdev_raid_create", rpc_bdev_raid_create, SPDK_RPC_RUNTIME)
SPDK_RPC_REGISTER_ALIAS_DEPRECATED(bdev_raid_create, construct_raid_bdev)
