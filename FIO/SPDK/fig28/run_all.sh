#!/bin/bash

../generate_raid_config.sh 512 8 1
sudo rm -r results
mkdir results

echo "generating results on 4K I/O size"
sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=18 -rw=randread -rwmixread=0 -bs=4k -numjobs=2 ../raid6_spdk_d.conf > results/4K.log
echo "generating results on 8K I/O size"
sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=18 -rw=randread -rwmixread=0 -bs=8k -numjobs=2 ../raid6_spdk_d.conf > results/8K.log
echo "generating results on 16K I/O size"
sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=20 -rw=randread -rwmixread=0 -bs=16k -numjobs=2 ../raid6_spdk_d.conf > results/16K.log
echo "generating results on 32K I/O size"
sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=20 -rw=randread -rwmixread=0 -bs=32k -numjobs=2 ../raid6_spdk_d.conf > results/32K.log
echo "generating results on 64K I/O size"
sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=14 -rw=randread -rwmixread=0 -bs=64k -numjobs=2 ../raid6_spdk_d.conf > results/64K.log
echo "generating results on 128K I/O size"
sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=11 -rw=randread -rwmixread=0 -bs=128k -numjobs=2 ../raid6_spdk_d.conf > results/128K.log
