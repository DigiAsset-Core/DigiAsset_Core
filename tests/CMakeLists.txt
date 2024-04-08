# 'Google_test' is the subproject name
project(Google_tests)

# 'lib' is the folder with Google Test sources
add_subdirectory(lib/googletest)
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)
set(CMAKE_CXX_STANDARD 11)


set(TESTS
        BitIO.cpp
        DatabaseChain.cpp
        DigiByteCore.cpp
        IPFS.cpp
        Base58Tests.cpp
        DigiAssetTest.cpp
        DigiAssetRulesTest.cpp
        DigiAssetTransactionTest.cpp
        TestHelpers.cpp TestHelpers.h RPCMethods.cpp RPCMethods.h)

#load RPC tests
file(GLOB RPC_METHOD_TEST_SOURCES RPC_Methods/*.cpp)
foreach(SOURCE_FILE ${RPC_METHOD_TEST_SOURCES})
    list(APPEND TESTS ${SOURCE_FILE})
endforeach()

# 'Google_Tests_run' is the target name
# 'test1.cpp tests2.cpp' are source files with tests
add_executable(Google_Tests_run ${TESTS})   #digibyteapi
target_link_libraries(Google_Tests_run digiasset_core_lib sqlite3 curl)
target_link_libraries(Google_Tests_run gtest gtest_main)

target_link_libraries(Google_Tests_run /lib/x86_64-linux-gnu/libjsoncpp.so jsonrpccpp-common jsonrpccpp-client)
