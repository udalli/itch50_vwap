#!/bin/bash

set -x

rm -rf build
mkdir build && cd build && cmake .. && make all && valgrind --tool=callgrind ./ITCH50_Hourly_VWAP ../01302019.NASDAQ_ITCH50
perf -e cycles,instructions ./ITCH50_Hourly_VWAP ../01302019.NASDAQ_ITCH50
