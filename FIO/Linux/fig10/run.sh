#!/bin/bash

io_size=$1

mkdir -p results

if [[ "$io_size" == "4" ]]
then
  sudo fio --filename=/dev/md0 --size=8196M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randwrite --rw=randwrite --bs=4K --iodepth=6 --numjobs=2 > results/${io_size}K.log
  exit 0
fi

if [[ "$io_size" == "8" ]]
then
  sudo fio --filename=/dev/md0 --size=8196M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randwrite --rw=randwrite --bs=8K --iodepth=5 --numjobs=2 > results/${io_size}K.log
  exit 0
fi

if [[ "$io_size" == "16" ]]
then
  sudo fio --filename=/dev/md0 --size=8196M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randwrite --rw=randwrite --bs=16K --iodepth=4 --numjobs=2 > results/${io_size}K.log
  exit 0
fi

if [[ "$io_size" == "32" ]]
then
  sudo fio --filename=/dev/md0 --size=8196M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randwrite --rw=randwrite --bs=32K --iodepth=7 --numjobs=1 > results/${io_size}K.log
  exit 0
fi

if [[ "$io_size" == "64" ]]
then
  sudo fio --filename=/dev/md0 --size=8196M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randwrite --rw=randwrite --bs=64K --iodepth=7 --numjobs=1 > results/${io_size}K.log
  exit 0
fi

if [[ "$io_size" == "128" ]]
then
  sudo fio --filename=/dev/md0 --size=8196M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randwrite --rw=randwrite --bs=128K --iodepth=6 --numjobs=1 > results/${io_size}K.log
  exit 0
fi

if [[ "$io_size" == "256" ]]
then
  sudo fio --filename=/dev/md0 --size=8196M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randwrite --rw=randwrite --bs=256K --iodepth=4 --numjobs=1 > results/${io_size}K.log
  exit 0
fi

if [[ "$io_size" == "512" ]]
then
  sudo fio --filename=/dev/md0 --size=8196M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randwrite --rw=randwrite --bs=512K --iodepth=3 --numjobs=1 > results/${io_size}K.log
  exit 0
fi

if [[ "$io_size" == "1024" ]]
then
  sudo fio --filename=/dev/md0 --size=8196M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randwrite --rw=randwrite --bs=1024K --iodepth=2 --numjobs=1 > results/${io_size}K.log
  exit 0
fi

if [[ "$io_size" == "2048" ]]
then
  sudo fio --filename=/dev/md0 --size=8196M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randwrite --rw=randwrite --bs=2048K --iodepth=2 --numjobs=1 > results/${io_size}K.log
  exit 0
fi

if [[ "$io_size" == "3584" ]]
then
  sudo fio --filename=/dev/md0 --size=8196M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randwrite --rw=randwrite --bs=3584K --iodepth=1 --numjobs=1 > results/${io_size}K.log
  exit 0
fi

echo "We do not have a pre-written command for this I/O size. Feel free to run your own command."