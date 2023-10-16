cmake_minimum_required(VERSION 3.21)
project(digiasset_core-cli)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
set(CMAKE_CXX_STANDARD 11)
FIND_PACKAGE(JSONCPP REQUIRED)
FIND_PACKAGE(JSONRPCCPP REQUIRED)
find_package(CURL REQUIRED)

add_library(llibcurl STATIC IMPORTED)


set(HEADER_FILES
        ../src/Config.h
        ../src/DigiByteCore.cpp
        )

set(SOURCE_FILES
        ../src/Config.cpp
        ../src/DigiByteCore.cpp
        )

add_executable(digiasset_core-cli main.cpp ${SOURCE_FILES} ${HEADER_FILES})




target_link_libraries(digiasset_core-cli PRIVATE CURL::libcurl)
target_link_libraries(digiasset_core-cli PRIVATE ssl)
target_link_libraries(digiasset_core-cli PRIVATE pthread)

target_link_libraries(digiasset_core-cli PRIVATE /lib/x86_64-linux-gnu/libjsoncpp.so)
target_link_libraries(digiasset_core-cli PRIVATE jsonrpccpp-common)
target_link_libraries(digiasset_core-cli PRIVATE jsonrpccpp-client)


install(TARGETS digiasset_core-cli DESTINATION bin)