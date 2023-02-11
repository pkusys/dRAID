#!/bin/bash

# This is a script for CentOS 8

cd ~

# clone the artifacts
git clone https://github.com/pkusys/dRAID.git
# run the setup script
cd dRAID/setup
./configure_cloudlab_centos.sh &