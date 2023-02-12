#!/bin/bash

username=`whoami`

sudo kill -9 $(ps aux | grep '[r]aid' | awk '{print $2}')
mkdir -p results

declare -a arr=("1" "2" "4" "6" "8" "12" "16" "24" "32" "48" "64" "72")

for i in "${arr[@]}"
do
  let total_io_depth=$i*2
  if [ ! -e results/${total_io_depth}.log ] || ! grep -q "Run status group" results/${total_io_depth}.log
  then
    echo "generating results on I/O depth $total_io_depth"
    ../generate_raid_config.sh 512 18 2
    ../run_server_remote_nvme.sh $username raid5 100g 512 18 2
    sleep 10
    sudo timeout -k 5 200 LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid5.conf -ioengine=../spdk_bdev -iodepth=$i -rw=randrw -rwmixread=50 -bs=128k -numjobs=2 > "results/${total_io_depth}.log"
  fi
done
