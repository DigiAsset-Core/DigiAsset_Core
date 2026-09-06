//
// Created by DigiAsset Core on 14/07/26.
//

#include "../tests/RPCMethods.h"
#include "AppMain.h"
#include "RPC/MethodList.h"
#include "utils.h"
#include "gtest/gtest.h"

using namespace std;


///Only parameter validation can be tested here.  Actually issuing an asset needs a funded
///wallet and IPFS node which the test environment doesn't have(see LAST_TASKS_NOTES.md).
TEST_F(RPCMethodsTest, issueasset) {
    ///rpc method we will be testing(if using as reference make sure you change value above and bellow this line)
    const std::string METHOD = "issueasset";
    bool result;

    //test invalid parameters throws an exception of type DigiByteException
    //0 parameters
    try {
        Json::Value params = Json::arrayValue;
        RPC::methods[METHOD](params);
        result = false;
    } catch (const DigiByteException& e) {
        result = true;
    } catch (...) {
        result = false;
    }
    EXPECT_TRUE(result);

    //parameter is not an object
    try {
        Json::Value params = Json::arrayValue;
        params.append("not an object");
        RPC::methods[METHOD](params);
        result = false;
    } catch (const DigiByteException& e) {
        result = true;
    } catch (...) {
        result = false;
    }
    EXPECT_TRUE(result);

    //missing name
    try {
        Json::Value params = Json::arrayValue;
        Json::Value config = Json::objectValue;
        config["amount"] = 100;
        params.append(config);
        RPC::methods[METHOD](params);
        result = false;
    } catch (const DigiByteException& e) {
        result = true;
    } catch (...) {
        result = false;
    }
    EXPECT_TRUE(result);

    //missing amount
    try {
        Json::Value params = Json::arrayValue;
        Json::Value config = Json::objectValue;
        config["name"] = "Test Asset";
        params.append(config);
        RPC::methods[METHOD](params);
        result = false;
    } catch (const DigiByteException& e) {
        result = true;
    } catch (...) {
        result = false;
    }
    EXPECT_TRUE(result);

    //invalid decimals
    try {
        Json::Value params = Json::arrayValue;
        Json::Value config = Json::objectValue;
        config["name"] = "Test Asset";
        config["amount"] = 100;
        config["decimals"] = 8; //3 bit field - max real asset divisibility is 7
        params.append(config);
        RPC::methods[METHOD](params);
        result = false;
    } catch (const DigiByteException& e) {
        result = true;
    } catch (...) {
        result = false;
    }
    EXPECT_TRUE(result);

    //invalid aggregation
    try {
        Json::Value params = Json::arrayValue;
        Json::Value config = Json::objectValue;
        config["name"] = "Test Asset";
        config["amount"] = 100;
        config["aggregation"] = "invalid";
        params.append(config);
        RPC::methods[METHOD](params);
        result = false;
    } catch (const DigiByteException& e) {
        result = true;
    } catch (...) {
        result = false;
    }
    EXPECT_TRUE(result);

    //rules are not supported yet
    try {
        Json::Value params = Json::arrayValue;
        Json::Value config = Json::objectValue;
        config["name"] = "Test Asset";
        config["amount"] = 100;
        config["rules"] = Json::objectValue;
        params.append(config);
        RPC::methods[METHOD](params);
        result = false;
    } catch (const DigiByteException& e) {
        result = true;
    } catch (...) {
        result = false;
    }
    EXPECT_TRUE(result);

    //zero amount
    try {
        Json::Value params = Json::arrayValue;
        Json::Value config = Json::objectValue;
        config["name"] = "Test Asset";
        config["amount"] = 0;
        params.append(config);
        RPC::methods[METHOD](params);
        result = false;
    } catch (const DigiByteException& e) {
        result = true;
    } catch (...) {
        result = false;
    }
    EXPECT_TRUE(result);

    //psp wrong type
    try {
        Json::Value params = Json::arrayValue;
        Json::Value config = Json::objectValue;
        config["name"] = "Test Asset";
        config["amount"] = 100;
        config["psp"] = "public";
        params.append(config);
        RPC::methods[METHOD](params);
        result = false;
    } catch (const DigiByteException& e) {
        result = true;
    } catch (...) {
        result = false;
    }
    EXPECT_TRUE(result);

    //psp array with non integer entry
    try {
        Json::Value params = Json::arrayValue;
        Json::Value config = Json::objectValue;
        config["name"] = "Test Asset";
        config["amount"] = 100;
        Json::Value pools = Json::arrayValue;
        pools.append(1);
        pools.append("local");
        config["psp"] = pools;
        params.append(config);
        RPC::methods[METHOD](params);
        result = false;
    } catch (const DigiByteException& e) {
        result = true;
    } catch (...) {
        result = false;
    }
    EXPECT_TRUE(result);

    //psp pool index out of range
    try {
        Json::Value params = Json::arrayValue;
        Json::Value config = Json::objectValue;
        config["name"] = "Test Asset";
        config["amount"] = 100;
        config["psp"] = 99;
        params.append(config);
        RPC::methods[METHOD](params);
        result = false;
    } catch (const DigiByteException& e) {
        result = true;
    } catch (...) {
        result = false;
    }
    EXPECT_TRUE(result);
}
