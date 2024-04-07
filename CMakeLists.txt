cmake_minimum_required(VERSION 3.24)
project(digiasset_core)

option(BUILD_CLI "Build CLI" ON)
option(BUILD_TEST "Build everything" OFF)


# Set compiler settings
set(CMAKE_CXX_STANDARD 11)
SET(CMAKE_CXX_FLAGS "-std=c++11 -g -Wall")

# Set version number
SET(MAJOR_VERSION 0)
SET(MINOR_VERSION 2)
SET(PATCH_VERSION 4)
SET(SO_VERSION    10)

# Add source directory
include_directories(src)
ADD_SUBDIRECTORY(src)

#create cli
if(BUILD_CLI)
    include_directories(cli)
    ADD_SUBDIRECTORY(cli)
endif()

#build test script
if(BUILD_TEST)
    set(GOOGLETEST_VERSION 1.11.0)
    ADD_SUBDIRECTORY(tests)
endif()