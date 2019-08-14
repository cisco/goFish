#!/bin/bash

OPENCV_VERSION=4.1.0

## Linux
if [[ "$OSTYPE" == "linux-gnu" ]]; then
    apt-get update
    #apt-get upgrade -y
## Mac OSX
elif [[ "$OSTYPE" == "darwin"* ]]; then
    brew update
fi

## Install ancillary dependencies
if [[ $1 = "FULL" ]]; then
    ## Linux
    if [[ "$OSTYPE" == "linux-gnu" ]]; then
         apt-get -y install cmake git gcc g++ golang-go python3 libcppunit-dev
    ## Mac OSX
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        brew install cmake
    fi
fi

## Install Opencv 4.1
if [[ $1 = "FULL" ]] || [[ $1 = "OPENCV" ]]; then
    if [[ "$OSTYPE" == "linux-gnu" ]]; then
         apt-get -y install build-essential
         apt-get -y install cmake git libgtk2.0-dev pkg-config libavcodec-dev libavformat-dev libswscale-dev
    fi
    ## Get OpenCV git repos.
    cd ~ || exit
    git clone --depth 1 https://github.com/opencv/opencv.git
    git clone --depth 1 https://github.com/opencv/opencv_contrib.git
    
    ## Sync up the OpenCV repos.
    cd ~/opencv_contrib || exit
    git checkout $OPENCV_VERSION
    cd ~/opencv || exit
    git checkout $OPENCV_VERSION
    mkdir build
    cd build || exit
    
    ## Make and install OpenCV.
    cmake -D CMAKE_BUILD_TYPE=RELEASE \
        -D CMAKE_INSTALL_PREFIX=/usr/local \
        -D OPENCV_GENERATE_PKGCONFIG=YES \
        -D OPENCV_EXTRA_MODULES_PATH=~/opencv_contrib/modules \
        -D BUILD_opencv_java=OFF \
        -D BUILD_opencv_python=OFF \
        -D WITH_FFMPEG=1 \
        -D WITH_TBB=OFF \
        -D BUILD_opencv_aruco=OFF\
        -D BUILD_opencv_bioinspired=OFF\
        -D BUILD_opencv_datasets=OFF\
        -D BUILD_opencv_dnns_easily_fooled=OFF\
        -D BUILD_opencv_dpm=OFF\
        -D BUILD_opencv_saliency=OFF\
        -D BUILD_opencv_matlab=OFF\
        -D BUILD_opencv_fuzzy=OFF\
        -D BUILD_opencv_hdf=OFF\
        -D BUILD_opencv_tracking=ON\
        -D BUILD_opencv_xfeatures2d=ON\
        -D BUILD_opencv_ximgproc=ON\
        -D BUILD_opencv_xobjdetect=ON\
        ..
    make -j7
    make install
    
    ## Remove the OpenCV git repos to save on space.
    cd ~ || exit
    rm -r ~/opencv*
fi

## Go to the project.
#cd ~/goFish || exit

#if [[ $1 = "FULL" ]]; then
#    cd static/ || exit
#    mkdir -p temp
#    mkdir -p videos
#    mkdir -p video-info
#    mkdir -p proc_videos
#    mkdir -p calibrate
#    cd ../ || exit
#fi

