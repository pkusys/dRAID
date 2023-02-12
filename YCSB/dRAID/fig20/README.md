# dRAID - Figure 20

1. Compile mkfs plugin for dRAID and YCSB:
```Bash
cd ~/dRAID/YCSB/dRAID
./compile.sh objstore
```

2. 
### Run the experiment for all data points
```Bash
cd ~/dRAID/FIO/dRAID/fig20
./run_all.sh
```

### Run the experiment for an individual data point

For each workload, run:
```Bash
cd ~/dRAID/YCSB/dRAID/fig20
./run.sh <workload> # must be one of [A,B,C,D,F]
```