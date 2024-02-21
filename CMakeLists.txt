cmake_minimum_required(VERSION 3.24)
project(digiasset_core)


# Set compiler settings
set(CMAKE_CXX_STANDARD 11)
SET(CMAKE_CXX_FLAGS "-std=c++11 -g -Wall")

# Set version number
SET(MAJOR_VERSION 0)
SET(MINOR_VERSION 2)
SET(PATCH_VERSION 1)
SET(SO_VERSION    5)

# Add source directory
include_directories(src)
ADD_SUBDIRECTORY(src)

#create cli
include_directories(cli)
ADD_SUBDIRECTORY(cli)

set(GOOGLETEST_VERSION 1.11.0)
ADD_SUBDIRECTORY(tests)