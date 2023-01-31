#!/bin/bash

stripe_width=$1

if [[ "$stripe_width" == "4" ]]
then
  sudo mdadm --create -v /dev/md0 --assume-clean --chunk=512 --level=5 --raid-devices=4 --size=134217728 /dev/nvme2n1 /dev/nvme3n1 /dev/nvme4n1 /dev/nvme5n1

  sudo fio --filename=/dev/md0 --size=8196M --numjobs=1 --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --rw=randwrite --group_reporting=1 --name=randwrite --bs=128K --iodepth=6

  sudo mdadm -S /dev/md0
  sudo mdadm --zero-superblock /dev/nvme2n1
  sudo mdadm --zero-superblock /dev/nvme3n1
  sudo mdadm --zero-superblock /dev/nvme4n1
  sudo mdadm --zero-superblock /dev/nvme5n1
  exit 0
fi

if [[ "$stripe_width" == "6" ]]
then
  sudo mdadm --create -v /dev/md0 --assume-clean --chunk=512 --level=5 --raid-devices=6 --size=134217728 /dev/nvme2n1 /dev/nvme3n1 /dev/nvme4n1 /dev/nvme5n1 /dev/nvme6n1 /dev/nvme7n1

  sudo fio --filename=/dev/md0 --size=8196M --numjobs=1 --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --rw=randwrite --group_reporting=1 --name=randwrite --bs=128K --iodepth=6

  sudo mdadm -S /dev/md0
  sudo mdadm --zero-superblock /dev/nvme2n1
  sudo mdadm --zero-superblock /dev/nvme3n1
  sudo mdadm --zero-superblock /dev/nvme4n1
  sudo mdadm --zero-superblock /dev/nvme5n1
  sudo mdadm --zero-superblock /dev/nvme6n1
  sudo mdadm --zero-superblock /dev/nvme7n1
  exit 0
fi

if [[ "$stripe_width" == "8" ]]
then
  sudo mdadm --create -v /dev/md0 --assume-clean --chunk=512 --level=5 --raid-devices=8 --size=134217728 /dev/nvme2n1 /dev/nvme3n1 /dev/nvme4n1 /dev/nvme5n1 /dev/nvme6n1 /dev/nvme7n1 /dev/nvme8n1 /dev/nvme9n1

  sudo fio --filename=/dev/md0 --size=8196M --numjobs=1 --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --rw=randwrite --group_reporting=1 --name=randwrite --bs=128K --iodepth=7

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

if [[ "$stripe_width" == "10" ]]
then
  sudo mdadm --create -v /dev/md0 --assume-clean --chunk=512 --level=5 --raid-devices=10 --size=134217728 /dev/nvme2n1 /dev/nvme3n1 /dev/nvme4n1 /dev/nvme5n1 /dev/nvme6n1 /dev/nvme7n1 /dev/nvme8n1 /dev/nvme9n1 /dev/nvme10n1 /dev/nvme11n1

  sudo fio --filename=/dev/md0 --size=8196M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --rw=randwrite --group_reporting=1 --name=randwrite --bs=128K --iodepth=8 --numjobs=1

  sudo mdadm -S /dev/md0
  sudo mdadm --zero-superblock /dev/nvme2n1
  sudo mdadm --zero-superblock /dev/nvme3n1
  sudo mdadm --zero-superblock /dev/nvme4n1
  sudo mdadm --zero-superblock /dev/nvme5n1
  sudo mdadm --zero-superblock /dev/nvme6n1
  sudo mdadm --zero-superblock /dev/nvme7n1
  sudo mdadm --zero-superblock /dev/nvme8n1
  sudo mdadm --zero-superblock /dev/nvme9n1
  sudo mdadm --zero-superblock /dev/nvme10n1
  sudo mdadm --zero-superblock /dev/nvme11n1
  exit 0
fi

if [[ "$stripe_width" == "12" ]]
then
  sudo mdadm --create -v /dev/md0 --assume-clean --chunk=512 --level=5 --raid-devices=12 --size=134217728 /dev/nvme2n1 /dev/nvme3n1 /dev/nvme4n1 /dev/nvme5n1 /dev/nvme6n1 /dev/nvme7n1 /dev/nvme8n1 /dev/nvme9n1 /dev/nvme10n1 /dev/nvme11n1 /dev/nvme12n1 /dev/nvme13n1

  sudo fio --filename=/dev/md0 --size=8196M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randwrite --rw=randwrite --bs=128K --iodepth=8 --numjobs=1

  sudo mdadm -S /dev/md0
  sudo mdadm --zero-superblock /dev/nvme2n1
  sudo mdadm --zero-superblock /dev/nvme3n1
  sudo mdadm --zero-superblock /dev/nvme4n1
  sudo mdadm --zero-superblock /dev/nvme5n1
  sudo mdadm --zero-superblock /dev/nvme6n1
  sudo mdadm --zero-superblock /dev/nvme7n1
  sudo mdadm --zero-superblock /dev/nvme8n1
  sudo mdadm --zero-superblock /dev/nvme9n1
  sudo mdadm --zero-superblock /dev/nvme10n1
  sudo mdadm --zero-superblock /dev/nvme11n1
  sudo mdadm --zero-superblock /dev/nvme12n1
  sudo mdadm --zero-superblock /dev/nvme13n1
  exit 0
fi

if [[ "$stripe_width" == "14" ]]
then
  sudo mdadm --create -v /dev/md0 --assume-clean --chunk=512 --level=5 --raid-devices=14 --size=134217728 /dev/nvme2n1 /dev/nvme3n1 /dev/nvme4n1 /dev/nvme5n1 /dev/nvme6n1 /dev/nvme7n1 /dev/nvme8n1 /dev/nvme9n1 /dev/nvme10n1 /dev/nvme11n1 /dev/nvme12n1 /dev/nvme13n1 /dev/nvme14n1 /dev/nvme15n1

  sudo fio --filename=/dev/md0 --size=8196M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randwrite --rw=randwrite --bs=128K --iodepth=8 --numjobs=1

  sudo mdadm -S /dev/md0
  sudo mdadm --zero-superblock /dev/nvme2n1
  sudo mdadm --zero-superblock /dev/nvme3n1
  sudo mdadm --zero-superblock /dev/nvme4n1
  sudo mdadm --zero-superblock /dev/nvme5n1
  sudo mdadm --zero-superblock /dev/nvme6n1
  sudo mdadm --zero-superblock /dev/nvme7n1
  sudo mdadm --zero-superblock /dev/nvme8n1
  sudo mdadm --zero-superblock /dev/nvme9n1
  sudo mdadm --zero-superblock /dev/nvme10n1
  sudo mdadm --zero-superblock /dev/nvme11n1
  sudo mdadm --zero-superblock /dev/nvme12n1
  sudo mdadm --zero-superblock /dev/nvme13n1
  sudo mdadm --zero-superblock /dev/nvme14n1
  sudo mdadm --zero-superblock /dev/nvme15n1
  exit 0
fi

if [[ "$stripe_width" == "16" ]]
then
  sudo mdadm --create -v /dev/md0 --assume-clean --chunk=512 --level=5 --raid-devices=16 --size=134217728 /dev/nvme2n1 /dev/nvme3n1 /dev/nvme4n1 /dev/nvme5n1 /dev/nvme6n1 /dev/nvme7n1 /dev/nvme8n1 /dev/nvme9n1 /dev/nvme10n1 /dev/nvme11n1 /dev/nvme12n1 /dev/nvme13n1 /dev/nvme14n1 /dev/nvme15n1 /dev/nvme16n1 /dev/nvme17n1

  sudo fio --filename=/dev/md0 --size=8196M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randwrite --rw=randwrite --bs=128K --iodepth=8 --numjobs=1

  sudo mdadm -S /dev/md0
  sudo mdadm --zero-superblock /dev/nvme2n1
  sudo mdadm --zero-superblock /dev/nvme3n1
  sudo mdadm --zero-superblock /dev/nvme4n1
  sudo mdadm --zero-superblock /dev/nvme5n1
  sudo mdadm --zero-superblock /dev/nvme6n1
  sudo mdadm --zero-superblock /dev/nvme7n1
  sudo mdadm --zero-superblock /dev/nvme8n1
  sudo mdadm --zero-superblock /dev/nvme9n1
  sudo mdadm --zero-superblock /dev/nvme10n1
  sudo mdadm --zero-superblock /dev/nvme11n1
  sudo mdadm --zero-superblock /dev/nvme12n1
  sudo mdadm --zero-superblock /dev/nvme13n1
  sudo mdadm --zero-superblock /dev/nvme14n1
  sudo mdadm --zero-superblock /dev/nvme15n1
  sudo mdadm --zero-superblock /dev/nvme16n1
  sudo mdadm --zero-superblock /dev/nvme17n1
  exit 0
fi

if [[ "$stripe_width" == "18" ]]
then
  sudo mdadm --create -v /dev/md0 --assume-clean --chunk=512 --level=5 --raid-devices=18 --size=134217728 /dev/nvme2n1 /dev/nvme3n1 /dev/nvme4n1 /dev/nvme5n1 /dev/nvme6n1 /dev/nvme7n1 /dev/nvme8n1 /dev/nvme9n1 /dev/nvme10n1 /dev/nvme11n1 /dev/nvme12n1 /dev/nvme13n1 /dev/nvme14n1 /dev/nvme15n1 /dev/nvme16n1 /dev/nvme17n1 /dev/nvme18n1 /dev/nvme19n1

  sudo fio --filename=/dev/md0 --size=8196M --time_based --runtime=15s --ramp_time=2s --ioengine=libaio --direct=1 --verify=0 --group_reporting=1 --name=randwrite --rw=randwrite --bs=128K --iodepth=8 --numjobs=1

  sudo mdadm -S /dev/md0
  sudo mdadm --zero-superblock /dev/nvme2n1
  sudo mdadm --zero-superblock /dev/nvme3n1
  sudo mdadm --zero-superblock /dev/nvme4n1
  sudo mdadm --zero-superblock /dev/nvme5n1
  sudo mdadm --zero-superblock /dev/nvme6n1
  sudo mdadm --zero-superblock /dev/nvme7n1
  sudo mdadm --zero-superblock /dev/nvme8n1
  sudo mdadm --zero-superblock /dev/nvme9n1
  sudo mdadm --zero-superblock /dev/nvme10n1
  sudo mdadm --zero-superblock /dev/nvme11n1
  sudo mdadm --zero-superblock /dev/nvme12n1
  sudo mdadm --zero-superblock /dev/nvme13n1
  sudo mdadm --zero-superblock /dev/nvme14n1
  sudo mdadm --zero-superblock /dev/nvme15n1
  sudo mdadm --zero-superblock /dev/nvme16n1
  sudo mdadm --zero-superblock /dev/nvme17n1
  sudo mdadm --zero-superblock /dev/nvme18n1
  sudo mdadm --zero-superblock /dev/nvme19n1
  exit 0
fi

echo "We do not have a pre-written command for this stripe width. Feel free to run your own command."