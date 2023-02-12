# SPDK RAID - Figure 16

### Run the experiment for all data points
```Bash
cd ~/dRAID/FIO/SPDK/fig16
./run_all.sh
```

### Run the experiment for an individual data point

For each of the stripe width, run:
```Bash
cd ~/dRAID/FIO/SPDK/fig16

./run.sh <stripe_width> # must be one of [4,6,8,10,12,14,16,18]
```