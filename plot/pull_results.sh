#!/bin/bash

username=$1

cd ../setup
node19=`python3 parse_manifest.py manifest.xml hosts.txt ip_addrs_100g.txt ip_addrs_25g.txt`
node0=$(head -n 1 hosts.txt)
cd -
declare -a fio_arr=("9" "10" "11" "12" "13" "14a" "14b" "15" "16" "18" "22" "23" "24" "25" "26" "27a" "27b" "28" "29" "30")
declare -a ycsb_arr=("19a" "19b" "20" "21")

# pull dRAID FIO results from node0
for i in "${fio_arr[@]}"
do
  scp -r "$username@$node0:~/dRAID/FIO/dRAID/fig${i}/results" "../FIO/dRAID/fig${i}/"
done
# pull SPDK FIO results from node0
for i in "${fio_arr[@]}"
do
  scp -r "$username@$node0:~/dRAID/FIO/SPDK/fig${i}/results" "../FIO/SPDK/fig${i}/"
done
# pull Linux FIO results from node19
for i in "${fio_arr[@]}"
do
  scp -r "$username@$node19:~/dRAID/FIO/Linux/fig${i}/results" "../FIO/Linux/fig${i}/"
done
# pull dRAID YCSB results from node0
for i in "${ycsb_arr[@]}"
do
  scp -r "$username@$node0:~/dRAID/YCSB/dRAID/fig${i}/results" "../YCSB/dRAID/fig${i}/"
done
# pull SPDK YCSB results from node0
for i in "${ycsb_arr[@]}"
do
  scp -r "$username@$node0:~/dRAID/YCSB/SPDK/fig${i}/results" "../YCSB/SPDK/fig${i}/"
done
