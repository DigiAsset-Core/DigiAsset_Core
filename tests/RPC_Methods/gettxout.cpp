//
// Created by RenzoDD on 25/04/24.
//

#include "AppMain.h"
#include "utils.h"
#include "RPC/MethodList.h"
#include "gtest/gtest.h"
#include "../tests/RPCMethods.h"

using namespace std;


TEST_F(RPCMethodsTest, gettxout) {
    ///rpc method we will be testing(if using as reference make sure you change value above and bellow this line)
    const std::string METHOD="gettxout";
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
    //4 parameters
    try {
        Json::Value params=Json::arrayValue;
        params.append("test");
        params.append(1);
        params.append(500);
        params.append("test");
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
        params.append(5);
        params.append("bad");
        params.append(10);
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
        params.append("18ac3e7343f016890c510e93f935261169d9e3f565436429830faf0934f4f8e4");
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
        params.append("18ac3e7343f016890c510e93f935261169d9e3f565436429830faf0934f4f8e4");
        params.append("5");
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
        params.append("18ac3e7343f016890c510e93f935261169d9e3f565436429830faf0934f4f8e4");
        params.append(5);
        params.append("true");
        RPC::methods[METHOD](params);
        result=false;
    } catch (const DigiByteException& e) {
        result=true;
    } catch (...) {
        result=false;
    }
    EXPECT_TRUE(result);

    //test invalid parameters throws an exception of type DigiByteException
    //txid not 64 characters
    try {
        Json::Value params=Json::arrayValue;
        params.append("18ac3e7343f016890c510e93f935261169d9e3f565436429830faf0934f4f84"); // 63 characters
        params.append(1);
        RPC::methods[METHOD](params);
        result=false;
    } catch (const DigiByteException& e) {
        result=true;
    } catch (...) {
        result=false;
    }

    //txid that doesn't exist
    try {
        Json::Value params=Json::arrayValue;
        params.append("78a5a8277300e5040b82ec0396f40eded43424f2065e5c3dd98b69bf4c9a1362");
        params.append(1);
        auto results=RPC::methods[METHOD](params).toJSON(1)["result"];
        EXPECT_TRUE(results.empty());
    } catch (...) {
        EXPECT_TRUE(false);
    }

    //txid that exists, unspend vout, with assets
    try {
        Json::Value params=Json::arrayValue;
        params.append("5168afe2c85f24c3985c3aeafe8dc4c836c57e89f0c63242c85e9376b1d49ac6");
        params.append(0);
        auto results=RPC::methods[METHOD](params).toJSON(1)["result"];
        EXPECT_TRUE(results["assets"].isArray());
        EXPECT_EQ(results["assets"].size(), 1);
        EXPECT_EQ(results["assets"][0]["assetId"].asString(),"Ua94nEKabzhJeDJtxGFXdviT185tYeHqyHKeWC");
        EXPECT_EQ(results["assets"][0]["assetIndex"].asInt64(),2);
        EXPECT_EQ(results["assets"][0]["cid"].asString(),"bafkreifkzom442xtxj2vll3pgbsnw4t4tgh5w54unh2j3kbkeal2nqp6uy");
        EXPECT_EQ(results["assets"][0]["count"].asInt64(),1);
        EXPECT_EQ(results["assets"][0]["decimals"].asInt(),2);
        EXPECT_EQ(results["assets"][0]["height"].asInt(),8432316);
        EXPECT_EQ(results["coinbase"].asBool(),false);
        EXPECT_EQ(results["digibyte"].asUInt(),600);
        EXPECT_TRUE(results["scriptPubKey"].isObject());
        EXPECT_EQ(results["scriptPubKey"]["hex"].asString(),"76a914b0d5d15901856da5e6713e18d9215625d32c651488ac");
    } catch (...) {
        EXPECT_TRUE(false);
    }

    //txid that exists spend vout
    try {
        Json::Value params=Json::arrayValue;
        params.append("7ad5a1fefa2c65473a8d6d6e1a0d242f7045c0cc90564222a91ae6d8124c2e7f");
        params.append(2);
        auto results=RPC::methods[METHOD](params).toJSON(1)["result"];
        EXPECT_TRUE(results.empty());
    } catch (...) {
        EXPECT_TRUE(false);
    }

    //txid that exists, unspend vout, without assets
    try {
        Json::Value params=Json::arrayValue;
        params.append("5168afe2c85f24c3985c3aeafe8dc4c836c57e89f0c63242c85e9376b1d49ac6");
        params.append(2);
        auto results=RPC::methods[METHOD](params).toJSON(1)["result"];
        EXPECT_TRUE(results["assets"].isArray());
        EXPECT_EQ(results["assets"].size(), 0);
        EXPECT_EQ(results["coinbase"].asBool(),false);
        EXPECT_EQ(results["digibyte"].asUInt(),200994344);
        EXPECT_TRUE(results["scriptPubKey"].isObject());
        EXPECT_EQ(results["scriptPubKey"]["hex"].asString(),"76a914a7731941a96f891504956370af1a21d775c234c088ac");
    } catch (...) {
        EXPECT_TRUE(false);
    }
}
