//
// Created by mctrivia on 01/04/24.
//

#include "AppMain.h"
#include "utils.h"
#include "BitcoinRpcServerMethods.h"
#include "gtest/gtest.h"
#include "../tests/RPCMethods.h"

using namespace std;


TEST_F(RPCMethodsTest, getdomainaddress) {
    ///rpc method we will be testing(if using as reference make sure you change value above and bellow this line)
    const std::string METHOD="getdomainaddress";
    bool result;

    //test invalid parameters throws an exception of type DigiByteException
    //0 parameters
    try {
        Json::Value params=Json::arrayValue;
        rpcMethods[METHOD](params);
        result=false;
    } catch (const DigiByteException& e) {
        result=true;
    } catch (...) {
        result=false;
    }
    EXPECT_TRUE(result);

    //test invalid parameters throws an exception of type DigiByteException
    //2 parameters
    try {
        Json::Value params=Json::arrayValue;
        params.append("bad1");
        params.append(0);
        rpcMethods[METHOD](params);
        result=false;
    } catch (const DigiByteException& e) {
        result=true;
    } catch (...) {
        result=false;
    }
    EXPECT_TRUE(result);

    //test invalid parameters throws an exception of type DigiByteException
    //1 parameter but wrong type
    try {
        Json::Value params=Json::arrayValue;
        params.append(0);
        rpcMethods[METHOD](params);
        result=false;
    } catch (const DigiByteException& e) {
        result=true;
    } catch (...) {
        result=false;
    }
    EXPECT_TRUE(result);

    //test invalid parameters throws an exception of type DigiByteException
    //test conversion non existing domain
    try {
        Json::Value params=Json::arrayValue;
        params.append("dgb1qunx.dgb");
        rpcMethods[METHOD](params);
        result=false;
    } catch (const DigiByteException& e) {
        result=true;
    } catch (...) {
        result=false;
    }
    EXPECT_TRUE(result);

    //test conversion known good domain
    try {
        Json::Value params=Json::arrayValue;
        params.append("mctrivia.dgb");
        auto results=rpcMethods[METHOD](params);
        EXPECT_EQ(results.asString(),"DDd3bSJKCD4M57PVawrw3ud7Mcw5DkXNRx");
    } catch (...) {
        EXPECT_TRUE(false);
    }

    //test conversion known good domain
    try {
        Json::Value params=Json::arrayValue;
        params.append("saltedlolly.dgb");
        auto results=rpcMethods[METHOD](params);
        EXPECT_EQ(results.asString(),"dgb1q9nwl9v2xqap0cdnx7ra08c0ftwct3lzhpfmdgk");
    } catch (...) {
        EXPECT_TRUE(false);
    }
}
