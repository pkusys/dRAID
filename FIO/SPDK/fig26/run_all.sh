#!/bin/bash

../generate_raid_config.sh 512 8 1
sudo rm -r results
mkdir results

echo "generating results on read ratio 0"
sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=6 -rw=randwrite -bs=128k -numjobs=2 ../raid6_spdk.conf > results/0.log
echo "generating results on read ratio 25"
sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=12 -rw=randrw -rwmixread=25 -bs=128k -numjobs=2 ../raid6_spdk.conf > results/25.log
echo "generating results on read ratio 50"
sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=17 -rw=randrw -rwmixread=50 -bs=128k -numjobs=2 ../raid6_spdk.conf > results/50.log
echo "generating results on read ratio 75"
sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=12 -rw=randrw -rwmixread=75 -bs=128k -numjobs=2 ../raid6_spdk.conf > results/75.log
echo "generating results on read ratio 100"
sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=12 -rw=randrw -rwmixread=100 -bs=128k -numjobs=2 ../raid6_spdk.conf > results/100.log
