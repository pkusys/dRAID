#!/bin/bash

username=`whoami`
conf_json=/users/${username}/artifacts/raid5_100g.json
cli_num=56

app_path=~/rocksdb/ycsb/build
cur_path=~/dRAID/YCSB/dRAID/fig20

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
  sudo timeout -k 5 180 ./test ../workloads/workloada.spec $cli_num /users/${username}/data \
   /users/${username}/data 1 rocksdb /users/${username}/bak $conf_json Raid0 1 > ${cur_path}/results/A.log
fi

cd $cur_path
if [ ! -e results/B.log ] || ! grep -q "total tput" results/B.log
then
	echo "generating results on YCSB-B"
  ../generate_raid_config.sh 512 8 1
  ../run_server_remote_nvme.sh $username raid5 100g 512 8 1
  sleep 6
  cd $app_path
  sudo timeout -k 5 180 ./test ../workloads/workloadb.spec $cli_num /users/${username}/data \
   /users/${username}/data 1 rocksdb /users/${username}/bak $conf_json Raid0 1 > ${cur_path}/results/B.log
fi

cd $cur_path
if [ ! -e results/C.log ] || ! grep -q "total tput" results/C.log
then
	echo "generating results on YCSB-C"
  ../generate_raid_config.sh 512 8 1
  ../run_server_remote_nvme.sh $username raid5 100g 512 8 1
  sleep 6
  cd $app_path
  sudo timeout -k 5 180 ./test ../workloads/workloadc.spec $cli_num /users/${username}/data \
   /users/${username}/data 1 rocksdb /users/${username}/bak $conf_json Raid0 1 > ${cur_path}/results/C.log
fi

cd $cur_path
if [ ! -e results/D.log ] || ! grep -q "total tput" results/D.log
then
	echo "generating results on YCSB-D"
  ../generate_raid_config.sh 512 8 1
  ../run_server_remote_nvme.sh $username raid5 100g 512 8 1
  sleep 6
  cd $app_path
  sudo timeout -k 5 180 ./test ../workloads/workloadd.spec $cli_num /users/${username}/data \
   /users/${username}/data 1 rocksdb /users/${username}/bak $conf_json Raid0 1 > ${cur_path}/results/D.log
fi

cd $cur_path
if [ ! -e results/F.log ] || ! grep -q "total tput" results/F.log
then
	echo "generating results on YCSB-F"
  ../generate_raid_config.sh 512 8 1
  ../run_server_remote_nvme.sh $username raid5 100g 512 8 1
  sleep 6
  cd $app_path
  sudo timeout -k 5 180 ./test ../workloads/workloadf.spec $cli_num /users/${username}/data \
   /users/${username}/data 1 rocksdb /users/${username}/bak $conf_json Raid0 1 > ${cur_path}/results/F.log
fi
