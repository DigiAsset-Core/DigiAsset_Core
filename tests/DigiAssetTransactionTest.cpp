//
// Created by mctrivia on 07/07/23.
// This test sweet tests all historical DigiAsset, KYC, and Exchange transactions up to block todo ???????
//
// ============================================================
// WARNING: THIS FILE HAS DOWNSTREAM TEST DEPENDENCIES
// ============================================================
// This test produces testFiles/rpcTest.db, which is required
// by the following test suites:
//
//   - RPCMethods         (tests/RPCMethods.cpp)
//   - PermanentStoragePool.mctrivia_allAddressesRecognized
//                        (tests/PermanentStoragePoolTest.cpp)
//
// If this test is disabled or skipped (e.g. no IPFS connection),
// those suites will FAIL with a clear error message rather than
// silently skip.  This is intentional — do not add GTEST_SKIP()
// to suppress those failures without also disabling this test.
//
// To run the full test suite with all dependents passing:
//   1. Ensure an IPFS node is reachable (config.cfg)
//   2. Ensure a DigiByte Core node is reachable (config.cfg)
//   3. Run this test first (suite name: DigiAssetTransaction)
// ============================================================

#include "AppMain.h"
#include "DigiAsset.h"
#include "DigiAssetRules.h"
#include "DigiByteCore.h"
#include "DigiByteTransaction.h"
#include "IPFS.h"
#include "Log.h"
#include "TestHelpers.h"
#include "gtest/gtest.h"
#include "utils.h"
#include <cmath>

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <vector>

using namespace std;



TEST(DigiAssetTransaction, existingAssetTransactions) {
    Log* log = Log::GetInstance("debug.log");
    log->setMinLevelToFile(Log::INFO);

    const bool showAll = false; ///set true if debugging and want it to show the txid before doing each test
    string errorList = "";

    //delete old test databases if still exists (including WAL/SHM to avoid SQLite confusion)
    //rpcTest.db is also cleaned up here in case a previous run crashed before TearDownTestSuite could remove it
    try {
        remove("../tests/testFiles/assetTest.db");
        remove("../tests/testFiles/assetTest.db-wal");
        remove("../tests/testFiles/assetTest.db-shm");
        remove("../tests/testFiles/rpcTest.db");
        remove("../tests/testFiles/rpcTest.db-wal");
        remove("../tests/testFiles/rpcTest.db-shm");
    } catch (...) {}

    IPFS ipfs("config.cfg", false);

    //Both fixtures are fetched with pinAlso set, so every machine that runs this suite keeps a copy
    //and becomes another source for the next one.  That only helps while at least one node still has
    //the file: these are ordinary ipfs content, nothing republishes them automatically, so if every
    //holder goes offline the cid stops resolving and no amount of pinning here brings it back.
    //When that happens say which file is missing - the whole of RPCMethodsTest is built from
    //assetTest.db, so a generic "ipfs timed out" turns into 40 unexplained failures later in the run
    struct Fixture {
        const char* cid;
        const char* path;
    };
    const Fixture fixtures[]{
            {"QmNPyr5tkm48cUu5iMbReiM8GN8AW6PRpzUztPFadaxC8j", "../tests/testFiles/assetTest.csv"},
            {"QmUXQ2SMCvNAL4THgMm2g5vM4t6dBzj78ArnW9YBFmk81m", "../tests/testFiles/assetTest.db"},
    };
    for (const auto& fixture: fixtures) {
        try {
            ipfs.downloadFile(fixture.cid, fixture.path, true);
        } catch (const IPFS::exceptionNoConnection&) {
            GTEST_SKIP() << "IPFS node not available - skipping transaction tests";
        } catch (const IPFS::exceptionTimeout&) {
            GTEST_SKIP() << "No IPFS peer served " << fixture.cid << " (" << fixture.path
                         << ") before the timeout.  If your connection is simply slow raise "
                            "ipfstimeoutdownload in config.cfg and try again.  If it stays unavailable "
                            "nobody is hosting the file any more and it has to be added back to IPFS by "
                            "someone holding a copy - every test that needs rpcTest.db depends on it.";
        }
    }

    //initialize prerequisites
    AppMain* main = AppMain::GetInstance();
    DigiByteCore dgb;
    dgb.setFileName("config.cfg");
    dgb.makeConnection();
    main->setDigiByteCore(&dgb);
    Database db("../tests/testFiles/assetTest.db");
    main->setDatabase(&db);
    main->setIPFS(&ipfs);
    ipfs.start();
    PermanentStoragePoolList psp("config.cfg");
    main->setPermanentStoragePoolList(&psp);
    RPC::Cache rpcCache;
    main->setRpcCache(&rpcCache);


    //download test files
    string txid;
    size_t testNumber = 0;
    size_t testTotal = 0;
    string lineText;
    try {
        std::ifstream file("../tests/testFiles/assetTest.csv");
        if (file.is_open()) {
            std::string line;
            DigiByteTransaction test;
            while (std::getline(file, line)) {
                if ((!showAll) && (testTotal>0) && (testNumber % 1000 == 0)) {
                    utils::printProgressBar(static_cast<float>(testNumber) / static_cast<float>(testTotal) );
                }

                //initialize some variables that are needed by the test script
                getrawtransaction_t raw;
                DigiAsset ua;
                DigiAssetRules ur;
                vector<uint8_t> serializedRule;
                size_t i, inputCount, outputCount, assetCount;
                AssetUTXO tau;
                unsigned int height;
                string address;
                string flags;
                size_t li = 2; //first value is always a char so lets scip
                string temp;


                char type = line[0];
                if (type == 'C') {
                    testTotal = stoi(TestHelpers::getCSVValue(line, li));
                    continue;
                }
                lineText = "";
                if ((type != 'N') && (type != 'R')) {
                    txid = TestHelpers::getCSVValue(line, li);
                    height = stoi(TestHelpers::getCSVValue(line, li));
                    testNumber++;
                    map<char, string> typeLong = {
                            {'T', "Asset Transaction"},
                            {'K', "KYC Transaction"},
                            {'E', "Exchange Transaction"}};
                    lineText = "Test: " + to_string(testNumber) + " of " + to_string(testTotal) + " - txid:" + txid +
                               " - " + typeLong[type];
                    if (showAll) std::cout << lineText << "\n";
                }

                string passed;
                switch (type) {
                    case 'T': //Test
                        flags = TestHelpers::getCSVValue(line, li);

                        //construct the transaction object and check type flags match
                        test = DigiByteTransaction(txid, height);
                        test.addToDatabase();

                        if (test.isNonAssetTransaction() != (flags[0] == '1')) passed += " isNonAssetTransaction";
                        if (test.isIssuance() != (flags[1] == '1')) passed += " isIssuance";
                        if (test.isUnintentionalBurn() != (flags[2] == '1')) passed += " isUnintentionalBurn";
                        if (test.isBurn() != (flags[3] == '1')) passed += " isBurn";
                        if (test.isBurn(true) != (flags[4] == '1')) passed += " isBurn";
                        if (test.isTransfer() != (flags[5] == '1')) passed += " isTransfer";
                        if (test.isTransfer(true) != (flags[6] == '1')) passed += " isTransfer";

                        //test inputs
                        inputCount = stoi(TestHelpers::getCSVValue(line, li));
                        for (i = 0; i < inputCount; i++) {
                            tau = test.getInput(i);
                            string failPrefix = " input[" + to_string(i) + "]";
                            if (to_string(tau.digibyte) != TestHelpers::getCSVValue(line, li)) {
                                passed += failPrefix + ".digibyte";
                            }
                            assetCount = stoi(TestHelpers::getCSVValue(line, li));
                            if (tau.assets.size() != assetCount) passed += failPrefix + ".assets.size";
                            for (size_t ii = 0; ii < assetCount; ii++) {
                                if (to_string(tau.assets[ii].getCount()) != TestHelpers::getCSVValue(line, li)) {
                                    passed += failPrefix + ".assets[" + to_string(ii) + "].getCount";
                                }
                                if (tau.assets[ii].getAssetId() != TestHelpers::getCSVValue(line, li)) {
                                    passed += failPrefix + ".assets[" + to_string(ii) + "].getDomainAssetId";
                                }
                                if (to_string(tau.assets[ii].getAssetIndex()) !=
                                    TestHelpers::getCSVValue(line, li)) {
                                    passed += failPrefix + ".assets[" + to_string(ii) + "].getAssetIndex";
                                }
                            }
                        }

                        //test outputs
                        outputCount = stoi(TestHelpers::getCSVValue(line, li));
                        for (i = 0; i < outputCount; i++) {
                            tau = test.getOutput(i);
                            string failPrefix = " output[" + to_string(i) + "]";
                            if (to_string(tau.digibyte) != TestHelpers::getCSVValue(line, li)) {
                                passed += failPrefix + ".digibyte";
                            }
                            assetCount = stoi(TestHelpers::getCSVValue(line, li));
                            if (tau.assets.size() != assetCount) {
                                passed += failPrefix + ".assets.size";
                            } else {
                                for (size_t ii = 0; ii < assetCount; ii++) {
                                    if (to_string(tau.assets[ii].getCount()) != TestHelpers::getCSVValue(line, li)) {
                                        passed += failPrefix + ".assets[" + to_string(ii) + "].getCount";
                                    }
                                    if (tau.assets[ii].getAssetId() != TestHelpers::getCSVValue(line, li)) {
                                        passed += failPrefix + ".assets[" + to_string(ii) + "].getDomainAssetId";
                                    }
                                    if (to_string(tau.assets[ii].getAssetIndex()) !=
                                        TestHelpers::getCSVValue(line, li)) {
                                        passed += failPrefix + ".assets[" + to_string(ii) + "].getAssetIndex";
                                    }
                                }
                            }
                        }

                        break;

                    case 'N': //New Asset(update database so next tests can be done)
                    case 'R': //Rule Change(update database so next tests can be done)
                        /*
                            oi = stoi(TestHelpers::getCSVValue(line, li));
                            ai = stoi(TestHelpers::getCSVValue(line, li));
                            ua = DigiAsset(test.getOutput(oi).assets[ai]);
                            serializedRule.clear();
                            try {
                                serializedRule=TestHelpers::hexToVector(TestHelpers::getCSVValue(line,li));
                            } catch (const exception& e) {
                                //rule may be empty
                            }
                            i=0;
                            deserialize(serializedRule,i,ur);
                            EXPECT_TRUE(ua.getRules()==ur);
                            */
                        break;


                    case 'K': //KYC
                        address = TestHelpers::getCSVValue(line, li);
                        test = DigiByteTransaction(txid, height);
                        test.addToDatabase();

                        if (test.isTransfer()) passed += " isTransfer";
                        if (test.isBurn()) passed += "isBurn";
                        if (!test.isNonAssetTransaction()) passed += " isNonAssetTransaction";
                        if (test.isUnintentionalBurn()) passed += " isUnintentionalBurn";
                        if (test.isIssuance()) passed += " isIssuance";
                        if (!test.isKYCTransaction()) passed += " isKYCTransaction";
                        if (test.isKYCRevoke()) passed += " isKYCRevoke";
                        if (!test.isKYCIssuance()) passed += " isKYCIssuance";
                        if (test.isExchangeTransaction()) passed += " isExchangeTransaction";
                        if (test.isStandardTransaction()) passed += " isStandardTransaction";

                        temp = TestHelpers::getCSVValue(line, li);
                        if (test.getKYC().getCountry() != temp) passed += " getKYC.getCountry";
                        temp = TestHelpers::getCSVValue(line, li);
                        if (temp == "null") temp = ""; //todo fix csv file
                        if (test.getKYC().getName() != temp) passed += " getKYC.getName";
                        temp = TestHelpers::getCSVValue(line, li);
                        if (temp.find('\0') != string::npos) temp = temp.substr(0, temp.find('\0', 0));
                        if (test.getKYC().getHash() != temp) passed += " getKYC.getHash";
                        if (test.getKYC().getHeightCreated() != height) passed += " getKYC.getHeightCreated";
                        break;


                    case 'E': //exchange rate
                        address = TestHelpers::getCSVValue(line, li);
                        test = DigiByteTransaction(txid, height);
                        test.addToDatabase();

                        if (!test.isExchangeTransaction()) passed += " isExchangeTransaction";
                        if (test.isBurn()) passed += " isBurn";
                        if (test.isTransfer()) passed += " isTransfer";
                        if (test.isIssuance()) passed += " isIssuance";
                        if (test.isUnintentionalBurn()) passed += " isUnintentionalBurn";
                        if (!test.isNonAssetTransaction()) passed += " isNonAssetTransaction";
                        if (test.isKYCIssuance()) passed += " isKYCIssuance";
                        if (test.isKYCRevoke()) passed += " isKYCRevoke";
                        if (test.isKYCTransaction()) passed += " isKYCTransaction";
                        if (test.getExchangeRateCount() != 10) passed += " getExchangeRateCount";

                        try {
                            for (size_t i = 0; i < 10; i++) {
                                double value = stod(TestHelpers::getCSVValue(line, li));
                                if (!TestHelpers::approximatelyEqual(test.getExchangeRate(i), value)) {
                                    passed += " getExchangeRate[" + to_string(i) + "]";
                                }
                            }

                        } catch (const exception& e) {
                            //some don't have test values
                        }
                }
                EXPECT_EQ(passed, "");
                if (!passed.empty()) {
                    cout << lineText << " - " << passed << "\n";
                    errorList += (lineText + " - " + passed + "\n");
                }
            }
            cout << "\n";
            file.close();
        } else {
            std::cout << "\n\nTest file failed to open\n";
            EXPECT_TRUE(false); //test file failed to open
        }
    } catch (const exception& e) {
        std::cout << "\n\nMajor Error Thrown In: \n"
                  << lineText << "\n";
        std::cout << e.what() << "\n";

        EXPECT_TRUE(false);
    }
    if (!errorList.empty()) {
        std::cout << "\n\nFailed Tests: \n"
                  << errorList << "\n";
    }

    //pause to let IPFS catch up
    while (db.getIPFSJobCount()>0) {
        chrono::minutes dura(1);
        this_thread::sleep_for(dura);
    }

    //flush WAL to main DB file so the plain-file copy is complete
    db.walCheckpoint();

    //copy processed database if all good so rpc tests can run on it
    if (errorList.empty()) {
        utils::copyFile("../tests/testFiles/assetTest.db", "../tests/testFiles/rpcTest.db");
    }

    //stop IPFS thread before resetting AppMain to prevent CRITICAL log spam from worker accessing null db
    ipfs.stop();

    //clean up
    main->reset();

    //remove downloaded test files
    remove("../tests/testFiles/assetTest.db");
    remove("../tests/testFiles/assetTest.db-wal");
    remove("../tests/testFiles/assetTest.db-shm");
    remove("../tests/testFiles/assetTest.csv");
}
