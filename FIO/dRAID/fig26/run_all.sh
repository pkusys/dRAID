#!/bin/bash

username=`whoami`

sudo kill -9 $(ps aux | grep '[r]aid' | awk '{print $2}')
mkdir -p results

if [ ! -e results/0.log ] || ! grep -q "Run status group" results/0.log
then
  echo "generating results on read ratio 0"
  ../generate_raid_config.sh 512 8 2
  ../run_server_remote_nvme.sh $username raid6 100g 512 8 2
  sleep 3
  sudo timeout -k 5 60 LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid6.conf -ioengine=../spdk_bdev -iodepth=12 -rw=randwrite -bs=128k -numjobs=2 > results/0.log
fi

if [ ! -e results/25.log ] || ! grep -q "Run status group" results/25.log
then
  echo "generating results on read ratio 25"
  ../generate_raid_config.sh 512 8 2
  ../run_server_remote_nvme.sh $username raid6 100g 512 8 2
  sleep 3
  sudo timeout -k 5 60 LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid6.conf -ioengine=../spdk_bdev -iodepth=18 -rw=randrw -rwmixread=25 -bs=128k -numjobs=2 > results/25.log
fi

if [ ! -e results/50.log ] || ! grep -q "Run status group" results/50.log
then
  echo "generating results on read ratio 50"
  ../generate_raid_config.sh 512 8 2
  ../run_server_remote_nvme.sh $username raid6 100g 512 8 2
  sleep 3
  sudo timeout -k 5 60 LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid6.conf -ioengine=../spdk_bdev -iodepth=36 -rw=randrw -rwmixread=50 -bs=128k -numjobs=2 > results/50.log
fi

if [ ! -e results/75.log ] || ! grep -q "Run status group" results/75.log
then
  echo "generating results on read ratio 75"
  ../generate_raid_config.sh 512 8 2
  ../run_server_remote_nvme.sh $username raid6 100g 512 8 2
  sleep 3
  sudo timeout -k 5 60 LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid6.conf -ioengine=../spdk_bdev -iodepth=24 -rw=randrw -rwmixread=75 -bs=128k -numjobs=2 > results/75.log
fi

if [ ! -e results/100.log ] || ! grep -q "Run status group" results/100.log
then
  echo "generating results on read ratio 100"
  ../generate_raid_config.sh 512 8 2
  ../run_server_remote_nvme.sh $username raid6 100g 512 8 2
  sleep 3
  sudo timeout -k 5 60 LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid6.conf -ioengine=../spdk_bdev -iodepth=12 -rw=randrw -rwmixread=100 -bs=128k -numjobs=2 > results/100.log
fi
