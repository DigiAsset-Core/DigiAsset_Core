//
// Created by mctrivia on 01/04/24.
//

#include "AppMain.h"
#include "utils.h"
#include "RPC/MethodList.h"
#include "gtest/gtest.h"
#include "../tests/RPCMethods.h"

using namespace std;


TEST_F(RPCMethodsTest, getipfscount) {
    ///rpc method we will be testing(if using as reference make sure you change value above and bellow this line)
    const std::string METHOD="getipfscount";
    bool result;

    //test invalid parameters throws an exception of type DigiByteException
    //1 parameters
    try {
        Json::Value params=Json::arrayValue;
        params.append(15000000);
        RPC::methods[METHOD](params);
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
        auto results=RPC::methods[METHOD](params).toJSON(1)["result"];
        EXPECT_TRUE(results.isUInt());
    } catch (...) {
        result=false;
    }
    EXPECT_TRUE(result);
}
