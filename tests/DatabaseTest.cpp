//
// Tests for Database class — uses a temporary database file.
// Extends the basic constructor test already in DatabaseChain.cpp.
//

#include "AppMain.h"
#include "Database.h"
#include "gtest/gtest.h"

#include <cstdio>
#include <string>

using namespace std;

static const string DB_PATH = "../tests/testFiles/_database_test.db";

class DatabaseTest : public ::testing::Test {
protected:
    Database* db = nullptr;

    void SetUp() override {
        remove(DB_PATH.c_str());
        remove((DB_PATH + "-wal").c_str());
        remove((DB_PATH + "-shm").c_str());
        db = new Database(DB_PATH);
    }

    void TearDown() override {
        delete db;
        db = nullptr;
        remove(DB_PATH.c_str());
        remove((DB_PATH + "-wal").c_str());
        remove((DB_PATH + "-shm").c_str());
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DatabaseTest, constructor_createsDatabase) {
    // If constructor throws, SetUp would have failed — reaching here means success
    EXPECT_NE(db, nullptr);
}

TEST_F(DatabaseTest, constructor_invalidPath_throws) {
    EXPECT_THROW(
        Database("/nonexistent_path/cannot_create.db"),
        Database::exceptionFailedToOpen
    );
}

// ─────────────────────────────────────────────────────────────────────────────
// Block table
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DatabaseTest, getBlockHeight_initiallyAtGenesis) {
    // Database constructor inserts the DigiAsset genesis block at height 1
    EXPECT_EQ(db->getBlockHeight(), 1u);
}

TEST_F(DatabaseTest, insertBlock_and_getBlockHeight) {
    db->insertBlock(2, "0000000000000000000000000000000000000000000000000000000000000002",
                    1389392877, 1, 1.0);
    EXPECT_EQ(db->getBlockHeight(), 2u);
}

TEST_F(DatabaseTest, getBlockHash_returnsCorrectHash) {
    const string hash = "0000000000000000000000000000000000000000000000000000000000000002";
    db->insertBlock(2, hash, 1389392877, 1, 1.0);
    EXPECT_EQ(db->getBlockHash(2), hash);
}

TEST_F(DatabaseTest, insertMultipleBlocks_heightUpdates) {
    db->insertBlock(2, "0000000000000000000000000000000000000000000000000000000000000002", 1000, 1, 1.0);
    db->insertBlock(3, "0000000000000000000000000000000000000000000000000000000000000003", 1001, 1, 1.0);
    db->insertBlock(4, "0000000000000000000000000000000000000000000000000000000000000004", 1002, 1, 1.0);
    EXPECT_EQ(db->getBlockHeight(), 4u);
}

TEST_F(DatabaseTest, isHeightIndexed_onlyBelowNewestBlock) {
    db->insertBlock(2, "0000000000000000000000000000000000000000000000000000000000000002", 1000, 1, 1.0);
    db->insertBlock(3, "0000000000000000000000000000000000000000000000000000000000000003", 1001, 1, 1.0);

    // the newest block in the database is the one about to be processed, so it is not done yet
    EXPECT_TRUE(db->isHeightIndexed(2));
    EXPECT_FALSE(db->isHeightIndexed(3));
    EXPECT_FALSE(db->isHeightIndexed(4));
}

TEST_F(DatabaseTest, isHeightIndexed_zeroMeansUnknown) {
    // 0 is what callers pass when they don't know when the output was created
    EXPECT_FALSE(db->isHeightIndexed(0));
}

// ─────────────────────────────────────────────────────────────────────────────
// KYC table
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DatabaseTest, addKYC_and_getAddressKYC) {
    db->insertBlock(100, "0000000000000000000000000000000000000000000000000000000000000064", 1000000, 1, 1.0);
    db->addKYC("dgb1qtest", "USA", "Alice Smith", "0000000000000000000000000000000000000000000000000000000000000064", 100);
    KYC kyc = db->getAddressKYC("dgb1qtest");
    EXPECT_FALSE(kyc.empty());
    EXPECT_EQ(kyc.getAddress(), "dgb1qtest");
    EXPECT_EQ(kyc.getCountry(), "USA");
    EXPECT_EQ(kyc.getName(), "Alice Smith");
    EXPECT_TRUE(kyc.valid(100));
}

TEST_F(DatabaseTest, revokeKYC_makesInvalid) {
    db->insertBlock(100, "0000000000000000000000001000000000000000000000000000000000000064", 1000000, 1, 1.0);
    db->addKYC("dgb1qrevoke", "USA", "Bob", "0000000000000000000000001000000000000000000000000000000000000064", 100);
    db->insertBlock(200, "00000000000000000000000010000000000000000000000000000000000000c8", 2000000, 1, 1.0);
    db->revokeKYC("dgb1qrevoke", 200);
    KYC kyc = db->getAddressKYC("dgb1qrevoke");
    EXPECT_TRUE(kyc.valid(150));
    EXPECT_FALSE(kyc.valid(200));
}

// ─────────────────────────────────────────────────────────────────────────────
// Pruning flags
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DatabaseTest, pruneFlags_defaultValues) {
    EXPECT_EQ(db->getBeenPrunedExchangeHistory(), -1);
    EXPECT_EQ(db->getBeenPrunedUTXOHistory(), -1);
    EXPECT_EQ(db->getBeenPrunedVoteHistory(), -1);
    EXPECT_FALSE(db->getBeenPrunedNonAssetUTXOHistory());
}

TEST_F(DatabaseTest, pruneFlags_canBeSet) {
    db->setBeenPrunedExchangeHistory(500);
    EXPECT_EQ(db->getBeenPrunedExchangeHistory(), 500);

    db->setBeenPrunedNonAssetUTXOHistory(true);
    EXPECT_TRUE(db->getBeenPrunedNonAssetUTXOHistory());
}

// ─────────────────────────────────────────────────────────────────────────────
// Watch address table
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DatabaseTest, isWatchAddress_notWatchedByDefault) {
    EXPECT_FALSE(db->isWatchAddress("dgb1qunwatched"));
}

TEST_F(DatabaseTest, addWatchAddress_isNowWatched) {
    db->addWatchAddress("dgb1qwatched");
    EXPECT_TRUE(db->isWatchAddress("dgb1qwatched"));
    EXPECT_FALSE(db->isWatchAddress("dgb1qother"));
}

// ─────────────────────────────────────────────────────────────────────────────
// Encrypted keys table
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DatabaseTest, addEncryptedKey_and_getEncryptedKey) {
    vector<uint8_t> keyData = {0xDE, 0xAD, 0xBE, 0xEF};
    Blob blob(keyData);
    db->addEncryptedKey("dgb1qkeyaddr", blob);
    Blob retrieved = db->getEncryptedKey("dgb1qkeyaddr");
    EXPECT_EQ(retrieved.toHex(), blob.toHex());
}

TEST_F(DatabaseTest, getEncryptedKey_nonExistent_throws) {
    EXPECT_THROW(
        db->getEncryptedKey("dgb1qnonexistent"),
        Database::exceptionFailedSelect
    );
}

// ─────────────────────────────────────────────────────────────────────────────
// Transaction support
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DatabaseTest, startAndEndTransaction_doesNotThrow) {
    EXPECT_NO_THROW({
        db->startTransaction();
        db->insertBlock(2, "0000000000000000000000000000000000000000000000000000000000000002", 1000, 1, 1.0);
        db->endTransaction();
    });
    EXPECT_EQ(db->getBlockHeight(), 2u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Exchange rate watch
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DatabaseTest, addExchangeRate_doesNotThrow) {
    EXPECT_NO_THROW(
        db->addExchangeRate("dgb1qexchange", 0, 1, 0.001)
    );
}
