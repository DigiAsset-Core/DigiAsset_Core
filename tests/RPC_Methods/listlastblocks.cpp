//
// Created by mctrivia on 07/04/24.
//

//todo should add more blocks to test database so this can be tested better
#include "AppMain.h"
#include "utils.h"
#include "RPC/MethodList.h"
#include "gtest/gtest.h"
#include "../tests/RPCMethods.h"

using namespace std;


TEST_F(RPCMethodsTest, listlastblocks) {
    ///rpc method we will be testing(if using as reference make sure you change value above and bellow this line)
    const std::string METHOD="listlastblocks";
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
    //3 parameters
    try {
        Json::Value params=Json::arrayValue;
        params.append(100);
        params.append(100);
        params.append("bad");
        RPC::methods[METHOD](params);
        result=false;
    } catch (const DigiByteException& e) {
        result=true;
    } catch (...) {
        result=false;
    }
    EXPECT_TRUE(result);

    //test invalid parameters throws an exception of type DigiByteException
    //wrong type
    try {
        Json::Value params=Json::arrayValue;
        params.append(-1);
        RPC::methods[METHOD](params);
        result=false;
    } catch (const DigiByteException& e) {
        result=true;
    } catch (...) {
        result=false;
    }
    EXPECT_TRUE(result);

    //test invalid parameters throws an exception of type DigiByteException
    //wrong type
    try {
        Json::Value params=Json::arrayValue;
        params.append(100);
        params.append(-1);
        RPC::methods[METHOD](params);
        result=false;
    } catch (const DigiByteException& e) {
        result=true;
    } catch (...) {
        result=false;
    }
    EXPECT_TRUE(result);

    //try searching with limit 100
    try {
        Json::Value params=Json::arrayValue;
        params.append(100);
        auto results=RPC::methods[METHOD](params).toJSON(1)["result"];
        EXPECT_TRUE(results.isArray());
        EXPECT_EQ(results.size(),1);
        EXPECT_EQ(results[0]["height"].asUInt(),1);
        EXPECT_EQ(results[0]["hash"].asString(),"4da631f2ac1bed857bd968c67c913978274d8aabed64ab2bcebc1665d7f4d3a0");
        EXPECT_EQ(results[0]["time"].asUInt(),1389392876);
        EXPECT_EQ(results[0]["algo"].asUInt(),1);
    } catch (...) {
        EXPECT_TRUE(false);
    }

    //try searching with limit 100 and start of 5
    try {
        Json::Value params=Json::arrayValue;
        params.append(100);
        params.append(5);
        auto results=RPC::methods[METHOD](params).toJSON(1)["result"];
        EXPECT_TRUE(results.isArray());
        EXPECT_EQ(results.size(),1);
        EXPECT_EQ(results[0]["height"].asUInt(),1);
        EXPECT_EQ(results[0]["hash"].asString(),"4da631f2ac1bed857bd968c67c913978274d8aabed64ab2bcebc1665d7f4d3a0");
        EXPECT_EQ(results[0]["time"].asUInt(),1389392876);
        EXPECT_EQ(results[0]["algo"].asUInt(),1);
    } catch (...) {
        EXPECT_TRUE(false);
    }
}
