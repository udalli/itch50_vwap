#!/bin/bash

if [ -z "$1" ]
then
  echo "Missing arguments, usage: 'benchmark <NASDAQ_ITCH50 file (unzipped)>'"
  echo "Example: ${0} 01302019.NASDAQ_ITCH50"
exit 1
fi

ITCH50_FILE_PATH=`realpath ${1}`
echo "Benchmarking ${ITCH50_FILE_PATH}"
  
set -x

rm -rf build
mkdir build && cd build && cmake .. && make all 
/usr/bin/time -v ./ITCH50_Hourly_VWAP ${ITCH50_FILE_PATH} &> "${ITCH50_FILE_PATH}.time"
perf record -o "${ITCH50_FILE_PATH}.perf.data" -e cache-references,cache-misses,cycles,instructions,branches,faults,migrations ./ITCH50_Hourly_VWAP "${ITCH50_FILE_PATH}"
valgrind --tool=callgrind --collect-systime=msec --callgrind-out-file="${ITCH50_FILE_PATH}.callgrind.out" ./ITCH50_Hourly_VWAP "${ITCH50_FILE_PATH}"
