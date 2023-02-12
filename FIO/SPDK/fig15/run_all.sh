#!/bin/bash

../generate_raid_config.sh 512 8 1
sudo rm -r results
mkdir results

echo "generating results on 4K I/O size"
sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=12 -rw=randread -rwmixread=0 -bs=4k -numjobs=3 ../raid5_spdk_d.conf > results/4K.log
echo "generating results on 8K I/O size"
sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=9 -rw=randread -rwmixread=0 -bs=8k -numjobs=4 ../raid5_spdk_d.conf > results/8K.log
echo "generating results on 16K I/O size"
sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=10 -rw=randread -rwmixread=0 -bs=16k -numjobs=4 ../raid5_spdk_d.conf > results/16K.log
echo "generating results on 32K I/O size"
sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=10 -rw=randread -rwmixread=0 -bs=32k -numjobs=4 ../raid5_spdk_d.conf > results/32K.log
echo "generating results on 64K I/O size"
sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=7 -rw=randread -rwmixread=0 -bs=64k -numjobs=4 ../raid5_spdk_d.conf > results/64K.log
echo "generating results on 128K I/O size"
sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=5 -rw=randread -rwmixread=0 -bs=128k -numjobs=4 ../raid5_spdk_d.conf > results/128K.log
