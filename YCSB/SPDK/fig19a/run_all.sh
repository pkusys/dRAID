#!/bin/bash

username=`whoami`
conf_json=/users/${username}/artifacts/raid5_spdk_100g.json
cli_num=36


mkfs_path=~/spdk/test/blobfs/mkfs
app_path=~/rocksdb/ycsb/build
cur_path=~/dRAID/YCSB/SPDK/fig19a

sudo kill -9 $(ps aux | grep '[r]aid' | awk '{print $2}')
mkdir -p results

cd $cur_path
if [ ! -e results/A.log ] || ! grep -q "total tput" results/A.log
then
	echo "generating results on YCSB-A"
  cd $mkfs_path
  sudo ./mkfs $conf_json Raid0
	if [[ $? == 0 ]]
  then
		sleep 1
		cd $app_path
		sudo ./test ../workloads/workloada.spec $cli_num /users/${username}/data \
		/users/${username}/data 1 rocksdb /users/${username}/bak $conf_json Raid0 1 24 > ${cur_path}/results/A.log
	fi
fi

cd $cur_path
if [ ! -e results/B.log ] || ! grep -q "total tput" results/B.log
then
	echo "generating results on YCSB-B"
  cd $mkfs_path
  sudo ./mkfs $conf_json Raid0
	if [[ $? == 0 ]]
  then
		sleep 1
		cd $app_path
		sudo ./test ../workloads/workloadb.spec $cli_num /users/${username}/data \
		/users/${username}/data 1 rocksdb /users/${username}/bak $conf_json Raid0 1 > ${cur_path}/results/B.log
	fi
fi

cd $cur_path
if [ ! -e results/C.log ] || ! grep -q "total tput" results/C.log
then
	echo "generating results on YCSB-C"
  cd $mkfs_path
  sudo ./mkfs $conf_json Raid0
	if [[ $? == 0 ]]
  then
		sleep 1
		cd $app_path
		sudo ./test ../workloads/workloadc.spec $cli_num /users/${username}/data \
		/users/${username}/data 1 rocksdb /users/${username}/bak $conf_json Raid0 1 > ${cur_path}/results/C.log
	fi
fi

cd $cur_path
if [ ! -e results/D.log ] || ! grep -q "total tput" results/D.log
then
	echo "generating results on YCSB-D"
  cd $mkfs_path
  sudo ./mkfs $conf_json Raid0
	if [[ $? == 0 ]]
  then
		sleep 1
		cd $app_path
		sudo ./test ../workloads/workloadd.spec $cli_num /users/${username}/data \
		/users/${username}/data 1 rocksdb /users/${username}/bak $conf_json Raid0 1 > ${cur_path}/results/D.log
	fi
fi

cd $cur_path
if [ ! -e results/F.log ] || ! grep -q "total tput" results/F.log
then
	echo "generating results on YCSB-F"
  cd $mkfs_path
  sudo ./mkfs $conf_json Raid0
	if [[ $? == 0 ]]
  then
		sleep 1
		cd $app_path
		sudo ./test ../workloads/workloadf.spec $cli_num /users/${username}/data \
		/users/${username}/data 1 rocksdb /users/${username}/bak $conf_json Raid0 1 > ${cur_path}/results/F.log
	fi
fi
