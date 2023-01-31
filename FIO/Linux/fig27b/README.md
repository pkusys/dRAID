# Linux RAID - Figure 14b


1. mount RAID by running :
```Bash
cd ~/dRAID/FIO/Linux/fig14b
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