#!/bin/bash

io_size=$1

if [[ "$io_size" == "4" ]]
then
  sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=40 -rw=randread -bs=4k -numjobs=2 ../raid5_spdk.conf
  exit 0
fi

if [[ "$io_size" == "8" ]]
then
  sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=50 -rw=randread -bs=8k -numjobs=2 ../raid5_spdk.conf
  exit 0
fi

if [[ "$io_size" == "16" ]]
then
  sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=44 -rw=randread -bs=16k -numjobs=2 ../raid5_spdk.conf
  exit 0
fi

if [[ "$io_size" == "32" ]]
then
  sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=32 -rw=randread -bs=32k -numjobs=2 ../raid5_spdk.conf
  exit 0
fi

if [[ "$io_size" == "64" ]]
then
  sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=24 -rw=randread -bs=64k -numjobs=2 ../raid5_spdk.conf
  exit 0
fi

if [[ "$io_size" == "128" ]]
then
  sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=16 -rw=randread -bs=128k -numjobs=2 ../raid5_spdk.conf
  exit 0
fi

echo "We do not have a pre-written command for this I/O size. Feel free to run your own command."