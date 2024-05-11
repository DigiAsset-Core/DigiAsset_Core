//
// Created by mctrivia on 31/03/24.
//

#include "../tests/RPCMethods.h"
#include "AppMain.h"
#include "RPC/MethodList.h"
#include "utils.h"
#include "gtest/gtest.h"

using namespace std;


TEST_F(RPCMethodsTest, getaddressholdings) {
    ///rpc method we will be testing(if using as reference make sure you change value above and bellow this line)
    const std::string METHOD="getaddressholdings";
    bool result;

    //test invalid parameters throws an exception of type DigiByteException
    //0 parameters
    try {
        Json::Value params=Json::arrayValue;
        RPC::methods[METHOD](params);
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
        RPC::methods[METHOD](params);
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
        RPC::methods[METHOD](params);
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
        auto results=RPC::methods[METHOD](params).toJSON(1)["result"];
        EXPECT_TRUE(results.isObject());
        EXPECT_TRUE(results.empty());
    } catch (...) {
        EXPECT_TRUE(false);
    }

    //test address that should have holdings
    try {
        Json::Value params=Json::arrayValue;
        params.append("DBhar3Ge6iGFDqHRT8NGfdh112KtLYXksy");
        auto results=RPC::methods[METHOD](params).toJSON(1)["result"];
        EXPECT_TRUE(results.isObject());
        EXPECT_FALSE(results.empty());
        EXPECT_EQ(results["1"].asUInt64(),3000864466);
        EXPECT_EQ(results["1177"].asUInt64(),5);
        EXPECT_EQ(results["442"].asUInt64(),7);
        EXPECT_EQ(results["63"].asUInt64(),656);
    } catch (...) {
        EXPECT_TRUE(false);
    }
}