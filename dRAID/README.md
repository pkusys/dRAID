# dRAID Implementation

## Contents

- `dRAID/` contains core implementation of dRAID and is adapted from SPDK.
  - `shared/` contains global variables and parameters shared by all code.
  - `host/` contains the host-side implementation.
    - `bdev_raid.h` & `bdev_raid.cc` contain the common controller logic shared by both RAID-5 and RAID-6.
    - `raid5.cc` contains core implementation of RAID-5 host controller.
    - `raid6.cc` contains core implementation of RAID-6 host controller.
    - `bdev_raid_rpc.cc` is for SPDK RPC framework integration
    - `rpc_raid_main.cc` is the basic test program for dRAID for debugging purpose.
  - `server/` contains the server-side implementation.
    - `raid5.cc` contains core implementation of RAID-5 server controller.
    - `raid6.cc` contains core implementation of RAID-6 server controller.
    - `nvme0.json` is the configuration for NVMe SSD. Note: this PCIe address is hardcoded for CloudLab testbed. You probably need to change it if you use a different testbed.
    - `malloc0.json` is the configuration for RAM Disk.
  - `scripts/` contains scripts to run the basic test program.

## Prerequisite

You must complete all the CloudLab setup steps under `setup/` before you proceed further.

## Compile

You only need to compile host-side code on the first node (node0). To compile the host-side code, run:
```Bash
cd ~/dRAID/dRAID/host
make rpc_raid_main
```

## Run Basic Test Program

This program uses RAM disk to verify dRAID read/write functionality. You will need the first 4 nodes of your testbed to run this program. Run basic test script on node0::
```Bash
cd ~/dRAID/dRAID/scripts
./run_basic_test.sh # enter yes when it prompts
```
- You should see `bdev io write completed successfully` and `bdev io read completed successfully`.
- Ignore the error message `io_device Raid0 not unregistered`. We have not implemented graceful shutdown yet.