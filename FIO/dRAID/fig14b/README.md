# dRAID - Figure 14b

### Run the experiment for all data points
```Bash
cd ~/dRAID/FIO/dRAID/fig14b
./run_all.sh # rerun until you see no more output. if a data point keeps failing to generate, consider using run.sh to generate it.
```

### Run the experiment for an individual data point

For each of the I/O depth, run:
```Bash
cd ~/dRAID/FIO/dRAID/fig14b

./run.sh <io_depth> # must be one of [2,4,8,12,16,24,32,48,64,96,128,144]
# enter yes when it prompts
```
***
Note: This is the experiment that you will often run into race conditions during the startup. Please be patient and rerun it for a few times.:kissing_heart: