#!/bin/bash

## Runs on Docker image.
cd /goFish

## Test Go
cd goServer || exit
go test -v
cd .. || exit

## Test C++
cd tests || exit
mkdir -p build
cd build || exit
cmake .. && make

./run_tests