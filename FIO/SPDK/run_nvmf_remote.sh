#!/bin/bash

username=`whoami`
interface=$1

hosts="$HOME/artifacts/hosts.txt"

let i=0

while read -u10 line
do
  if [[ $i -gt 0 ]]
  then
    echo "start $line"
    sudo ssh -tt "root@$line" "/users/$username/dRAID/FIO/SPDK/run_nvmf.sh $username $interface"
    echo "$line is READY!"
  else
    echo "skipping $line"
  fi
  let i+=1
done 10< $hosts