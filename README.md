# NASDAQ ITCH50 VWAP Analyzer
The repository contains a standalone C++ application that parses an
ITCH50 file (decompressed) and generates a csv file containing VWAP
(Volume Weighted Average Price) for each stock.

## Links
[![Hashmap benchmarks](https://martin.ankerl.com/2019/04/01/hashmap-benchmarks-01-overview/)]

[![NQTVITCH Specification](https://www.nasdaqtrader.com/content/technicalsupport/specifications/dataproducts/NQTVITCHspecification.pdf)]

## Build
mkdir build

cd build

cmake ..

make

## Run
wget https://emi.nasdaq.com/ITCH/Nasdaq%20ITCH/01302019.NASDAQ_ITCH50.gz

gunzip -d 01302019.NASDAQ_ITCH50.gz

./ITCH50_Hourly_VWAP ./01302019.NASDAQ_ITCH50
