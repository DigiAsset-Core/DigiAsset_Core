//
// Tests for RPC::Methods::shutdown — uses RPCMethodsTest fixture.
//

#include "AppMain.h"
#include "RPC/MethodList.h"
#include "gtest/gtest.h"
#include "../tests/RPCMethods.h"

#include <csignal>
#include <jsoncpp/json/value.h>

using namespace std;

TEST_F(RPCMethodsTest, shutdown_returnsTrue) {
    //shutdown raises SIGTERM for the daemon's main thread to finish the job; the test
    //binary has no handler installed so ignore it or the whole test run dies here
    auto previousHandler = std::signal(SIGTERM, SIG_IGN);
    Json::Value params = Json::arrayValue;
    auto response = RPC::Methods::shutdown(params);
    std::signal(SIGTERM, previousHandler);
    Json::Value json = response.toJSON(1);
    // shutdown always returns true
    EXPECT_TRUE(json["result"].isBool());
    EXPECT_TRUE(json["result"].asBool());
}
