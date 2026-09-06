//
// Tests for RPC::MethodList — verifies that the method map is populated
// with all expected methods. No AppMain or external dependencies required.
//

#include "RPC/MethodList.h"
#include "gtest/gtest.h"

#include <string>
#include <vector>

using namespace std;

// ─────────────────────────────────────────────────────────────────────────────
// Method map population
// ─────────────────────────────────────────────────────────────────────────────

TEST(RPC_MethodList, methodMap_isNotEmpty) {
    EXPECT_FALSE(RPC::methods.empty());
}

TEST(RPC_MethodList, methodMap_containsExpectedMethods) {
    const vector<string> expected = {
        "addressstats",
        "algostats",
        "asyncclear",
        "asyncget",
        "asyncstart",
        "debugwaittimes",
        "getaddressholdings",
        "getaddresskyc",
        "getassetdata",
        "getassetholders",
        "getassetindexes",
        "getdgbequivalent",
        "getdomainaddress",
        "getencryptedkey",
        "getexchangerates",
        "getipfscount",
        "getpsp",
        "getrandom",
        "getrawtransaction",
        "gettxout",
        "listaddresshistory",
        "listassetissuances",
        "listassets",
        "listlastassets",
        "listlastassetspageindexes",
        "listlastblocks",
        "listunspent",
        "resyncmetadata",
        "send",
        "sendmany",
        "sendtoaddress",
        "shutdown",
        "syncstate",
        "version",
    };

    for (const auto& name : expected) {
        EXPECT_NE(RPC::methods.find(name), RPC::methods.end())
            << "Method '" << name << "' not found in RPC::methods map";
    }
}

TEST(RPC_MethodList, methodMap_callablesAreNonNull) {
    for (const auto& entry : RPC::methods) {
        EXPECT_TRUE(static_cast<bool>(entry.second))
            << "Method '" << entry.first << "' has null callable";
    }
}
