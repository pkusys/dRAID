#!/bin/bash

io_size=$1

mkdir -p results

if [[ "$io_size" == "4" ]]
then
  sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=12 -rw=randread -rwmixread=0 -bs=4k -numjobs=3 ../raid5_spdk_d.conf > results/${io_size}K.log
  exit 0
fi

if [[ "$io_size" == "8" ]]
then
  sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=9 -rw=randread -rwmixread=0 -bs=8k -numjobs=4 ../raid5_spdk_d.conf > results/${io_size}K.log
  exit 0
fi

if [[ "$io_size" == "16" ]]
then
  sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=10 -rw=randread -rwmixread=0 -bs=16k -numjobs=4 ../raid5_spdk_d.conf > results/${io_size}K.log
  exit 0
fi

if [[ "$io_size" == "32" ]]
then
  sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=10 -rw=randread -rwmixread=0 -bs=32k -numjobs=4 ../raid5_spdk_d.conf > results/${io_size}K.log
  exit 0
fi

if [[ "$io_size" == "64" ]]
then
  sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=7 -rw=randread -rwmixread=0 -bs=64k -numjobs=4 ../raid5_spdk_d.conf > results/${io_size}K.log
  exit 0
fi

if [[ "$io_size" == "128" ]]
then
  sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=5 -rw=randread -rwmixread=0 -bs=128k -numjobs=4 ../raid5_spdk_d.conf > results/${io_size}K.log
  exit 0
fi

echo "We do not have a pre-written command for this I/O size. Feel free to run your own command."