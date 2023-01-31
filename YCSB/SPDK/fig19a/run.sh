#!/bin/bash

username=`whoami`
workload=$1
conf_json=/users/kyleshu/artifacts/raid5_spdk_100g.json
cli_num=36


mkfs_path=~/spdk/test/blobfs/mkfs
app_path=~/rocksdb/ycsb/build

sudo kill -9 $(ps aux | grep '[r]aid' | awk '{print $2}')

if [[ "$workload" == "A" ]]
then
  cd $mkfs_path
  sudo ./mkfs $conf_json Raid0
  sleep 1
  cd $app_path
  sudo ./test ../workloads/workloada.spec $cli_num /users/kyleshu/data /users/kyleshu/data 1 rocksdb /users/kyleshu/bak $conf_json Raid0 1 24
  exit 0
fi

if [[ "$workload" == "B" ]]
then
  cd $mkfs_path
  sudo ./mkfs $conf_json Raid0
  sleep 1
  cd $app_path
  sudo ./test ../workloads/workloadb.spec $cli_num /users/kyleshu/data /users/kyleshu/data 1 rocksdb /users/kyleshu/bak $conf_json Raid0 1
  exit 0
fi


if [[ "$workload" == "C" ]]
then
  cd $mkfs_path
  sudo ./mkfs $conf_json Raid0
  sleep 1
  cd $app_path
  sudo ./test ../workloads/workloadc.spec $cli_num /users/kyleshu/data /users/kyleshu/data 1 rocksdb /users/kyleshu/bak $conf_json Raid0 1
  exit 0
fi


if [[ "$workload" == "D" ]]
then
  cd $mkfs_path
  sudo ./mkfs $conf_json Raid0
  sleep 1
  cd $app_path
  sudo ./test ../workloads/workloadd.spec $cli_num /users/kyleshu/data /users/kyleshu/data 1 rocksdb /users/kyleshu/bak $conf_json Raid0 1
  exit 0
fi


if [[ "$workload" == "F" ]]
then
  cd $mkfs_path
  sudo ./mkfs $conf_json Raid0
  sleep 1
  cd $app_path
  sudo ./test ../workloads/workloadf.spec $cli_num /users/kyleshu/data /users/kyleshu/data 1 rocksdb /users/kyleshu/bak $conf_json Raid0 1
  exit 0
fi
