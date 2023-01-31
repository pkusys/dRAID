#!/bin/bash

username=`whoami`
read_ratio=$1

sudo kill -9 $(ps aux | grep '[r]aid' | awk '{print $2}')

if [[ "$read_ratio" == "0" ]]
then
  ../generate_raid_config.sh 512 8 2
  ../run_server_remote_nvme.sh $username raid5 100g 512 8 2
  sleep 3
  sudo LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid5.conf -ioengine=../spdk_bdev -iodepth=17 -rw=randwrite -bs=128k -numjobs=2
  exit 0
fi

if [[ "$read_ratio" == "25" ]]
then
  ../generate_raid_config.sh 512 8 2
  ../run_server_remote_nvme.sh $username raid5 100g 512 8 2
  sleep 3
  sudo LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid5.conf -ioengine=../spdk_bdev -iodepth=24 -rw=randrw -rwmixread=25 -bs=128k -numjobs=2
  exit 0
fi

if [[ "$read_ratio" == "50" ]]
then
  ../generate_raid_config.sh 512 8 2
  ../run_server_remote_nvme.sh $username raid5 100g 512 8 2
  sleep 3
  sudo LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid5.conf -ioengine=../spdk_bdev -iodepth=48 -rw=randrw -rwmixread=50 -bs=128k -numjobs=2
  exit 0
fi

if [[ "$read_ratio" == "75" ]]
then
  ../generate_raid_config.sh 512 8 2
  ../run_server_remote_nvme.sh $username raid5 100g 512 8 2
  sleep 3
  sudo LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid5.conf -ioengine=../spdk_bdev -iodepth=32 -rw=randrw -rwmixread=75 -bs=128k -numjobs=2
  exit 0
fi

if [[ "$read_ratio" == "100" ]]
then
  ../generate_raid_config.sh 512 8 2
  ../run_server_remote_nvme.sh $username raid5 100g 512 8 2
  sleep 3
  sudo LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid5.conf -ioengine=../spdk_bdev -iodepth=16 -rw=randrw -rwmixread=100 -bs=128k -numjobs=2
  exit 0
fi

echo "We do not have a pre-written command for this read ratio. Feel free to run your own command."