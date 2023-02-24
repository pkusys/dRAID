#!/bin/bash

io_depth=$1

mkdir -p results

if [[ "$io_depth" == "1" ]] || [[ "$io_depth" == "2" ]] || [[ "$io_depth" == "3" ]] || [[ "$io_depth" == "4" ]] || [[ "$io_depth" == "5" ]] || [[ "$io_depth" == "6" ]] || [[ "$io_depth" == "7" ]]
then
  sudo fio --filename=/dev/md0 --size=8196M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randwrite --rw=randrw --rwmixread=50 --bs=128K --iodepth=$io_depth --numjobs=1 > results/${io_depth}.log
  exit 0
fi

echo "We do not have a pre-written command for this I/O depth. Feel free to run your own command."