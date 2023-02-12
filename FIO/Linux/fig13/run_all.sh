#!/bin/bash

trap "./unmount.sh" SIGINT

sudo rm -r results
mkdir results
./mount.sh

echo "generating results on read ratio 0"
sudo fio --filename=/dev/md0 --size=8196M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randrw --rw=randrw --bs=128K --iodepth=4 --numjobs=2 --rwmixread=0 > results/0.log
echo "generating results on read ratio 25"
sudo fio --filename=/dev/md0 --size=8196M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randrw --rw=randrw --bs=128K --iodepth=4 --numjobs=2 --rwmixread=25 > results/25.log
echo "generating results on read ratio 50"
sudo fio --filename=/dev/md0 --size=8196M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randrw --rw=randrw --bs=128K --iodepth=3 --numjobs=4 --rwmixread=50 > results/50.log
echo "generating results on read ratio 75"
sudo fio --filename=/dev/md0 --size=8196M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randrw --rw=randrw --bs=128K --iodepth=6 --numjobs=4 --rwmixread=75 > results/75.log
echo "generating results on read ratio 100"
sudo fio --filename=/dev/md0 --size=8196M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randrw --rw=randrw --bs=128K --iodepth=8 --numjobs=4 --rwmixread=100 > results/100.log

./unmount.sh
