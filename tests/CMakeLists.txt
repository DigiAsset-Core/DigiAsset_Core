# 'Google_test' is the subproject name
project(Google_tests)

# 'lib' is the folder with Google Test sources
add_subdirectory(lib/googletest)
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
set(CMAKE_CXX_STANDARD 14)

# Ensure testFiles directory exists (not tracked by git, needed by tests at runtime)
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/testFiles)  # googletest 1.14+ requires C++14 minimum; C++17 breaks system libjsonrpccpp headers

find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)
find_package(jsoncpp CONFIG QUIET)
find_package(JSONCPP QUIET)

set(TESTS
        AppMainTest.cpp
        BitIO.cpp
        DigiByteDomainTest.cpp
        LocalPoolTest.cpp
        DatabaseChain.cpp
        DigiByteCore.cpp
        IPFS.cpp
        Base58Tests.cpp
        DigiAssetTest.cpp
        DigiAssetRulesTest.cpp
        AssetWalletRulesTest.cpp
        DigiAssetTransactionTest.cpp
        DigiByteTransactionBuilderTest.cpp
        DigiAssetRulesEncodeTest.cpp
        PermanentStoragePoolTest.cpp
        PermanentStoragePoolListTest.cpp
        ConfigTest.cpp
        EventBroadcasterTest.cpp
        KYCTest.cpp
        BlobTest.cpp
        UtilsTest.cpp
        SHA256Test.cpp
        DigiAssetTypesTest.cpp
        UniqueTaskQueueTest.cpp
        ThreadedTest.cpp
        LogTest.cpp
        InstanceLockTest.cpp
        RPC_ResponseTest.cpp
        RPC_CacheTest.cpp
        RPC_ServerTest.cpp
        DigiByteExceptionTest.cpp
        Database_StatementTest.cpp
        DatabaseTest.cpp
        ChainAnalyzerTest.cpp
        RPC_MethodListTest.cpp
        CurlHandlerTest.cpp
        PermanentStoragePoolMetaProcessorTest.cpp
        TestHelpers.cpp TestHelpers.h RPCMethods.cpp RPCMethods.h)

#load RPC tests
file(GLOB RPC_METHOD_TEST_SOURCES RPC_Methods/*.cpp)
foreach(SOURCE_FILE ${RPC_METHOD_TEST_SOURCES})
    list(APPEND TESTS ${SOURCE_FILE})
endforeach()

# 'Google_Tests_run' is the target name
# 'test1.cpp tests2.cpp' are source files with tests
add_executable(Google_Tests_run ${TESTS})   #digibyteapi
find_package(SQLite3 QUIET)
find_package(CURL REQUIRED)
target_link_libraries(Google_Tests_run digiasset_core_lib CURL::libcurl)
if(TARGET SQLite::SQLite3)
    target_link_libraries(Google_Tests_run SQLite::SQLite3)
else()
    target_link_libraries(Google_Tests_run sqlite3)
endif()
target_link_libraries(Google_Tests_run gtest gtest_main)
target_link_libraries(Google_Tests_run OpenSSL::SSL OpenSSL::Crypto Threads::Threads)
if(TARGET jsoncpp_lib)
    target_link_libraries(Google_Tests_run jsoncpp_lib)
elseif(TARGET jsoncpp_object)
    target_link_libraries(Google_Tests_run jsoncpp_object)
else()
    target_link_libraries(Google_Tests_run ${JSONCPP_LIBRARIES})
endif()
find_package(libjson-rpc-cpp CONFIG QUIET)
if(TARGET libjson-rpc-cpp::jsonrpccpp-common)
    target_link_libraries(Google_Tests_run libjson-rpc-cpp::jsonrpccpp-common libjson-rpc-cpp::jsonrpccpp-client)
else()
    target_link_libraries(Google_Tests_run jsonrpccpp-common jsonrpccpp-client)
endif()
if(WIN32)
    target_link_libraries(Google_Tests_run ws2_32 mswsock bcrypt)
endif()
