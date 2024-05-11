//
// Created by mctrivia on 31/03/24.
//

#include "AppMain.h"
#include "utils.h"
#include "RPC/MethodList.h"
#include "gtest/gtest.h"
#include "../tests/RPCMethods.h"

using namespace std;


TEST_F(RPCMethodsTest, getassetindexes) {
    ///rpc method we will be testing(if using as reference make sure you change value above and bellow this line)
    const std::string METHOD="getassetindexes";
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

    //test asset with invalid assetId
    try {
        Json::Value params=Json::arrayValue;
        params.append("La8u7WcjtGmDdZaUhSmspUw5oPkzCch1ALrJzh");
        auto results=RPC::methods[METHOD](params).toJSON(1)["result"];
        EXPECT_TRUE(results.isArray());
        EXPECT_EQ(results.size(),0);
    } catch (...) {
        EXPECT_TRUE(false);
    }

    //test known good asset with only 1 index
    try {
        Json::Value params=Json::arrayValue;
        params.append("La3LdaAT4PEYsN4qZ2vnjdeWbE6hY7VrMZHYBU");
        auto results=RPC::methods[METHOD](params).toJSON(1)["result"];
        EXPECT_TRUE(results.isArray());
        EXPECT_EQ(results.size(),1);
        EXPECT_EQ(results[0].asUInt(),245);
    } catch (...) {
        EXPECT_TRUE(false);
    }

    //test known good asset with multiple asset indexes
    try {
        Json::Value params=Json::arrayValue;
        params.append("Uh7bZqJKs5tH4Dm64A39kkQMhUNrsa43LyLcdJ");
        auto results=RPC::methods[METHOD](params).toJSON(1)["result"];
        EXPECT_TRUE(results.isArray());
        EXPECT_EQ(results.size(),10);
        EXPECT_EQ(results[0].asUInt(),4595);
        EXPECT_EQ(results[1].asUInt(),4596);
        EXPECT_EQ(results[2].asUInt(),4597);
        EXPECT_EQ(results[3].asUInt(),4598);
        EXPECT_EQ(results[4].asUInt(),4599);
        EXPECT_EQ(results[5].asUInt(),4600);
        EXPECT_EQ(results[6].asUInt(),4601);
        EXPECT_EQ(results[7].asUInt(),4602);
        EXPECT_EQ(results[8].asUInt(),4603);
        EXPECT_EQ(results[9].asUInt(),4604);
    } catch (...) {
        EXPECT_TRUE(false);
    }
}
