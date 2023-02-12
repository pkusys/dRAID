#!/bin/bash

trap "./unmount.sh" SIGINT

sudo rm -r results
mkdir results
./mount.sh

declare -a arr=("1" "2" "3" "4" "5" "6" "7")

for i in "${arr[@]}"
do
  echo "generating results on I/O depth $i"
  sudo fio --filename=/dev/md0 --size=8196M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randwrite --rw=randrw --rwmixread=50 --bs=128K --iodepth=$i --numjobs=1 > "results/${i}.log"
done

./unmount.sh
