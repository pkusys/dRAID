#!/bin/bash

../generate_raid_config.sh 512 8 1
sudo rm -r results
mkdir results

echo "generating results on 4K I/O size"
sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=44 -rw=randwrite -bs=4k -numjobs=2 ../raid5_spdk.conf > results/4K.log
echo "generating results on 8K I/O size"
sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=48 -rw=randwrite -bs=8k -numjobs=2 ../raid5_spdk.conf > results/8K.log
echo "generating results on 16K I/O size"
sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=32 -rw=randwrite -bs=16k -numjobs=2 ../raid5_spdk.conf > results/16K.log
echo "generating results on 32K I/O size"
sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=20 -rw=randwrite -bs=32k -numjobs=2 ../raid5_spdk.conf > results/32K.log
echo "generating results on 64K I/O size"
sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=15 -rw=randwrite -bs=64k -numjobs=2 ../raid5_spdk.conf > results/64K.log
echo "generating results on 128K I/O size"
sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=10 -rw=randwrite -bs=128k -numjobs=2 ../raid5_spdk.conf > results/128K.log
echo "generating results on 256K I/O size"
sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=4 -rw=randwrite -bs=256k -numjobs=4 ../raid5_spdk.conf > results/256K.log
echo "generating results on 512K I/O size"
sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=3 -rw=randwrite -bs=512k -numjobs=4 ../raid5_spdk.conf > results/512K.log
echo "generating results on 1024K I/O size"
sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=4 -rw=randwrite -bs=1024k -numjobs=2 ../raid5_spdk.conf > results/1024K.log
echo "generating results on 2048K I/O size"
sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=5 -rw=randwrite -bs=2048k -numjobs=2 ../raid5_spdk.conf > results/2048K.log
echo "generating results on 3584K I/O size"
sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=2 -rw=randwrite -bs=3584k -numjobs=4 ../raid5_spdk.conf > results/3584K.log
