#!/bin/bash

# This is a script for Ubuntu 18

cd ~

# install dependencies of Linux RAID
sudo apt update -y
sudo apt install nvme-cli -y
sudo apt install mdadm -y
sudo apt install libaio-dev -y
sudo modprobe nvme-rdma
sudo mkdir /raid
# install FIO
git clone https://github.com/axboe/fio
cd fio
make
sudo make install
sudo ldconfig
cd ..