#!/bin/bash

username=`whoami`
conf_json=/users/kyleshu/artifacts/raid5_100g_d.json
cli_num=24

app_path=~/rocksdb/ycsb/build
cur_path=${mkfs_path}/fig21

sudo kill -9 $(ps aux | grep '[r]aid' | awk '{print $2}')
mkdir -p results

cd $cur_path
if [ ! -e results/A.log ] || ! grep -q "total tput" results/A.log
then
	echo "generating results on YCSB-A"
  ../generate_raid_config.sh 512 8 1
  ../run_server_remote_nvme.sh $username raid5 100g 512 8 1
  sleep 6
  cd $app_path
  cli_num=40
  sudo timeout -k 5 180 ./test ../workloads/workloada.spec $cli_num /users/kyleshu/data \
   /users/kyleshu/data 1 rocksdb /users/kyleshu/bak $conf_json Raid0 1 > ${cur_path}/results/A.log
fi

cli_num=24
cd $cur_path
if [ ! -e results/B.log ] || ! grep -q "total tput" results/B.log
then
	echo "generating results on YCSB-B"
  ../generate_raid_config.sh 512 8 1
  ../run_server_remote_nvme.sh $username raid5 100g 512 8 1
  sleep 6
  cd $app_path
  sudo timeout -k 5 180 ./test ../workloads/workloadb.spec $cli_num /users/kyleshu/data \
   /users/kyleshu/data 1 rocksdb /users/kyleshu/bak $conf_json Raid0 1 > ${cur_path}/results/B.log
fi

cd $cur_path
if [ ! -e results/C.log ] || ! grep -q "total tput" results/C.log
then
	echo "generating results on YCSB-C"
  ../generate_raid_config.sh 512 8 1
  ../run_server_remote_nvme.sh $username raid5 100g 512 8 1
  sleep 6
  cd $app_path
  sudo timeout -k 5 180 ./test ../workloads/workloadc.spec $cli_num /users/kyleshu/data \
   /users/kyleshu/data 1 rocksdb /users/kyleshu/bak $conf_json Raid0 1 > ${cur_path}/results/C.log
fi

cd $cur_path
if [ ! -e results/D.log ] || ! grep -q "total tput" results/D.log
then
	echo "generating results on YCSB-D"
  ../generate_raid_config.sh 512 8 1
  ../run_server_remote_nvme.sh $username raid5 100g 512 8 1
  sleep 6
  cd $app_path
  sudo timeout -k 5 180 ./test ../workloads/workloadd.spec $cli_num /users/kyleshu/data \
   /users/kyleshu/data 1 rocksdb /users/kyleshu/bak $conf_json Raid0 1 > ${cur_path}/results/D.log
fi

cd $cur_path
if [ ! -e results/F.log ] || ! grep -q "total tput" results/F.log
then
	echo "generating results on YCSB-F"
  ../generate_raid_config.sh 512 8 1
  ../run_server_remote_nvme.sh $username raid5 100g 512 8 1
  sleep 6
  cd $app_path
  sudo timeout -k 5 180 ./test ../workloads/workloadf.spec $cli_num /users/kyleshu/data \
   /users/kyleshu/data 1 rocksdb /users/kyleshu/bak $conf_json Raid0 1 > ${cur_path}/results/F.log
fi
