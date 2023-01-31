#!/bin/bash

username=`whoami`
workload=$1
conf_json=/users/kyleshu/artifacts/raid5_spdk_100g_d.json
cli_num=24

app_path=~/rocksdb/ycsb/build

sudo kill -9 $(ps aux | grep '[r]aid' | awk '{print $2}')

if [[ "$workload" == "A" ]]
then
  cd $app_path
  cli_num=40
  sudo ./test ../workloads/workloada.spec $cli_num /users/kyleshu/data /users/kyleshu/data 1 rocksdb /users/kyleshu/bak $conf_json Raid0 1
  exit 0
fi

if [[ "$workload" == "B" ]]
then
  cd $app_path
  sudo ./test ../workloads/workloadb.spec $cli_num /users/kyleshu/data /users/kyleshu/data 1 rocksdb /users/kyleshu/bak $conf_json Raid0 1
  exit 0
fi


if [[ "$workload" == "C" ]]
then
  cd $app_path
  sudo ./test ../workloads/workloadc.spec $cli_num /users/kyleshu/data /users/kyleshu/data 1 rocksdb /users/kyleshu/bak $conf_json Raid0 1
  exit 0
fi


if [[ "$workload" == "D" ]]
then
  cd $app_path
  sudo ./test ../workloads/workloadd.spec $cli_num /users/kyleshu/data /users/kyleshu/data 1 rocksdb /users/kyleshu/bak $conf_json Raid0 1
  exit 0
fi


if [[ "$workload" == "F" ]]
then
  cd $app_path
  sudo ./test ../workloads/workloadf.spec $cli_num /users/kyleshu/data /users/kyleshu/data 1 rocksdb /users/kyleshu/bak $conf_json Raid0 1
  exit 0
fi
