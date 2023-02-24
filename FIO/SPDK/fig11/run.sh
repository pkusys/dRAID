#!/bin/bash

chunk_size=$1

mkdir -p results

if [[ "$chunk_size" == "32" ]]
then
  ../generate_raid_config.sh 32 8 1
  sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=22 -rw=randrw -rwmixread=0 -bs=128k -numjobs=2 ../raid5_spdk.conf > results/${chunk_size}K.log
  exit 0
fi

if [[ "$chunk_size" == "64" ]]
then
  ../generate_raid_config.sh 64 8 1
  sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=16 -rw=randrw -rwmixread=0 -bs=128k -numjobs=2 ../raid5_spdk.conf > results/${chunk_size}K.log
  exit 0
fi

if [[ "$chunk_size" == "128" ]]
then
  ../generate_raid_config.sh 128 8 1
  sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=14 -rw=randrw -rwmixread=0 -bs=128k -numjobs=2 ../raid5_spdk.conf > results/${chunk_size}K.log
  exit 0
fi

if [[ "$chunk_size" == "256" ]]
then
  ../generate_raid_config.sh 256 8 1
  sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=14 -rw=randrw -rwmixread=0 -bs=128k -numjobs=2 ../raid5_spdk.conf > results/${chunk_size}K.log
  exit 0
fi

if [[ "$chunk_size" == "512" ]]
then
  ../generate_raid_config.sh 512 8 1
  sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=10 -rw=randrw -rwmixread=0 -bs=128k -numjobs=2 ../raid5_spdk.conf > results/${chunk_size}K.log
  exit 0
fi

if [[ "$chunk_size" == "1024" ]]
then
  ../generate_raid_config.sh 1024 8 1
  sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=12 -rw=randrw -rwmixread=0 -bs=128k -numjobs=2 ../raid5_spdk.conf > results/${chunk_size}K.log
  exit 0
fi

echo "We do not have a pre-written command for this chunk size. Feel free to run your own command."