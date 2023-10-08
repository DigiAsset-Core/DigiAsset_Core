//
// Created by mctrivia on 02/02/23.
//

#include "ChainAnalyzer.h"
#include "DigiByteCore.h"
#include "Database.h"
#include "BitIO.h"
#include "KYC.h"
#include "DigiByteTransaction.h"
#include "DigiAsset.h"
#include "Config.h"
#include <chrono>
#include <thread>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <fstream>
#include "static_block.hpp"
#include "Log.h"

using namespace std;


// Static block to register our callback function with IPFS Controller
static_block {
    IPFS::registerCallback(CHAINANALYZER_CALLBACK_NEWMETADATA_ID, ChainAnalyzer::_callbackNewMetadata);
}

unsigned int ChainAnalyzer::_pinAssetIcon = CHAINANALYZER_DEFAULT_PINASSETICON;
unsigned int ChainAnalyzer::_pinAssetDescription = CHAINANALYZER_DEFAULT_PINASSETDESCRIPTION;
unsigned int ChainAnalyzer::_pinAssetExtra = CHAINANALYZER_DEFAULT_PINASSETEXTRA;
unsigned int ChainAnalyzer::_pinAssetPermanent = CHAINANALYZER_DEFAULT_PINPERMANENT;
std::map<std::string, int> ChainAnalyzer::_pinAssetExtraMimeTypes;

/*
 ██████╗ ██████╗ ███╗   ██╗███████╗████████╗██████╗ ██╗   ██╗ ██████╗████████╗ ██████╗ ██████╗
██╔════╝██╔═══██╗████╗  ██║██╔════╝╚══██╔══╝██╔══██╗██║   ██║██╔════╝╚══██╔══╝██╔═══██╗██╔══██╗
██║     ██║   ██║██╔██╗ ██║███████╗   ██║   ██████╔╝██║   ██║██║        ██║   ██║   ██║██████╔╝
██║     ██║   ██║██║╚██╗██║╚════██║   ██║   ██╔══██╗██║   ██║██║        ██║   ██║   ██║██╔══██╗
╚██████╗╚██████╔╝██║ ╚████║███████║   ██║   ██║  ██║╚██████╔╝╚██████╗   ██║   ╚██████╔╝██║  ██║
 ╚═════╝ ╚═════╝ ╚═╝  ╚═══╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝  ╚═════╝   ╚═╝    ╚═════╝ ╚═╝  ╚═╝
 */

ChainAnalyzer::ChainAnalyzer(DigiByteCore& digibyteCore) {
    //store inputs
    _dgb = &digibyteCore;

    //reset config variables
    resetConfig();
}

ChainAnalyzer::~ChainAnalyzer() {
    stop();
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
    _pruneAge = 5760;           //number of blocks to keep for roll back protection(-1 don't prune, default is 1 day)
    _pruneInterval = (int) ceil(_pruneAge / PRUNE_INTERVAL_DIVISOR / 100) * 100;
    _pruneExchangeHistory = true;
    _pruneUTXOHistory = true;
    _pruneVoteHistory = true;

    //default config values(meta data)
    _pinAssetIcon = CHAINANALYZER_DEFAULT_PINASSETICON;
    _pinAssetDescription = CHAINANALYZER_DEFAULT_PINASSETDESCRIPTION;
    _pinAssetExtra = CHAINANALYZER_DEFAULT_PINASSETEXTRA;
    _pinAssetPermanent = CHAINANALYZER_DEFAULT_PINPERMANENT;
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
    } catch (const exceptionConfigFileMissing& e) {
        //no config file so just ignore
    }
}

void ChainAnalyzer::loadConfig() {
    Config config = Config(_configFileName);

    //load values in to class(chain data)
    setPruneAge(config.getInteger("pruneage", 5760));   //-1 for don't prune, default daily
    setPruneExchangeHistory(config.getBool("pruneexchangehistory", true));
    setPruneUTXOHistory(config.getBool("pruneutxohistory", true));
    setPruneVoteHistory(config.getBool("prunevotehistory", true));
    setStoreNonAssetUTXO(config.getBool("storenonassetutxo", false));

    //load values in to class(meta data)
    _pinAssetIcon = config.getInteger("pinasseticon", CHAINANALYZER_DEFAULT_PINASSETICON);
    _pinAssetDescription = config.getInteger("pinassetdescription", CHAINANALYZER_DEFAULT_PINASSETDESCRIPTION);
    _pinAssetExtra = config.getInteger("pinassetextra", CHAINANALYZER_DEFAULT_PINASSETEXTRA);
    _pinAssetPermanent = config.getInteger("pinassetpermanent", CHAINANALYZER_DEFAULT_PINPERMANENT);
    _pinAssetExtraMimeTypes = config.getIntegerMap("pinassetextra");

    //make sure IPFS is running
    IPFS::GetInstance(_configFileName);
}

void ChainAnalyzer::saveConfig() {
    Config config = Config(_configFileName);
    config.setInteger("pruneage", _pruneAge);
    config.setBool("pruneexchangehistory", _pruneExchangeHistory);
    config.setBool("pruneutxohistory", _pruneUTXOHistory);
    config.setBool("prunevotehistory", _pruneVoteHistory);
    config.setBool("storenonassetutxo", _storeNonAssetUTXOs);
    config.setIntegerMap("pinassetextra", _pinAssetExtraMimeTypes);
    config.write();
}


bool ChainAnalyzer::shouldPruneExchangeHistory() const {
    return _pruneExchangeHistory;
}

void ChainAnalyzer::setPruneExchangeHistory(bool shouldPrune) {
    Database* db = Database::GetInstance();
    if (!shouldPrune && (db->getBeenPrunedExchangeHistory() >= 0)) throw exceptionAlreadyPruned();
    _pruneExchangeHistory = shouldPrune;
}

bool ChainAnalyzer::shouldPruneUTXOHistory() const {
    return _pruneUTXOHistory;
}

void ChainAnalyzer::setPruneUTXOHistory(bool shouldPrune) {
    Database* db = Database::GetInstance();
    if (!shouldPrune && (db->getBeenPrunedUTXOHistory() >= 0)) throw exceptionAlreadyPruned();
    _pruneUTXOHistory = shouldPrune;
}

bool ChainAnalyzer::shouldPruneVoteHistory() const {
    return _pruneVoteHistory;
}

void ChainAnalyzer::setPruneVoteHistory(bool shouldPrune) {
    Database* db = Database::GetInstance();
    if (!shouldPrune && (db->getBeenPrunedVoteHistory() >= 0)) throw exceptionAlreadyPruned();
    _pruneVoteHistory = shouldPrune;
}

bool ChainAnalyzer::shouldStoreNonAssetUTXO() const {
    return _storeNonAssetUTXOs;
}

void ChainAnalyzer::setStoreNonAssetUTXO(bool shouldStore) {
    Database* db = Database::GetInstance();
    if (shouldStore && (db->getBeenPrunedNonAssetUTXOHistory())) throw exceptionAlreadyPruned();
    _storeNonAssetUTXOs = shouldStore;
}


/**
 * returns 0 if we should not prune right now otherwise returns height we can prune up to
 * @param height
 * @return
 */
unsigned int ChainAnalyzer::pruneMax(unsigned int height) {
    if (_pruneAge < 0) return 0; //no pruning
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
    Database* db = Database::GetInstance();

    //let database access to wallet
    db->setDigiByteCore(*_dgb);

    //make sure everything is set up
    db->disableWriteVerification();

    //find block we left off at
    _height = db->getBlockHeight();
    _nextHash = _dgb->getBlockHash(_height);

    //clear the block we left off on just in case it was partially processed
    db->clearBlocksAboveHeight(_height);

    //make sure database knows if we want to store non asset utxos
    if (!shouldStoreNonAssetUTXO()) {
        //mark as has been pruned if we aren't keeping and database will not store them
        db->setBeenPrunedNonAssetUTXOHistory(true);
    }
}

void ChainAnalyzer::mainFunction() {
    phaseRewind();
    phaseSync();
}

void ChainAnalyzer::shutdownFunction() {
    _state = STOPPED;
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
    log->addMessage("Rewinding");
    Database* db = Database::GetInstance();

///should start at what ever number left off at since blocks is set only after finishing

    //check if we need to rewind
    string hash = _dgb->getBlockHash(_height);
    if (hash != _nextHash) {
        _state = ChainAnalyzer::REWINDING;

        //rewind until correct
        while (hash != _nextHash) {
            _height--;
            hash = _dgb->getBlockHash(_height);
            try {
                _nextHash = db->getBlockHash(_height);
            } catch (const Database::exceptionDataPruned& e) {
                //we rolled back to point that has been pruned so restart chain analyser
                log->addMessage("Rewinded blocks past prune point.  Need to restart sync", Log::WARNING);
                restart();
                return;
            }
        }

        //delete all data above & including _height
        db->clearBlocksAboveHeight(_height);
    }
}

void ChainAnalyzer::phaseSync() {
    Database* db = Database::GetInstance();
    Log* log = Log::GetInstance();

    //start syncing
    string hash = _dgb->getBlockHash(_height);
    bool fastMode = false;
    chrono::steady_clock::time_point beginTime;
    stringstream ss;
    while (hash == _nextHash) {
        if (_height % 100 == 0) fastMode = (_state < -1000);

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
        blockinfo_t blockData = _dgb->getBlock(hash);   //get the next blocks data
        _state = 0 - blockData.confirmations;                   //calculate how far behind we are
        if (!fastMode) ss << "(" << setw(8) << (_state+1) << ") ";  //+1 because message is related to after block is done

        //process each tx in block
        if (shouldStoreNonAssetUTXO() || (_height >= 8432316)) {    //only non asset utxo below this height
            for (string& tx: blockData.tx) processTX(tx, blockData.height);
        }

        //show run time stats
        if (fastMode) {
            if (_height % 100 == 99) {
                chrono::steady_clock::time_point endTime = chrono::steady_clock::now();
                ss << " in " << setw(6)
                   << chrono::duration_cast<chrono::milliseconds>(endTime - beginTime).count() / 100
                   << " ms per block";
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

        //prune database
        phasePrune();

        //if fully synced pause until new block
        while (blockData.nextblockhash.empty()) {
            //mark as synced
            _state=SYNCED;

            //pause for 0.5 sec
            chrono::milliseconds dura(500);
            this_thread::sleep_for(dura);

            //check current block has not changed
            string currentHash = _dgb->getBlockHash(_height);
            if (hash != currentHash) {
                _state = REWINDING;
                return;
            }

            //update blockData so we can exit loop
            blockData = _dgb->getBlock(hash);
        }

        //set what block we will work on next
        _nextHash = blockData.nextblockhash;
        _height++;
        db->setBlockHash(_height, _nextHash);
        hash = _dgb->getBlockHash(_height);
    }
}

void ChainAnalyzer::phasePrune() {
    Database* db = Database::GetInstance();

    //check if time to prune
    unsigned int pruneHeight = pruneMax(_height);
    if (pruneHeight == 0) return;

    //prune the data
    if (shouldPruneExchangeHistory()) db->pruneExchange(min(pruneHeight, _height - DigiAsset::EXCHANGE_RATE_LENIENCY));
    if (shouldPruneUTXOHistory()) db->pruneUTXO(pruneHeight);
    if (shouldPruneVoteHistory()) db->pruneVote(pruneHeight);
}

void ChainAnalyzer::restart() {
    Database* db = Database::GetInstance();
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
    DigiByteTransaction tx(txid, *_dgb, height);
    bool needDownloadMeta = _pinAssetIcon || _pinAssetDescription || _pinAssetExtra || _pinAssetPermanent;
    tx.addToDatabase(needDownloadMeta ? CHAINANALYZER_CALLBACK_NEWMETADATA_ID : "");
}

void ChainAnalyzer::_callbackNewMetadata(const string& cid, const string& extra, const string& content, bool failed) {
    //failed will always be false since no maxSleep ever set

    IPFS* ipfs = IPFS::GetInstance();
    Json::CharReaderBuilder rbuilder;
    Json::Value metadata;
    istringstream s(content);
    string errs;
    Json::parseFromStream(rbuilder, s, &metadata, &errs);

    //Calculate max size for permanent
    unsigned int permanentSpace = configSizeToInt(_pinAssetPermanent);  //Max allowed by core operator
    bool permanentAmountLoaded = (permanentSpace == 0); //Set to true if we won't check it

    //Just in case someone tries to name multiple files icon (bad idea since wallets may not show as desired)
    unsigned int pinIconSize = _pinAssetIcon;
    unsigned int pinDescriptionSize = _pinAssetDescription;

    //Check if there is a data.urls section
    if (!metadata.isMember("data") || !metadata["data"].isObject()) return;  //Improperly formatted
    Json::Value data = metadata["data"];
    if (!data.isMember("urls") || !data["urls"].isArray()) return;  //Improperly formatted
    Json::Value urls = data["urls"];

    //Go through URLs and pin those we care about
    for (const auto& obj: urls) {
        //Ignore all links that don't have name and url
        if (!obj.isMember("name") || !obj["name"].isString()) continue;
        if (!obj.isMember("url") || !obj["url"].isString()) continue;
        string name = obj["name"].asString();
        string url = obj["url"].asString();

        //Ignore links not on IPFS
        if (!isIPFSLink(url)) continue;  //Not on IPFS so ignore

        //Check how much space is available if we have not done so already
        //We don't do this earlier because it is a waste of time if there are no IPFS links
        if (!permanentAmountLoaded) {
            permanentAmountLoaded = true;
            Database* db = Database::GetInstance();
            unsigned int size = db->getPermanentSize(extra);  //Extra is txid
            if (size < permanentSpace) permanentSpace = size;  //Pick smaller of size and permanentSpace
        }

        //If space in permanent space, use that first
        if (permanentSpace > 0) {
            unsigned int fileSize = ipfs->getSize(url.substr(7));
            if (permanentSpace > fileSize) {
                permanentSpace -= fileSize;
                ipfs->pin(url.substr(7));
                continue;
            }
        }

        //If no space in permanent then check if standard file name
        if (name == "icon") {
            ipfs->pin(url.substr(7), pinIconSize);
            pinIconSize = 0;
            continue;
        }
        if (name == "description") {
            ipfs->pin(url.substr(7), pinDescriptionSize);
            pinDescriptionSize = 0;
            continue;
        }

        //It is an extra file so pin based on type
        string mimeType =
                (obj.isMember("mimeType") && obj["mimeType"].isString()) ? obj["mimeType"].asString() : "";
        ipfs->pin(url.substr(7), extraFileLengthByMimeType(mimeType));
    }
}

unsigned int ChainAnalyzer::configSizeToInt(unsigned int value) {
    return (value == 1) ? std::numeric_limits<unsigned int>::max() : value;
}

bool ChainAnalyzer::isIPFSLink(const string& url) {
    const char* prefix = "ipfs://";
    const size_t prefixLength = 7; // Length of "ipfs://"

    // Check if the input string is at least as long as the prefix
    if (url.length() < prefixLength) {
        return false;
    }

    // Compare the characters case-insensitively
    for (size_t i = 0; i < prefixLength; ++i) {
        if (tolower(url[i]) != prefix[i]) {
            return false;
        }
    }

    return true;
}

unsigned int ChainAnalyzer::extraFileLengthByMimeType(const string& mimeType) {
    // Try to find an exact match in the map
    auto exactMatch = _pinAssetExtraMimeTypes.find(mimeType);
    if (exactMatch != _pinAssetExtraMimeTypes.end()) {
        return exactMatch->second;
    }

    // If no exact match found, check for wildcard matches
    string mimeWildCardValue = mimeType.substr(0, mimeType.find('/')) + "/*";
    auto itr = _pinAssetExtraMimeTypes.find(mimeWildCardValue);
    if (itr != _pinAssetExtraMimeTypes.end()) {
        return itr->second;
    }

    // If no matches found, return the default value _pinAssetExtra
    return _pinAssetExtra;
}
