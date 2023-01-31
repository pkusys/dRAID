#!/bin/bash

# This is a script for CentOS 8

cd ~

# basic configuration for SPDK
sudo sh -c "echo -e \"*        hard    memlock         unlimited\n*        soft    memlock         unlimited\" >> /etc/security/limits.conf"
sudo ifconfig eno34 mtu 4200
sudo sysctl -w vm.nr_hugepages=32768

# clone customized version of SPDK and FIO for dRAID
git clone https://github.com/kyleshu/draid-spdk.git
cd draid-spdk
git submodule update --init
sudo scripts/pkgdep.sh --all
cd ..
git clone https://github.com/axboe/fio
cd fio
make
sudo make install
sudo ldconfig
cd ..
cd draid-spdk
./configure --with-rdma --with-fio=$HOME/fio --with-isal --with-raid5 --disable-unit-tests
make
cd ..

# clone the original SPDK as baseline
git clone https://github.com/kyleshu/spdk.git
cd spdk
git submodule update --init
./configure --with-rdma --with-fio=$HOME/fio --with-isal --with-raid5 --disable-unit-tests
make
cd ..

# clone customized version of rocksdb for testing
git clone https://github.com/kyleshu/rocksdb.git