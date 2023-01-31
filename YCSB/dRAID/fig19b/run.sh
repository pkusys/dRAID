#!/bin/bash

username=`whoami`
workload=$1
conf_json=/users/kyleshu/artifacts/raid5_100g_d.json
cli_num=30


mkfs_path=~/dRAID/YCSB/dRAID
app_path=~/rocksdb/ycsb/build
cur_path=${mkfs_path}/fig19b

sudo kill -9 $(ps aux | grep '[r]aid' | awk '{print $2}')

if [[ "$workload" == "A" ]]
then
  ../generate_raid_config.sh 512 8 1
  ../run_server_remote_nvme.sh $username raid5 100g 512 8 1
  sleep 6
  cd $mkfs_path
  sudo ./mkfs $conf_json Raid0
  sleep 1
  cd $cur_path
  ../run_server_remote_nvme.sh $username raid5 100g 512 8 1
  sleep 6
  cd $app_path
  sudo ./test ../workloads/workloada.spec $cli_num /users/kyleshu/data /users/kyleshu/data 1 rocksdb /users/kyleshu/bak $conf_json Raid0 1
  exit 0
fi

if [[ "$workload" == "B" ]]
then
  ../generate_raid_config.sh 512 8 1
  ../run_server_remote_nvme.sh $username raid5 100g 512 8 1
  sleep 6
  cd $mkfs_path
  sudo ./mkfs $conf_json Raid0
  sleep 1
  cd $cur_path
  ../run_server_remote_nvme.sh $username raid5 100g 512 8 1
  sleep 6
  cd $app_path
  sudo ./test ../workloads/workloadb.spec $cli_num /users/kyleshu/data /users/kyleshu/data 1 rocksdb /users/kyleshu/bak $conf_json Raid0 1
  exit 0
fi


if [[ "$workload" == "C" ]]
then
  ../generate_raid_config.sh 512 8 1
  ../run_server_remote_nvme.sh $username raid5 100g 512 8 1
  sleep 6
  cd $mkfs_path
  sudo ./mkfs $conf_json Raid0
  sleep 1
  cd $cur_path
  ../run_server_remote_nvme.sh $username raid5 100g 512 8 1
  sleep 6
  cd $app_path
  sudo ./test ../workloads/workloadc.spec $cli_num /users/kyleshu/data /users/kyleshu/data 1 rocksdb /users/kyleshu/bak $conf_json Raid0 1
  exit 0
fi


if [[ "$workload" == "D" ]]
then
  ../generate_raid_config.sh 512 8 1
  ../run_server_remote_nvme.sh $username raid5 100g 512 8 1
  sleep 6
  cd $mkfs_path
  sudo ./mkfs $conf_json Raid0
  sleep 1
  cd $cur_path
  ../run_server_remote_nvme.sh $username raid5 100g 512 8 1
  sleep 6
  cd $app_path
  sudo ./test ../workloads/workloadd.spec $cli_num /users/kyleshu/data /users/kyleshu/data 1 rocksdb /users/kyleshu/bak $conf_json Raid0 1
  exit 0
fi


if [[ "$workload" == "F" ]]
then
  ../generate_raid_config.sh 512 8 1
  ../run_server_remote_nvme.sh $username raid5 100g 512 8 1
  sleep 6
  cd $mkfs_path
  sudo ./mkfs $conf_json Raid0
  sleep 1
  cd $cur_path
  ../run_server_remote_nvme.sh $username raid5 100g 512 8 1
  sleep 6
  cd $app_path
  sudo ./test ../workloads/workloadf.spec $cli_num /users/kyleshu/data /users/kyleshu/data 1 rocksdb /users/kyleshu/bak $conf_json Raid0 1
  exit 0
fi
