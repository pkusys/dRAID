# Linux RAID - Figure 26

### Run the experiment for all data points
```Bash
cd ~/dRAID/FIO/Linux/fig26
./run_all.sh # enter y when it prompts
```

### Run the experiment for an individual data point

1. mount RAID by running:
```Bash
cd ~/dRAID/FIO/Linux/fig26
./mount.sh # enter y when it prompts
```

2. Run the experiment:
```Bash
./run.sh <read_ratio> # must be one of [0,25,50,75,100]
```

3. Once you are done, unmount RAID by running:
```Bash
./unmount.sh
```