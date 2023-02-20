# dRAID FIO Experiments

If not specified, all the following operations are from node0, which serves as the host. 

>Make sure you have successfully run the basic test under `dRAID/` before you proceed.

1. Setup SPDK on node0:
```Bash
cd ~/draid-spdk/scripts
sudo ./setup.sh reset
sudo HUGEMEM=90000 PCI_BLOCKED="0000:c5:00.0 0000:c6:00.0" ./setup.sh
```

2. Compile FIO plugin for dRAID:
```Bash
cd ~/dRAID/FIO/dRAID
make spdk_bdev
```

3. Follow the instruction of each experiment to reproduce the results.
   - We have not thoroughly tested `run_all.sh` yet, and they may contain bugs.
   - If `run_all.sh` does not work, please manually generate each data point.
   - Figure 17 requires a different testbed setup and a bit code hacking, which we do not include in this artifact.
   - Ignore the warning `RPC client command timeout`. It does not affect the experiments.
   - Ignore the error message `io_device Raid0 not unregistered`. We have not implemented graceful shutdown yet.
   - If you see RDMA related errors or the experiment hangs at the beginning (more often observed for large stripe width), this is due to race conditions caused by imperfect implementation of the start-up process. You can safely kill and rerun the command.