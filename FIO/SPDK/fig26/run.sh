#!/bin/bash

read_ratio=$1

if [[ "$read_ratio" == "0" ]]
then
  sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=6 -rw=randwrite -bs=128k -numjobs=2 ../raid6_spdk.conf
  exit 0
fi

if [[ "$read_ratio" == "25" ]]
then
  sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=12 -rw=randrw -rwmixread=25 -bs=128k -numjobs=2 ../raid6_spdk.conf
  exit 0
fi

if [[ "$read_ratio" == "50" ]]
then
  sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=17 -rw=randrw -rwmixread=50 -bs=128k -numjobs=2 ../raid6_spdk.conf
  exit 0
fi

if [[ "$read_ratio" == "75" ]]
then
  sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=12 -rw=randrw -rwmixread=75 -bs=128k -numjobs=2 ../raid6_spdk.conf
  exit 0
fi

if [[ "$read_ratio" == "100" ]]
then
  sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=12 -rw=randrw -rwmixread=100 -bs=128k -numjobs=2 ../raid6_spdk.conf
  exit 0
fi

echo "We do not have a pre-written command for this read ratio. Feel free to run your own command."