#!/bin/bash

io_depth=$1

mkdir -p results

let io_depth_per_job=$io_depth/4

if [[ "$io_depth" == "4" ]] || [[ "$io_depth" == "8" ]] || [[ "$io_depth" == "12" ]] || [[ "$io_depth" == "16" ]] || [[ "$io_depth" == "24" ]] || [[ "$io_depth" == "32" ]] || [[ "$io_depth" == "40" ]] || [[ "$io_depth" == "48" ]]
then
  sudo LD_PRELOAD=../../../../spdk/build/fio/spdk_bdev /usr/local/bin/fio -iodepth=$io_depth_per_job -rw=randrw -rwmixread=0 -bs=128k -numjobs=4 ../raid6_spdk.conf > results/${io_depth}.log
  exit 0
fi

echo "We do not have a pre-written command for this I/O depth. Feel free to run your own command."