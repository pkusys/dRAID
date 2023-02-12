# SPDK RAID - Figure 30

### Run the experiment for all data points
```Bash
cd ~/dRAID/FIO/SPDK/fig30
./run_all.sh
```

### Run the experiment for an individual data point

1. Generate the host-side configuration file on node0:
```Bash
cd ~/dRAID/FIO/SPDK/fig30
../generate_raid_config.sh 512 8 1
```

2. For each of the I/O size, run:
```Bash
./run.sh <io_size_in_kb> # must be one of [4,8,16,32,64,128]
```