# Linux RAID - Figure 13


1. mount RAID by running :
```Bash
cd ~/dRAID/FIO/Linux/fig13
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