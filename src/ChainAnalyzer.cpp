//
// Created by mctrivia on 02/02/23.
//

#include "ChainAnalyzer.h"
#include "EventBroadcaster.h"
#include "AppMain.h"
#include "BitIO.h"
#include "Config.h"
#include "Database.h"
#include "DigiAsset.h"
#include "DigiAssetConstants.h"
#include "DigiDollar.h"
#include "DigiByteCore.h"
#include "DigiByteTransaction.h"
#include "KYC.h"
#include "Log.h"
#include "PermanentStoragePool/PermanentStoragePoolList.h"
#include "utils.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <thread>

using namespace std;


std::map<std::string, int> ChainAnalyzer::_pinAssetExtraMimeTypes;

/*
 ██████╗ ██████╗ ███╗   ██╗███████╗████████╗██████╗ ██╗   ██╗ ██████╗████████╗ ██████╗ ██████╗
██╔════╝██╔═══██╗████╗  ██║██╔════╝╚══██╔══╝██╔══██╗██║   ██║██╔════╝╚══██╔══╝██╔═══██╗██╔══██╗
██║     ██║   ██║██╔██╗ ██║███████╗   ██║   ██████╔╝██║   ██║██║        ██║   ██║   ██║██████╔╝
██║     ██║   ██║██║╚██╗██║╚════██║   ██║   ██╔══██╗██║   ██║██║        ██║   ██║   ██║██╔══██╗
╚██████╗╚██████╔╝██║ ╚████║███████║   ██║   ██║  ██║╚██████╔╝╚██████╗   ██║   ╚██████╔╝██║  ██║
 ╚═════╝ ╚═════╝ ╚═╝  ╚═══╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝  ╚═════╝   ╚═╝    ╚═════╝ ╚═╝  ╚═╝
 */

ChainAnalyzer::ChainAnalyzer() {
    //reset config variables
    resetConfig();
}

ChainAnalyzer::~ChainAnalyzer() {
    stop();
    watchdogStop(); //shutdownFunction does this on the normal path; belt and braces for the rest
}

/*
 ██████╗ ██████╗ ███╗   ██╗███████╗██╗ ██████╗
██╔════╝██╔═══██╗████╗  ██║██╔════╝██║██╔════╝
██║     ██║   ██║██╔██╗ ██║█████╗  ██║██║  ███╗
██║     ██║   ██║██║╚██╗██║██╔══╝  ██║██║   ██║
╚██████╗╚██████╔╝██║ ╚████║██║     ██║╚██████╔╝
 ╚═════╝ ╚═════╝ ╚═╝  ╚═══╝╚═╝     ╚═╝ ╚═════╝
 */

void ChainAnalyzer::resetConfig() {
    stop();

    //default state values
    _height = 1;
    _nextHash = "";

    //default config values(chain data)
    _pruneAge = 5760; //number of blocks to keep for roll back protection(-1 don't prune, default is 1 day)
    _pruneInterval = (int) ceil(_pruneAge / PRUNE_INTERVAL_DIVISOR / 100) * 100;
    _pruneExchangeHistory = true;
    _pruneUTXOHistory = true;
    _pruneVoteHistory = true;
    _verifyDatabaseWrite = true;
    _showAllBlockSyncTime = false;
    _trackDigiDollar = true;
}

/**
 * Changes what config file we should use
 * @param fileName
 */
void ChainAnalyzer::setFileName(const std::string& fileName) {
    //make change
    _configFileName = fileName;

    //make sure chain analyzer is shut down and reset
    resetConfig();

    //if file exists load it
    try {
        loadConfig();
    } catch (const Config::exceptionConfigFileMissing& e) {
        //no config file so just ignore
    }
}

void ChainAnalyzer::loadConfig() {
    Config config = Config(_configFileName);

    //load values in to class(chain data)
    setPruneAge(config.getInteger("pruneage", 5760)); //-1 for don't prune, default daily
    setPruneExchangeHistory(config.getBool("pruneexchangehistory", true));
    setPruneUTXOHistory(config.getBool("pruneutxohistory", true));
    setPruneVoteHistory(config.getBool("prunevotehistory", true));
    setStoreNonAssetUTXO(config.getBool("storenonassetutxo", false));
    setTrackDigiDollar(config.getBool("trackdigidollar", true));
    _verifyDatabaseWrite = config.getBool("verifydatabasewrite", true);
    _showAllBlockSyncTime = config.getBool("showallblocksynctimes", false);
}

/**
 * This function is used for testing purposes.  It allows creating the object without starting but still having realistic values
 * @param databaseHeight
 * @param syncLevel
 */
void ChainAnalyzer::loadFake(unsigned int databaseHeight, int syncLevel) {
    _height = databaseHeight;
    _state = syncLevel;
}

void ChainAnalyzer::saveConfig() {
    Config config = Config(_configFileName);
    config.setInteger("pruneage", _pruneAge);
    config.setBool("pruneexchangehistory", _pruneExchangeHistory);
    config.setBool("pruneutxohistory", _pruneUTXOHistory);
    config.setBool("prunevotehistory", _pruneVoteHistory);
    config.setBool("storenonassetutxo", _storeNonAssetUTXOs);
    config.setBool("trackdigidollar", _trackDigiDollar);
    config.setIntegerMap("pinassetextra", _pinAssetExtraMimeTypes);
    config.write();
}


bool ChainAnalyzer::shouldPruneExchangeHistory() const {
    return _pruneExchangeHistory;
}

void ChainAnalyzer::setPruneExchangeHistory(bool shouldPrune) {
    Database* db = AppMain::GetInstance()->getDatabase();
    if (!shouldPrune && (db->getBeenPrunedExchangeHistory() >= 0)) throw exceptionAlreadyPruned();
    _pruneExchangeHistory = shouldPrune;
}

bool ChainAnalyzer::shouldPruneUTXOHistory() const {
    return _pruneUTXOHistory;
}

void ChainAnalyzer::setPruneUTXOHistory(bool shouldPrune) {
    Database* db = AppMain::GetInstance()->getDatabase();
    if (!shouldPrune && (db->getBeenPrunedUTXOHistory() >= 0)) throw exceptionAlreadyPruned();
    _pruneUTXOHistory = shouldPrune;
}

bool ChainAnalyzer::shouldPruneVoteHistory() const {
    return _pruneVoteHistory;
}

void ChainAnalyzer::setPruneVoteHistory(bool shouldPrune) {
    Database* db = AppMain::GetInstance()->getDatabase();
    if (!shouldPrune && (db->getBeenPrunedVoteHistory() >= 0)) throw exceptionAlreadyPruned();
    _pruneVoteHistory = shouldPrune;
}

bool ChainAnalyzer::shouldStoreNonAssetUTXO() const {
    return _storeNonAssetUTXOs;
}

void ChainAnalyzer::setStoreNonAssetUTXO(bool shouldStore) {
    Database* db = AppMain::GetInstance()->getDatabase();
    if (shouldStore && (db->getBeenPrunedNonAssetUTXOHistory())) throw exceptionAlreadyPruned();
    _storeNonAssetUTXOs = shouldStore;
}

bool ChainAnalyzer::shouldTrackDigiDollar() const {
    return _trackDigiDollar;
}

void ChainAnalyzer::setTrackDigiDollar(bool shouldTrack) {
    //Turning tracking off leaves a hole in the DigiDollar history that later blocks cannot fill,
    //so clear the sync marker.  Turning it back on then triggers a full backfill rather than
    //silently serving balances that are missing everything from the gap.
    if (!shouldTrack && _trackDigiDollar) {
        AppMain::GetInstance()->getDatabase()->setDigiDollarSyncHeight(0);
    }
    _trackDigiDollar = shouldTrack;
}

/**
 * returns 0 if we should not prune right now otherwise returns height we can prune up to
 * @param height
 * @return
 */
unsigned int ChainAnalyzer::pruneMax(unsigned int height) {
    if (_pruneAge < 0) return 0;                //no pruning
    if (height % _pruneInterval != 0) return 0; //not time to prune
    if (height - _pruneAge < 0) return 0;
    return height - _pruneAge;
}

void ChainAnalyzer::setPruneAge(int age) {
    _pruneAge = age;
    _pruneInterval = (int) ceil(1.0 * _pruneAge / PRUNE_INTERVAL_DIVISOR / 100) *
                     100; //make sure prune interval is multiple of 100
}


/*
██╗      ██████╗  ██████╗ ██████╗
██║     ██╔═══██╗██╔═══██╗██╔══██╗
██║     ██║   ██║██║   ██║██████╔╝
██║     ██║   ██║██║   ██║██╔═══╝
███████╗╚██████╔╝╚██████╔╝██║
╚══════╝ ╚═════╝  ╚═════╝ ╚═╝
 */

void ChainAnalyzer::startupFunction() {
    //mark as initializing
    _state = INITIALIZING;
    AppMain* main = AppMain::GetInstance();
    Database* db = main->getDatabase();
    DigiByteCore* dgb = main->getDigiByteCore();

    //make sure everything is set up
    if (!_verifyDatabaseWrite) db->disableWriteVerification();

    //find block we left off at
    _height = db->getBlockHeight();
    _nextHash = dgb->getBlockHash(_height);

    //clear the block we left off on just in case it was partially processed
    Log* log = Log::GetInstance();
    log->addMessage("Repairing database from shutdown");
    db->clearBlocksAboveHeight(_height);
    log->addMessage("Repair complete");

    //make sure database knows if we want to store non asset utxos
    if (!shouldStoreNonAssetUTXO()) {
        //mark as has been pruned if we aren't keeping and database will not store them
        db->setBeenPrunedNonAssetUTXOHistory(true);
    }

    //start watching for steps that never finish
    watchdogStart();

    //remember the newest oracle epoch so the sync loop can skip re-reading commitments it has
    _lastOracleEpoch = db->getDigiDollarLastEpoch();

    //rewind far enough to index the DigiDollar era if we have never done it
    phaseDigiDollarBackfill();
}

/**
 * DigiDollar activated at block 23,869,440, which is below where most existing nodes are already
 * synced to.  Balances and vaults can only be reconstructed by replaying those blocks, so on the
 * first run after upgrading we rewind to just below activation and let the normal sync path redo
 * that range.  clearBlocksAboveHeight() removes the stale rows on the way down, so the replay is
 * additive rather than duplicating anything.
 *
 * The marker is only set once, after the rewind is scheduled.  From then on ordinary reorg
 * handling keeps the DigiDollar tables consistent, so this never runs again unless the operator
 * turns tracking off and back on.
 */
void ChainAnalyzer::phaseDigiDollarBackfill() {
    if (!shouldTrackDigiDollar()) return;

    AppMain* main = AppMain::GetInstance();
    Database* db = main->getDatabase();
    Log* log = Log::GetInstance();

    const unsigned int activation = DigiAssetConstants::DIGIDOLLAR_ACTIVATION_HEIGHT;

    //nothing to backfill until the chain we have indexed actually reaches DigiDollar
    if (static_cast<unsigned int>(_height) < activation) return;

    //already indexed
    if (db->getDigiDollarSyncHeight() >= activation) return;

    //A node that prunes UTXO history below the activation height cannot be rewound that far.
    //Say so plainly rather than starting a rewind that will restart the whole sync.
    int prunedTo = db->getBeenPrunedUTXOHistory();
    if ((prunedTo >= 0) && (static_cast<unsigned int>(prunedTo) >= activation)) {
        log->addMessage(
                "DigiDollar history cannot be indexed: UTXO history is pruned to " + to_string(prunedTo) +
                        ", above the DigiDollar activation height " + to_string(activation) +
                        ".  Resync from scratch or set trackdigidollar=0 to silence this.",
                Log::CRITICAL);
        return;
    }

    log->addMessage("DigiDollar indexing has not been run.  Rewinding from " + to_string(_height) +
                            " to " + to_string(activation - 1) + " to index it - this will take a while.",
                    Log::WARNING);

    _height = static_cast<int>(activation) - 1;
    db->clearBlocksAboveHeight(_height);
    _nextHash = db->getBlockHash(_height);
    _lastOracleEpoch = 0;

    //Mark it done now rather than at the end of the replay.  The rewind has already discarded
    //everything above this height, so an interruption leaves the normal sync path to finish the
    //job - re-running the rewind on the next start would throw away good work.
    db->setDigiDollarSyncHeight(activation);
}

void ChainAnalyzer::mainFunction() {
    try {
        phaseRewind();
        phaseSync();
        _lastErrorMessage.clear(); //a pass that got through means whatever it was is over
        _repeatErrorCount = 0;
    } catch (const std::exception& e) {
        handleSyncError(e.what());
    } catch (...) {
        handleSyncError("unknown error");
    }
}

/**
 * Handles a pass that ended in an error.
 *
 * A failure part way through leaves the thread loop to simply start the pass over.  The exception
 * itself used to go nowhere - the thread pool drops it without looking - so an error that repeats
 * forever was an invisible spin at full speed: one report had 3,851,461 rewind attempts logged and
 * not one line saying what had gone wrong.  Say what happened, and slow down once it is clear the
 * error is not going to clear on its own, so the node stays diagnosable instead of filling the disk.
 *
 * The retry itself is kept - a failure part way through a block is usually a passing condition and
 * the next pass picks up where this one left off.
 *
 * @param message - what went wrong
 */
void ChainAnalyzer::handleSyncError(const string& message) {
    Log* log = Log::GetInstance();

    if (message == _lastErrorMessage) {
        _repeatErrorCount++;
    } else {
        _lastErrorMessage = message;
        _repeatErrorCount = 1;
    }

    if (_repeatErrorCount < REPEAT_ERRORS_BEFORE_PAUSE) {
        log->addMessage("Chain analyzer error: " + message, Log::ERROR);
        pause(1);
        return;
    }
    if (_repeatErrorCount == REPEAT_ERRORS_BEFORE_PAUSE) {
        //say it once at this point rather than every pass from here on
        log->addMessage("Chain analyzer has hit the same error " + to_string(_repeatErrorCount) +
                                " times in a row and is not getting past it: " + message +
                                ".  Still retrying, now every " + to_string(REPEAT_FAILURE_PAUSE_SECONDS) + " seconds",
                        Log::CRITICAL);
    }
    pause(REPEAT_FAILURE_PAUSE_SECONDS);
}

/**
 * Waits, but gives up as soon as a shutdown is requested so a pause can never hold up an exit
 * @param seconds - how long to wait for
 */
void ChainAnalyzer::pause(unsigned int seconds) {
    for (unsigned int i = 0; i < seconds * 2; i++) {
        if (stopRequested()) return;
        this_thread::sleep_for(chrono::milliseconds(500));
    }
}

void ChainAnalyzer::shutdownFunction() {
    _state = STOPPED;
    watchdogStop();
}

/*
██╗    ██╗ █████╗ ████████╗ ██████╗██╗  ██╗██████╗  ██████╗  ██████╗
██║    ██║██╔══██╗╚══██╔══╝██╔════╝██║  ██║██╔══██╗██╔═══██╗██╔════╝
██║ █╗ ██║███████║   ██║   ██║     ███████║██║  ██║██║   ██║██║  ███╗
██║███╗██║██╔══██║   ██║   ██║     ██╔══██║██║  ██║██║   ██║██║   ██║
╚███╔███╔╝██║  ██║   ██║   ╚██████╗██║  ██║██████╔╝╚██████╔╝╚██████╔╝
 ╚══╝╚══╝ ╚═╝  ╚═╝   ╚═╝    ╚═════╝╚═╝  ╚═╝╚═════╝  ╚═════╝  ╚═════╝
 */

long long ChainAnalyzer::steadySeconds() {
    return chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now().time_since_epoch()).count();
}

void ChainAnalyzer::watchdogStart() {
    if (_watchdogRunning) return;
    _watchdogRunning = true;
    _watchdogThread = std::thread(&ChainAnalyzer::watchdogTask, this);
}

void ChainAnalyzer::watchdogStop() {
    _watchdogRunning = false;
    if (_watchdogThread.joinable()) _watchdogThread.join();
}

/**
 * Records what the analyzer is about to do so the watchdog can name it if it never comes back
 * @param step - human readable description, eg "block 24081128 transaction abc123..."
 */
void ChainAnalyzer::watchdogWorkingOn(const string& step) {
    {
        lock_guard<mutex> lock(_watchdogMutex);
        _watchdogStep = step;
    }
    _watchdogSince = steadySeconds();
}

/**
 * Says the analyzer is deliberately doing nothing(waiting for a new block, or shutting down)
 * so the watchdog stays quiet
 */
void ChainAnalyzer::watchdogIdle() {
    _watchdogSince = 0;
}

/**
 * Complains whenever a single step has been running for longer than _stallWarningSeconds, and
 * again every _stallWarningSeconds after that for as long as it keeps running.
 *
 * Everything the analyzer logs is written after a block completes, so a step that blocks forever
 * produces no output at all - which is what a node stuck on a single issuance looked like from
 * the outside.  The message names the block and transaction so the cause can actually be found.
 */
void ChainAnalyzer::watchdogTask() {
    Log* log = Log::GetInstance();
    long long warnedForSince = 0;
    long long nextWarnAfter = _stallWarningSeconds;

    while (_watchdogRunning) {
        this_thread::sleep_for(chrono::milliseconds(200));

        long long since = _watchdogSince;
        if (since == 0) continue; //idle on purpose

        //a different step than the one last warned about starts the count over
        if (since != warnedForSince) {
            warnedForSince = since;
            nextWarnAfter = _stallWarningSeconds;
        }

        long long elapsed = steadySeconds() - since;
        if (elapsed < nextWarnAfter) continue;
        nextWarnAfter = elapsed + _stallWarningSeconds;
        _stallWarnings++;

        string step;
        {
            lock_guard<mutex> lock(_watchdogMutex);
            step = _watchdogStep;
        }
        log->addMessage("Still working on " + step + " after " + to_string(elapsed) +
                                " seconds.  Sync is not frozen, it is waiting on something outside the node - "
                                "usually the IPFS daemon, DigiByte Core, or a storage pool server",
                        Log::WARNING);
    }
}

/*
██████╗ ██╗  ██╗ █████╗ ███████╗███████╗███████╗
██╔══██╗██║  ██║██╔══██╗██╔════╝██╔════╝██╔════╝
██████╔╝███████║███████║███████╗█████╗  ███████╗
██╔═══╝ ██╔══██║██╔══██║╚════██║██╔══╝  ╚════██║
██║     ██║  ██║██║  ██║███████║███████╗███████║
╚═╝     ╚═╝  ╚═╝╚═╝  ╚═╝╚══════╝╚══════╝╚══════╝
 */

void ChainAnalyzer::phaseRewind() {
    Log* log = Log::GetInstance();
    watchdogWorkingOn("rewinding from height " + to_string(_height));

    AppMain* main = AppMain::GetInstance();
    Database* db = main->getDatabase();
    DigiByteCore* dgb = main->getDigiByteCore();

    ///should start at what ever number left off at since blocks is set only after finishing

    //check if we need to rewind
    string hash = dgb->getBlockHash(_height);
    if (hash != _nextHash) {
        //only say so when there is really something to rewind.  Every pass comes through here, so
        //saying it up front turned a pass that kept failing into millions of meaningless lines
        log->addMessage("Rewinding Phase Started");
        _state = ChainAnalyzer::REWINDING;

        //rewind until correct
        unsigned int originalHeight = _height;
        while (hash != _nextHash) {
            _height--;
            hash = dgb->getBlockHash(_height);
            try {
                _nextHash = db->getBlockHash(_height);
            } catch (const Database::exceptionDataPruned& e) {
                //we rolled back to point that has been pruned so restart chain analyser
                log->addMessage("Rewound blocks past prune point.  Need to restart sync", Log::WARNING);
                restart();
                return;
            }
        }
        log->addMessage("Rewinding " + to_string(originalHeight - _height) + " blocks");

        //delete all data above & including _height
        db->clearBlocksAboveHeight(_height);
        log->addMessage("Rewinding Phase Ended");

        //a chain that keeps reorganising back to the same height is either a very busy fork or
        //something undoing the same work over and over.  Rewinds are normal so the first few are
        //left alone, but past that stop hammering the node and the disk
        if (static_cast<unsigned int>(_height) == _lastRewindHeight) {
            _repeatRewindCount++;
        } else {
            _lastRewindHeight = _height;
            _repeatRewindCount = 1;
        }
        if (_repeatRewindCount > REWINDS_TO_SAME_HEIGHT_BEFORE_PAUSE) {
            log->addMessage("Rewound to height " + to_string(_height) + " " + to_string(_repeatRewindCount) +
                                    " times in a row.  Waiting " + to_string(REPEAT_FAILURE_PAUSE_SECONDS) +
                                    " seconds before carrying on",
                            Log::WARNING);
            pause(REPEAT_FAILURE_PAUSE_SECONDS);
        }
    }
}

void ChainAnalyzer::phaseSync() {
    Log* log = Log::GetInstance();

    AppMain* main = AppMain::GetInstance();
    Database* db = main->getDatabase();
    DigiByteCore* dgb = main->getDigiByteCore();

    //start syncing
    watchdogWorkingOn("looking up block " + to_string(_height) + " in DigiByte Core");
    string hash = dgb->getBlockHash(_height);
    bool fastMode = false;
    chrono::steady_clock::time_point beginTime;
    chrono::steady_clock::time_point beginTotalTime;
    long totalProcessed = 0;
    stringstream ss;
    bool inTransaction = false;
    blockinfo_t blockData = dgb->getBlock(hash); //get first blocks data in syncing process(all future ones are at end of loop)
    while ((hash == _nextHash) && !stopRequested()) {
        if (totalProcessed == 0) {
            beginTotalTime = chrono::steady_clock::now();
        }
        if (!_showAllBlockSyncTime && (_height % 100 == 0)) fastMode = (_state < -110);

        //show processing block
        if (fastMode) {
            if (_height % 100 == 0) {
                ss << "processed blocks: " << setw(9) << _height << " to " << setw(9) << (_height + 99);
                beginTime = chrono::steady_clock::now();
            }
        } else {
            ss << "processed block: " << setw(9) << _height;
            beginTime = chrono::steady_clock::now();
        }

        //process block
        _state = 0 - blockData.confirmations;                        //calculate how far behind we are
        if (!fastMode) ss << "(" << setw(8) << (_state + 1) << ") "; //+1 because message is related to after block is done

        //batch writes in a transaction (every block in slow mode, every 100 blocks in fast mode)
        bool startBatch = fastMode ? (_height % 100 == 0) : true;
        bool endBatch = fastMode ? (_height % 100 == 99) : true;
        if (startBatch) {
            db->startTransaction();
            inTransaction = true;
        }

        //record the oracle DGB/USD price this block commits to, if any
        if (shouldTrackDigiDollar() && !blockData.tx.empty()) {
            captureOracleCommitment(blockData.height, blockData.tx[0]);
        }

        //process each tx in block
        if (shouldStoreNonAssetUTXO() || (_height >= 8432316)) { //only non asset utxo below this height
            for (string& tx: blockData.tx) {
                //named here rather than per block: an issuance whose metadata the ipfs node
                //can not produce blocks on one transaction, and that is the one worth printing
                watchdogWorkingOn("block " + to_string(blockData.height) + " transaction " + tx);
                processTX(tx, blockData.height);
            }
        }

        if (!fastMode) {
            //near the tip: let event stream subscribers know a block was processed
            EventBroadcaster::GetInstance()->broadcast(
                    "{\"event\":\"newBlock\",\"height\":" + to_string(_height) +
                    ",\"blocksBehind\":" + to_string(0 - _state) + "}");
        }

        if (endBatch && inTransaction) {
            watchdogWorkingOn("writing block " + to_string(blockData.height) + " to the database");
            db->endTransaction();
            inTransaction = false;
        }

        //periodically checkpoint WAL
        if (!inTransaction && (fastMode ? (_height % 100 == 99) : (_height % 1000 == 999))) db->walCheckpoint();


        //show run time stats
        totalProcessed++;
        if (fastMode) {
            if (_height % 100 == 99) {
                //estimate sync time left
                chrono::steady_clock::time_point endTime = chrono::steady_clock::now();
                unsigned long msRemaining = blockData.confirmations * chrono::duration_cast<chrono::milliseconds>(endTime - beginTotalTime).count() / totalProcessed;

                //show message
                ss << " in " << setw(6)
                   << chrono::duration_cast<chrono::milliseconds>(endTime - beginTime).count() / 100
                   << " ms per block - ";

                // Convert to the most significant unit
                const unsigned long msPerMinute = 60000;
                const unsigned long msPerHour = 3600000;
                const unsigned long msPerDay = 86400000;
                if (msRemaining >= msPerDay * 2) {
                    // Convert to days if more than 2 days
                    double days = msRemaining / static_cast<double>(msPerDay);
                    ss << std::fixed << std::setprecision(1) << days << " days left to sync";
                } else if (msRemaining >= msPerHour * 2) {
                    // Convert to hours if more than 2 hours
                    double hours = msRemaining / static_cast<double>(msPerHour);
                    ss << std::fixed << std::setprecision(1) << hours << " hours left to sync";
                } else {
                    // Convert to minutes for anything less
                    double minutes = msRemaining / static_cast<double>(msPerMinute);
                    ss << std::fixed << std::setprecision(1) << minutes << " minutes left to sync";
                }

                log->addMessage(ss.str());
                ss.str("");
                ss.clear();
            }
        } else {
            chrono::steady_clock::time_point endTime = chrono::steady_clock::now();
            ss << " in " << setw(6)
               << chrono::duration_cast<chrono::milliseconds>(endTime - beginTime).count() << " ms per block";
            log->addMessage(ss.str());
            ss.str("");
            ss.clear();
        }

        //clear invalid RPC cached
        AppMain::GetInstance()->getRpcCache()->newBlockAdded();

        //prune database
        phasePrune();

        //if fully synced pause until new block
        watchdogIdle(); //waiting for the chain to move is not a stall
        while (blockData.nextblockhash.empty()) {
            //a new block can be minutes away - don't hold up shutdown waiting for one
            if (stopRequested()) return;

            //see if any performance indexes need to be added(do before marking as synced will set state to BUSY if there is anything to do)
            db->executePerformanceIndex(_state);

            //mark as synced
            _state = SYNCED;
            totalProcessed = 0; //don't track waiting time

            //pause for 0.5 sec
            chrono::milliseconds dura(500);
            this_thread::sleep_for(dura);

            //check current block has not changed
            string currentHash = dgb->getBlockHash(_height);
            if (hash != currentHash) {
                _state = REWINDING;
                return;
            }

            //update blockData so we can exit loop
            blockData = dgb->getBlock(hash);
        }

        //The block DigiDollar activated on has now been indexed by the normal sync path, so record
        //it.  The marker is the only thing that tells a later start this database already has
        //DigiDollar in it, and it used to be written solely by phaseDigiDollarBackfill.  That left
        //any database built by syncing forward - including a bootstrap image - claiming height 0,
        //so the next start rewound all the way back here to redo work that was already done
        if (shouldTrackDigiDollar() &&
            (_height == static_cast<int>(DigiAssetConstants::DIGIDOLLAR_ACTIVATION_HEIGHT))) {
            db->setDigiDollarSyncHeight(DigiAssetConstants::DIGIDOLLAR_ACTIVATION_HEIGHT);
        }

        //get what would be next block based on the block we just processed
        _nextHash = blockData.nextblockhash;

        //get what actually is the next block(we check both ways because if they don't match there was a rollback)
        _height++;
        watchdogWorkingOn("looking up block " + to_string(_height) + " in DigiByte Core");
        hash = dgb->getBlockHash(_height);
        blockData = dgb->getBlock(hash);

        //save the next block to be processed to the database
        db->insertBlock(blockData.height, blockData.hash, blockData.time, blockData.algo, blockData.difficulty);
    }
    if (inTransaction) db->endTransaction();
    watchdogIdle();
}

/**
 * Reads the DigiDollar oracle price commitment out of a block's coinbase.
 *
 * Roughly two thirds of blocks carry one, but every block inside a 40 block epoch republishes the
 * same consensus value, so only the first block of each new epoch is worth reading.  Skipping the
 * rest keeps this to one extra getrawtransaction per 40 blocks instead of one per block, which
 * matters a great deal during the activation backfill.
 *
 * @param height       block being processed
 * @param coinbaseTxid txid of the block's coinbase transaction
 */
void ChainAnalyzer::captureOracleCommitment(unsigned int height, const string& coinbaseTxid) {
    if (height < DigiAssetConstants::DIGIDOLLAR_ACTIVATION_HEIGHT) return;

    unsigned int epoch = height / DigiAssetConstants::DIGIDOLLAR_ORACLE_EPOCH_LENGTH;
    if ((epoch != 0) && (epoch <= _lastOracleEpoch)) return; //already have this epoch

    AppMain* main = AppMain::GetInstance();
    try {
        getrawtransaction_t coinbase = main->getDigiByteCore()->getRawTransaction(coinbaseTxid);
        DigiDollar::OracleCommitment commitment;
        if (!DigiDollar::findOracleCommitment(coinbase, commitment)) return; //miner published none

        main->getDatabase()->addDigiDollarRate(height, commitment.epoch, commitment.price,
                                               static_cast<unsigned int>(commitment.timestamp),
                                               commitment.participants);
        _lastOracleEpoch = commitment.epoch;
    } catch (const exception& e) {
        //A missing or unreadable commitment is not fatal - the price is republished every epoch,
        //so the next one recovers.  Never let it stop the chain sync.
        Log::GetInstance()->addMessage(
                "Could not read DigiDollar oracle commitment at height " + to_string(height) + ": " + e.what(),
                Log::WARNING);
    }
}

void ChainAnalyzer::phasePrune() {

    //check if time to prune
    unsigned int pruneHeight = pruneMax(_height);
    if (pruneHeight == 0) return;

    //prune the data
    Database* db = AppMain::GetInstance()->getDatabase();
    if (shouldPruneExchangeHistory()) db->pruneExchange(min(pruneHeight, _height - DigiAsset::EXCHANGE_RATE_LENIENCY));
    if (shouldPruneUTXOHistory()) db->pruneUTXO(pruneHeight);
    if (shouldPruneVoteHistory()) db->pruneVote(pruneHeight);
}

void ChainAnalyzer::restart() {
    Database* db = AppMain::GetInstance()->getDatabase();
    db->reset();
    _height = 1;
    _nextHash = DIGIBYTE_BLOCK1_HASH;
}

/*
██████╗ ██████╗  ██████╗  ██████╗███████╗███████╗███████╗
██╔══██╗██╔══██╗██╔═══██╗██╔════╝██╔════╝██╔════╝██╔════╝
██████╔╝██████╔╝██║   ██║██║     █████╗  ███████╗███████╗
██╔═══╝ ██╔══██╗██║   ██║██║     ██╔══╝  ╚════██║╚════██║
██║     ██║  ██║╚██████╔╝╚██████╗███████╗███████║███████║
╚═╝     ╚═╝  ╚═╝ ╚═════╝  ╚═════╝╚══════╝╚══════╝╚══════╝
 */

void ChainAnalyzer::processTX(const string& txid, unsigned int height) {
    //get raw transaction
    std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
    DigiByteTransaction tx(txid, height, !_storeNonAssetUTXOs);
    auto duration = std::chrono::steady_clock::now() - startTime;
    _processTransactionRunTime += std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
    _processTransactionRunCount++;

    //add transaction to database
    startTime = std::chrono::steady_clock::now();
    tx.addToDatabase();
    duration = std::chrono::steady_clock::now() - startTime;
    _saveTransactionRunTime += std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
    _saveTransactionRunCount++;

    //get list of addresses that have been changed
    startTime = std::chrono::steady_clock::now();
    vector<string> addresses;
    size_t inputCount = tx.getInputCount();
    for (size_t i = 0; i < inputCount; i++) {
        addresses.emplace_back(tx.getInput(i).address);
    }
    size_t outputCount = tx.getOutputCount();
    for (size_t i = 0; i < outputCount; i++) {
        addresses.emplace_back(tx.getOutput(i).address);
    }

    // Remove duplicates from addresses
    std::sort(addresses.begin(), addresses.end());               // Sort the vector
    auto last = std::unique(addresses.begin(), addresses.end()); // Remove consecutive duplicates
    addresses.erase(last, addresses.end());                      // Erase the non-unique elements

    //invalidate rpc caches based on addresses that have changed
    RPC::Cache* cache = AppMain::GetInstance()->getRpcCache();
    for (auto address: addresses) {
        cache->addressChanged(address);
    }
    duration = std::chrono::steady_clock::now() - startTime;
    _clearAddressCacheRunTime += std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
    _clearAddressCacheRunCount++;

    //let event stream subscribers know about asset activity.  Only asset bearing
    //transactions get events so there is no firehose during initial sync
    if (tx.isIssuance() || tx.isTransfer(true) || tx.isBurn(true)) {
        EventBroadcaster* events = EventBroadcaster::GetInstance();

        //unique list of asset ids involved(inputs too - a full burn has no asset outputs)
        vector<string> assetIds;
        for (size_t i = 0; i < inputCount; i++) {
            for (const DigiAsset& asset: tx.getInput(i).assets) assetIds.emplace_back(asset.getAssetId());
        }
        for (size_t i = 0; i < outputCount; i++) {
            for (const DigiAsset& asset: tx.getOutput(i).assets) assetIds.emplace_back(asset.getAssetId());
        }
        sort(assetIds.begin(), assetIds.end());
        assetIds.erase(unique(assetIds.begin(), assetIds.end()), assetIds.end());
        string assetIdJson;
        for (const string& id: assetIds) {
            if (!assetIdJson.empty()) assetIdJson += ",";
            assetIdJson += "\"" + id + "\"";
        }

        string type = tx.isIssuance() ? "assetIssued" : (tx.isBurn(true) ? "assetBurn" : "assetTransfer");
        events->broadcast("{\"event\":\"" + type + "\",\"assetIds\":[" + assetIdJson +
                          "],\"txid\":\"" + txid + "\",\"height\":" + to_string(height) + "}");

        //addresses whose holdings changed(list deduped above; asset ids and addresses
        //are base58/bech32 so no json escaping needed)
        string addressJson;
        for (const string& address: addresses) {
            if (address.empty()) continue;
            if (!addressJson.empty()) addressJson += ",";
            addressJson += "\"" + address + "\"";
        }
        events->broadcast("{\"event\":\"balanceChanged\",\"addresses\":[" + addressJson +
                          "],\"txid\":\"" + txid + "\",\"height\":" + to_string(height) + "}");
    }

    //DigiDollar activity gets its own events for the same reason asset activity does - it is rare
    //enough not to flood subscribers during sync
    if (tx.isDigiDollarTransaction()) {
        EventBroadcaster* events = EventBroadcaster::GetInstance();

        uint64_t amount = 0;
        for (const auto& ddOutput: tx.getDigiDollarOutputs()) amount += ddOutput.second;

        string type = tx.isDigiDollarMint()
                              ? "digiDollarMint"
                              : (tx.isDigiDollarRedeem() ? "digiDollarRedeem" : "digiDollarTransfer");

        string addressJson;
        for (const string& address: addresses) {
            if (address.empty()) continue;
            if (!addressJson.empty()) addressJson += ",";
            addressJson += "\"" + address + "\"";
        }

        //cents is the protocol's own unit, so it is reported without conversion
        events->broadcast("{\"event\":\"" + type + "\",\"cents\":" + to_string(amount) +
                          ",\"addresses\":[" + addressJson + "],\"txid\":\"" + txid +
                          "\",\"height\":" + to_string(height) + "}");
    }
}


/**
 * Gets the current sync state
 */
int ChainAnalyzer::getSync() const {
    return _state;
}
unsigned int ChainAnalyzer::getSyncHeight() const {
    return _height;
}
