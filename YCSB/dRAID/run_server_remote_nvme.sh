#!/bin/bash

username=$1
raid_option=$2
network=$3
chunk=$4
raid_size=$5
num_qp=$6
hosts="$HOME/artifacts/hosts.txt"

let i=0

while read -u10 line
do
  if [[ $i -gt 0 ]] && [[ $i -le $raid_size ]]
  then
    echo "start $line"
    sudo ssh -tt "root@$line" "/users/$username/dRAID/dRAID/scripts/run_server_nvme.sh $username $raid_option $i $network $chunk $raid_size $num_qp"
    echo "$line is READY!"
  else
    if [[ $i -eq 0 ]]
    then
      echo "skipping $line"
    else
      echo "kill $line"
      sudo ssh -tt "root@$line" "/users/$username/dRAID/dRAID/scripts/kill_server.sh $username"
    fi
  fi
  let i+=1
done 10< $hosts