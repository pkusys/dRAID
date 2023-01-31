#!/bin/bash

username=`whoami`
io_size=$1

sudo kill -9 $(ps aux | grep '[r]aid' | awk '{print $2}')

if [[ "$io_size" == "4" ]]
then
  ../generate_raid_config.sh 512 8 2
  ../run_server_remote_nvme.sh $username raid5 100g 512 8 2
  sleep 3
  sudo LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid5_d.conf -ioengine=../spdk_bdev -iodepth=42 -rw=randwrite -bs=4k -numjobs=2
  exit 0
fi

if [[ "$io_size" == "8" ]]
then
  ../generate_raid_config.sh 512 8 2
  ../run_server_remote_nvme.sh $username raid5 100g 512 8 2
  sleep 3
  sudo LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid5_d.conf -ioengine=../spdk_bdev -iodepth=42 -rw=randwrite -bs=8k -numjobs=2
  exit 0
fi

if [[ "$io_size" == "16" ]]
then
  ../generate_raid_config.sh 512 8 2
  ../run_server_remote_nvme.sh $username raid5 100g 512 8 2
  sleep 3
  sudo LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid5_d.conf -ioengine=../spdk_bdev -iodepth=41 -rw=randwrite -bs=16k -numjobs=2
  exit 0
fi

if [[ "$io_size" == "32" ]]
then
  ../generate_raid_config.sh 512 8 2
  ../run_server_remote_nvme.sh $username raid5 100g 512 8 2
  sleep 3
  sudo LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid5_d.conf -ioengine=../spdk_bdev -iodepth=32 -rw=randwrite -bs=32k -numjobs=2
  exit 0
fi

if [[ "$io_size" == "64" ]]
then
  ../generate_raid_config.sh 512 8 2
  ../run_server_remote_nvme.sh $username raid5 100g 512 8 2
  sleep 3
  sudo LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid5_d.conf -ioengine=../spdk_bdev -iodepth=24 -rw=randwrite -bs=64k -numjobs=2
  exit 0
fi

if [[ "$io_size" == "128" ]]
then
  ../generate_raid_config.sh 512 8 2
  ../run_server_remote_nvme.sh $username raid5 100g 512 8 2
    sleep 3
  sudo LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid5_d.conf -ioengine=../spdk_bdev -iodepth=16 -rw=randwrite -bs=128k -numjobs=2
  exit 0
fi

echo "We do not have a pre-written command for this I/O size. Feel free to run your own command."