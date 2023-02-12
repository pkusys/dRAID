#!/bin/bash

sudo rm -r results
mkdir results

echo "generating results on stripe width 4"
../generate_raid_config.sh 512 4 1
sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=3 -rw=randread -rwmixread=0 -bs=128k -numjobs=4 ../raid5_spdk_d.conf > results/4.log
echo "generating results on stripe width 6"
../generate_raid_config.sh 512 6 1
sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=5 -rw=randread -rwmixread=0 -bs=128k -numjobs=4 ../raid5_spdk_d.conf > results/6.log
echo "generating results on stripe width 8"
../generate_raid_config.sh 512 8 1
sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=5 -rw=randread -rwmixread=0 -bs=128k -numjobs=4 ../raid5_spdk_d.conf > results/8.log
echo "generating results on stripe width 10"
../generate_raid_config.sh 512 10 1
sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=5 -rw=randread -rwmixread=0 -bs=128k -numjobs=4 ../raid5_spdk_d.conf > results/10.log
echo "generating results on stripe width 12"
../generate_raid_config.sh 512 12 1
sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=5 -rw=randread -rwmixread=0 -bs=128k -numjobs=4 ../raid5_spdk_d.conf > results/12.log
echo "generating results on stripe width 14"
../generate_raid_config.sh 512 14 1
sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=5 -rw=randread -rwmixread=0 -bs=128k -numjobs=4 ../raid5_spdk_d.conf > results/14.log
echo "generating results on stripe width 16"
../generate_raid_config.sh 512 16 1
sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=4 -rw=randread -rwmixread=0 -bs=128k -numjobs=4 ../raid5_spdk_d.conf > results/16.log
echo "generating results on stripe width 18"
../generate_raid_config.sh 512 18 1
sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=6 -rw=randread -rwmixread=0 -bs=128k -numjobs=2 ../raid5_spdk_d.conf > results/18.log
