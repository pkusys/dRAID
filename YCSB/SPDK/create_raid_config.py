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
    config = dict()
    config["method"] = "bdev_raid_create"
    params = dict()
    params["name"] = "Raid0"
    params["strip_size_kb"] = int(sys.argv[3])
    params["raid_level"] = sys.argv[4]
    params["num_qp"] = int(sys.argv[6])
    base_rpcs = list()
    size = int(sys.argv[5])
    for i in range(1, size + 1):
        base_rpc = dict()
        base_rpc["uri"] = ip_addr_list[i].strip()
        if i == 1 and sys.argv[7] == 'true':
            base_rpc["degraded"] = True
        else:
            base_rpc["degraded"] = False
        base_rpcs.append(base_rpc)
    params["base_rpcs"] = base_rpcs
    config["params"] = params
    configs.append(config)
    subsystem["config"] = configs
    subsystems.append(subsystem)
    conf["subsystems"] = subsystems
    json_string = json.dumps(conf, indent=2)

    with open(sys.argv[2], "w", newline="") as f:
        f.write(json_string)
