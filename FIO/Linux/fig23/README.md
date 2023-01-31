# Linux RAID - Figure 23


1. mount RAID by running :
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