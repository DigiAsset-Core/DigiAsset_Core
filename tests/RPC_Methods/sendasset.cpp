//
// Created by DigiAsset Core on 14/07/26.
//

#include "../tests/RPCMethods.h"
#include "AppMain.h"
#include "RPC/MethodList.h"
#include "utils.h"
#include "gtest/gtest.h"

using namespace std;


///Only parameter validation can be tested here.  Actually sending assets needs a wallet
///holding assets which the test environment doesn't have(see LAST_TASKS_NOTES.md).
TEST_F(RPCMethodsTest, sendasset) {
    ///rpc method we will be testing(if using as reference make sure you change value above and bellow this line)
    const std::string METHOD = "sendasset";
    bool result;

    //test invalid parameters throws an exception of type DigiByteException
    //0 parameters
    try {
        Json::Value params = Json::arrayValue;
        RPC::methods[METHOD](params);
        result = false;
    } catch (const DigiByteException& e) {
        result = true;
    } catch (...) {
        result = false;
    }
    EXPECT_TRUE(result);

    //2 parameters(too few)
    try {
        Json::Value params = Json::arrayValue;
        params.append("dgb1qtqt4vrsjfnncr7wjvdvhw7evgzsyj39kaxhg6z");
        params.append(3);
        RPC::methods[METHOD](params);
        result = false;
    } catch (const DigiByteException& e) {
        result = true;
    } catch (...) {
        result = false;
    }
    EXPECT_TRUE(result);

    //5 parameters(too many)
    try {
        Json::Value params = Json::arrayValue;
        params.append("dgb1qtqt4vrsjfnncr7wjvdvhw7evgzsyj39kaxhg6z");
        params.append(3);
        params.append(1);
        params.append(Json::objectValue);
        params.append("extra");
        RPC::methods[METHOD](params);
        result = false;
    } catch (const DigiByteException& e) {
        result = true;
    } catch (...) {
        result = false;
    }
    EXPECT_TRUE(result);

    //address is not a string
    try {
        Json::Value params = Json::arrayValue;
        params.append(5);
        params.append(3);
        params.append(1);
        RPC::methods[METHOD](params);
        result = false;
    } catch (const DigiByteException& e) {
        result = true;
    } catch (...) {
        result = false;
    }
    EXPECT_TRUE(result);

    //asset is not a string or int
    try {
        Json::Value params = Json::arrayValue;
        params.append("dgb1qtqt4vrsjfnncr7wjvdvhw7evgzsyj39kaxhg6z");
        params.append(Json::objectValue);
        params.append(1);
        RPC::methods[METHOD](params);
        result = false;
    } catch (const DigiByteException& e) {
        result = true;
    } catch (...) {
        result = false;
    }
    EXPECT_TRUE(result);

    //asset that doesn't exist
    try {
        Json::Value params = Json::arrayValue;
        params.append("dgb1qtqt4vrsjfnncr7wjvdvhw7evgzsyj39kaxhg6z");
        params.append("La3VNKcPCzUnryZcHr1ZJDL74LGLzQzzYzzzzz");
        params.append(1);
        RPC::methods[METHOD](params);
        result = false;
    } catch (const DigiByteException& e) {
        result = true;
    } catch (...) {
        result = false;
    }
    EXPECT_TRUE(result);

    //sending DigiByte(assetIndex 1) is not allowed
    try {
        Json::Value params = Json::arrayValue;
        params.append("dgb1qtqt4vrsjfnncr7wjvdvhw7evgzsyj39kaxhg6z");
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

    //dryrun of wrong type rejected
    try {
        Json::Value options = Json::objectValue;
        options["dryrun"] = "yes";
        Json::Value params = Json::arrayValue;
        params.append("dgb1qtqt4vrsjfnncr7wjvdvhw7evgzsyj39kaxhg6z");
        params.append(2);
        params.append(1);
        params.append(options);
        RPC::methods[METHOD](params);
        result = false;
    } catch (const DigiByteException& e) {
        result = true;
    } catch (...) {
        result = false;
    }
    EXPECT_TRUE(result);
}
