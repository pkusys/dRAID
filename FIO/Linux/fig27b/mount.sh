#!/bin/bash

sudo mdadm --create -v /dev/md0 --assume-clean --chunk=512 --level=6 --raid-devices=18 --size=134217728 /dev/nvme2n1 /dev/nvme3n1 /dev/nvme4n1 /dev/nvme5n1 /dev/nvme6n1 /dev/nvme7n1 /dev/nvme8n1 /dev/nvme9n1 /dev/nvme10n1 /dev/nvme11n1 /dev/nvme12n1 /dev/nvme13n1 /dev/nvme14n1 /dev/nvme15n1 /dev/nvme16n1 /dev/nvme17n1 /dev/nvme18n1 /dev/nvme19n1
