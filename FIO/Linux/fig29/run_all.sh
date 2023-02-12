#!/bin/bash

stripe_width=4

function cleanup()
{
    sudo mdadm -S /dev/md0
    let i=1
    while [ $i -le $stripe_width ]
    do
      let i+=1
      sudo mdadm --zero-superblock "/dev/nvme${i}n1"
    done
}

trap "cleanup; exit 0" SIGINT

sudo rm -r results
mkdir results

stripe_width=4
echo "generating results on stripe width $stripe_width"
sudo mdadm --create -v /dev/md0 --assume-clean --chunk=512 --level=6 --raid-devices=4 --size=134217728 /dev/nvme2n1 /dev/nvme3n1 /dev/nvme4n1 /dev/nvme5n1
sudo mdadm /dev/md0 -f /dev/nvme2n1
sudo fio --filename=/dev/md0 --size=2048M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randwrite --rw=randread --rwmixread=0 --bs=128K --iodepth=5 --numjobs=1 > results/4.log
cleanup
stripe_width=6
echo "generating results on stripe width $stripe_width"
sudo mdadm --create -v /dev/md0 --assume-clean --chunk=512 --level=6 --raid-devices=6 --size=134217728 /dev/nvme2n1 /dev/nvme3n1 /dev/nvme4n1 /dev/nvme5n1 /dev/nvme6n1 /dev/nvme7n1
sudo mdadm /dev/md0 -f /dev/nvme2n1
sudo fio --filename=/dev/md0 --size=2048M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randwrite --rw=randread --rwmixread=0 --bs=128K --iodepth=4 --numjobs=1 > results/6.log
cleanup
stripe_width=8
echo "generating results on stripe width $stripe_width"
sudo mdadm --create -v /dev/md0 --assume-clean --chunk=512 --level=6 --raid-devices=8 --size=134217728 /dev/nvme2n1 /dev/nvme3n1 /dev/nvme4n1 /dev/nvme5n1 /dev/nvme6n1 /dev/nvme7n1 /dev/nvme8n1 /dev/nvme9n1
sudo mdadm /dev/md0 -f /dev/nvme2n1
sudo fio --filename=/dev/md0 --size=2048M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randwrite --rw=randread --rwmixread=0 --bs=128K --iodepth=5 --numjobs=1 > results/8.log
cleanup
stripe_width=10
echo "generating results on stripe width $stripe_width"
sudo mdadm --create -v /dev/md0 --assume-clean --chunk=512 --level=6 --raid-devices=10 --size=134217728 /dev/nvme2n1 /dev/nvme3n1 /dev/nvme4n1 /dev/nvme5n1 /dev/nvme6n1 /dev/nvme7n1 /dev/nvme8n1 /dev/nvme9n1 /dev/nvme10n1 /dev/nvme11n1
sudo mdadm /dev/md0 -f /dev/nvme2n1
sudo fio --filename=/dev/md0 --size=2048M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randwrite --rw=randread --rwmixread=0 --bs=128K --iodepth=4 --numjobs=1 > results/10.log
cleanup
stripe_width=12
echo "generating results on stripe width $stripe_width"
sudo mdadm --create -v /dev/md0 --assume-clean --chunk=512 --level=6 --raid-devices=12 --size=134217728 /dev/nvme2n1 /dev/nvme3n1 /dev/nvme4n1 /dev/nvme5n1 /dev/nvme6n1 /dev/nvme7n1 /dev/nvme8n1 /dev/nvme9n1 /dev/nvme10n1 /dev/nvme11n1 /dev/nvme12n1 /dev/nvme13n1
sudo mdadm /dev/md0 -f /dev/nvme2n1
sudo fio --filename=/dev/md0 --size=2048M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randwrite --rw=randread --rwmixread=0 --bs=128K --iodepth=4 --numjobs=1 > results/12.log
cleanup
stripe_width=14
echo "generating results on stripe width $stripe_width"
sudo mdadm --create -v /dev/md0 --assume-clean --chunk=512 --level=6 --raid-devices=14 --size=134217728 /dev/nvme2n1 /dev/nvme3n1 /dev/nvme4n1 /dev/nvme5n1 /dev/nvme6n1 /dev/nvme7n1 /dev/nvme8n1 /dev/nvme9n1 /dev/nvme10n1 /dev/nvme11n1 /dev/nvme12n1 /dev/nvme13n1 /dev/nvme14n1 /dev/nvme15n1
sudo mdadm /dev/md0 -f /dev/nvme2n1
sudo fio --filename=/dev/md0 --size=2048M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randwrite --rw=randread --rwmixread=0 --bs=128K --iodepth=3 --numjobs=1 > results/14.log
cleanup
stripe_width=16
echo "generating results on stripe width $stripe_width"
sudo mdadm --create -v /dev/md0 --assume-clean --chunk=512 --level=6 --raid-devices=16 --size=134217728 /dev/nvme2n1 /dev/nvme3n1 /dev/nvme4n1 /dev/nvme5n1 /dev/nvme6n1 /dev/nvme7n1 /dev/nvme8n1 /dev/nvme9n1 /dev/nvme10n1 /dev/nvme11n1 /dev/nvme12n1 /dev/nvme13n1 /dev/nvme14n1 /dev/nvme15n1 /dev/nvme16n1 /dev/nvme17n1
sudo mdadm /dev/md0 -f /dev/nvme2n1
sudo fio --filename=/dev/md0 --size=2048M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randwrite --rw=randread --rwmixread=0 --bs=128K --iodepth=2 --numjobs=1 > results/16.log
cleanup
stripe_width=18
echo "generating results on stripe width $stripe_width"
sudo mdadm --create -v /dev/md0 --assume-clean --chunk=512 --level=6 --raid-devices=18 --size=134217728 /dev/nvme2n1 /dev/nvme3n1 /dev/nvme4n1 /dev/nvme5n1 /dev/nvme6n1 /dev/nvme7n1 /dev/nvme8n1 /dev/nvme9n1 /dev/nvme10n1 /dev/nvme11n1 /dev/nvme12n1 /dev/nvme13n1 /dev/nvme14n1 /dev/nvme15n1 /dev/nvme16n1 /dev/nvme17n1 /dev/nvme18n1 /dev/nvme19n1
sudo mdadm /dev/md0 -f /dev/nvme2n1
sudo fio --filename=/dev/md0 --size=2048M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randwrite --rw=randread --rwmixread=0 --bs=128K --iodepth=2 --numjobs=1 > results/18.log
cleanup
