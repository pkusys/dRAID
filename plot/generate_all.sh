#!/bin/bash

declare -a fio_arr=("9" "10" "11" "12" "13" "14a" "14b" "15" "16" "18" "22" "23" "24" "25" "26" "27a" "27b" "28" "29" "30")
declare -a ycsb_arr=("19a" "19b" "20" "21")

for i in "${fio_arr[@]}"
do
  python3 python_scripts/plot_fig${i}.py ../FIO/dRAID/fig${i}/results ../FIO/SPDK/fig${i}/results ../FIO/Linux/fig${i}/results
done

for i in "${ycsb_arr[@]}"
do
  python3 python_scripts/plot_fig${i}.py ../YCSB/dRAID/fig${i}/results ../YCSB/SPDK/fig${i}/results
done