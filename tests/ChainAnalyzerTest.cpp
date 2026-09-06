//
// Tests for ChainAnalyzer — uses loadFake() to avoid needing a live node.
//

#include "AppMain.h"
#include "ChainAnalyzer.h"
#include "Database.h"
#include "gtest/gtest.h"

#include <chrono>
#include <cstdio>
#include <string>
#include <thread>

using namespace std;

static const string DB_PATH = "../tests/testFiles/_chainanalyzer_test.db";

class ChainAnalyzerTest : public ::testing::Test {
protected:
    AppMain* appMain = nullptr;
    Database* db = nullptr;
    ChainAnalyzer* analyzer = nullptr;

    void SetUp() override {
        remove(DB_PATH.c_str());
        remove((DB_PATH + "-wal").c_str());
        remove((DB_PATH + "-shm").c_str());

        appMain = AppMain::GetInstance();
        db = new Database(DB_PATH);
        appMain->setDatabase(db);

        analyzer = new ChainAnalyzer();
    }

    void TearDown() override {
        appMain->reset();
        delete analyzer;
        delete db;
        remove(DB_PATH.c_str());
        remove((DB_PATH + "-wal").c_str());
        remove((DB_PATH + "-shm").c_str());
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Initial state
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ChainAnalyzerTest, initialState_isStopped) {
    EXPECT_EQ(analyzer->getSync(), +ChainAnalyzer::STOPPED);
}

// ─────────────────────────────────────────────────────────────────────────────
// loadFake() — sets state without needing a live node
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ChainAnalyzerTest, loadFake_setsSyncedState) {
    analyzer->loadFake(1000, +ChainAnalyzer::SYNCED);
    EXPECT_EQ(analyzer->getSync(), +ChainAnalyzer::SYNCED);
}

TEST_F(ChainAnalyzerTest, loadFake_setsSyncHeight) {
    analyzer->loadFake(17579454, +ChainAnalyzer::SYNCED);
    EXPECT_EQ(analyzer->getSyncHeight(), 17579454u);
}

TEST_F(ChainAnalyzerTest, loadFake_setsNegativeSyncLevel) {
    // Negative sync = how many blocks behind
    analyzer->loadFake(1000, -50);
    EXPECT_EQ(analyzer->getSync(), -50);
}

TEST_F(ChainAnalyzerTest, loadFake_stoppedState) {
    analyzer->loadFake(500, +ChainAnalyzer::STOPPED);
    EXPECT_EQ(analyzer->getSync(), +ChainAnalyzer::STOPPED);
}

TEST_F(ChainAnalyzerTest, loadFake_initializingState) {
    analyzer->loadFake(0, +ChainAnalyzer::INITIALIZING);
    EXPECT_EQ(analyzer->getSync(), +ChainAnalyzer::INITIALIZING);
}

// ─────────────────────────────────────────────────────────────────────────────
// Config setters (no assertions on stored values — just verify no crash)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ChainAnalyzerTest, setPruneAge_doesNotThrow) {
    EXPECT_NO_THROW(analyzer->setPruneAge(1440));
    EXPECT_NO_THROW(analyzer->setPruneAge(-1));
}

TEST_F(ChainAnalyzerTest, setPruneFlags_doesNotThrow) {
    EXPECT_NO_THROW(analyzer->setPruneExchangeHistory(true));
    EXPECT_NO_THROW(analyzer->setPruneUTXOHistory(false));
    EXPECT_NO_THROW(analyzer->setPruneVoteHistory(true));
    EXPECT_NO_THROW(analyzer->setStoreNonAssetUTXO(false));
}

// ─────────────────────────────────────────────────────────────────────────────
// Constants
// ─────────────────────────────────────────────────────────────────────────────

TEST(ChainAnalyzer_Constants, stateConstants_areDistinct) {
    EXPECT_NE(+ChainAnalyzer::SYNCED,       +ChainAnalyzer::STOPPED);
    EXPECT_NE(+ChainAnalyzer::STOPPED,      +ChainAnalyzer::INITIALIZING);
    EXPECT_NE(+ChainAnalyzer::INITIALIZING, +ChainAnalyzer::REWINDING);
    EXPECT_NE(+ChainAnalyzer::REWINDING,    +ChainAnalyzer::BUSY);
}

TEST(ChainAnalyzer_Constants, synced_isZero) {
    EXPECT_EQ(+ChainAnalyzer::SYNCED, 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// Stall watchdog
// ─────────────────────────────────────────────────────────────────────────────
//
// The analyzer only logs once a block completes, so a step that never returns - a wedged IPFS
// daemon, an unreachable storage pool server - produced no output at all.  "Is anyone's node
// stuck on block 24,081,128?" had to be answered by reading the chain because the log said
// nothing after the previous block.
//
// Driving the watchdog directly here rather than through a running analyzer: the real thing
// needs a live DigiByte Core, and a one second threshold keeps the test to a few seconds.

class WatchdogProbe : public ChainAnalyzer {
public:
    explicit WatchdogProbe(unsigned int stallSeconds) {
        _stallWarningSeconds = stallSeconds;
    }
    using ChainAnalyzer::watchdogIdle;
    using ChainAnalyzer::watchdogStart;
    using ChainAnalyzer::watchdogStop;
    using ChainAnalyzer::watchdogWorkingOn;

    unsigned int warnings() const { return _stallWarnings.load(); }
};

TEST(ChainAnalyzerWatchdog, reportsAStepThatDoesNotFinish) {
    WatchdogProbe probe(1);
    probe.watchdogStart();
    probe.watchdogWorkingOn("block 24081128 transaction dd86362f6d765181e5bd90503b85166ead6e39e50971b3c66ed10f6cffe64cc8");
    this_thread::sleep_for(chrono::milliseconds(2500));
    probe.watchdogStop();

    EXPECT_GE(probe.warnings(), 1u) << "a step running well past the limit must be reported";
}

TEST(ChainAnalyzerWatchdog, keepsReportingWhileTheStepIsStillRunning) {
    WatchdogProbe probe(1);
    probe.watchdogStart();
    probe.watchdogWorkingOn("block 24081128 transaction dd86362f");
    this_thread::sleep_for(chrono::milliseconds(3500));
    probe.watchdogStop();

    EXPECT_GE(probe.warnings(), 2u) << "one warning and then silence looks the same as recovering";
}

TEST(ChainAnalyzerWatchdog, saysNothingWhileIdle) {
    WatchdogProbe probe(1);
    probe.watchdogStart();
    probe.watchdogWorkingOn("block 24081128 transaction dd86362f");
    probe.watchdogIdle(); //eg the node is synced and waiting for the chain to move
    this_thread::sleep_for(chrono::milliseconds(2500));
    probe.watchdogStop();

    EXPECT_EQ(probe.warnings(), 0u) << "a synced node must not warn about waiting for a block";
}

TEST(ChainAnalyzerWatchdog, saysNothingWhileStepsKeepFinishing) {
    WatchdogProbe probe(2);
    probe.watchdogStart();
    for (int i = 0; i < 10; i++) {
        probe.watchdogWorkingOn("block 24081128 transaction " + to_string(i));
        this_thread::sleep_for(chrono::milliseconds(250));
    }
    probe.watchdogIdle();
    probe.watchdogStop();

    EXPECT_EQ(probe.warnings(), 0u) << "elapsed is per step, not since the watchdog started";
}
