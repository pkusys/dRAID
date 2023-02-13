#!/bin/bash

username=`whoami`

mkdir -p results

if [ ! -e results/32K.log ] || ! grep -q "clat" results/32K.log
then
  sudo kill -9 $(ps aux | grep '[r]aid' | awk '{print $2}')
  echo "generating results on 32K chunk size"
  ../generate_raid_config.sh 32 8 2
  ../run_server_remote_nvme.sh $username raid6 100g 32 8 2
  sleep 3
  sudo timeout -k 5 60 sh -c "LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid6.conf -ioengine=../spdk_bdev -iodepth=22 -rw=randwrite -bs=128k -numjobs=2 > results/32K.log"
fi

if [ ! -e results/64K.log ] || ! grep -q "clat" results/64K.log
then
  sudo kill -9 $(ps aux | grep '[r]aid' | awk '{print $2}')
  echo "generating results on 64K chunk size"
  ../generate_raid_config.sh 64 8 2
  ../run_server_remote_nvme.sh $username raid6 100g 64 8 2
  sleep 3
  sudo timeout -k 5 60 sh -c "LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid6.conf -ioengine=../spdk_bdev -iodepth=16 -rw=randwrite -bs=128k -numjobs=2 > results/64K.log"
fi

if [ ! -e results/128K.log ] || ! grep -q "clat" results/128K.log
then
  sudo kill -9 $(ps aux | grep '[r]aid' | awk '{print $2}')
  echo "generating results on 128K chunk size"
  ../generate_raid_config.sh 128 8 2
  ../run_server_remote_nvme.sh $username raid6 100g 128 8 2
  sleep 3
  sudo timeout -k 5 60 sh -c "LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid6.conf -ioengine=../spdk_bdev -iodepth=12 -rw=randwrite -bs=128k -numjobs=2 > results/128K.log"
fi

if [ ! -e results/256K.log ] || ! grep -q "clat" results/256K.log
then
  sudo kill -9 $(ps aux | grep '[r]aid' | awk '{print $2}')
  echo "generating results on 256K chunk size"
  ../generate_raid_config.sh 256 8 2
  ../run_server_remote_nvme.sh $username raid6 100g 256 8 2
  sleep 3
  sudo timeout -k 5 60 sh -c "LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid6.conf -ioengine=../spdk_bdev -iodepth=12 -rw=randwrite -bs=128k -numjobs=2 > results/256K.log"
fi

if [ ! -e results/512K.log ] || ! grep -q "clat" results/512K.log
then
  sudo kill -9 $(ps aux | grep '[r]aid' | awk '{print $2}')
  echo "generating results on 512K chunk size"
  ../generate_raid_config.sh 512 8 2
  ../run_server_remote_nvme.sh $username raid6 100g 512 8 2
  sleep 3
  sudo timeout -k 5 60 sh -c "LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid6.conf -ioengine=../spdk_bdev -iodepth=12 -rw=randwrite -bs=128k -numjobs=2 > results/512K.log"
fi

if [ ! -e results/1024K.log ] || ! grep -q "clat" results/1024K.log
then
  sudo kill -9 $(ps aux | grep '[r]aid' | awk '{print $2}')
  echo "generating results on 1024K chunk size"
  ../generate_raid_config.sh 1024 8 2
  ../run_server_remote_nvme.sh $username raid6 100g 1024 8 2
  sleep 3
  sudo timeout -k 5 60 sh -c "LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid6.conf -ioengine=../spdk_bdev -iodepth=12 -rw=randwrite -bs=128k -numjobs=2 > results/1024K.log"
fi
