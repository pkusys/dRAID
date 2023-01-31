#!/bin/bash

username=`whoami`
raid_option=$1
network=$2
chunk=$3
raid_size=$4
num_qp=$5
hosts="$HOME/artifacts/hosts.txt"

let i=0

while read -u10 line
do
  if [[ $i -gt 0 ]] && [[ $i -le $raid_size ]]
  then
    echo "start $line"
    sudo ssh -tt "root@$line" "/users/$username/dRAID/dRAID/scripts/run_server_malloc.sh $username $raid_option $i $network $chunk $raid_size $num_qp"
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