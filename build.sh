#!/bin/bash

## Runs on Docker image.
cd /goFish

## Build Go
cd goServer || exit
go build
cd .. || exit

## Build C++
cd findFish || exit
mkdir -p build
cd build || exit
cmake .. && make
cd ../.. || exit

cd /goFish
ln -sfn /goFish/goServer/goFish /goFish/GoFish
ln -sfn /goFish/findFish/findFish /goFish/FishFinder
