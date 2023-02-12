# SPDK RAID - Figure 24

### Run the experiment for all data points
```Bash
cd ~/dRAID/FIO/SPDK/fig24
./run_all.sh
```

### Run the experiment for an individual data point

For each of the chunk size, run:
```Bash
cd ~/dRAID/FIO/SPDK/fig24

./run.sh <chunk_size_in_kb> # must be one of [32,64,128,256,512,1024]
```