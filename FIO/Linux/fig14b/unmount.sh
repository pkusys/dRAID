#!/bin/bash


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