cmake_minimum_required(VERSION 3.5.1)
project(test_fish)

# compile flags and options
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "-O2 -Wall")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)


# OpenCV
FIND_PACKAGE( OpenCV 4.0.0 REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} )

# CppUnit
FIND_PACKAGE(CppUnit REQUIRED)
include_directories( ${CPPUNIT_INCLUDE_DIR} )

file(GLOB INC_SRC
    "../findFish/resources/includes/*.h"
    "../findFish/resources/*.cc"
    "headers/*.h"
    "*.cc"
)

include_directories(
    "../findFish/resources/includes/"
    "headers/"
    )

# findFish executable 
add_executable( run_tests ${INC_SRC} )
target_link_libraries( run_tests ${OpenCV_LIBS} ${CPPUNIT_LIBRARIES})
