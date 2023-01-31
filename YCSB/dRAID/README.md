# dRAID YCSB Experiments

If not specified, all the following operations are from node0, which servers as the host. 

>Make sure you have successfully run the basic test under `dRAID/` before you proceed.

1. Setup SPDK on node0:
```Bash
cd ~/draid-spdk/scripts
sudo ./setup.sh reset
sudo HUGEMEM=90000 PCI_BLOCKED="0000:c5:00.0 0000:c6:00.0" ./setup.sh
```

2. Follow the instruction of each experiment to reproduce the results.