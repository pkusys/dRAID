#!/bin/bash

username=`whoami`

sudo kill -9 $(ps aux | grep '[r]aid' | awk '{print $2}')
mkdir -p results

if [ ! -e results/4K.log ] || ! grep -q "Run status group" results/4K.log
then
  echo "generating results on 4K I/O size"
  ../generate_raid_config.sh 512 8 2
  ../run_server_remote_nvme.sh $username raid6 100g 512 8 2
  sleep 3
  sudo timeout -k 5 60 LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid6_d.conf -ioengine=../spdk_bdev -iodepth=28 -rw=randwrite -bs=4k -numjobs=2 > results/4K.log
fi

if [ ! -e results/8K.log ] || ! grep -q "Run status group" results/8K.log
then
  echo "generating results on 8K I/O size"
  ../generate_raid_config.sh 512 8 2
  ../run_server_remote_nvme.sh $username raid6 100g 512 8 2
  sleep 3
  sudo timeout -k 5 60 LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid6_d.conf -ioengine=../spdk_bdev -iodepth=28 -rw=randwrite -bs=8k -numjobs=2 > results/8K.log
fi

if [ ! -e results/16K.log ] || ! grep -q "Run status group" results/16K.log
then
  echo "generating results on 16K I/O size"
  ../generate_raid_config.sh 512 8 2
  ../run_server_remote_nvme.sh $username raid6 100g 512 8 2
  sleep 3
  sudo timeout -k 5 60 LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid6_d.conf -ioengine=../spdk_bdev -iodepth=27 -rw=randwrite -bs=16k -numjobs=2 > results/16K.log
fi

if [ ! -e results/32K.log ] || ! grep -q "Run status group" results/32K.log
then
  echo "generating results on 32K I/O size"
  ../generate_raid_config.sh 512 8 2
  ../run_server_remote_nvme.sh $username raid6 100g 512 8 2
  sleep 3
  sudo timeout -k 5 60 LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid6_d.conf -ioengine=../spdk_bdev -iodepth=21 -rw=randwrite -bs=32k -numjobs=2 > results/32K.log
fi

if [ ! -e results/64K.log ] || ! grep -q "Run status group" results/64K.log
then
  echo "generating results on 64K I/O size"
  ../generate_raid_config.sh 512 8 2
  ../run_server_remote_nvme.sh $username raid6 100g 512 8 2
  sleep 3
  sudo timeout -k 5 60 LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid6_d.conf -ioengine=../spdk_bdev -iodepth=16 -rw=randwrite -bs=64k -numjobs=2 > results/64K.log
fi

if [ ! -e results/128K.log ] || ! grep -q "Run status group" results/128K.log
then
  echo "generating results on 128K I/O size"
  ../generate_raid_config.sh 512 8 2
  ../run_server_remote_nvme.sh $username raid6 100g 512 8 2
  sleep 3
  sudo timeout -k 5 60 LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid6_d.conf -ioengine=../spdk_bdev -iodepth=10 -rw=randwrite -bs=128k -numjobs=2 > results/128K.log
fi
