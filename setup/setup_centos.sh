#!/bin/bash

# This is a script for CentOS 8

cd ~

# clone the artifacts
git clone https://github.com/pkusys/dRAID.git
# run the setup script
cd dRAID/setup
sh -c "nohup ./configure_cloudlab_centos.sh > /dev/null 2>&1 &"