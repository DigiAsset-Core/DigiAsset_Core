//
// Created by mctrivia on 08/04/24.
//

#include "AppMain.h"
#include "utils.h"
#include "RPC/MethodList.h"
#include "gtest/gtest.h"
#include "../tests/RPCMethods.h"

using namespace std;


TEST_F(RPCMethodsTest, listunspent) {
    ///rpc method we will be testing(if using as reference make sure you change value above and bellow this line)
    const std::string METHOD="listunspent";
    bool result;

    //test invalid parameters throws an exception of type DigiByteException
    //6 parameters
    try {
        Json::Value params=Json::arrayValue;
        params.append(100);
        params.append(100);
        params.append(Json::nullValue);
        params.append(true);
        params.append(Json::nullValue);
        params.append(100);
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

    //test invalid parameters throws an exception of type DigiByteException
    //wrong type
    try {
        Json::Value params=Json::arrayValue;
        params.append(100);
        params.append(1000);
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
        params.append(1000);
        params.append(Json::nullValue);
        params.append(true);
        params.append(1000);
        RPC::methods[METHOD](params);
        result=false;
    } catch (const DigiByteException& e) {
        result=true;
    } catch (...) {
        result=false;
    }
    EXPECT_TRUE(result);

    //try searching with limit 100 and start of 5
    try {
        Json::Value params=Json::arrayValue;
        params.append(Json::nullValue);
        params.append(Json::nullValue);
        Json::Value addresses=Json::arrayValue;
        addresses.append("dgb1qlk3hldeynl3prqw259u8gv0jh7w5nwppxlvt3v");
        params.append(addresses);
        auto results=RPC::methods[METHOD](params).toJSON(1)["result"];
        EXPECT_TRUE(results.isArray());
        EXPECT_EQ(results.size(),6);
        EXPECT_EQ(results[0]["address"].asString(),"dgb1qlk3hldeynl3prqw259u8gv0jh7w5nwppxlvt3v");
        EXPECT_EQ(results[0]["digibyte"].asInt64(),976154000);
        EXPECT_DOUBLE_EQ(results[0]["amount"].asDouble(),9.76154000);
        EXPECT_EQ(results[0]["txid"].asString(),"16c6123209477a67adcecc0afac2e52852c06e92459d9cf1221d95ce5931cfd8");
        EXPECT_EQ(results[0]["vout"].asUInt(),1);
        EXPECT_EQ(results[1]["address"].asString(),"dgb1qlk3hldeynl3prqw259u8gv0jh7w5nwppxlvt3v");
        EXPECT_EQ(results[1]["digibyte"].asInt64(),976136000);
        EXPECT_DOUBLE_EQ(results[1]["amount"].asDouble(),9.76136000);
        EXPECT_EQ(results[1]["txid"].asString(),"2d2a618db016f443fe86334b12c226b48b202e3e3290b6b91af4f19b3d3108dc");
        EXPECT_EQ(results[1]["vout"].asUInt(),1);
        EXPECT_EQ(results[2]["address"].asString(),"dgb1qlk3hldeynl3prqw259u8gv0jh7w5nwppxlvt3v");
        EXPECT_EQ(results[2]["digibyte"].asInt64(),975573000);
        EXPECT_DOUBLE_EQ(results[2]["amount"].asDouble(),9.75573000);
        EXPECT_EQ(results[2]["txid"].asString(),"5f04a662aa95b8c7818e6901a40d9aeb7f9a0f12024c5fb879bdfb85ad3b92ca");
        EXPECT_EQ(results[2]["vout"].asUInt(),1);
        EXPECT_EQ(results[3]["address"].asString(),"dgb1qlk3hldeynl3prqw259u8gv0jh7w5nwppxlvt3v");
        EXPECT_EQ(results[3]["digibyte"].asInt64(),976320000);
        EXPECT_DOUBLE_EQ(results[3]["amount"].asDouble(),9.76320000);
        EXPECT_EQ(results[3]["txid"].asString(),"7a14c4302fd2b1725a4e5088d60fa6317fdf55b45144ce497f3eda940555941d");
        EXPECT_EQ(results[3]["vout"].asUInt(),1);
        EXPECT_EQ(results[4]["address"].asString(),"dgb1qlk3hldeynl3prqw259u8gv0jh7w5nwppxlvt3v");
        EXPECT_EQ(results[4]["digibyte"].asInt64(),919271000);
        EXPECT_DOUBLE_EQ(results[4]["amount"].asDouble(),9.19271000);
        EXPECT_EQ(results[4]["txid"].asString(),"8ef63821a7f9b27c6092781c7f638c98f6fadeff715d07055b3bf8d96d85d30c");
        EXPECT_EQ(results[4]["vout"].asUInt(),1);
        EXPECT_EQ(results[5]["address"].asString(),"dgb1qlk3hldeynl3prqw259u8gv0jh7w5nwppxlvt3v");
        EXPECT_EQ(results[5]["digibyte"].asInt64(),976241000);
        EXPECT_DOUBLE_EQ(results[5]["amount"].asDouble(),9.76241000);
        EXPECT_EQ(results[5]["txid"].asString(),"af36debd4e706b9532b4438acb3b750e56c7156bfa99d22aa24564a0fe6c26d1");
        EXPECT_EQ(results[5]["vout"].asUInt(),1);
    } catch (...) {
        EXPECT_TRUE(false);
    }
}