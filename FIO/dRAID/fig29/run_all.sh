#!/bin/bash

username=`whoami`

mkdir -p results

if [ ! -e results/4.log ] || ! grep -q "clat" results/4.log
then
  sudo kill -9 $(ps aux | grep '[r]aid' | awk '{print $2}')
  echo "generating results on stripe width 4"
  ../generate_raid_config.sh 512 4 2
  ../run_server_remote_nvme.sh $username raid6 100g 512 4 2
  sleep 3
  sudo timeout -k 5 60 sh -c "LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid6_d.conf -ioengine=../spdk_bdev -iodepth=6 -rw=randrw -rwmixread=100 -bs=128k -numjobs=2 > results/4.log"
fi

if [ ! -e results/6.log ] || ! grep -q "clat" results/6.log
then
  sudo kill -9 $(ps aux | grep '[r]aid' | awk '{print $2}')
  echo "generating results on stripe width 6"
  ../generate_raid_config.sh 512 6 2
  ../run_server_remote_nvme.sh $username raid6 100g 512 6 2
  sleep 4
  sudo timeout -k 5 80 sh -c "LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid6_d.conf -ioengine=../spdk_bdev -iodepth=12 -rw=randrw -rwmixread=100 -bs=128k -numjobs=2 > results/6.log"
fi

if [ ! -e results/8.log ] || ! grep -q "clat" results/8.log
then
  sudo kill -9 $(ps aux | grep '[r]aid' | awk '{print $2}')
  echo "generating results on stripe width 8"
  ../generate_raid_config.sh 512 8 2
  ../run_server_remote_nvme.sh $username raid6 100g 512 8 2
  sleep 5
  sudo timeout -k 5 100 sh -c "LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid6_d.conf -ioengine=../spdk_bdev -iodepth=13 -rw=randrw -rwmixread=100 -bs=128k -numjobs=2 > results/8.log"
fi

if [ ! -e results/10.log ] || ! grep -q "clat" results/10.log
then
  sudo kill -9 $(ps aux | grep '[r]aid' | awk '{print $2}')
  echo "generating results on stripe width 10"
  ../generate_raid_config.sh 512 10 2
  ../run_server_remote_nvme.sh $username raid6 100g 512 10 2
  sleep 6
  sudo timeout -k 5 120 sh -c "LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid6_d.conf -ioengine=../spdk_bdev -iodepth=13 -rw=randrw -rwmixread=100 -bs=128k -numjobs=2 > results/10.log"
fi

if [ ! -e results/12.log ] || ! grep -q "clat" results/12.log
then
  sudo kill -9 $(ps aux | grep '[r]aid' | awk '{print $2}')
  echo "generating results on stripe width 12"
  ../generate_raid_config.sh 512 12 2
  ../run_server_remote_nvme.sh $username raid6 100g 512 12 2
  sleep 7
  sudo timeout -k 5 140 sh -c "LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid6_d.conf -ioengine=../spdk_bdev -iodepth=13 -rw=randrw -rwmixread=100 -bs=128k -numjobs=2 > results/12.log"
fi

if [ ! -e results/14.log ] || ! grep -q "clat" results/14.log
then
  sudo kill -9 $(ps aux | grep '[r]aid' | awk '{print $2}')
  echo "generating results on stripe width 14"
  ../generate_raid_config.sh 512 14 2
  ../run_server_remote_nvme.sh $username raid6 100g 512 14 2
  sleep 8
  sudo timeout -k 5 160 sh -c "LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid6_d.conf -ioengine=../spdk_bdev -iodepth=13 -rw=randrw -rwmixread=100 -bs=128k -numjobs=2 > results/14.log"
fi

if [ ! -e results/16.log ] || ! grep -q "clat" results/16.log
then
  sudo kill -9 $(ps aux | grep '[r]aid' | awk '{print $2}')
  echo "generating results on stripe width 16"
  ../generate_raid_config.sh 512 16 2
  ../run_server_remote_nvme.sh $username raid6 100g 512 16 2
  sleep 9
  sudo timeout -k 5 180 sh -c "LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid6_d.conf -ioengine=../spdk_bdev -iodepth=14 -rw=randrw -rwmixread=100 -bs=128k -numjobs=2 > results/16.log"
fi

if [ ! -e results/18.log ] || ! grep -q "clat" results/18.log
then
  sudo kill -9 $(ps aux | grep '[r]aid' | awk '{print $2}')
  echo "generating results on stripe width 18"
  ../generate_raid_config.sh 512 18 2
  ../run_server_remote_nvme.sh $username raid6 100g 512 18 2
  sleep 10
  sudo timeout -k 5 200 sh -c "LD_PRELOAD=../spdk_bdev /usr/local/bin/fio ../raid6_d.conf -ioengine=../spdk_bdev -iodepth=14 -rw=randrw -rwmixread=100 -bs=128k -numjobs=2 > results/18.log"
fi
