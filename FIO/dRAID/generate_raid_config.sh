#!/bin/bash

chunk=$1
raid_size=$2
num_qp=$3

cd ..

python3 create_raid_config.py $HOME/artifacts/ip_addrs_100g.txt $HOME/artifacts/raid5_100g.json $chunk 5 $raid_size $num_qp false
python3 create_raid_config.py $HOME/artifacts/ip_addrs_100g.txt $HOME/artifacts/raid6_100g.json $chunk 6 $raid_size $num_qp false
python3 create_raid_config.py $HOME/artifacts/ip_addrs_25g.txt $HOME/artifacts/raid5_25g.json $chunk 5 $raid_size $num_qp false
python3 create_raid_config.py $HOME/artifacts/ip_addrs_25g.txt $HOME/artifacts/raid6_25g.json $chunk 6 $raid_size $num_qp false
python3 create_raid_config.py $HOME/artifacts/ip_addrs_100g.txt $HOME/artifacts/raid5_100g_d.json $chunk 5 $raid_size $num_qp true
python3 create_raid_config.py $HOME/artifacts/ip_addrs_100g.txt $HOME/artifacts/raid6_100g_d.json $chunk 6 $raid_size $num_qp true
python3 create_raid_config.py $HOME/artifacts/ip_addrs_25g.txt $HOME/artifacts/raid5_25g_d.json $chunk 5 $raid_size $num_qp true
python3 create_raid_config.py $HOME/artifacts/ip_addrs_25g.txt $HOME/artifacts/raid6_25g_d.json $chunk 6 $raid_size $num_qp true

python3 create_spdk_raid_config.py $HOME/artifacts/ip_addrs_100g.txt $HOME/artifacts/raid5_spdk_100g.json $chunk 5 $raid_size false
python3 create_spdk_raid_config.py $HOME/artifacts/ip_addrs_100g.txt $HOME/artifacts/raid6_spdk_100g.json $chunk 6 $raid_size false
python3 create_spdk_raid_config.py $HOME/artifacts/ip_addrs_25g.txt $HOME/artifacts/raid5_spdk_25g.json $chunk 5 $raid_size false
python3 create_spdk_raid_config.py $HOME/artifacts/ip_addrs_25g.txt $HOME/artifacts/raid6_spdk_25g.json $chunk 6 $raid_size false
python3 create_spdk_raid_config.py $HOME/artifacts/ip_addrs_100g.txt $HOME/artifacts/raid5_spdk_100g_d.json $chunk 5 $raid_size true
python3 create_spdk_raid_config.py $HOME/artifacts/ip_addrs_100g.txt $HOME/artifacts/raid6_spdk_100g_d.json $chunk 6 $raid_size true
python3 create_spdk_raid_config.py $HOME/artifacts/ip_addrs_25g.txt $HOME/artifacts/raid5_spdk_25g_d.json $chunk 5 $raid_size true
python3 create_spdk_raid_config.py $HOME/artifacts/ip_addrs_25g.txt $HOME/artifacts/raid6_spdk_25g_d.json $chunk 6 $raid_size true

cd -