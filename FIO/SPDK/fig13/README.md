# SPDK RAID - Figure 13

### Run the experiment for all data points
```Bash
cd ~/dRAID/FIO/SPDK/fig13
./run_all.sh
```

### Run the experiment for an individual data point

1. Generate the host-side configuration file on node0:
```Bash
cd ~/dRAID/FIO/SPDK/fig13
../generate_raid_config.sh 512 8 1
```

2. For each of the read ratio, run:
```Bash
./run.sh <read_ratio> # must be one of [0,25,50,75,100]
```