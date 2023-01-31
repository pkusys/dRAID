#!/bin/bash

io_size=$1

if [[ "$io_size" == "4" ]]
then
  sudo fio --filename=/dev/md0 --size=2048M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randwrite --rw=randread --rwmixread=0 --bs=4K --iodepth=12 --numjobs=1
  exit 0
fi

if [[ "$io_size" == "8" ]]
then
  sudo fio --filename=/dev/md0 --size=2048M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randwrite --rw=randread --rwmixread=0 --bs=8K --iodepth=12 --numjobs=1
  exit 0
fi

if [[ "$io_size" == "16" ]]
then
  sudo fio --filename=/dev/md0 --size=2048M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randwrite --rw=randread --rwmixread=0 --bs=16K --iodepth=8 --numjobs=1
  exit 0
fi

if [[ "$io_size" == "32" ]]
then
  sudo fio --filename=/dev/md0 --size=2048M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randwrite --rw=randread --rwmixread=0 --bs=32K --iodepth=8 --numjobs=1
  exit 0
fi

if [[ "$io_size" == "64" ]]
then
  sudo fio --filename=/dev/md0 --size=2048M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randwrite --rw=randread --rwmixread=0 --bs=64K --iodepth=5 --numjobs=1
  exit 0
fi

if [[ "$io_size" == "128" ]]
then
  sudo fio --filename=/dev/md0 --size=2048M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randwrite --rw=randread --rwmixread=0 --bs=128K --iodepth=5 --numjobs=1
  exit 0
fi

echo "We do not have a pre-written command for this I/O size. Feel free to run your own command."