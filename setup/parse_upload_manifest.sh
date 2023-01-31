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
echo "$line uploaded!"

while read -u10 line
do
  echo "scp manifests to $line"
  ssh -tt "$username@$line" "mkdir ~/artifacts"
  scp hosts.txt "$username@$line:~/artifacts/"
  scp ip_addrs_100g.txt "$username@$line:~/artifacts/"
  scp ip_addrs_25g.txt "$username@$line:~/artifacts/"
  scp "$manifest" "$username@$line:~/artifacts/"
  echo "$line uploaded!"
done 10< hosts.txt