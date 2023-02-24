#!/bin/bash

io_size=$1

mkdir -p results

if [[ "$io_size" == "4" ]]
then
  sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=18 -rw=randread -rwmixread=0 -bs=4k -numjobs=2 ../raid6_spdk_d.conf > results/${io_size}K.log
  exit 0
fi

if [[ "$io_size" == "8" ]]
then
  sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=18 -rw=randread -rwmixread=0 -bs=8k -numjobs=2 ../raid6_spdk_d.conf > results/${io_size}K.log
  exit 0
fi

if [[ "$io_size" == "16" ]]
then
  sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=20 -rw=randread -rwmixread=0 -bs=16k -numjobs=2 ../raid6_spdk_d.conf > results/${io_size}K.log
  exit 0
fi

if [[ "$io_size" == "32" ]]
then
  sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=20 -rw=randread -rwmixread=0 -bs=32k -numjobs=2 ../raid6_spdk_d.conf > results/${io_size}K.log
  exit 0
fi

if [[ "$io_size" == "64" ]]
then
  sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=14 -rw=randread -rwmixread=0 -bs=64k -numjobs=2 ../raid6_spdk_d.conf > results/${io_size}K.log
  exit 0
fi

if [[ "$io_size" == "128" ]]
then
  sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=11 -rw=randread -rwmixread=0 -bs=128k -numjobs=2 ../raid6_spdk_d.conf > results/${io_size}K.log
  exit 0
fi

echo "We do not have a pre-written command for this I/O size. Feel free to run your own command."