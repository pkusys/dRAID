#!/bin/bash

username=`whoami`
io_size=$1

sudo kill -9 $(ps aux | grep '[r]aid' | awk '{print $2}')

if [[ "$io_size" == "4" ]]
then
  ../generate_raid_config.sh 512 8 2
  ../run_server_remote_nvme.sh $username raid5 100g 512 8 2
  sleep 3
  sudo LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid5.conf -ioengine=../spdk_bdev -iodepth=60 -rw=randwrite -bs=4k -numjobs=2
  exit 0
fi

if [[ "$io_size" == "8" ]]
then
  ../generate_raid_config.sh 512 8 2
  ../run_server_remote_nvme.sh $username raid5 100g 512 8 2
  sleep 3
  sudo LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid5.conf -ioengine=../spdk_bdev -iodepth=60 -rw=randwrite -bs=8k -numjobs=2
  exit 0
fi

if [[ "$io_size" == "16" ]]
then
  ../generate_raid_config.sh 512 8 2
  ../run_server_remote_nvme.sh $username raid5 100g 512 8 2
  sleep 3
  sudo LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid5.conf -ioengine=../spdk_bdev -iodepth=52 -rw=randwrite -bs=16k -numjobs=2
  exit 0
fi

if [[ "$io_size" == "32" ]]
then
  ../generate_raid_config.sh 512 8 2
  ../run_server_remote_nvme.sh $username raid5 100g 512 8 2
  sleep 3
  sudo LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid5.conf -ioengine=../spdk_bdev -iodepth=36 -rw=randwrite -bs=32k -numjobs=2
  exit 0
fi

if [[ "$io_size" == "64" ]]
then
  ../generate_raid_config.sh 512 8 2
  ../run_server_remote_nvme.sh $username raid5 100g 512 8 2
  sleep 3
  sudo LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid5.conf -ioengine=../spdk_bdev -iodepth=26 -rw=randwrite -bs=64k -numjobs=2
  exit 0
fi

if [[ "$io_size" == "128" ]]
then
  ../generate_raid_config.sh 512 8 2
  ../run_server_remote_nvme.sh $username raid5 100g 512 8 2
  sleep 3
  sudo LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid5.conf -ioengine=../spdk_bdev -iodepth=17 -rw=randwrite -bs=128k -numjobs=2
  exit 0
fi

if [[ "$io_size" == "256" ]]
then
  ../generate_raid_config.sh 512 8 2
  ../run_server_remote_nvme.sh $username raid5 100g 512 8 2
  sleep 3
  sudo LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid5.conf -ioengine=../spdk_bdev -iodepth=12 -rw=randwrite -bs=256k -numjobs=2
  exit 0
fi

if [[ "$io_size" == "512" ]]
then
  ../generate_raid_config.sh 512 8 2
  ../run_server_remote_nvme.sh $username raid5 100g 512 8 2
  sleep 3
  sudo LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid5.conf -ioengine=../spdk_bdev -iodepth=8 -rw=randwrite -bs=512k -numjobs=2
  exit 0
fi

if [[ "$io_size" == "1024" ]]
then
  ../generate_raid_config.sh 512 8 2
  ../run_server_remote_nvme.sh $username raid5 100g 512 8 2
  sleep 3
  sudo LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid5.conf -ioengine=../spdk_bdev -iodepth=5 -rw=randwrite -bs=1024k -numjobs=2
  exit 0
fi

if [[ "$io_size" == "2048" ]]
then
  ../generate_raid_config.sh 512 8 2
  ../run_server_remote_nvme.sh $username raid5 100g 512 8 2
  sleep 3
  sudo LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid5.conf -ioengine=../spdk_bdev -iodepth=5 -rw=randwrite -bs=2048k -numjobs=2
  exit 0
fi

if [[ "$io_size" == "3584" ]]
then
  ../generate_raid_config.sh 512 8 2
  ../run_server_remote_nvme.sh $username raid5 100g 512 8 2
  sleep 3
  sudo LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid5.conf -ioengine=../spdk_bdev -iodepth=2 -rw=randwrite -bs=3584k -numjobs=2
  exit 0
fi

echo "We do not have a pre-written command for this I/O size. Feel free to run your own command."