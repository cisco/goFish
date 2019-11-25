# !/bin/bash

## Runs on Docker image.
cd /goFish

## Build Go
cd goServer || exit
go build
cd .. || exit

## Build C++
cd findFish || exit
mkdir -p build || exit
cd build || exit
cmake .. && make
cd ../.. || exit

cd /goFish
ln -sfn ./goServer/goServer /goFish/GoFish
ln -sfn ./findFish/build/findFish /goFish/FishFinder