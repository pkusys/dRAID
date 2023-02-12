#!/bin/bash

../generate_raid_config.sh 512 6 1
sudo rm -r results
mkdir results

echo "generating results on 4K I/O size"
sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=40 -rw=randread -bs=4k -numjobs=2 ../raid6_spdk.conf > results/4K.log
echo "generating results on 8K I/O size"
sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=50 -rw=randread -bs=8k -numjobs=2 ../raid6_spdk.conf > results/8K.log
echo "generating results on 16K I/O size"
sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=44 -rw=randread -bs=16k -numjobs=2 ../raid6_spdk.conf > results/16K.log
echo "generating results on 32K I/O size"
sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=34 -rw=randread -bs=32k -numjobs=2 ../raid6_spdk.conf > results/32K.log
echo "generating results on 64K I/O size"
sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=25 -rw=randread -bs=64k -numjobs=2 ../raid6_spdk.conf > results/64K.log
echo "generating results on 128K I/O size"
sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=17 -rw=randread -bs=128k -numjobs=2 ../raid6_spdk.conf > results/128K.log
