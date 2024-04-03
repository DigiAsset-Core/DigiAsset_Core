//
// Created by mctrivia on 01/04/24.
//

#include "AppMain.h"
#include "utils.h"
#include "BitcoinRpcServerMethods.h"
#include "gtest/gtest.h"
#include "../tests/RPCMethods.h"

using namespace std;


TEST_F(RPCMethodsTest, listaddresshistory) {
    ///rpc method we will be testing(if using as reference make sure you change value above and bellow this line)
    const std::string METHOD="listaddresshistory";
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
    //5 parameters
    try {
        Json::Value params=Json::arrayValue;
        params.append("test");
        params.append(1);
        params.append(500);
        params.append(50);
        params.append(50);
        rpcMethods[METHOD](params);
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
        rpcMethods[METHOD](params);
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
        params.append("address");
        params.append("bad");
        rpcMethods[METHOD](params);
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
        params.append("address");
        params.append(5);
        params.append("bad");
        rpcMethods[METHOD](params);
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
        params.append("address");
        params.append(5);
        params.append(500);
        params.append("bad");
        rpcMethods[METHOD](params);
        result=false;
    } catch (const DigiByteException& e) {
        result=true;
    } catch (...) {
        result=false;
    }
    EXPECT_TRUE(result);

    //test invalid parameters throws an exception of type DigiByteException
    //min bigger then max
    try {
        Json::Value params=Json::arrayValue;
        params.append("DMF9dg9WDHDRCAqGWwkhS2Fh3DjnFUhUck");
        params.append(15121024);
        params.append(15072843);
        rpcMethods[METHOD](params);
        result=false;
    } catch (const DigiByteException& e) {
        result=true;
    } catch (...) {
        result=false;
    }

    //address that doesn't exist
    try {
        Json::Value params=Json::arrayValue;
        params.append("test");
        auto results=rpcMethods[METHOD](params);
        EXPECT_TRUE(results.isArray());
        EXPECT_TRUE(results.empty());
    } catch (...) {
        EXPECT_TRUE(false);
    }

    //address that exists
    try {
        Json::Value params=Json::arrayValue;
        params.append("DMF9dg9WDHDRCAqGWwkhS2Fh3DjnFUhUck");
        auto results=rpcMethods[METHOD](params);
        EXPECT_TRUE(results.isArray());
        EXPECT_EQ(results.size(),111);
        EXPECT_EQ(results[0].asString(),"588655c5cf3d7eda2a8a98b4b05b38cd5afd9ee0af9103503f73e05ba532aad4");
        EXPECT_EQ(results[1].asString(),"bc30a60e897b7a87a7eadbf00d6a4c260dfb6de56dc287e61e9192a9e418d088");
        EXPECT_EQ(results[2].asString(),"a8491ab575f72703e7aee5ce4f31919b18a6bf5f8fe3ad44d0897f2a058fa93a");
    } catch (...) {
        EXPECT_TRUE(false);
    }

    //show only newer txids
    try {
        Json::Value params=Json::arrayValue;
        params.append("DMF9dg9WDHDRCAqGWwkhS2Fh3DjnFUhUck");
        params.append(15213018);
        auto results=rpcMethods[METHOD](params);
        EXPECT_TRUE(results.isArray());
        EXPECT_EQ(results.size(),12);
        EXPECT_EQ(results[0].asString(),"aa020b4345692b18fe8cd84035940337164f3e86cad36fc5ed75cab38852f018");
        EXPECT_EQ(results[1].asString(),"9e310f45e98f2204bc89632070424fde67814c5b28cc7ac1e90d04c08bd72874");
        EXPECT_EQ(results[2].asString(),"c39cab2427b65420171aa773482713815b3ca2a485e65c30aacf60e880beda5e");
        EXPECT_EQ(results[3].asString(),"ca81f44a1c9e68015832a5376c50535e1954743abcf6c517338f576c90669f49");
        EXPECT_EQ(results[4].asString(),"fa51af69fe3dcc908ebc6e9c343b6715cf4ce8aec8aea4577b1087b040670825");
        EXPECT_EQ(results[5].asString(),"8e572e6105e0114ef3daf35a372dcc01a4222a9edbb0210bcd47f366f0e5e530");
	    EXPECT_EQ(results[6].asString(),"4bc53c9777398bb1e1c98429151c02397f6661c6b834bbd6cb2e45a3eb8c766b");
        EXPECT_EQ(results[7].asString(),"5967e561b367c567bb73c0582cb5326f930e8f9d274348a7a016f7da12598253");
        EXPECT_EQ(results[8].asString(),"63151c5ea79d50eeb587a166d227ac401733545051c1919699780d1410582e4d");
        EXPECT_EQ(results[9].asString(),"375997e9fdc1b9f653b4508b85c04da14127cfabb4e5b460f6951f169d9e586e");
        EXPECT_EQ(results[10].asString(),"20685aa9376cddac3a0f2d21d13babc16fca7ad1e130646f46c82a03c96de89f");
        EXPECT_EQ(results[11].asString(),"331e4bfbcd3166b723d11c7f2009cd3d56a03e9ec1ce254855b49bb0fb7497c2");
    } catch (...) {
        EXPECT_TRUE(false);
    }

    //show txs in a range
    try {
        Json::Value params=Json::arrayValue;
        params.append("DMF9dg9WDHDRCAqGWwkhS2Fh3DjnFUhUck");
        params.append(15212491);
        params.append(15213702);
        auto results=rpcMethods[METHOD](params);
        EXPECT_TRUE(results.isArray());
        EXPECT_EQ(results.size(),20);
        EXPECT_EQ(results[0].asString(),"aa9553b1ed1cbf6d29f9013d8f87622c26307118bed2cb7479a535cea4f00ee5");
        EXPECT_EQ(results[19].asString(),"4bc53c9777398bb1e1c98429151c02397f6661c6b834bbd6cb2e45a3eb8c766b");
    } catch (...) {
        EXPECT_TRUE(false);
    }

    //select all but limit to 10
    try {
        Json::Value params=Json::arrayValue;
        params.append("DMF9dg9WDHDRCAqGWwkhS2Fh3DjnFUhUck");
        params.append(Json::nullValue);
        params.append(Json::nullValue);
        params.append(10);
        auto results=rpcMethods[METHOD](params);
        EXPECT_TRUE(results.isArray());
        EXPECT_EQ(results.size(),10);
        EXPECT_EQ(results[0].asString(),"588655c5cf3d7eda2a8a98b4b05b38cd5afd9ee0af9103503f73e05ba532aad4");
        EXPECT_EQ(results[1].asString(),"bc30a60e897b7a87a7eadbf00d6a4c260dfb6de56dc287e61e9192a9e418d088");
        EXPECT_EQ(results[2].asString(),"a8491ab575f72703e7aee5ce4f31919b18a6bf5f8fe3ad44d0897f2a058fa93a");
        EXPECT_EQ(results[3].asString(),"d408cece28e6ae98d3cd8d21989b62255e9e2b429e2e582039ae9a83f9c47d50");
        EXPECT_EQ(results[4].asString(),"32c7b27e9533c637cc6c5c15e42b6a06c776184bd73a3bef64b2999a705eb122");
        EXPECT_EQ(results[5].asString(),"3348f253524c909c5f9b8f3114f4b61ae68d3640138d76fa4f7b5312569d4458");
        EXPECT_EQ(results[6].asString(),"742a6c711c1359d799310cb7ac6bdcb3d86b33c0896cca5a7681ecac97afc497");
        EXPECT_EQ(results[7].asString(),"8992f848247b6ddefe480e7770289697e75f7e657d194ceb0571ae6f6254b138");
        EXPECT_EQ(results[8].asString(),"9c8f8d879cca60d83a4561b4db015f953b6cf02b7e488e3bc6adf1ed5a8ef910");
        EXPECT_EQ(results[9].asString(),"9ee5c2045e338ea13aac311d4068c63e3160550f7ab7a66ae0862edf46bb85e1");
        utils::printJson(results);
    } catch (...) {
        EXPECT_TRUE(false);
    }
}
