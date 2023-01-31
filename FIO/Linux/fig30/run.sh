#!/bin/bash

io_size=$1

if [[ "$io_size" == "4" ]]
then
  sudo fio --filename=/dev/md0 --size=8196M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randwrite --rw=randwrite --bs=4K --iodepth=6 --numjobs=2
  exit 0
fi

if [[ "$io_size" == "8" ]]
then
  sudo fio --filename=/dev/md0 --size=8196M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randwrite --rw=randwrite --bs=8K --iodepth=6 --numjobs=2
  exit 0
fi

if [[ "$io_size" == "16" ]]
then
  sudo fio --filename=/dev/md0 --size=8196M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randwrite --rw=randwrite --bs=16K --iodepth=4 --numjobs=2
  exit 0
fi

if [[ "$io_size" == "32" ]]
then
  sudo fio --filename=/dev/md0 --size=8196M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randwrite --rw=randwrite --bs=32K --iodepth=4 --numjobs=2
  exit 0
fi

if [[ "$io_size" == "64" ]]
then
  sudo fio --filename=/dev/md0 --size=8196M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randwrite --rw=randwrite --bs=64K --iodepth=3 --numjobs=2
  exit 0
fi

if [[ "$io_size" == "128" ]]
then
  sudo fio --filename=/dev/md0 --size=8196M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randwrite --rw=randwrite --bs=128K --iodepth=4 --numjobs=1
  exit 0
fi

echo "We do not have a pre-written command for this I/O size. Feel free to run your own command."