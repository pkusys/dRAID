#!/bin/bash

io_size=$1

if [[ "$io_size" == "4" ]]
then
  sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=21 -rw=randwrite -bs=4k -numjobs=2 ../raid6_spdk_d.conf
  exit 0
fi

if [[ "$io_size" == "8" ]]
then
  sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=21 -rw=randwrite -bs=8k -numjobs=2 ../raid6_spdk_d.conf
  exit 0
fi

if [[ "$io_size" == "16" ]]
then
  sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=21 -rw=randwrite -bs=16k -numjobs=2 ../raid6_spdk_d.conf
  exit 0
fi

if [[ "$io_size" == "32" ]]
then
  sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=13 -rw=randwrite -bs=32k -numjobs=2 ../raid6_spdk_d.conf
  exit 0
fi

if [[ "$io_size" == "64" ]]
then
  sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=8 -rw=randwrite -bs=64k -numjobs=2 ../raid6_spdk_d.conf
  exit 0
fi

if [[ "$io_size" == "128" ]]
then
  sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=6 -rw=randwrite -bs=128k -numjobs=2 ../raid6_spdk_d.conf
  exit 0
fi

echo "We do not have a pre-written command for this I/O size. Feel free to run your own command."