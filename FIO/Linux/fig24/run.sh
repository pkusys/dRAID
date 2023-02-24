#!/bin/bash

chunk_size=$1

mkdir -p results

if [[ "$chunk_size" == "32" ]]
then
  sudo mdadm --create -v /dev/md0 --assume-clean --chunk=32 --level=6 --raid-devices=8 --size=134217728 /dev/nvme2n1 /dev/nvme3n1 /dev/nvme4n1 /dev/nvme5n1 /dev/nvme6n1 /dev/nvme7n1 /dev/nvme8n1 /dev/nvme9n1
  
  sudo fio --filename=/dev/md0 --size=8196M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randwrite --rw=randwrite --bs=128K --iodepth=10 --numjobs=1 > results/${chunk_size}K.log
  
  sudo mdadm -S /dev/md0
  sudo mdadm --zero-superblock /dev/nvme2n1
  sudo mdadm --zero-superblock /dev/nvme3n1
  sudo mdadm --zero-superblock /dev/nvme4n1
  sudo mdadm --zero-superblock /dev/nvme5n1
  sudo mdadm --zero-superblock /dev/nvme6n1
  sudo mdadm --zero-superblock /dev/nvme7n1
  sudo mdadm --zero-superblock /dev/nvme8n1
  sudo mdadm --zero-superblock /dev/nvme9n1
  exit 0
fi

if [[ "$chunk_size" == "64" ]]
then
  sudo mdadm --create -v /dev/md0 --assume-clean --chunk=64 --level=6 --raid-devices=8 --size=134217728 /dev/nvme2n1 /dev/nvme3n1 /dev/nvme4n1 /dev/nvme5n1 /dev/nvme6n1 /dev/nvme7n1 /dev/nvme8n1 /dev/nvme9n1

  sudo fio --filename=/dev/md0 --size=8196M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randwrite --rw=randwrite --bs=128K --iodepth=9 --numjobs=1 > results/${chunk_size}K.log

  sudo mdadm -S /dev/md0
  sudo mdadm --zero-superblock /dev/nvme2n1
  sudo mdadm --zero-superblock /dev/nvme3n1
  sudo mdadm --zero-superblock /dev/nvme4n1
  sudo mdadm --zero-superblock /dev/nvme5n1
  sudo mdadm --zero-superblock /dev/nvme6n1
  sudo mdadm --zero-superblock /dev/nvme7n1
  sudo mdadm --zero-superblock /dev/nvme8n1
  sudo mdadm --zero-superblock /dev/nvme9n1
  exit 0
fi

if [[ "$chunk_size" == "128" ]]
then
  sudo mdadm --create -v /dev/md0 --assume-clean --chunk=128 --level=6 --raid-devices=8 --size=134217728 /dev/nvme2n1 /dev/nvme3n1 /dev/nvme4n1 /dev/nvme5n1 /dev/nvme6n1 /dev/nvme7n1 /dev/nvme8n1 /dev/nvme9n1

  sudo fio --filename=/dev/md0 --size=8196M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randwrite --rw=randwrite --bs=128K --iodepth=8 --numjobs=1 > results/${chunk_size}K.log

  sudo mdadm -S /dev/md0
  sudo mdadm --zero-superblock /dev/nvme2n1
  sudo mdadm --zero-superblock /dev/nvme3n1
  sudo mdadm --zero-superblock /dev/nvme4n1
  sudo mdadm --zero-superblock /dev/nvme5n1
  sudo mdadm --zero-superblock /dev/nvme6n1
  sudo mdadm --zero-superblock /dev/nvme7n1
  sudo mdadm --zero-superblock /dev/nvme8n1
  sudo mdadm --zero-superblock /dev/nvme9n1
  exit 0
fi

if [[ "$chunk_size" == "256" ]]
then
  sudo mdadm --create -v /dev/md0 --assume-clean --chunk=256 --level=6 --raid-devices=8 --size=134217728 /dev/nvme2n1 /dev/nvme3n1 /dev/nvme4n1 /dev/nvme5n1 /dev/nvme6n1 /dev/nvme7n1 /dev/nvme8n1 /dev/nvme9n1

  sudo fio --filename=/dev/md0 --size=8196M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randwrite --rw=randwrite --bs=128K --iodepth=8 --numjobs=1 > results/${chunk_size}K.log

  sudo mdadm -S /dev/md0
  sudo mdadm --zero-superblock /dev/nvme2n1
  sudo mdadm --zero-superblock /dev/nvme3n1
  sudo mdadm --zero-superblock /dev/nvme4n1
  sudo mdadm --zero-superblock /dev/nvme5n1
  sudo mdadm --zero-superblock /dev/nvme6n1
  sudo mdadm --zero-superblock /dev/nvme7n1
  sudo mdadm --zero-superblock /dev/nvme8n1
  sudo mdadm --zero-superblock /dev/nvme9n1
  exit 0
fi

if [[ "$chunk_size" == "512" ]]
then
  sudo mdadm --create -v /dev/md0 --assume-clean --chunk=512 --level=6 --raid-devices=8 --size=134217728 /dev/nvme2n1 /dev/nvme3n1 /dev/nvme4n1 /dev/nvme5n1 /dev/nvme6n1 /dev/nvme7n1 /dev/nvme8n1 /dev/nvme9n1

  sudo fio --filename=/dev/md0 --size=8196M --numjobs=1 --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --bs=128K --iodepth=8 --rw=randwrite --group_reporting=1 --name=randwrite > results/${chunk_size}K.log

  sudo mdadm -S /dev/md0
  sudo mdadm --zero-superblock /dev/nvme2n1
  sudo mdadm --zero-superblock /dev/nvme3n1
  sudo mdadm --zero-superblock /dev/nvme4n1
  sudo mdadm --zero-superblock /dev/nvme5n1
  sudo mdadm --zero-superblock /dev/nvme6n1
  sudo mdadm --zero-superblock /dev/nvme7n1
  sudo mdadm --zero-superblock /dev/nvme8n1
  sudo mdadm --zero-superblock /dev/nvme9n1
  exit 0
fi

if [[ "$chunk_size" == "1024" ]]
then
  sudo mdadm --create -v /dev/md0 --assume-clean --chunk=1024 --level=6 --raid-devices=8 --size=134217728 /dev/nvme2n1 /dev/nvme3n1 /dev/nvme4n1 /dev/nvme5n1 /dev/nvme6n1 /dev/nvme7n1 /dev/nvme8n1 /dev/nvme9n1

  sudo fio --filename=/dev/md0 --size=8196M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randwrite --rw=randwrite --bs=128K --iodepth=7 --numjobs=1 > results/${chunk_size}K.log

  sudo mdadm -S /dev/md0
  sudo mdadm --zero-superblock /dev/nvme2n1
  sudo mdadm --zero-superblock /dev/nvme3n1
  sudo mdadm --zero-superblock /dev/nvme4n1
  sudo mdadm --zero-superblock /dev/nvme5n1
  sudo mdadm --zero-superblock /dev/nvme6n1
  sudo mdadm --zero-superblock /dev/nvme7n1
  sudo mdadm --zero-superblock /dev/nvme8n1
  sudo mdadm --zero-superblock /dev/nvme9n1
  exit 0
fi

echo "We do not have a pre-written command for this chunk size. Feel free to run your own command."