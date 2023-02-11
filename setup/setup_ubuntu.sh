#!/bin/bash

# This is a script for Ubuntu

cd ~

# clone the artifacts
git clone https://github.com/pkusys/dRAID.git
# run the setup script
cd dRAID/setup
./configure_cloudlab_ubuntu.sh