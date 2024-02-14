# NASDAQ ITCH50 VWAP Analyzer
The repository contains a standalone C++ application that parses an ITCH50 file (decompressed) and generates a csv file per hour with VWAP (Volume Weighted Average Price) for each stock.
The ITCH50 file is memory mapped and the order info is stored in an ankerl::unordered_dense::map (ref_num -> {stock, price}).

## Links
* Hashmap benchmarks: https://martin.ankerl.com/2019/04/01/hashmap-benchmarks-01-overview/
* ankerl::unordered_dense: https://github.com/martinus/unordered_dense
* ITCH Specification: https://www.nasdaqtrader.com/content/technicalsupport/specifications/dataproducts/NQTVITCHspecification.pdf

## Dependencies
* CMake
* Boost

## Build
mkdir build

cd build

cmake ..

make

## Run
wget https://emi.nasdaq.com/ITCH/Nasdaq%20ITCH/01302019.NASDAQ_ITCH50.gz

gunzip -d 01302019.NASDAQ_ITCH50.gz

./ITCH50_Hourly_VWAP ./01302019.NASDAQ_ITCH50
