//
// Tests for the local Permanent Storage Pool (src/PermanentStoragePool/pools/local.cpp)
//
// The local pool keeps its own sqlite database("local.db" in the working directory).
// These tests create and remove that file so they must not run in a directory with a
// real local.db(the test build's bin/ folder is fine).
//
// Includes a regression test for the localExists() inversion bug: before the fix the pool
// ignored its own database whenever it existed, so subscriptions were lost on restart.
//

#include "DigiAsset.h"
#include "DigiByteTransaction.h"
#include "IPFS.h"
#include "PermanentStoragePool/pools/local.h"
#include "utils.h"
#include "gtest/gtest.h"
#include <cstdio>

using namespace std;

namespace {
    ///expose the protected bad-reporting function(base class version needs an AppMain database)
    struct testableLocal : public local {
        using local::_reportAssetBad;
    };

    ///builds an issuance transaction with the given metadata cid
    DigiByteTransaction makeIssuance(const string& sha256hex) {
        DigiAsset asset(IPFS::sha256ToCID(sha256hex), 1000, 0, true, DigiAsset::AGGREGABLE);
        DigiByteTransaction tx;
        tx.setIssuance(asset);
        tx.addDigiAssetOutput("dgb1qrecipient", {asset});
        return tx;
    }
} // namespace

TEST(LocalPool, poolLifecycle) {
    remove(PSP_LOCAL_DB_FILENAME); //make sure no leftover state

    DigiByteTransaction txA = makeIssuance("1111111111111111111111111111111111111111111111111111111111111111");
    DigiByteTransaction txB = makeIssuance("2222222222222222222222222222222222222222222222222222222222222222");

    {
        testableLocal pool;

        //no pool database: nothing is part of the pool and checking must NOT create the file
        EXPECT_EQ(pool.serializeMetaProcessor(txA), "");
        EXPECT_FALSE(utils::fileExists(PSP_LOCAL_DB_FILENAME));
        EXPECT_FALSE(pool.isAssetBad("anyAsset"));
        EXPECT_FALSE(utils::fileExists(PSP_LOCAL_DB_FILENAME));

        //enabling a transaction creates the database and registers the metadata cid
        pool.enable(txA);
        EXPECT_TRUE(utils::fileExists(PSP_LOCAL_DB_FILENAME));
        EXPECT_EQ(pool.serializeMetaProcessor(txA), "1");
        EXPECT_EQ(pool.serializeMetaProcessor(txB), ""); //different cid not in pool

        //bad asset tracking
        pool._reportAssetBad("badAsset123");
        EXPECT_TRUE(pool.isAssetBad("badAsset123"));
        EXPECT_FALSE(pool.isAssetBad("goodAsset456"));

        //meta processor pins everything
        auto processor = pool.deserializeMetaProcessor("1");
        EXPECT_NE(processor, nullptr);
    }

    //a fresh pool instance must see the persisted state(regression: localExists() was
    //inverted so this returned "" before the fix)
    {
        testableLocal pool2;
        EXPECT_EQ(pool2.serializeMetaProcessor(txA), "1");
        EXPECT_TRUE(pool2.isAssetBad("badAsset123"));
    }

    remove(PSP_LOCAL_DB_FILENAME); //clean up
}

TEST(LocalPool, metadata) {
    local pool;
    EXPECT_EQ(pool.getName(), "Local Storage");
    EXPECT_EQ(pool.getURL(), "NA");
    EXPECT_FALSE(pool.getDescription().empty());
    DigiByteTransaction tx = makeIssuance("3333333333333333333333333333333333333333333333333333333333333333");
    EXPECT_EQ(pool.getCost(tx), (uint64_t) 0);
}
