# dRAID - Figure 19a

1. Compile mkfs plugin for dRAID and YCSB:
```Bash
cd ~/dRAID/YCSB/dRAID
./compile.sh rocksdb
```

2. For each workload, run:
```Bash
cd ~/dRAID/YCSB/dRAID/fig19a
./run.sh <workload> # must be one of [A,B,C,D,F]
```