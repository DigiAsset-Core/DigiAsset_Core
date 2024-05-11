//
// Created by mctrivia on 31/03/24.
//

#include "AppMain.h"
#include "utils.h"
#include "RPC/MethodList.h"
#include "gtest/gtest.h"
#include "../tests/RPCMethods.h"

using namespace std;


TEST_F(RPCMethodsTest, getdgbequivalent) {
    ///rpc method we will be testing(if using as reference make sure you change value above and bellow this line)
    const std::string METHOD="getdgbequivalent";
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
        params.append(0);
        RPC::methods[METHOD](params);
        result=false;
    } catch (const DigiByteException& e) {
        result=true;
    } catch (...) {
        result=false;
    }
    EXPECT_TRUE(result);

    //test invalid parameters throws an exception of type DigiByteException
    //4 parameters
    try {
        Json::Value params=Json::arrayValue;
        params.append("bad1");
        params.append(0);
        params.append(10);
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
    //3 parameter but wrong type
    try {
        Json::Value params=Json::arrayValue;
        params.append(0);
        params.append(0);
        params.append(0);
        RPC::methods[METHOD](params);
        result=false;
    } catch (const DigiByteException& e) {
        result=true;
    } catch (...) {
        result=false;
    }
    EXPECT_TRUE(result);

    //test invalid parameters throws an exception of type DigiByteException
    //3 parameter but wrong type
    try {
        Json::Value params=Json::arrayValue;
        params.append("test");
        params.append("test");
        params.append(0);
        RPC::methods[METHOD](params);
        result=false;
    } catch (const DigiByteException& e) {
        result=true;
    } catch (...) {
        result=false;
    }
    EXPECT_TRUE(result);

    //test invalid parameters throws an exception of type DigiByteException
    //3 parameter but wrong type
    try {
        Json::Value params=Json::arrayValue;
        params.append("test");
        params.append(10);
        params.append(0);
        RPC::methods[METHOD](params);
        result=false;
    } catch (const DigiByteException& e) {
        result=true;
    } catch (...) {
        result=false;
    }
    EXPECT_TRUE(result);

    //test invalid parameters throws an exception of type DigiByteException
    //3 parameter but wrong type
    try {
        Json::Value params=Json::arrayValue;
        params.append("test");
        params.append(0);
        params.append("test");
        RPC::methods[METHOD](params);
        result=false;
    } catch (const DigiByteException& e) {
        result=true;
    } catch (...) {
        result=false;
    }
    EXPECT_TRUE(result);

    //test conversion from 1 US to DGB
    try {
        Json::Value params=Json::arrayValue;
        params.append("dgb1qunxh378eltj2jrwza5sj9grvu5xud43vqvudwh");
        params.append(0);
        params.append(100000000);
        auto results=RPC::methods[METHOD](params).toJSON(1)["result"];
        EXPECT_EQ(results.asUInt64(),9887716274);
    } catch (...) {
        EXPECT_TRUE(false);
    }

    //test conversion from 1 BTC to DGB
    try {
        Json::Value params=Json::arrayValue;
        params.append("dgb1qlk3hldeynl3prqw259u8gv0jh7w5nwppxlvt3v");
        params.append(0);
        params.append(100000000);
        auto results=RPC::methods[METHOD](params).toJSON(1)["result"];
        EXPECT_EQ(results.asUInt64(),381777420134996);
    } catch (...) {
        EXPECT_TRUE(false);
    }
}
