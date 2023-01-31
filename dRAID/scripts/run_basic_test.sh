#!/bin/bash

cd ~/dRAID/dRAID/scripts

./run_server_remote_malloc.sh raid5 100g 512 3 1
./generate_raid_config.sh 512 3 1

cd ~/draid-spdk/scripts
sudo ./setup.sh reset
sudo HUGEMEM=90000 PCI_BLOCKED="0000:c5:00.0 0000:c6:00.0" ./setup.sh

cd ~/dRAID/dRAID/host
sudo ./rpc_raid_main -c ~/artifacts/raid5_100g.json -b Raid0

cd ~/dRAID/dRAID/scripts