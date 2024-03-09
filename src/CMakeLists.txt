cmake_minimum_required(VERSION 3.21)
project(digiasset_core)

#Build Version.h
configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/Version.h.in"
        "${CMAKE_CURRENT_SOURCE_DIR}/Version.h"
)


set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
set(CMAKE_CXX_STANDARD 11)
FIND_PACKAGE(JSONCPP REQUIRED)
FIND_PACKAGE(JSONRPCCPP REQUIRED)
find_package(CURL REQUIRED)

#add_library(ldigibyteapi STATIC IMPORTED)
add_library(lsqlite3 STATIC IMPORTED)
add_library(llibcryptopp STATIC IMPORTED)
add_library(llibcurl STATIC IMPORTED)

#find_package(cryptopp CONFIG REQUIRED)


set(HEADER_FILES
        static_block.hpp
        Threaded.h
        ChainAnalyzer.h
        Database.h
        DigiByteCore.h
        Blob.h
        BitIO.h
        KYC.h
        DigiAsset.h
        DigiAssetRules.h
        IPFS.h
        DigiByteTransaction.h
        Base58.h
        serialize.h
        DigiAssetTypes.h
        DigiByteDomain.h
        Config.h
        BitcoinRpcServer.h
        DigiByteCore_Types.h
        DigiByteCore_Exception.h
        Log.h
        utils.h
        PermanentStoragePool/PermanentStoragePoolMetaProcessor.h
        PermanentStoragePool/PermanentStoragePool.h
        PermanentStoragePool/PermanentStoragePoolList.h
        PermanentStoragePool/pools/mctrivia.h
        PermanentStoragePool/pools/local.h
        AppMain.h
        CurlHandler.h
        Version.h
        OldStream.h
        UniqueTaskQueue.h
        )

set(SOURCE_FILES
        Threaded.cpp
        ChainAnalyzer.cpp
        Database.cpp
        DigiByteCore.cpp
        Blob.cpp
        BitIO.cpp
        KYC.cpp
        DigiAsset.cpp
        DigiAssetRules.cpp
        IPFS.cpp
        DigiByteTransaction.cpp
        Base58.cpp
        DigiAssetTypes.cpp
        DigiByteDomain.cpp
        Config.cpp
        BitcoinRpcServer.cpp
        Log.cpp
        utils.cpp
        PermanentStoragePool/PermanentStoragePoolMetaProcessor.cpp
        PermanentStoragePool/PermanentStoragePool.cpp
        PermanentStoragePool/PermanentStoragePoolList.cpp
        PermanentStoragePool/pools/mctrivia.cpp
        PermanentStoragePool/pools/local.cpp
        AppMain.cpp
        CurlHandler.cpp
        OldStream.cpp
        UniqueTaskQueue.cpp
        )

add_executable(digiasset_core main.cpp ${SOURCE_FILES} ${HEADER_FILES})


target_link_libraries(digiasset_core PRIVATE CURL::libcurl)
#target_link_libraries(digiasset_core PUBLIC digibyteapi)
target_link_libraries(digiasset_core PUBLIC sqlite3)
target_link_libraries(digiasset_core PRIVATE cryptopp)
target_link_libraries(digiasset_core PRIVATE ssl)
target_link_libraries(digiasset_core PRIVATE crypto)
target_link_libraries(digiasset_core PRIVATE pthread)

target_link_libraries(digiasset_core PRIVATE /lib/x86_64-linux-gnu/libjsoncpp.so)
target_link_libraries(digiasset_core PRIVATE jsonrpccpp-common)
target_link_libraries(digiasset_core PRIVATE jsonrpccpp-client)
#target_link_libraries(digiasset_core PRIVATE cryptopp::cryptopp)


install(TARGETS digiasset_core DESTINATION bin)
add_library(digiasset_core_lib STATIC ${SOURCE_FILES} ${HEADER_FILES})