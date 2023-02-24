# dRAID - Figure 9

### Run the experiment for all data points
```Bash
cd ~/dRAID/FIO/dRAID/fig9
./run_all.sh # rerun until you see no more output. if a data point keeps failing to generate, consider using run.sh to generate it.
```

### Run the experiment for an individual data point

For each of the I/O size, run:
```Bash
cd ~/dRAID/FIO/dRAID/fig9

./run.sh <io_size_in_kb> # must be one of [4,8,16,32,64,128]
# enter yes when it prompts
```