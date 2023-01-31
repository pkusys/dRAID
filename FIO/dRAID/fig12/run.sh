#!/bin/bash

username=`whoami`
stripe_width=$1

sudo kill -9 $(ps aux | grep '[r]aid' | awk '{print $2}')

if [[ "$stripe_width" == "4" ]]
then
  ../generate_raid_config.sh 512 4 2
  ../run_server_remote_nvme.sh $username raid5 100g 512 4 2
  sleep 3
  sudo LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid5.conf -ioengine=../spdk_bdev -iodepth=8 -rw=randwrite -bs=128k -numjobs=2
  exit 0
fi

if [[ "$stripe_width" == "6" ]]
then
  ../generate_raid_config.sh 512 6 2
  ../run_server_remote_nvme.sh $username raid5 100g 512 6 2
  sleep 4
  sudo LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid5.conf -ioengine=../spdk_bdev -iodepth=12 -rw=randwrite -bs=128k -numjobs=2
  exit 0
fi

if [[ "$stripe_width" == "8" ]]
then
  ../generate_raid_config.sh 512 8 2
  ../run_server_remote_nvme.sh $username raid5 100g 512 8 2
  sleep 5
  sudo LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid5.conf -ioengine=../spdk_bdev -iodepth=17 -rw=randwrite -bs=128k -numjobs=2
  exit 0
fi

if [[ "$stripe_width" == "10" ]]
then
  ../generate_raid_config.sh 512 10 2
  ../run_server_remote_nvme.sh $username raid5 100g 512 10 2
  sleep 6
  sudo LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid5.conf -ioengine=../spdk_bdev -iodepth=20 -rw=randwrite -bs=128k -numjobs=2
  exit 0
fi

if [[ "$stripe_width" == "12" ]]
then
  ../generate_raid_config.sh 512 12 2
  ../run_server_remote_nvme.sh $username raid5 100g 512 12 2
  sleep 7
  sudo LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid5.conf -ioengine=../spdk_bdev -iodepth=24 -rw=randwrite -bs=128k -numjobs=2
  exit 0
fi

if [[ "$stripe_width" == "14" ]]
then
  ../generate_raid_config.sh 512 14 2
  ../run_server_remote_nvme.sh $username raid5 100g 512 14 2
  sleep 8
  sudo LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid5.conf -ioengine=../spdk_bdev -iodepth=28 -rw=randwrite -bs=128k -numjobs=2
  exit 0
fi

if [[ "$stripe_width" == "16" ]]
then
  ../generate_raid_config.sh 512 16 2
  ../run_server_remote_nvme.sh $username raid5 100g 512 16 2
  sleep 9
  sudo LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid5.conf -ioengine=../spdk_bdev -iodepth=32 -rw=randwrite -bs=128k -numjobs=2
  exit 0
fi

if [[ "$stripe_width" == "18" ]]
then
  ../generate_raid_config.sh 512 18 2
  ../run_server_remote_nvme.sh $username raid5 100g 512 18 2
  sleep 10
  sudo LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid5.conf -ioengine=../spdk_bdev -iodepth=36 -rw=randwrite -bs=128k -numjobs=2
  exit 0
fi

echo "We do not have a pre-written command for this stripe width. Feel free to run your own command."