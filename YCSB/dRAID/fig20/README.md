# dRAID - Figure 20

1. Compile mkfs plugin for dRAID and YCSB:
```Bash
cd ~/dRAID/YCSB/dRAID
./compile.sh objstore
```

2. For each workload, run:
```Bash
cd ~/dRAID/YCSB/dRAID/fig20
./run.sh <workload> # must be one of [A,B,C,D,F]
```