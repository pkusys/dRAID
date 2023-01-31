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