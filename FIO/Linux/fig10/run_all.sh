#!/bin/bash

trap "./unmount.sh; exit 0" SIGINT

sudo rm -r results
mkdir results
./mount.sh

echo "generating results on 4K I/O size"
sudo fio --filename=/dev/md0 --size=8196M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randwrite --rw=randwrite --bs=4K --iodepth=6 --numjobs=2 > results/4K.log
echo "generating results on 8K I/O size"
sudo fio --filename=/dev/md0 --size=8196M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randwrite --rw=randwrite --bs=8K --iodepth=5 --numjobs=2 > results/8K.log
echo "generating results on 16K I/O size"
sudo fio --filename=/dev/md0 --size=8196M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randwrite --rw=randwrite --bs=16K --iodepth=4 --numjobs=2 > results/16K.log
echo "generating results on 32K I/O size"
sudo fio --filename=/dev/md0 --size=8196M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randwrite --rw=randwrite --bs=32K --iodepth=7 --numjobs=1 > results/32K.log
echo "generating results on 64K I/O size"
sudo fio --filename=/dev/md0 --size=8196M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randwrite --rw=randwrite --bs=64K --iodepth=7 --numjobs=1 > results/64K.log
echo "generating results on 128K I/O size"
sudo fio --filename=/dev/md0 --size=8196M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randwrite --rw=randwrite --bs=128K --iodepth=6 --numjobs=1 > results/128K.log
echo "generating results on 256K I/O size"
sudo fio --filename=/dev/md0 --size=8196M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randwrite --rw=randwrite --bs=256K --iodepth=4 --numjobs=1 > results/256K.log
echo "generating results on 512K I/O size"
sudo fio --filename=/dev/md0 --size=8196M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randwrite --rw=randwrite --bs=512K --iodepth=3 --numjobs=1 > results/512K.log
echo "generating results on 1024K I/O size"
sudo fio --filename=/dev/md0 --size=8196M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randwrite --rw=randwrite --bs=1024K --iodepth=2 --numjobs=1 > results/1024K.log
echo "generating results on 2048K I/O size"
sudo fio --filename=/dev/md0 --size=8196M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randwrite --rw=randwrite --bs=2048K --iodepth=2 --numjobs=1 > results/2048K.log
echo "generating results on 3584K I/O size"
sudo fio --filename=/dev/md0 --size=8196M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randwrite --rw=randwrite --bs=3584K --iodepth=1 --numjobs=1 > results/3584K.log

./unmount.sh
