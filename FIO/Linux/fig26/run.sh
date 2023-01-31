#!/bin/bash

read_ratio=$1

if [[ "$read_ratio" == "0" ]]
then
  sudo fio --filename=/dev/md0 --size=8196M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randrw --rw=randrw --bs=128K --iodepth=4 --numjobs=2 --rwmixread=0
  exit 0
fi

if [[ "$read_ratio" == "25" ]]
then
  sudo fio --filename=/dev/md0 --size=8196M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randrw --rw=randrw --bs=128K --iodepth=4 --numjobs=2 --rwmixread=25
  exit 0
fi

if [[ "$read_ratio" == "50" ]]
then
  sudo fio --filename=/dev/md0 --size=8196M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randrw --rw=randrw --bs=128K --iodepth=4 --numjobs=2 --rwmixread=50
  exit 0
fi

if [[ "$read_ratio" == "75" ]]
then
  sudo fio --filename=/dev/md0 --size=8196M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randrw --rw=randrw --bs=128K --iodepth=6 --numjobs=2 --rwmixread=75
  exit 0
fi

if [[ "$read_ratio" == "100" ]]
then
  sudo fio --filename=/dev/md0 --size=8196M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randrw --rw=randrw --bs=128K --iodepth=18 --numjobs=2 --rwmixread=100
  exit 0
fi

echo "We do not have a pre-written command for this read ratio. Feel free to run your own command."