cmake_minimum_required(VERSION 3.5.1)
project(findFish)

# compile flags and options
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# OpenCV
find_package( OpenCV 4.0.0 REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} )

# findFish executable 
add_executable( findFish findFish.cc )
target_link_libraries( findFish ${OpenCV_LIBS} )
