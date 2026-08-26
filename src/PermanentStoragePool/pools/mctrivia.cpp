//
// Created by mctrivia on 04/11/23.
//

#include "mctrivia.h"
#include "AppMain.h"
#include "CurlHandler.h"
#include "Log.h"
#include "DigiAssetConstants.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <set>
#include <thread>
using namespace std;

///The bad list refresh happens on the chain analyzer's thread(isAssetBad is called while
///processing an issuance), so an unresponsive pool server used to stop the whole node on
///whichever block the next issuance was in.  Bounded here: a stale bad list is harmless
static const unsigned int PSP_SERVER_TIMEOUT_MS = 30000;

///max wait for the metadata of the issuance being costed.  Same reasoning: getCost runs on an
///rpc thread and used to wait for a cid the node may never be able to fetch
static const unsigned int PSP_METADATA_WAIT_MS = 30000;

mctrivia::mctrivia() : _keepRunning(false){};
mctrivia::~mctrivia() { stop(); }



string mctrivia::getName() {
    return "MCTrivia's PSP";
}
string mctrivia::getDescription() {
    return "Originally operated by digiassetX Inc and continued to run by Matthew Cornelisse.  This pool makes sure asset metadata is always available and pays others DigiAsset Core nodes to help distribute the metadata.";
}
string mctrivia::getURL() {
    return "https://ipfs.digiassetx.com";
}

/**
 * Returns the cost of including in psp.
 * May throw DigiAsset::exceptionInvalidMetaData, or PermanentStoragePool::exceptionCantEnablePSP()
 * @param tx
 * @return in DGB sats
 */
uint64_t mctrivia::getCost(const DigiByteTransaction& tx) {
    //check if even an issuance tx
    if (!tx.isIssuance()) return 0;

    uint64_t size = 0;

    //find the metadata and get its size
    DigiAsset asset = tx.getIssuedAsset();
    string cid = asset.getCID();
    IPFS* ipfs = AppMain::GetInstance()->getIPFS();
    size += ipfs->getSize(cid);

    //download the metadata and decode it.  Bounded: an issuance whose metadata the node can not
    //produce should fail the rpc call with an error, not leave the caller waiting forever
    string metadataStr = ipfs->callOnDownloadSync(cid, "", PSP_METADATA_WAIT_MS);
    Json::CharReaderBuilder rbuilder;
    Json::Value metadata;
    istringstream s(metadataStr);
    string errs;
    if (!Json::parseFromStream(rbuilder, s, &metadata, &errs)) throw DigiAsset::exceptionInvalidMetaData(); //Improperly formatted

    //Check if there is a data.urls section
    if (!metadata.isMember("data") || !metadata["data"].isObject()) throw DigiAsset::exceptionInvalidMetaData(); //Improperly formatted
    Json::Value data = metadata["data"];
    if (!data.isMember("urls") || !data["urls"].isArray()) throw DigiAsset::exceptionInvalidMetaData(); //Improperly formatted
    Json::Value urls = data["urls"];

    //Go through URLs and get the size of the ones we care about
    for (const auto& obj: urls) {
        //Ignore all links that don't have name and url
        if (!obj.isMember("name") || !obj["name"].isString()) continue;
        if (!obj.isMember("url") || !obj["url"].isString()) continue;
        string name = obj["name"].asString();
        string url = obj["url"].asString();

        //Ignore links not on IPFS
        if (!ipfs->isIPFSurl(url)) throw PermanentStoragePool::exceptionCantEnablePSP(); //Not on IPFS so can't be included in the PSP

        //add to size value
        size += ipfs->getSize(url.substr(7));
    }

    //convert size to DGB sats: $1.20 USD per MB, exchange rate is DGB sats per US dollar.
    //Must stay the exact inverse of serializeMetaProcessor's
    //   bytes = 1000000 * dgb / (exchangeRate * 1.2)
    //or pay ins won't buy the number of bytes the pool expects
    Database* db = AppMain::GetInstance()->getDatabase();
    double exchangeRate = db->getCurrentExchangeRate(DigiAssetConstants::standardExchangeRates[1]); //USD
    return static_cast<uint64_t>(ceil(size * 1.2 * exchangeRate / 1000000.0));
}

/**
 * Throws
 *      DigiAsset::exceptionInvalidMetaData - Asset meta data is improperly formed
 *      PermanentStoragePool::exceptionCantEnablePSP - transaction is not an issuance
 *      DigiByteTransaction::exceptionNotEnoughFunds - not enough funds left in the inputs to pay for added output
 * @param tx
 */
void mctrivia::enable(DigiByteTransaction& tx) {
    //make sure transaction is even an issuance
    if (!tx.isIssuance()) throw exceptionCantEnablePSP();

    //get cost
    uint64_t cost = getCost(tx);

    //an output below the dust threshold would make the whole tx unrelayable.  Overpaying
    //slightly is harmless(pay in simply buys more storage than needed)
    if (cost < DigiAssetConstants::DIGIBYTE_DUST) cost = DigiAssetConstants::DIGIBYTE_DUST;

    //check if there is already an output to output address:
    const string outputAddress = "dgb1qjnzadu643tsfzjqjydnh06s9lgzp3m4sg3j68x";
    for (size_t i = 0; i < tx.getOutputCount(); i++) {
        if (tx.getOutput(i).address == outputAddress) throw exceptionCantEnablePSP(); //extremely unlikely so don't bother handling the edge case
    }

    //create an output transaction and try to add it to the tx
    tx.addDigiByteOutput(outputAddress, cost);
}
void mctrivia::_setConfig(const Config& config) {
    _visible = config.getBool("psp1visible", true);
}

/**
 * Starts the keep alive thread
 */
void mctrivia::start() {
    if (!_keepRunning.load()) {
        _keepRunning.store(true);
        _keepAliveThread = std::thread(&mctrivia::keepAliveTask, this);
    }
}

/**
 * Stops the keep alive thread
 */
void mctrivia::stop() {
    _keepRunning.store(false);
    if (_keepAliveThread.joinable()) {
        _keepAliveThread.join();
    }
}

/**
 * Lets the server know we are sharing data
 */
void mctrivia::keepAliveTask() {
    while (_keepRunning.load()) {
        //make keep alive request
        try {
            _callServer(KEEP_ALIVE);
        } catch (...) {
        }

        //sleep for 20 minutes in 1-second increments so stop() returns promptly
        for (int i = 0; i < 1200 && _keepRunning.load(); i++) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}

/**
 * Determines if a transaction is part of PSP and encodes needed data for processing the meta data
 * "" means not part of pool
 * ,: are not allowed in return string
 */
string mctrivia::serializeMetaProcessor(const DigiByteTransaction& tx) {
    //check if known bad asset
    const string assetId = tx.getIssuedAsset().getAssetId();
    if (isAssetBad(assetId)) return "";

    //pool pay in address list
    uint64_t dgb = 0;
    const std::set<std::string> addresses = {
            "dgb1q84h0g4lpy0prppc2507wf7ngne26thza0sntgr",
            "dgb1q8c6p9nht8055lr5fczcvc4v29hunluqv3n3gaf",
            "dgb1qatvzudt2jey06kx8zn3a6p0nw689s9dxkjp57g",
            "dgb1qfc9029kc8ptvqt2nuqe4sxtps2nd83kq7pugtm",
            "dgb1qhucf64cleqdme9637vukgxau8aflpk00thlq98",
            "dgb1qj4glly6ka7py8pkdme9t0vh77s0gym0vq2esee",
            "dgb1qjnzadu643tsfzjqjydnh06s9lgzp3m4sg3j68x",
            "dgb1qkqggn9y85tlyxdfhg9ls3ygph4nd58j0acnlz6",
            "dgb1qm4putt429lu9mlc6ypukky0fq3q9spm7pjwcy8",
            "dgb1qnseslpvugsxcnvmz7m4emvmlgeryg80ujduspw",
            "dgb1qnynkfl44ztsw3et6rq9yhxmefrcm8ufd3afm3e",
            "dgb1qva97ew3zdwyadm5aqstqxe6xzzgxmxm7d6m3uw",
            "dgb1qxhx0ahcmuxxmlwvnkjdq6dhmnem570g587m7hk",
            "dgb1qylaqaen0jqs2sk7jlc74yarw5lg4nzwtac9vyp"};

    //go through outputs and add up what was sent to above addresses
    for (unsigned int i = 0; i < tx.getOutputCount(); i++) {
        AssetUTXO output = tx.getOutput(i);
        if (addresses.find(output.address) != addresses.end()) {
            dgb += output.digibyte;
        }
    }

    //if not paid in to psp return blank
    if (dgb == 0) return "";
    if (tx.getHeight() < 12642645) return "";

    //look up lowest accepted exchange rate
    Database* db = AppMain::GetInstance()->getDatabase();
    double exchangeRate = db->getAcceptedExchangeRate(DigiAssetConstants::standardExchangeRates[1], tx.getHeight());
    uint64_t bytes = ceil(1000000 * dgb / (exchangeRate * 1.2)) + 1; //1.2 is price in US$ per MB, ceil and +1 just in case of rounding error

    //return "S_bytes"
    return "S_" + to_string(bytes);
}

/**
 * Generic function to create a MetaProcessor object for this pool from the serialized Data
 * @param serializedData
 * @return
 */
unique_ptr<PermanentStoragePoolMetaProcessor> mctrivia::deserializeMetaProcessor(const string& serializedData) {
    return unique_ptr<PermanentStoragePoolMetaProcessor>(new mctriviaMetaProcessor(serializedData, _poolIndex));
}

/**
 * Function to make calls to PSP monitoring server
 * @param command
 * @return
 */
void mctrivia::_callServer(ServerCalls command, const string& extra) {
    string commandStr;
    string address = "NA";
    switch (command) {
        case UNSUBSCRIBE:
            commandStr = "unsubscribe";
            break;
        case KEEP_ALIVE:
            commandStr = "keepalive";
            address = getPayoutAddress();
            break;
        case REPORT:
            commandStr = "report";
            break;
    }

    //get values inside loop in case they have changed
    IPFS* ipfs = AppMain::GetInstance()->getIPFS();
    string peerId = ipfs->getPeerId();
    string url = "https://ipfs.digiassetx.com/" + commandStr;
    if (!extra.empty()) url += "/" + extra;
    CurlHandler::post(url, {{"address", address},
                            {"peerId", peerId},
                            {"visible", (_visible ? "v" : "h")},
                            {"secret", _secretCode}},
                      PSP_SERVER_TIMEOUT_MS);
    if (command==KEEP_ALIVE) {
        Log* log=Log::GetInstance();
        log->addMessage("Reported online to ipfs.digiassetx.com with server id: "+peerId);
    }

    //update the bad list
    updateBadList();
}
bool mctrivia::isAssetBad(const std::string& assetId) {
    //make sure bad list is populated(there are known bad assets so an empty list means we have not checked yet)
    unsigned int currentTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    if (currentTime - _badTime > 1200) updateBadList();

    //see if assetIndex in bad list
    std::lock_guard<std::mutex> lock(_badListMutex);
    auto it = find(_badAssets.begin(), _badAssets.end(), assetId);
    return it != _badAssets.end();
}
void mctrivia::_reportAssetBad(const std::string& assetId) {
    //send to server
    try {
        _callServer(REPORT, assetId);
    } catch (...) {
        throw exceptionCouldntReport();
    }
}
void mctrivia::updateBadList() {
    std::lock_guard<std::mutex> lock(_badListMutex);

    //stamp the attempt, not the success.  This runs on the chain analyzer's thread, and
    //leaving it unstamped on failure meant an unreachable pool server was retried on every
    //single issuance - each one paying the full timeout before the block could finish
    _badTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    try {
        //make curl request
        const string url = "https://ipfs.digiassetx.com/bad.json";
        string readBuffer = CurlHandler::get(url, PSP_SERVER_TIMEOUT_MS);

        //convert to json object
        Json::Value root;
        Json::Reader reader;
        if (!reader.parse(readBuffer, root)) return;

        //save bad assets
        for (const Json::Value& value: root["assets"]) {
            string assetId = value.asString();
            if (find(_badAssets.begin(), _badAssets.end(), assetId) == _badAssets.end()) {
                reportAssetBad(assetId, true);
                _badAssets.push_back(assetId);
            }
        }

        //save bad files
        for (const Json::Value& value: root["cids"]) {
            string assetId = value.asString();
            if (find(_badFiles.begin(), _badFiles.end(), assetId) == _badFiles.end()) {
                reportFileBad(assetId, true);
                _badFiles.push_back(assetId);
            }
        }

    } catch (const exception& e) {
        Log* log = Log::GetInstance();
        log->addMessage("Could not refresh the bad asset list from ipfs.digiassetx.com.  "
                        "Retrying in 20 minutes",
                        Log::DEBUG);
    }
}
void mctrivia::_reportFileBad(const string& cid) {
    //send to server
    try {
        _callServer(REPORT, cid);
    } catch (...) {
        throw exceptionCouldntReport();
    }
}

mctriviaMetaProcessor::mctriviaMetaProcessor(const string& serializedData, unsigned int poolIndex) : PermanentStoragePoolMetaProcessor(poolIndex) {
    //store pool Index
    _poolIndex = poolIndex;

    //for now first 2 bytes are always "S-" so just skip them
    _spaceLeft = stoull(serializedData.substr(2));
}

bool mctriviaMetaProcessor::_shouldPinFile(const std::string& name, const std::string& mimeType, const std::string& cid) {
    IPFS* ipfs = AppMain::GetInstance()->getIPFS();
    unsigned int size = ipfs->getSize(cid);
    if (size <= _spaceLeft) {
        _spaceLeft -= size;
        return true;
    }
    return false;
}
