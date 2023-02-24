#!/bin/bash

username=`whoami`
chunk_size=$1

mkdir -p results
sudo kill -9 $(ps aux | grep '[r]aid' | awk '{print $2}')

if [[ "$chunk_size" == "32" ]]
then
  ../generate_raid_config.sh 32 8 2
  ../run_server_remote_nvme.sh $username raid6 100g 32 8 2
  sleep 3
  sudo LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid6.conf -ioengine=../spdk_bdev -iodepth=22 -rw=randwrite -bs=128k -numjobs=2 > results/${chunk_size}K.log
  exit 0
fi

if [[ "$chunk_size" == "64" ]]
then
  ../generate_raid_config.sh 64 8 2
  ../run_server_remote_nvme.sh $username raid6 100g 64 8 2
  sleep 3
  sudo LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid6.conf -ioengine=../spdk_bdev -iodepth=16 -rw=randwrite -bs=128k -numjobs=2 > results/${chunk_size}K.log
  exit 0
fi

if [[ "$chunk_size" == "128" ]]
then
  ../generate_raid_config.sh 128 8 2
  ../run_server_remote_nvme.sh $username raid6 100g 128 8 2
  sleep 3
  sudo LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid6.conf -ioengine=../spdk_bdev -iodepth=12 -rw=randwrite -bs=128k -numjobs=2 > results/${chunk_size}K.log
  exit 0
fi

if [[ "$chunk_size" == "256" ]]
then
  ../generate_raid_config.sh 256 8 2
  ../run_server_remote_nvme.sh $username raid6 100g 256 8 2
  sleep 3
  sudo LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid6.conf -ioengine=../spdk_bdev -iodepth=12 -rw=randwrite -bs=128k -numjobs=2 > results/${chunk_size}K.log
  exit 0
fi

if [[ "$chunk_size" == "512" ]]
then
  ../generate_raid_config.sh 512 8 2
  ../run_server_remote_nvme.sh $username raid6 100g 512 8 2
  sleep 3
  sudo LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid6.conf -ioengine=../spdk_bdev -iodepth=12 -rw=randwrite -bs=128k -numjobs=2 > results/${chunk_size}K.log
  exit 0
fi

if [[ "$chunk_size" == "1024" ]]
then
  ../generate_raid_config.sh 1024 8 2
  ../run_server_remote_nvme.sh $username raid6 100g 1024 8 2
  sleep 3
  sudo LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid6.conf -ioengine=../spdk_bdev -iodepth=12 -rw=randwrite -bs=128k -numjobs=2 > results/${chunk_size}K.log
  exit 0
fi

echo "We do not have a pre-written command for this chunk size. Feel free to run your own command."