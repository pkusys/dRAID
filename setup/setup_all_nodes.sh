#!/bin/bash

manifest=$1
username=$2

# generate host names and ip addresses from manifest
line=`python3 parse_manifest.py ${manifest} hosts.txt ip_addrs_100g.txt ip_addrs_25g.txt`

echo "scp manifests to $line"
ssh -tt "$username@$line" "mkdir ~/artifacts"
scp hosts.txt "$username@$line:~/artifacts/"
scp ip_addrs_100g.txt "$username@$line:~/artifacts/"
scp ip_addrs_25g.txt "$username@$line:~/artifacts/"
scp "$manifest" "$username@$line:~/artifacts/"
ssh -tt "$username@$line" "git clone https://github.com/pkusys/dRAID.git"
ssh -tt "$username@$line" "dRAID/setup/configure_cloudlab_ubuntu.sh"
echo "$line is ready!"

while read -u10 line
do
  echo "scp manifests to $line"
  ssh -tt "$username@$line" "mkdir ~/artifacts"
  scp hosts.txt "$username@$line:~/artifacts/"
  scp ip_addrs_100g.txt "$username@$line:~/artifacts/"
  scp ip_addrs_25g.txt "$username@$line:~/artifacts/"
  scp "$manifest" "$username@$line:~/artifacts/"
  ssh -tt "$username@$line" "git clone https://github.com/pkusys/dRAID.git"
  ssh "$username@$line" "nohup dRAID/setup/configure_cloudlab_centos.sh > foo.out 2> foo.err < /dev/null &"
  echo "Uploaded to $line!"
done 10< hosts.txt

while read -u10 line
do
  while ssh "$username@$line" '[ ! -d ~/rocksdb ]'
  do
    echo "Setup is ongoing on $line"
    sleep 10
  done
  echo "$line is ready!"
done 10< hosts.txt

echo "You are all set!"