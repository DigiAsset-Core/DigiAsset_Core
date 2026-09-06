//
// Created by DigiAsset Core on 14/07/26.
//

#include "../tests/RPCMethods.h"
#include "AppMain.h"
#include "RPC/MethodList.h"
#include "utils.h"
#include "gtest/gtest.h"

using namespace std;


///Only parameter validation can be tested here.  The full result depends on what the
///connected wallet holds which the test environment can't control(see LAST_TASKS_NOTES.md).
TEST_F(RPCMethodsTest, getwalletbalances) {
    ///rpc method we will be testing(if using as reference make sure you change value above and bellow this line)
    const std::string METHOD = "getwalletbalances";
    bool result;

    //test invalid parameters throws an exception of type DigiByteException
    //2 parameters(too many)
    try {
        Json::Value params = Json::arrayValue;
        params.append(1);
        params.append(1);
        RPC::methods[METHOD](params);
        result = false;
    } catch (const DigiByteException& e) {
        result = true;
    } catch (...) {
        result = false;
    }
    EXPECT_TRUE(result);

    //minconf is not an integer
    try {
        Json::Value params = Json::arrayValue;
        params.append("bad");
        RPC::methods[METHOD](params);
        result = false;
    } catch (const DigiByteException& e) {
        result = true;
    } catch (...) {
        result = false;
    }
    EXPECT_TRUE(result);

    //minconf is negative
    try {
        Json::Value params = Json::arrayValue;
        params.append(-1);
        RPC::methods[METHOD](params);
        result = false;
    } catch (const DigiByteException& e) {
        result = true;
    } catch (...) {
        result = false;
    }
    EXPECT_TRUE(result);
}
