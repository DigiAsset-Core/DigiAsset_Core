//
// Created by mctrivia on 27/05/23.
//

#include "DigiByteCore.h"
#include "Config.h"
#include "gtest/gtest.h"
#include <cmath>

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
        failed = false; //want this exception
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
        failed = false; //want this exception
    } catch (const std::exception& e) {
        failed = true;
    }
    EXPECT_FALSE(failed);

    //change to corrupt config file
    test.setFileName("../tests/testFiles/DigiByteCore_bad2.cfg");
    failed = true;
    try {
        test.makeConnection();
    } catch (const Config::exceptionCorruptConfigFile& e) {
        failed = false; //want this exception
    } catch (const std::exception& e) {
        failed = true;
    }
    EXPECT_FALSE(failed);
}

TEST(DigiByteCore, WalletSelection) {
    DigiByteCore test;
    test.setFileName("config.cfg");
    test.makeConnection();

    //with one wallet loaded we name it explicitly rather than leaving core to guess, and with
    //several the operator has to say which in config.cfg.  Either way plain node commands must
    //still work, since once a wallet is picked every command goes to the /wallet/<name> endpoint
    EXPECT_GT(test.getBlockCount(), 1);

    const std::string wallet = test.getWalletName();
    if (!wallet.empty()) {
        Json::Value params = Json::arrayValue;
        Json::Value loaded = test.sendcommand("listwallets", params);
        bool found = false;
        for (const Json::Value& name: loaded) {
            if (name.asString() == wallet) found = true;
        }
        EXPECT_TRUE(found);
    }
}

TEST(DigiByteCore, GetBlockCount) {
    DigiByteCore test;
    test.setFileName("config.cfg");
    test.makeConnection();
    EXPECT_GT(test.getBlockCount(), 1);
}

TEST(DigiByteCore, GetBlockHash) {
    //test a block that should be synced
    DigiByteCore test;
    test.setFileName("config.cfg");
    test.makeConnection();
    EXPECT_EQ(test.getBlockHash(1), "4da631f2ac1bed857bd968c67c913978274d8aabed64ab2bcebc1665d7f4d3a0");

    //test a block that probably does not exist yet.  The node answers with an error of its own, so
    //that error has to reach us - it used to be replaced with "Core Offline", which sends whoever
    //is reading the message off looking at connectivity while the node is perfectly healthy
    bool failed = true;
    try {
        test.getBlockHash(1000000000);
    } catch (const DigiByteCore::exceptionCoreOffline& e) {
        failed = true; //the node was not offline, it gave us an answer
    } catch (const DigiByteException& e) {
        failed = false; //want this exception
        EXPECT_FALSE(e.getMessage().empty());
    } catch (const std::exception& e) {
        failed = true;
    }
    EXPECT_FALSE(failed);
}