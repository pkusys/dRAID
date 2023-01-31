# SPDK RAID - Figure 15

1. Generate the host-side configuration file on node0:
```Bash
cd ~/dRAID/FIO/SPDK/fig15
../generate_raid_config.sh 512 8 1
```

2. For each of the I/O size, run:
```Bash
./run.sh <io_size_in_kb> # must be one of [4,8,16,32,64,128]
```