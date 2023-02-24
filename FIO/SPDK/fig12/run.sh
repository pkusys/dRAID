#!/bin/bash

stripe_width=$1

mkdir -p results

if [[ "$stripe_width" == "4" ]]
then
  ../generate_raid_config.sh 512 4 1
  sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=6 -rw=randrw -rwmixread=0 -bs=128k -numjobs=2 ../raid5_spdk.conf > results/${stripe_width}.log
  exit 0
fi

if [[ "$stripe_width" == "6" ]]
then
  ../generate_raid_config.sh 512 6 1
  sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=8 -rw=randrw -rwmixread=0 -bs=128k -numjobs=2 ../raid5_spdk.conf > results/${stripe_width}.log
  exit 0
fi

if [[ "$stripe_width" == "8" ]]
then
  ../generate_raid_config.sh 512 8 1
  sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=10 -rw=randrw -rwmixread=0 -bs=128k -numjobs=2 ../raid5_spdk.conf > results/${stripe_width}.log
  exit 0
fi

if [[ "$stripe_width" == "10" ]]
then
  ../generate_raid_config.sh 512 10 1
  sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=12 -rw=randrw -rwmixread=0 -bs=128k -numjobs=2 ../raid5_spdk.conf > results/${stripe_width}.log
  exit 0
fi

if [[ "$stripe_width" == "12" ]]
then
  ../generate_raid_config.sh 512 12 1
  sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=14 -rw=randrw -rwmixread=0 -bs=128k -numjobs=2 ../raid5_spdk.conf > results/${stripe_width}.log
  exit 0
fi

if [[ "$stripe_width" == "14" ]]
then
  ../generate_raid_config.sh 512 14 1
  sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=16 -rw=randrw -rwmixread=0 -bs=128k -numjobs=2 ../raid5_spdk.conf > results/${stripe_width}.log
  exit 0
fi

if [[ "$stripe_width" == "16" ]]
then
  ../generate_raid_config.sh 512 16 1
  sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=9 -rw=randrw -rwmixread=0 -bs=128k -numjobs=4 ../raid5_spdk.conf > results/${stripe_width}.log
  exit 0
fi

if [[ "$stripe_width" == "18" ]]
then
  ../generate_raid_config.sh 512 18 1
  sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=18 -rw=randrw -rwmixread=0 -bs=128k -numjobs=4 ../raid5_spdk.conf > results/${stripe_width}.log
  exit 0
fi

echo "We do not have a pre-written command for this stripe width. Feel free to run your own command."