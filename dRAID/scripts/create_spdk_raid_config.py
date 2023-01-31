import os
import json
import sys

if __name__ == '__main__':
    src = open(sys.argv[1], 'r')
    ip_addr_list = src.readlines()
    src.close()

    conf = dict()
    subsystems = list()
    subsystem = dict()
    subsystem["subsystem"] = "bdev"
    configs = list()
    size = int(sys.argv[5])
    node_index = list()
    node_index.append(int(ip_addr_list[0].strip().split('.')[3]) - 1)
    for i in range(1, size + 1):
        config = dict()
        config["method"] = "bdev_nvme_attach_controller"
        params = dict()
        params["name"] = "Nvmf" + str(i)
        params["trtype"] = "rdma"
        params["traddr"] = ip_addr_list[i].strip()
        node_index.append(int(params["traddr"].split('.')[3]) - 1)
        params["trsvcid"] = "4420"
        params["subnqn"] = "nqn.2016-06.io.spdk:cnode" + str(node_index[i])
        params["adrfam"] = "IPv4"
        config["params"] = params
        configs.append(config)
    config = dict()
    config["method"] = "bdev_raid_create"
    params = dict()
    params["name"] = "Raid0"
    params["strip_size_kb"] = int(sys.argv[3])
    params["raid_level"] = sys.argv[4]
    if sys.argv[6] == 'true':
        params["degraded"] = True
    else:
        params["degraded"] = False
    base_bdevs = list()
    for i in range(1, size + 1):
        base_bdev = "Nvmf" + str(i) + "n1"
        base_bdevs.append(base_bdev)
    params["base_bdevs"] = base_bdevs
    config["params"] = params
    configs.append(config)
    subsystem["config"] = configs
    subsystems.append(subsystem)
    conf["subsystems"] = subsystems
    json_string = json.dumps(conf, indent=2)

    with open(sys.argv[2], "w", newline="") as f:
        f.write(json_string)
