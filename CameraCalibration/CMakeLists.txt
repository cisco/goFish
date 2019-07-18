cmake_minimum_required(VERSION 3.5.1)
project(calibrate)

# compile flags and options
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "-O2 -Wall")

# OpenCV
find_package( OpenCV 4.0.0 REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} )

file(GLOB INC_SRC
    "src/includes/*.h"
    "src/*.cc"
)

# calibrate executable 
add_executable( calibrate ${INC_SRC} )
target_link_libraries( calibrate ${OpenCV_LIBS} )
