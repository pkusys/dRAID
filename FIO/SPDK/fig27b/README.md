# SPDK RAID - Figure 27b

1. Generate the host-side configuration file on node0:
```Bash
cd ~/dRAID/FIO/SPDK/fig27b
../generate_raid_config.sh 512 18 1
```

2. For each of the io depth, run:
```Bash
./run.sh <io_depth> # must be one of [4,8,12,16,24,32,36,42,48,60]
```