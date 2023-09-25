//
// Created by mctrivia on 27/05/23.
//

#include <cmath>
#include "gtest/gtest.h"
#include "DigiByteCore.h"
#include "Config.h"

using namespace std;


TEST(DigiByteCore, MakeConnection) {
    ///This will only pass if _DigiByteCore has the correct settings.  Following test will check this and output warning

    //try connecting with valid config
    DigiByteCore test;
    test.setFileName("config.cfg");
    bool failed = false;
    try {
        test.makeConnection();
    } catch (const Config::exceptionConfigFileInvalid& e) {
        failed = true;
        cout << "\n\n";
        cout << "***********************************************************\n";
        cout << "* Test failed because bin/config.cfg is not set correctly *\n";
        cout << "* for this wallet. Please fix and re run tests.           *\n";
        cout << "***********************************************************\n\n";
    } catch (const Config::exceptionCorruptConfigFile& e) {
        failed = true;
        cout << "\n\n";
        cout << "*******************************************************************************\n";
        cout << "* Test failed because bin/config.cfg is corrupt. Please fix and re run tests. *\n";
        cout << "*******************************************************************************\n\n";
    } catch (const Config::exceptionConfigFileMissing& e) {
        failed = true;
        cout << "\n\n";
        cout << "*******************************************************************************\n";
        cout << "* Test failed because bin/config.cfg is missing. Please fix and re run tests. *\n";
        cout << "*******************************************************************************\n\n";
    } catch (const DigiByteCore::exceptionCoreOffline& e) {
        failed = true;
        cout << "\n\n";
        cout << "****************************************************************************\n";
        cout << "* Test failed because Core Wallet is offline. Please fix and re run tests. *\n";
        cout << "****************************************************************************\n\n";
    } catch (const std::exception& e) {
        failed = true;
    }
    EXPECT_GT(test.getBlockCount(), 1);
    EXPECT_FALSE(failed);

    //change to missing config file
    test.setFileName("../tests/testFiles/DigiByteCore_bad0.cfg");
    failed = true;
    try {
        test.makeConnection();
    } catch (const Config::exceptionConfigFileMissing& e) {
        failed = false;   //want this exception
    } catch (const std::exception& e) {
        failed = true;
    }
    EXPECT_FALSE(failed);

    //change to bad config file
    test.setFileName("../tests/testFiles/DigiByteCore_bad1.cfg");
    failed = true;
    try {
        test.makeConnection();
    } catch (const Config::exceptionConfigFileInvalid& e) {
        failed = false;   //want this exception
    } catch (const std::exception& e) {
        failed = true;
    }
    EXPECT_FALSE(failed);

    //change to corrupt config file
    cout << "83\n";
    test.setFileName("../tests/testFiles/DigiByteCore_bad2.cfg");
    failed = true;
    try {
        cout << "88\n";
        test.makeConnection();
    } catch (const Config::exceptionCorruptConfigFile& e) {
        failed = false;   //want this exception
    } catch (const std::exception& e) {
        failed = true;
    }
    EXPECT_FALSE(failed);
}

TEST(DigiByteCore, GetBlockCount) {
    DigiByteCore test;
    cout << "100\n";
    test.setFileName("config.cfg");
    cout << "102\n";
    test.makeConnection();
    EXPECT_GT(test.getBlockCount(), 1);
}

TEST(DigiByteCore, GetBlockHash) {
    //test a block that should be synced
    DigiByteCore test;
    cout << "110\n";
    test.setFileName("config.cfg");
    cout << "112\n";
    test.makeConnection();
    EXPECT_EQ(test.getBlockHash(1), "4da631f2ac1bed857bd968c67c913978274d8aabed64ab2bcebc1665d7f4d3a0");

    //test a block that probably does not exist yet
    bool failed = true;
    try {
        test.getBlockHash(1000000000);
    } catch (const exception& e) {
        failed = false;   //want this exception
    } catch (const std::exception& e) {
        failed = true;
    }
    EXPECT_FALSE(failed);
}