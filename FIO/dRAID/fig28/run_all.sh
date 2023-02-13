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
  sudo timeout -k 5 60 sh -c "LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid6_d.conf -ioengine=../spdk_bdev -iodepth=24 -rw=randrw -rwmixread=100 -bs=4k -numjobs=2 > results/4K.log"
fi

if [ ! -e results/8K.log ] || ! grep -q "clat" results/8K.log
then
  sudo kill -9 $(ps aux | grep '[r]aid' | awk '{print $2}')
  echo "generating results on 8K I/O size"
  ../generate_raid_config.sh 512 8 2
  ../run_server_remote_nvme.sh $username raid6 100g 512 8 2
  sleep 3
  sudo timeout -k 5 60 sh -c "LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid6_d.conf -ioengine=../spdk_bdev -iodepth=24 -rw=randrw -rwmixread=100 -bs=8k -numjobs=2 > results/8K.log"
fi

if [ ! -e results/16K.log ] || ! grep -q "clat" results/16K.log
then
  sudo kill -9 $(ps aux | grep '[r]aid' | awk '{print $2}')
  echo "generating results on 16K I/O size"
  ../generate_raid_config.sh 512 8 2
  ../run_server_remote_nvme.sh $username raid6 100g 512 8 2
  sleep 3
  sudo timeout -k 5 60 sh -c "LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid6_d.conf -ioengine=../spdk_bdev -iodepth=24 -rw=randrw -rwmixread=100 -bs=16k -numjobs=2 > results/16K.log"
fi

if [ ! -e results/32K.log ] || ! grep -q "clat" results/32K.log
then
  sudo kill -9 $(ps aux | grep '[r]aid' | awk '{print $2}')
  echo "generating results on 32K I/O size"
  ../generate_raid_config.sh 512 8 2
  ../run_server_remote_nvme.sh $username raid6 100g 512 8 2
  sleep 3
  sudo timeout -k 5 60 sh -c "LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid6_d.conf -ioengine=../spdk_bdev -iodepth=24 -rw=randrw -rwmixread=100 -bs=32k -numjobs=2 > results/32K.log"
fi

if [ ! -e results/64K.log ] || ! grep -q "clat" results/64K.log
then
  sudo kill -9 $(ps aux | grep '[r]aid' | awk '{print $2}')
  echo "generating results on 64K I/O size"
  ../generate_raid_config.sh 512 8 2
  ../run_server_remote_nvme.sh $username raid6 100g 512 8 2
  sleep 3
  sudo timeout -k 5 60 sh -c "LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid6_d.conf -ioengine=../spdk_bdev -iodepth=19 -rw=randrw -rwmixread=100 -bs=64k -numjobs=2 > results/64K.log"
fi

if [ ! -e results/128K.log ] || ! grep -q "clat" results/128K.log
then
  sudo kill -9 $(ps aux | grep '[r]aid' | awk '{print $2}')
  echo "generating results on 128K I/O size"
  ../generate_raid_config.sh 512 8 2
  ../run_server_remote_nvme.sh $username raid6 100g 512 8 2
  sleep 3
  sudo timeout -k 5 60 sh -c "LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid6_d.conf -ioengine=../spdk_bdev -iodepth=15 -rw=randrw -rwmixread=100 -bs=128k -numjobs=2 > results/128K.log"
fi
