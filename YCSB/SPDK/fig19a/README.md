# SPDK - Figure 19a

1. Compile mkfs plugin for SPDK and YCSB:
```Bash
cd ~/dRAID/YCSB/SPDK
./compile.sh rocksdb
```

2. Generate the host-side configuration file on node0:
```Bash
cd ~/dRAID/YCSB/SPDK/fig19a
../generate_raid_config.sh 512 8 1
```

3. For each workload, run:
```Bash
./run.sh <workload> # must be one of [A,B,C,D,F]
```