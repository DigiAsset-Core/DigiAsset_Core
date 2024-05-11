cmake_minimum_required(VERSION 3.21)
project(digiasset_core-web)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
set(CMAKE_CXX_STANDARD 17)


set(HEADER_FILES
        ../src/Config.h
        )

set(SOURCE_FILES
        ../src/Config.cpp
        )

add_executable(digiasset_core-web main.cpp ${SOURCE_FILES} ${HEADER_FILES})

install(TARGETS digiasset_core-web DESTINATION bin)