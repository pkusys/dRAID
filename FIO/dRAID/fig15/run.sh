#!/bin/bash

username=`whoami`
io_size=$1

mkdir -p results
sudo kill -9 $(ps aux | grep '[r]aid' | awk '{print $2}')

if [[ "$io_size" == "4" ]]
then
  ../generate_raid_config.sh 512 8 2
  ../run_server_remote_nvme.sh $username raid5 100g 512 8 2
  sleep 3
  sudo LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid5_d.conf -ioengine=../spdk_bdev -iodepth=24 -rw=randrw -rwmixread=100 -bs=4k -numjobs=2 > results/${io_size}K.log
  exit 0
fi

if [[ "$io_size" == "8" ]]
then
  ../generate_raid_config.sh 512 8 2
  ../run_server_remote_nvme.sh $username raid5 100g 512 8 2
  sleep 3
  sudo LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid5_d.conf -ioengine=../spdk_bdev -iodepth=24 -rw=randrw -rwmixread=100 -bs=8k -numjobs=2 > results/${io_size}K.log
  exit 0
fi

if [[ "$io_size" == "16" ]]
then
  ../generate_raid_config.sh 512 8 2
  ../run_server_remote_nvme.sh $username raid5 100g 512 8 2
  sleep 3
  sudo LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid5_d.conf -ioengine=../spdk_bdev -iodepth=24 -rw=randrw -rwmixread=100 -bs=16k -numjobs=2 > results/${io_size}K.log
  exit 0
fi

if [[ "$io_size" == "32" ]]
then
  ../generate_raid_config.sh 512 8 2
  ../run_server_remote_nvme.sh $username raid5 100g 512 8 2
  sleep 3
  sudo LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid5_d.conf -ioengine=../spdk_bdev -iodepth=24 -rw=randrw -rwmixread=100 -bs=32k -numjobs=2 > results/${io_size}K.log
  exit 0
fi

if [[ "$io_size" == "64" ]]
then
  ../generate_raid_config.sh 512 8 2
  ../run_server_remote_nvme.sh $username raid5 100g 512 8 2
  sleep 3
  sudo LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid5_d.conf -ioengine=../spdk_bdev -iodepth=20 -rw=randrw -rwmixread=100 -bs=64k -numjobs=2 > results/${io_size}K.log
  exit 0
fi

if [[ "$io_size" == "128" ]]
then
  ../generate_raid_config.sh 512 8 2
  ../run_server_remote_nvme.sh $username raid5 100g 512 8 2
  sleep 3
  sudo LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid5_d.conf -ioengine=../spdk_bdev -iodepth=16 -rw=randrw -rwmixread=100 -bs=128k -numjobs=2 > results/${io_size}K.log
  exit 0
fi

echo "We do not have a pre-written command for this I/O size. Feel free to run your own command."