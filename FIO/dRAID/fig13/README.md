# dRAID - Figure 13

### Run the experiment for all data points
```Bash
cd ~/dRAID/FIO/dRAID/fig13
./run_all.sh # rerun until you see no more output. if a data point keeps failing to generate, consider using run.sh to generate it.
```

### Run the experiment for an individual data point

For each of the read ratio, run:
```Bash
cd ~/dRAID/FIO/dRAID/fig13

./run.sh <read_ratio> # must be one of [0,25,50,75,100]
# enter yes when it prompts
```