# Linux RAID - Figure 25

### Run the experiment for all data points
```Bash
cd ~/dRAID/FIO/Linux/fig25
./run_all.sh # enter y when it prompts
```

### Run the experiment for an individual data point

For each of the stripe width, run:
```Bash
cd ~/dRAID/FIO/Linux/fig25

./run.sh <stripe_width> # must be one of [4,6,8,10,12,14,16,18]
# enter y when it prompts 
```