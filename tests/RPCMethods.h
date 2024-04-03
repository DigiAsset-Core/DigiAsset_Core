//
// Created by mctrivia on 31/03/24.
//
#ifndef RPC_METHODS_TEST_H
#define RPC_METHODS_TEST_H

#include "gtest/gtest.h"
#include "AppMain.h"

class RPCMethodsTest : public ::testing::Test {
protected:
    static void SetUpTestSuite();
    static void TearDownTestSuite();

    static AppMain* appMain;
    static DigiByteCore* dgb;
    static Database* db;
    static IPFS* ipfs;
    static PermanentStoragePoolList* psp;
    static ChainAnalyzer* analyzer;
};

#endif // RPC_METHODS_TEST_H
