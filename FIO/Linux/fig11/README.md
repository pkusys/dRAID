# Linux RAID - Figure 11

### Run the experiment for all data points
```Bash
cd ~/dRAID/FIO/Linux/fig11
./run_all.sh # enter y when it prompts
```

### Run the experiment for an individual data point

For each of the chunk size, run:
```Bash
cd ~/dRAID/FIO/Linux/fig11

./run.sh <chunk_size_in_kb> # must be one of [32,64,128,256,512,1024]
# enter y when it prompts

```