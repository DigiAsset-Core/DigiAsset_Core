//
// Created by mctrivia on 01/04/24.
//

#include "AppMain.h"
#include "utils.h"
#include "BitcoinRpcServerMethods.h"
#include "gtest/gtest.h"
#include "../tests/RPCMethods.h"

using namespace std;


TEST_F(RPCMethodsTest, getpsp) {
    ///rpc method we will be testing(if using as reference make sure you change value above and bellow this line)
    const std::string METHOD="getpsp";
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
        params.append(1);
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
    //non existant psp
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

    //test proper usage
    try {
        Json::Value params=Json::arrayValue;
        params.append(1);
        auto results=rpcMethods[METHOD](params);
        EXPECT_TRUE(results.isObject());
        EXPECT_EQ(results["description"],"Originally operated by digiassetX Inc and continued to run by Matthew Cornelisse.  This pool makes sure asset metadata is always available and pays others DigiAsset Core nodes to help distribute the metadata.");
        EXPECT_EQ(results["files"].size(),3098);
        EXPECT_EQ(results["files"][0].asString(),"QmNMnCP97azWtKD4dV4VKavgoxkbpuF1LLa6wURyoUgMrY");
        EXPECT_EQ(results["files"][1].asString(),"QmNNajyRMQBtGfN59RMpksRrQXHGLh3avgeHmvQB8JWzFn");
        EXPECT_EQ(results["files"][2].asString(),"QmNNgHEZekd47AWKubnse6gkLxeZ5j6d7bbqKKS4nU8x2c");
        EXPECT_EQ(results["name"].asString(),"MCTrivia's PSP");
        EXPECT_EQ(results["url"].asString(),"https://ipfs.digiassetx.com");
    } catch (...) {
        result=false;
    }
    EXPECT_TRUE(result);
}
