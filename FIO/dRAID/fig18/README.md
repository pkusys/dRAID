# dRAID - Figure 18

### Run the experiment for all data points
```Bash
cd ~/dRAID/FIO/dRAID/fig18
./run_all.sh
```

### Run the experiment for an individual data point

For each of the I/O size, run:
```Bash
cd ~/dRAID/FIO/dRAID/fig18

./run.sh <io_size_in_kb> # must be one of [4,8,16,32,64,128]
# enter yes when it prompts
```