# dRAID - Figure 11

### Run the experiment for all data points
```Bash
cd ~/dRAID/FIO/dRAID/fig11
./run_all.sh # rerun until you see no more output. if a data point keeps failing to generate, consider using run.sh to generate it.
```

### Run the experiment for an individual data point

For each of the chunk size, run:
```Bash
cd ~/dRAID/FIO/dRAID/fig11

./run.sh <chunk_size_in_kb> # must be one of [32,64,128,256,512,1024]
# enter yes when it prompts
```