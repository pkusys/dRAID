#!/bin/bash

../generate_raid_config.sh 512 18 1
sudo rm -r results
mkdir results

declare -a arr=("1" "2" "3" "4" "5" "6" "7" "9" "12" "20")

for i in "${arr[@]}"
do
  let total_io_depth=$i*4
  echo "generating results on I/O depth $total_io_depth"
  sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=$i -rw=randrw -rwmixread=0 -bs=128k -numjobs=4 ../raid5_spdk.conf > "results/${total_io_depth}.log"
done
