cmake_minimum_required(VERSION 3.24)
project(digiasset_core)

option(BUILD_CLI "Build CLI" ON)
option(BUILD_WEB "Build Web Server" ON)
option(BUILD_TEST "Build Tests" OFF)


# Set compiler settings
set(CMAKE_CXX_STANDARD 11)
SET(CMAKE_CXX_FLAGS "-std=c++11 -g -Wall")

# Set version number
SET(MAJOR_VERSION 0)
SET(MINOR_VERSION 2)
SET(PATCH_VERSION 6)
SET(SO_VERSION    0)

# Add source directory
include_directories(src)
ADD_SUBDIRECTORY(src)

#create cli
if(BUILD_CLI)
    include_directories(cli)
    ADD_SUBDIRECTORY(cli)
endif()

#create web
if(BUILD_WEB)
    include_directories(web)
    ADD_SUBDIRECTORY(web)
endif()

#build test script
if(BUILD_TEST)
    set(GOOGLETEST_VERSION 1.11.0)
    ADD_SUBDIRECTORY(tests)
endif()