#!/bin/bash

username=$1

sudo kill -9 $(ps aux | grep '[r]aid' | awk '{print $2}')
sudo kill -9 $(ps aux | grep '[n]vmf_tgt' | awk '{print $2}')
cd /users/$username/draid-spdk
sudo ./scripts/setup.sh reset