# dRAID - Figure 27a

### Run the experiment for all data points
```Bash
cd ~/dRAID/FIO/dRAID/fig27a
./run_all.sh
```

### Run the experiment for an individual data point

For each of the I/O depth, run:
```Bash
cd ~/dRAID/FIO/dRAID/fig27a

./run.sh <io_depth> # must be one of [2,4,8,16,32,40,48,56,64,72,80,88,104,128]
# enter yes when it prompts
```
***
Note: This is the experiment that you will often run into race conditions during the startup. Please be patient and rerun it for a few times.:kissing_heart: