//
// Created by mctrivia on 07/07/23.
// This test sweet tests all historical DigiAsset, KYC, and Exchange transactions up to block todo ???????

#include "AppMain.h"
#include "DigiAsset.h"
#include "DigiAssetRules.h"
#include "DigiByteCore.h"
#include "DigiByteTransaction.h"
#include "IPFS.h"
#include "Log.h"
#include "TestHelpers.h"
#include "gtest/gtest.h"
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

    const bool showAll = true; ///set true if debugging and want it to show the txid before doing each test
    string errorList = "";

    //delete old test database if still exists
    try {
        remove("../tests/testFiles/assetTest.db");
    } catch (...) {}

    IPFS ipfs("config.cfg", false);
    ipfs.downloadFile("QmNPyr5tkm48cUu5iMbReiM8GN8AW6PRpzUztPFadaxC8j", "../tests/testFiles/assetTest.csv", true);
    ipfs.downloadFile("QmY34pNFnmtEoJLTtiuAtKsGMf9AkJQPtuA3MzgBzZrMoB", "../tests/testFiles/assetTest.db", true);

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
                if ((!showAll) && (testNumber % 100 == 0)) {
                    cout << "*";
                    std::cout.flush();
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
    chrono::minutes dura(30);
    this_thread::sleep_for(dura);

    //clean up
    main->reset();
}
