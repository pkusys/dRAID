# SPDK - Figure 20

1. Compile mkfs plugin for SPDK and YCSB:
```Bash
cd ~/dRAID/YCSB/SPDK
./compile.sh objstore
```

2. Generate the host-side configuration file on node0:
```Bash
cd ~/dRAID/YCSB/SPDK/fig20
../generate_raid_config.sh 512 8 1
```

3. For each workload, run:
```Bash
./run.sh <workload> # must be one of [A,B,C,D,F]
```