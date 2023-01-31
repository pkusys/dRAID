#!/bin/bash

username=$1
interface=$2

name=`hostname -s`
ip_addr=`ip address show dev "$interface" | grep -oP "inet\s+\K\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}"`

sudo kill -9 $(ps aux | grep '[r]aid' | awk '{print $2}')
sudo kill -9 $(ps aux | grep '[n]vmf_tgt' | awk '{print $2}')

cd /users/$username/spdk/scripts

sudo rm nohup.out

sudo ./setup.sh reset
sudo HUGEMEM=10240 PCI_ALLOWED="0000:c6:00.0" ./setup.sh

sudo sh -c "nohup ../build/bin/nvmf_tgt -m 0x1000 &"

sleep 3

sudo ./rpc.py nvmf_create_transport -t RDMA -u 8192 -m 8 -c 0
sudo ./rpc.py bdev_nvme_attach_controller -b Nvme0 -t PCIe -a 0000:c6:00.0
sudo ./rpc.py nvmf_create_subsystem "nqn.2016-06.io.spdk:c$name" -a -s SPDK00000000000000 -d SPDK_Controller0
sleep 1
sudo ./rpc.py nvmf_subsystem_add_ns "nqn.2016-06.io.spdk:c$name" Nvme0n1
sudo ./rpc.py nvmf_subsystem_add_listener "nqn.2016-06.io.spdk:c$name" -t rdma -a $ip_addr -s 4420
