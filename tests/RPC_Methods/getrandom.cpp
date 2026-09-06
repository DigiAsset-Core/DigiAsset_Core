//
// Created by DigiAsset Core on 14/07/26.
//

#include "../tests/RPCMethods.h"
#include "AppMain.h"
#include "RPC/MethodList.h"
#include "utils.h"
#include "gtest/gtest.h"

using namespace std;


TEST_F(RPCMethodsTest, getrandom) {
    ///rpc method we will be testing(if using as reference make sure you change value above and bellow this line)
    const std::string METHOD = "getrandom";
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

    //txid form with bad txid length
    try {
        Json::Value params = Json::arrayValue;
        params.append("abc123");
        params.append(0);
        RPC::methods[METHOD](params);
        result = false;
    } catch (const DigiByteException& e) {
        result = true;
    } catch (...) {
        result = false;
    }
    EXPECT_TRUE(result);

    //txid form missing vout
    try {
        Json::Value params = Json::arrayValue;
        params.append("0000000000000000000000000000000000000000000000000000000000000000");
        RPC::methods[METHOD](params);
        result = false;
    } catch (const DigiByteException& e) {
        result = true;
    } catch (...) {
        result = false;
    }
    EXPECT_TRUE(result);

    //txid form with too many parameters
    try {
        Json::Value params = Json::arrayValue;
        params.append("0000000000000000000000000000000000000000000000000000000000000000");
        params.append(0);
        params.append(100);
        params.append("extra");
        RPC::methods[METHOD](params);
        result = false;
    } catch (const DigiByteException& e) {
        result = true;
    } catch (...) {
        result = false;
    }
    EXPECT_TRUE(result);
}
