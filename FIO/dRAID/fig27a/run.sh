#!/bin/bash

username=`whoami`
io_depth=$1

sudo kill -9 $(ps aux | grep '[r]aid' | awk '{print $2}')

let io_depth_per_job=$io_depth/2

if [[ "$io_depth" == "2" ]] || [[ "$io_depth" == "4" ]] || [[ "$io_depth" == "8" ]] || [[ "$io_depth" == "16" ]] || [[ "$io_depth" == "32" ]] || [[ "$io_depth" == "40" ]] || [[ "$io_depth" == "48" ]] || [[ "$io_depth" == "56" ]] || [[ "$io_depth" == "64" ]] || [[ "$io_depth" == "72" ]] || [[ "$io_depth" == "80" ]] || [[ "$io_depth" == "88" ]] || [[ "$io_depth" == "104" ]] || [[ "$io_depth" == "128" ]]
then
  ../generate_raid_config.sh 512 18 2
  ../run_server_remote_nvme.sh $username raid6 100g 512 18 2
  sleep 10
  sudo LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid6.conf -ioengine=../spdk_bdev -iodepth=$io_depth_per_job -rw=randwrite -bs=128k -numjobs=2
  exit 0
fi

echo "We do not have a pre-written command for this I/O depth. Feel free to run your own command."