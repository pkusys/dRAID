#!/bin/bash

io_size=$1

if [[ "$io_size" == "4" ]]
then
  sudo fio --filename=/dev/md0 --size=128M --numjobs=4 --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --bs=4K --iodepth=16 --rw=randread --group_reporting=1 --name=randread
  exit 0
fi

if [[ "$io_size" == "8" ]]
then
  sudo fio --filename=/dev/md0 --size=128M --numjobs=4 --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --rw=randread --group_reporting=1 --name=randread --bs=8K --iodepth=16
  exit 0
fi

if [[ "$io_size" == "16" ]]
then
  sudo fio --filename=/dev/md0 --size=128M --numjobs=4 --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --rw=randread --group_reporting=1 --name=randread --bs=16K --iodepth=16
  exit 0
fi

if [[ "$io_size" == "32" ]]
then
  sudo fio --filename=/dev/md0 --size=128M --numjobs=4 --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --rw=randread --group_reporting=1 --name=randread --bs=32K --iodepth=16
  exit 0
fi

if [[ "$io_size" == "64" ]]
then
  sudo fio --filename=/dev/md0 --size=128M --numjobs=4 --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --rw=randread --group_reporting=1 --name=randread --bs=64K --iodepth=16
  exit 0
fi

if [[ "$io_size" == "128" ]]
then
  sudo fio --filename=/dev/md0 --size=128M --numjobs=4 --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --rw=randread --group_reporting=1 --name=randread --bs=128K --iodepth=8
  exit 0
fi

echo "We do not have a pre-written command for this I/O size. Feel free to run your own command."