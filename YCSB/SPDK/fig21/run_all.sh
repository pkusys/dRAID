#!/bin/bash

username=`whoami`
workload=$1
conf_json=/users/${username}/artifacts/raid5_spdk_100g_d.json
cli_num=24

app_path=~/rocksdb/ycsb/build
cur_path=~/dRAID/YCSB/SPDK/fig21

sudo kill -9 $(ps aux | grep '[r]aid' | awk '{print $2}')
mkdir -p results

cd $cur_path
if [ ! -e results/A.log ] || ! grep -q "total tput" results/A.log
then
  cd $app_path
	cli_num=40
  sudo ./test ../workloads/workloada.spec $cli_num /users/${username}/data \
   /users/${username}/data 1 rocksdb /users/${username}/bak $conf_json Raid0 1 > ${cur_path}/results/A.log
fi

cli_num=24
cd $cur_path
if [ ! -e results/B.log ] || ! grep -q "total tput" results/B.log
then
  cd $app_path
  sudo ./test ../workloads/workloadb.spec $cli_num /users/${username}/data \
   /users/${username}/data 1 rocksdb /users/${username}/bak $conf_json Raid0 1 > ${cur_path}/results/B.log
fi

cd $cur_path
if [ ! -e results/C.log ] || ! grep -q "total tput" results/C.log
then
  cd $app_path
  sudo ./test ../workloads/workloadc.spec $cli_num /users/${username}/data \
   /users/${username}/data 1 rocksdb /users/${username}/bak $conf_json Raid0 1 > ${cur_path}/results/C.log
fi

cd $cur_path
if [ ! -e results/D.log ] || ! grep -q "total tput" results/D.log
then
  cd $app_path
  sudo ./test ../workloads/workloadd.spec $cli_num /users/${username}/data \
   /users/${username}/data 1 rocksdb /users/${username}/bak $conf_json Raid0 1 > ${cur_path}/results/D.log
fi

cd $cur_path
if [ ! -e results/F.log ] || ! grep -q "total tput" results/F.log
then
  cd $app_path
  sudo ./test ../workloads/workloadf.spec $cli_num /users/${username}/data \
   /users/${username}/data 1 rocksdb /users/${username}/bak $conf_json Raid0 1 > ${cur_path}/results/F.log
fi
