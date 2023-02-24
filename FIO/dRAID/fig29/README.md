# dRAID - Figure 29

### Run the experiment for all data points
```Bash
cd ~/dRAID/FIO/dRAID/fig29
./run_all.sh # rerun until you see no more output. if a data point keeps failing to generate, consider using run.sh to generate it.
```

### Run the experiment for an individual data point

For each of the stripe width, run:
```Bash
cd ~/dRAID/FIO/dRAID/fig29

./run.sh ./run.sh <stripe_width> # must be one of [4,6,8,10,12,14,16,18]
# enter yes when it prompts
```