#!/bin/bash

chunk_size=$1

if [[ "$chunk_size" == "32" ]]
then
  ../generate_raid_config.sh 32 8 1
  sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=10 -rw=randwrite -bs=128k -numjobs=2 ../raid6_spdk.conf
  exit 0
fi

if [[ "$chunk_size" == "64" ]]
then
  ../generate_raid_config.sh 64 8 1
  sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=7 -rw=randwrite -bs=128k -numjobs=2 ../raid6_spdk.conf
  exit 0
fi

if [[ "$chunk_size" == "128" ]]
then
  ../generate_raid_config.sh 128 8 1
  sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=6 -rw=randwrite -bs=128k -numjobs=2 ../raid6_spdk.conf
  exit 0
fi

if [[ "$chunk_size" == "256" ]]
then
  ../generate_raid_config.sh 256 8 1
  sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=6 -rw=randwrite -bs=128k -numjobs=2 ../raid6_spdk.conf
  exit 0
fi

if [[ "$chunk_size" == "512" ]]
then
  ../generate_raid_config.sh 512 8 1
  sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=6 -rw=randwrite -bs=128k -numjobs=2 ../raid6_spdk.conf
  exit 0
fi

if [[ "$chunk_size" == "1024" ]]
then
  ../generate_raid_config.sh 1024 8 1
  sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=6 -rw=randwrite -bs=128k -numjobs=2 ../raid6_spdk.conf
  exit 0
fi

echo "We do not have a pre-written command for this chunk size. Feel free to run your own command."