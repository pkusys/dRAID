# Linux RAID - Figure 23

### Run the experiment for all data points
```Bash
cd ~/dRAID/FIO/Linux/fig23
./run_all.sh # enter y when it prompts
```

### Run the experiment for an individual data point

1. mount RAID by running:
```Bash
cd ~/dRAID/FIO/Linux/fig23
./mount.sh # enter y when it prompts
```

2. Run the experiment:
```Bash
./run.sh <io_size_in_kb> # must be one of [4,8,16,32,64,128,256,512,1024,2048,3072]
```

3. Once you are done, unmount RAID by running:
```Bash
./unmount.sh
```