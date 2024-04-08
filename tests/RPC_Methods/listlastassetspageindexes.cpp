//
// Created by mctrivia on 07/04/24.
//

#include "AppMain.h"
#include "utils.h"
#include "RPC/MethodList.h"
#include "gtest/gtest.h"
#include "../tests/RPCMethods.h"

using namespace std;


TEST_F(RPCMethodsTest, listlastassetspageindexes) {
    ///rpc method we will be testing(if using as reference make sure you change value above and bellow this line)
    const std::string METHOD="listlastassetspageindexes";
    bool result;

    //test invalid parameters throws an exception of type DigiByteException
    //3 parameters
    try {
        Json::Value params=Json::arrayValue;
        params.append(5);
        params.append(1);
        params.append(true);
        RPC::methods[METHOD](params);
        result=false;
    } catch (const DigiByteException& e) {
        result=true;
    } catch (...) {
        result=false;
    }
    EXPECT_TRUE(result);

    //Pages of 100
    try {
        Json::Value params=Json::arrayValue;
        params.append(100);
        auto results=RPC::methods[METHOD](params).toJSON(1)["result"];
        EXPECT_TRUE(results.isArray());
        EXPECT_EQ(results.size(),47);
        unsigned int i=0;
        for (int assetIndex=4634;assetIndex>0;assetIndex-=100) {
            EXPECT_TRUE(results[i]["skips"].isArray());
            EXPECT_TRUE(results[i]["skips"].empty());
            EXPECT_EQ(results[i]["start"].asUInt(), assetIndex);
            i++;
        }
    } catch (...) {
        EXPECT_TRUE(false);
    }
}
