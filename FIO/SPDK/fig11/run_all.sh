#!/bin/bash

sudo rm -r results
mkdir results

echo "generating results on 32K chunk size"
../generate_raid_config.sh 32 8 1
sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=22 -rw=randrw -rwmixread=0 -bs=128k -numjobs=2 ../raid5_spdk.conf > results/32K.log
echo "generating results on 64K chunk size"
../generate_raid_config.sh 64 8 1
sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=16 -rw=randrw -rwmixread=0 -bs=128k -numjobs=2 ../raid5_spdk.conf > results/64K.log
echo "generating results on 128K chunk size"
../generate_raid_config.sh 128 8 1
sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=14 -rw=randrw -rwmixread=0 -bs=128k -numjobs=2 ../raid5_spdk.conf > results/128K.log
echo "generating results on 256K chunk size"
../generate_raid_config.sh 256 8 1
sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=14 -rw=randrw -rwmixread=0 -bs=128k -numjobs=2 ../raid5_spdk.conf > results/256K.log
echo "generating results on 512K chunk size"
../generate_raid_config.sh 512 8 1
sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=10 -rw=randrw -rwmixread=0 -bs=128k -numjobs=2 ../raid5_spdk.conf > results/512K.log
echo "generating results on 1024K chunk size"
../generate_raid_config.sh 1024 8 1
sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=12 -rw=randrw -rwmixread=0 -bs=128k -numjobs=2 ../raid5_spdk.conf > results/1024K.log
