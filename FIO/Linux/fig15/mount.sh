#!/bin/bash

sudo mdadm --create -v /dev/md0 --assume-clean --chunk=512 --level=5 --raid-devices=8 --size=134217728 /dev/nvme2n1 /dev/nvme3n1 /dev/nvme4n1 /dev/nvme5n1 /dev/nvme6n1 /dev/nvme7n1 /dev/nvme8n1 /dev/nvme9n1


sudo mdadm /dev/md0 -f /dev/nvme2n1