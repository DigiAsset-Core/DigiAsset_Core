//
// Created by mctrivia on 31/03/24.
//

#include "AppMain.h"
#include "utils.h"
#include "BitcoinRpcServerMethods.h"
#include "gtest/gtest.h"
#include "../tests/RPCMethods.h"

using namespace std;


TEST_F(RPCMethodsTest, getaddresskyc) {
    ///rpc method we will be testing(if using as reference make sure you change value above and bellow this line)
    const std::string METHOD="getaddresskyc";
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
        params.append("bad2");
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
        params.append(5);
        rpcMethods[METHOD](params);
        result=false;
    } catch (const DigiByteException& e) {
        result=true;
    } catch (...) {
        result=false;
    }
    EXPECT_TRUE(result);

    //test address that hasn't been used
    try {
        Json::Value params=Json::arrayValue;
        params.append("DL9qVq1qSHwjs3ZJNEAfeKJYooPTCS7BLD");
        auto results=rpcMethods[METHOD](params);
        EXPECT_TRUE(results.isObject());
        EXPECT_TRUE(results.isMember("address"));
        EXPECT_EQ(results["address"].asString(),"DL9qVq1qSHwjs3ZJNEAfeKJYooPTCS7BLD");
        EXPECT_FALSE(results.isMember("country"));
        EXPECT_FALSE(results.isMember("name"));
        EXPECT_FALSE(results.isMember("hash"));
    } catch (...) {
        EXPECT_TRUE(false);
    }

    //test address that should have kyc with name
    try {
        Json::Value params=Json::arrayValue;
        params.append("DP6p2hn7FSHRosjGRgSfnue4epaFgc4EHv");
        auto results=rpcMethods[METHOD](params);
        EXPECT_TRUE(results.isObject());
        EXPECT_TRUE(results.isMember("address"));
        EXPECT_EQ(results["address"].asString(),"DP6p2hn7FSHRosjGRgSfnue4epaFgc4EHv");
        EXPECT_TRUE(results.isMember("country"));
        EXPECT_EQ(results["country"].asString(),"CAN");
        EXPECT_TRUE(results.isMember("name"));
        EXPECT_EQ(results["name"].asString(),"MATTHEW KENNETH CORNELISSE");
        EXPECT_FALSE(results.isMember("hash"));
    } catch (...) {
        EXPECT_TRUE(false);
    }

    //test address that should have kyc with name
    try {
        Json::Value params=Json::arrayValue;
        params.append("dgb1qzhmej4tt4y4t8a2n3ntp35z02aq438hgs54u5u");
        auto results=rpcMethods[METHOD](params);
        EXPECT_TRUE(results.isObject());
        EXPECT_TRUE(results.isMember("address"));
        EXPECT_EQ(results["address"].asString(),"dgb1qzhmej4tt4y4t8a2n3ntp35z02aq438hgs54u5u");
        EXPECT_TRUE(results.isMember("country"));
        EXPECT_EQ(results["country"].asString(),"GBR");
        EXPECT_FALSE(results.isMember("name"));
        EXPECT_TRUE(results.isMember("hash"));
        EXPECT_EQ(results["hash"].asString(),"4306f363c81de72b19af82845f1153d55cdee506a94adf15e52adb8d976a0893");
    } catch (...) {
        EXPECT_TRUE(false);
    }
}