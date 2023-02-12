# SPDK RAID - Figure 10

### Run the experiment for all data points
```Bash
cd ~/dRAID/FIO/SPDK/fig10
./run_all.sh
```

### Run the experiment for an individual data point

1. Generate the host-side configuration file on node0:
```Bash
cd ~/dRAID/FIO/SPDK/fig10
../generate_raid_config.sh 512 8 1
```

2. For each of the I/O size, run:
```Bash
./run.sh <io_size_in_kb> # must be one of [4,8,16,32,64,128,256,512,1024,2048,3584]
```