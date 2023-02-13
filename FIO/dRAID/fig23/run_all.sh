#!/bin/bash

username=`whoami`

mkdir -p results

if [ ! -e results/4K.log ] || ! grep -q "clat" results/4K.log
then
  sudo kill -9 $(ps aux | grep '[r]aid' | awk '{print $2}')
  echo "generating results on 4K I/O size"
  ../generate_raid_config.sh 512 8 2
  ../run_server_remote_nvme.sh $username raid6 100g 512 8 2
  sleep 3
  sudo timeout -k 5 60 sh -c "LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid6.conf -ioengine=../spdk_bdev -iodepth=40 -rw=randwrite -bs=4k -numjobs=2 > results/4K.log"
fi

if [ ! -e results/8K.log ] || ! grep -q "clat" results/8K.log
then
  sudo kill -9 $(ps aux | grep '[r]aid' | awk '{print $2}')
  echo "generating results on 8K I/O size"
  ../generate_raid_config.sh 512 8 2
  ../run_server_remote_nvme.sh $username raid6 100g 512 8 2
  sleep 3
  sudo timeout -k 5 60 sh -c "LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid6.conf -ioengine=../spdk_bdev -iodepth=40 -rw=randwrite -bs=8k -numjobs=2 > results/8K.log"
fi

if [ ! -e results/16K.log ] || ! grep -q "clat" results/16K.log
then
  sudo kill -9 $(ps aux | grep '[r]aid' | awk '{print $2}')
  echo "generating results on 16K I/O size"
  ../generate_raid_config.sh 512 8 2
  ../run_server_remote_nvme.sh $username raid6 100g 512 8 2
  sleep 3
  sudo timeout -k 5 60 sh -c "LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid6.conf -ioengine=../spdk_bdev -iodepth=35 -rw=randwrite -bs=16k -numjobs=2 > results/16K.log"
fi

if [ ! -e results/32K.log ] || ! grep -q "clat" results/32K.log
then
  sudo kill -9 $(ps aux | grep '[r]aid' | awk '{print $2}')
  echo "generating results on 32K I/O size"
  ../generate_raid_config.sh 512 8 2
  ../run_server_remote_nvme.sh $username raid6 100g 512 8 2
  sleep 3
  sudo timeout -k 5 60 sh -c "LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid6.conf -ioengine=../spdk_bdev -iodepth=24 -rw=randwrite -bs=32k -numjobs=2 > results/32K.log"
fi

if [ ! -e results/64K.log ] || ! grep -q "clat" results/64K.log
then
  sudo kill -9 $(ps aux | grep '[r]aid' | awk '{print $2}')
  echo "generating results on 64K I/O size"
  ../generate_raid_config.sh 512 8 2
  ../run_server_remote_nvme.sh $username raid6 100g 512 8 2
  sleep 3
  sudo timeout -k 5 60 sh -c "LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid6.conf -ioengine=../spdk_bdev -iodepth=18 -rw=randwrite -bs=64k -numjobs=2 > results/64K.log"
fi

if [ ! -e results/128K.log ] || ! grep -q "clat" results/128K.log
then
  sudo kill -9 $(ps aux | grep '[r]aid' | awk '{print $2}')
  echo "generating results on 128K I/O size"
  ../generate_raid_config.sh 512 8 2
  ../run_server_remote_nvme.sh $username raid6 100g 512 8 2
  sleep 3
  sudo timeout -k 5 60 sh -c "LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid6.conf -ioengine=../spdk_bdev -iodepth=12 -rw=randwrite -bs=128k -numjobs=2 > results/128K.log"
fi

if [ ! -e results/256K.log ] || ! grep -q "clat" results/256K.log
then
  sudo kill -9 $(ps aux | grep '[r]aid' | awk '{print $2}')
  echo "generating results on 256K I/O size"
  ../generate_raid_config.sh 512 8 2
  ../run_server_remote_nvme.sh $username raid6 100g 512 8 2
  sleep 3
  sudo timeout -k 5 60 sh -c "LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid6.conf -ioengine=../spdk_bdev -iodepth=8 -rw=randwrite -bs=256k -numjobs=2 > results/256K.log"
fi

if [ ! -e results/512K.log ] || ! grep -q "clat" results/512K.log
then
  sudo kill -9 $(ps aux | grep '[r]aid' | awk '{print $2}')
  echo "generating results on 512K I/O size"
  ../generate_raid_config.sh 512 8 2
  ../run_server_remote_nvme.sh $username raid6 100g 512 8 2
  sleep 3
  sudo timeout -k 5 60 sh -c "LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid6.conf -ioengine=../spdk_bdev -iodepth=6 -rw=randwrite -bs=512k -numjobs=2 > results/512K.log"
fi

if [ ! -e results/1024K.log ] || ! grep -q "clat" results/1024K.log
then
  sudo kill -9 $(ps aux | grep '[r]aid' | awk '{print $2}')
  echo "generating results on 1024K I/O size"
  ../generate_raid_config.sh 512 8 2
  ../run_server_remote_nvme.sh $username raid6 100g 512 8 2
  sleep 3
  sudo timeout -k 5 60 sh -c "LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid6.conf -ioengine=../spdk_bdev -iodepth=4 -rw=randwrite -bs=1024k -numjobs=2 > results/1024K.log"
fi

if [ ! -e results/2048K.log ] || ! grep -q "clat" results/2048K.log
then
  sudo kill -9 $(ps aux | grep '[r]aid' | awk '{print $2}')
  echo "generating results on 2048K I/O size"
  ../generate_raid_config.sh 512 8 2
  ../run_server_remote_nvme.sh $username raid6 100g 512 8 2
  sleep 3
  sudo timeout -k 5 60 sh -c "LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid6.conf -ioengine=../spdk_bdev -iodepth=3 -rw=randwrite -bs=2048k -numjobs=2 > results/2048K.log"
fi

if [ ! -e results/3072K.log ] || ! grep -q "clat" results/3072K.log
then
  sudo kill -9 $(ps aux | grep '[r]aid' | awk '{print $2}')
  echo "generating results on 3072K I/O size"
  ../generate_raid_config.sh 512 8 3
  ../run_server_remote_nvme.sh $username raid6 100g 512 8 3
  sleep 3
  sudo timeout -k 5 60 sh -c "LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid6.conf -ioengine=../spdk_bdev -iodepth=2 -rw=randwrite -bs=3072k -numjobs=3 > results/3072K.log"
fi
