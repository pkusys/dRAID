#!/bin/bash

function cleanup()
{
    sudo mdadm -S /dev/md0
    sudo mdadm --zero-superblock /dev/nvme2n1
    sudo mdadm --zero-superblock /dev/nvme3n1
    sudo mdadm --zero-superblock /dev/nvme4n1
    sudo mdadm --zero-superblock /dev/nvme5n1
    sudo mdadm --zero-superblock /dev/nvme6n1
    sudo mdadm --zero-superblock /dev/nvme7n1
    sudo mdadm --zero-superblock /dev/nvme8n1
    sudo mdadm --zero-superblock /dev/nvme9n1
}

trap cleanup SIGINT

sudo rm -r results
mkdir results

echo "generating results on 32K chunk size"
sudo mdadm --create -v /dev/md0 --assume-clean --chunk=32 --level=6 --raid-devices=8 --size=134217728 /dev/nvme2n1 /dev/nvme3n1 /dev/nvme4n1 /dev/nvme5n1 /dev/nvme6n1 /dev/nvme7n1 /dev/nvme8n1 /dev/nvme9n1
sudo fio --filename=/dev/md0 --size=8196M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randwrite --rw=randwrite --bs=128K --iodepth=10 --numjobs=1 > results/32K.log
cleanup
echo "generating results on 64K chunk size"
sudo mdadm --create -v /dev/md0 --assume-clean --chunk=64 --level=6 --raid-devices=8 --size=134217728 /dev/nvme2n1 /dev/nvme3n1 /dev/nvme4n1 /dev/nvme5n1 /dev/nvme6n1 /dev/nvme7n1 /dev/nvme8n1 /dev/nvme9n1
sudo fio --filename=/dev/md0 --size=8196M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randwrite --rw=randwrite --bs=128K --iodepth=9 --numjobs=1 > results/64K.log
cleanup
echo "generating results on 128K chunk size"
sudo mdadm --create -v /dev/md0 --assume-clean --chunk=128 --level=6 --raid-devices=8 --size=134217728 /dev/nvme2n1 /dev/nvme3n1 /dev/nvme4n1 /dev/nvme5n1 /dev/nvme6n1 /dev/nvme7n1 /dev/nvme8n1 /dev/nvme9n1
sudo fio --filename=/dev/md0 --size=8196M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randwrite --rw=randwrite --bs=128K --iodepth=8 --numjobs=1 > results/128K.log
cleanup
echo "generating results on 256K chunk size"
sudo mdadm --create -v /dev/md0 --assume-clean --chunk=256 --level=6 --raid-devices=8 --size=134217728 /dev/nvme2n1 /dev/nvme3n1 /dev/nvme4n1 /dev/nvme5n1 /dev/nvme6n1 /dev/nvme7n1 /dev/nvme8n1 /dev/nvme9n1
sudo fio --filename=/dev/md0 --size=8196M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randwrite --rw=randwrite --bs=128K --iodepth=8 --numjobs=1 > results/256K.log
cleanup
echo "generating results on 512K chunk size"
sudo mdadm --create -v /dev/md0 --assume-clean --chunk=512 --level=6 --raid-devices=8 --size=134217728 /dev/nvme2n1 /dev/nvme3n1 /dev/nvme4n1 /dev/nvme5n1 /dev/nvme6n1 /dev/nvme7n1 /dev/nvme8n1 /dev/nvme9n1
sudo fio --filename=/dev/md0 --size=8196M --numjobs=1 --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --bs=128K --iodepth=8 --rw=randwrite --group_reporting=1 --name=randwrite > results/512K.log
cleanup
echo "generating results on 1024K chunk size"
sudo mdadm --create -v /dev/md0 --assume-clean --chunk=1024 --level=6 --raid-devices=8 --size=134217728 /dev/nvme2n1 /dev/nvme3n1 /dev/nvme4n1 /dev/nvme5n1 /dev/nvme6n1 /dev/nvme7n1 /dev/nvme8n1 /dev/nvme9n1
sudo fio --filename=/dev/md0 --size=8196M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randwrite --rw=randwrite --bs=128K --iodepth=7 --numjobs=1 > results/1024K.log
cleanup
