# Linux RAID - Figure 14a

### Run the experiment for all data points
```Bash
cd ~/dRAID/FIO/Linux/fig14a
./run_all.sh # enter y when it prompts
```

### Run the experiment for an individual data point

1. mount RAID by running:
```Bash
cd ~/dRAID/FIO/Linux/fig14a
./mount.sh # enter y when it prompts
```

2. Run the experiment:
```Bash
./run.sh <io_depth> # must be one of [1,2,3,4,5,6]
```

3. Once you are done, unmount RAID by running:
```Bash
./unmount.sh
```