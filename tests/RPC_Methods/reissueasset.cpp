//
// Tests for RPC::Methods::reissueasset — uses RPCMethodsTest fixture.
//

#include "../tests/RPCMethods.h"
#include "AppMain.h"
#include "RPC/MethodList.h"
#include "utils.h"
#include "gtest/gtest.h"

using namespace std;


///Only parameter validation can be tested here.  Actually reissuing needs a wallet that
///controls an issuer address which the test environment doesn't have(see LAST_TASKS_NOTES.md).
TEST_F(RPCMethodsTest, reissueasset) {
    ///rpc method we will be testing(if using as reference make sure you change value above and bellow this line)
    const std::string METHOD = "reissueasset";
    bool result;

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

    //1 parameter(too few)
    try {
        Json::Value params = Json::arrayValue;
        params.append(5);
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

    //DigiByte(assetIndex 1) is not an asset
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

    //locked assets can not be reissued(assetIndex 2 in the test db is locked)
    try {
        Json::Value params = Json::arrayValue;
        params.append(2);
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
        options["dryrun"] = 1;
        Json::Value params = Json::arrayValue;
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
