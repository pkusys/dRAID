#ifndef BDEV_RAID_COMMON_H
#define BDEV_RAID_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <thread>
#include <vector>
#include <cstring>
#include <numa.h>
#include <fstream>
#include <iostream>
#include <regex>

#include "spdk/stdinc.h"
#include "spdk/thread.h"
#include "spdk/bdev.h"
#include "spdk/bdev_module.h"
#include "spdk/env.h"
#include "spdk/event.h"
#include "spdk/log.h"
#include "spdk/string.h"
#include "spdk/bdev_zone.h"
#include "spdk/likely.h"
#include <raid.h>
#include <rte_hash.h>
#include <rte_random.h>
#include <gf_vect_mul.h>
#include <erasure_code.h>

#include <infiniband/verbs.h>
#include <rdma/rdma_cma.h>
#include <rdma/rdma_verbs.h>

#define CS_WT 0 // write
#define CS_RD 1 // read
#define SD_DF 2 // send diff
#define PR_NO_DIFF 3 // p parity (do not read original value)
#define PR_DIFF 4 // p parity (read original value)
#define FWD_RO 5 // forward diff (read only)
#define FWD_RW 6 // forward diff (read and write)
#define FWD_RW_DIFF 7 // forward diff (read and write)
#define RECON_NRT 8 // d reconstructed read (no returning data)
#define RECON_RT 9 // d reconstructed read (also return data)
#define RECON_NRT_DP 10 // dp reconstructed read (no returning data)
#define RECON_NRT_DP_Q 11 // dp reconstructed read (no returning data)
#define RECON_RT_DP 12 // dp reconstructed read (also return data)
#define RECON_NRT_DD 13 // dd reconstructed read (no returning data)
#define RECON_NRT_DD_P 14 // dd reconstructed read (no returning data) for p
#define RECON_NRT_DD_Q 15 // dd reconstructed read (no returning data) for q
#define RECON_RT_DD 16 // dd reconstructed read (also return data)
#define SD_RC 17 // server to server reconstructed read
#define FWD_MIX 18
#define PR_MIX 19
#define PR_PRO 20
#define PR_QRO 21
#define FWD_RW_PRO 22
#define FWD_RO_PRO 23

struct sg_entry {
    uint64_t addr;
    uint32_t len;
    uint32_t rkey;
};

struct cs_message_t {
    uint8_t fwd_num;
    uint8_t type;
    uint8_t data_index;
    uint64_t req_id;
    int8_t next_index;
    int8_t next_index2;
    uint64_t offset;
    uint64_t length;
    uint64_t fwd_offset;
    uint64_t fwd_length;
    uint64_t y_offset;
    uint64_t y_length;
    uint8_t x_index;
    uint8_t y_index;
    uint8_t last_index;
    uint8_t num_sge[2]; // first one is for read or write, second one is for reconstruction
    struct sg_entry sgl[32]; // maximum number of allowed sg_entries is 16, same as SPDK; maximum is 32
    // first num_sge[0] are for read or write, the following num_sge[1] are for reconstruction
    // if we do not need 32 sg_entries, we will just copy part of cs_message_t into the message
};

struct cs_resp_t {
    uint64_t req_id;
};

struct rdma_send_wr_list {
    struct ibv_send_wr *first;
    struct ibv_send_wr *last;
    size_t size;
};

struct rdma_recv_wr_list {
    struct ibv_recv_wr *first;
    struct ibv_recv_wr *last;
    size_t size;
};

struct send_wr_wrapper_server;
struct recv_wr_wrapper_server;
struct bdev_context_t;

struct rdma_qp_server {
    struct rdma_cm_id *cmd_cm_id;
    struct ibv_mr *cmd_mr;
    struct spdk_mem_map *mem_map;
    uint8_t *cmd_sendbuf;
    uint8_t *cmd_recvbuf;
    struct rdma_send_wr_list send_wrs;
    struct rdma_recv_wr_list recv_wrs;
    TAILQ_HEAD(, send_wr_wrapper_server) free_send;
};

struct send_wr_wrapper_server {
    struct rdma_qp_server *rqp;
    struct ibv_send_wr send_wr;
    struct cs_message_t *cs_msg;
    struct cs_resp_t *cs_resp;
    struct ibv_send_wr send_wr_write[16];
    struct recv_wr_wrapper_server *ctx;
    TAILQ_ENTRY(send_wr_wrapper_server) link;
};

struct recv_wr_wrapper_server {
    struct rdma_qp_server *rqp;
    struct ibv_recv_wr recv_wr;
    struct ibv_send_wr send_wr_read[16];
    struct cs_message_t *cs_msg;
    struct cs_resp_t *cs_resp;
    struct bdev_context_t *bdev_context;
};

static constexpr uint64_t kMaxBlockcnt = 256 * 1024 * 1024;
static constexpr uint8_t kReqTypeRW = 3; // setup initial data
static constexpr uint8_t kReqTypePartialWrite = 4; // client to data server
static constexpr uint8_t kReqTypeParity = 5; // client to parity server
static constexpr uint8_t kReqTypePeer = 6; // data server to parity server
static constexpr uint8_t kReqTypeReconRead = 7; // reconstructed read
static constexpr uint8_t kReqTypeStart = 8; // get bdev metadata
static constexpr uint8_t kReqTypeCallback = 9; // callback to client
static constexpr size_t kMsgSize = 512 * 1024; // 512KB
static constexpr size_t kBlkSize = 512; // at least 512, a common value is 4096
static constexpr uint32_t kStartPort = 4620;
static constexpr size_t bufferSize = sizeof(struct cs_message_t);
static constexpr int paraSend = 256;
static constexpr uint16_t batch_size = 128;

enum action {
    INIT,
    RDMA_READ_COMPLETE,
    RDMA_SEND_COMPLETE,
    IO_COMPLETE,
    CALLBACK
};

enum target {
    PEER1,
    PEER2
};

enum request_state {
    /// read
    READ_START, // 0
    READ_IO_DONE,
    /// write
    WRITE_START,
    WRITE_READ_DONE,
    WRITE_IO_DONE,
    /// fwd ro
    FW_RO_START,
    FW_RO_IO_DONE,
    // raid6 only
    FW_RO_PEND1,
    /// fwd rw
    FW_RW_START,
    FW_RW_PEND1,
    FW_RW_READ_IO1_DONE, // 10
    FW_RW_PEND2,
    FW_RW_PEND3,
    // raid6 only
    FW_RW_PEND4,
    /// fwd diff
    FW_DF_START,
    FW_DF_PEND1,
    FW_DF_READ_IO1_DONE,
    FW_DF_PEND2,
    FW_DF_PEND3,
    // raid6 only
    FW_DF_PEND4,
    /// fwd mix
    FW_MIX_START, // 20
    FW_MIX_PEND1,
    FW_MIX_READ_IO1_DONE,
    FW_MIX_PEND2,
    FW_MIX_PEND3,
    // raid6 only
    FW_MIX_PEND4,
    /// fwd rw pro, raid6 only
    FW_RW_PRO_START,
    FW_RW_PRO_PEND1,
    FW_RW_PRO_READ_IO1_DONE,
    FW_RW_PRO_PEND2,
    FW_RW_PRO_PEND3, // 30
    FW_RW_PRO_PEND4,
    /// fwd ro pro, raid6 only
    FW_RO_PRO_START,
    FW_RO_PRO_IO_DONE,
    FW_RO_PRO_PEND1,
    /// pr diff
    PR_DF_START,
    PR_IO1_DONE,
    PR_IO2_DONE,
    /// pr mix
    PR_MIX_START,
    PR_MIX_PEND1,
    // raid6 only
    PR_MIX_READ_IO1_DONE, // 40
    PR_MIX_IO2_DONE,
    /// pr pro, raid6 only
    PR_PRO_START,
    PR_PRO_PEND1,
    PR_PRO_READ_IO1_DONE,
    PR_PRO_PEND2,
    PR_PRO_PEND3,
    /// pr qro, raid6 only
    PR_QRO_START,
    PR_QRO_PEND1,
    PR_QRO_READ_IO1_DONE,
    PR_QRO_PEND2, // 50
    PR_QRO_PEND3,
    /// send diff
    SEND_DF_START,
    SEND_DF_READ_DONE,
    /// recon
    RECON_START,
    RECON_IO_DONE,
    /// recon read
    RECON_READ_START,
    RECON_READ_IO_DONE,
    RECON_READ_PEND,
    /// recon diff
    RECON_DF_START,
    RECON_DF_READ_DONE, // 60
    /// end state
    RELEASE
};

struct bdev_context_t {
    // bdev related
    struct spdk_bdev *bdev;
    struct spdk_bdev_desc *bdev_desc;
    struct spdk_io_channel *bdev_io_channel;
    struct spdk_thread *bdev_thread;
    char *bdev_name;
    struct spdk_bdev_io_wait_entry bdev_io_wait;
    uint32_t blk_size;
    uint32_t blk_size_shift;
    size_t buf_align;
    uint64_t blockcnt;

    enum request_state state;
    char *buff;
    char *buff1;
    char *buff2;
    char* buff3;
    char* buff4;
    uint64_t offset;
    uint64_t size;
    bool success;
    size_t recv_cnt;
};

static inline size_t num_lcores_per_numa_node() {
    return static_cast<size_t>(numa_num_configured_cpus() / numa_num_configured_nodes());
}

static inline std::vector<size_t> get_lcores_for_numa_node(size_t numa_node) {
    assert(numa_node <= static_cast<size_t>(numa_max_node()));

    std::vector<size_t> ret;
    size_t num_lcores = static_cast<size_t>(numa_num_configured_cpus());

    for (size_t i = 0; i < num_lcores; i++) {
        if (numa_node == static_cast<size_t>(numa_node_of_cpu(i))) {
            ret.push_back(i);
        }
    }

    return ret;
}

static inline void bind_to_core(std::thread &thread, size_t numa_node,
                                size_t numa_local_index) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    assert(numa_node <= 47);

    const std::vector<size_t> lcore_vec = get_lcores_for_numa_node(numa_node);
    if (numa_local_index >= lcore_vec.size()) {
        SPDK_ERRLOG(
                "Requested binding to core %zu (zero-indexed) on NUMA node %zu, "
                "which has only %zu cores. Ignoring, but this can cause very low "
                "performance.\n",
                numa_local_index, numa_node, lcore_vec.size());
        return;
    }

    const size_t global_index = lcore_vec.at(numa_local_index);

    CPU_SET(global_index, &cpuset);
    int rc = pthread_setaffinity_np(thread.native_handle(), sizeof(cpu_set_t),
                                    &cpuset);
    assert(rc == 0);
}

static std::string g_host_ip_addr = "192.168.2.1";
static uint8_t g_phy_port = 2;

static inline uint8_t get_phy_port(void)
{
    return g_phy_port;
}

static inline uint8_t get_phy_port_by_ip_addr(std::string ip_addr)
{
    std::regex nic_100g("(192\\.168\\.2\\.)([1-9]|[1-9][0-9])");
    if (std::regex_match(ip_addr, nic_100g)) {
        return 2;
    } else {
        return 1;
    }
}

static inline std::string get_host_ip_addr(void)
{
    return g_host_ip_addr;
}

static inline bool is_32aligned(void *p)
{
    return (size_t) p % 32 == 0;
}

#endif /* BDEV_RAID_COMMON_H */
