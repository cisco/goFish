## Build Go
cd goServer || exit
go build
cd .. || exit

## Build C++
cd findFish || exit
mkdir build
cd build || exit
cmake .. && make && cd ../..

cd CameraCalibration || exit
mkdir build
cd build || exit
cmake .. && make && cd ../..