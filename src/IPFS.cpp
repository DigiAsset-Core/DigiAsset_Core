//
// Created by mctrivia on 15/06/23.
//

#include <fstream>
#include <iostream>
#include "IPFS.h"
#include "Database.h"
#include "Config.h"
#include <curl/curl.h>

using namespace std;
using namespace std::chrono;

/*
███████╗██╗███╗   ██╗ ██████╗ ██╗     ███████╗████████╗ ██████╗ ███╗   ██╗
██╔════╝██║████╗  ██║██╔════╝ ██║     ██╔════╝╚══██╔══╝██╔═══██╗████╗  ██║
███████╗██║██╔██╗ ██║██║  ███╗██║     █████╗     ██║   ██║   ██║██╔██╗ ██║
╚════██║██║██║╚██╗██║██║   ██║██║     ██╔══╝     ██║   ██║   ██║██║╚██╗██║
███████║██║██║ ╚████║╚██████╔╝███████╗███████╗   ██║   ╚██████╔╝██║ ╚████║
╚══════╝╚═╝╚═╝  ╚═══╝ ╚═════╝ ╚══════╝╚══════╝   ╚═╝    ╚═════╝ ╚═╝  ╚═══╝
*/
IPFS* IPFS::_pinstance = nullptr;
mutex IPFS::_mutex;


string IPFS::_nodePrefix = "http://localhost:5001/api/v0/";
unsigned int IPFS::_timeoutPin = 1200; //number of seconds to try and pin a file
unsigned int IPFS::_timeoutDownload = 3600;    //number of seconds to try and download a file for
unsigned int IPFS::_timeoutRetry = 3600;   //on failed download how long to wait before retrying
unsigned int IPFS::_maxParallel = 10;    //max number of parallel ipfs tasks allowed

IPFS* IPFS::GetInstance() {
    std::lock_guard<std::mutex> lock(_mutex);
    if (_pinstance == nullptr) {
        _pinstance = new IPFS();
    }
    return _pinstance;
}

IPFS* IPFS::GetInstance(const string& configFile) {
    std::lock_guard<std::mutex> lock(_mutex);
    if (_pinstance == nullptr) {
        _pinstance = new IPFS();
    }
    Config config(configFile);
    //todo check _nodePrefix works before setting
    _nodePrefix = config.getString("ipfspath", "http://localhost:5001/api/v0/");
    _timeoutPin = config.getInteger("ipfstimeoutpin", 1200);
    _timeoutDownload = config.getInteger("ipfstimeoutdownload", 3600);
    _timeoutRetry = config.getInteger("ipfstimeoutretry", 3600);
    _maxParallel = config.getInteger("ipfsparallel", 10);
    _pinstance->start();
    return _pinstance;
}


/*
 ██████╗ ██████╗ ███╗   ██╗███████╗████████╗██████╗ ██╗   ██╗ ██████╗████████╗██╗ ██████╗ ███╗   ██╗
██╔════╝██╔═══██╗████╗  ██║██╔════╝╚══██╔══╝██╔══██╗██║   ██║██╔════╝╚══██╔══╝██║██╔═══██╗████╗  ██║
██║     ██║   ██║██╔██╗ ██║███████╗   ██║   ██████╔╝██║   ██║██║        ██║   ██║██║   ██║██╔██╗ ██║
██║     ██║   ██║██║╚██╗██║╚════██║   ██║   ██╔══██╗██║   ██║██║        ██║   ██║██║   ██║██║╚██╗██║
╚██████╗╚██████╔╝██║ ╚████║███████║   ██║   ██║  ██║╚██████╔╝╚██████╗   ██║   ██║╚██████╔╝██║ ╚████║
 ╚═════╝ ╚═════╝ ╚═╝  ╚═══╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝  ╚═════╝   ╚═╝   ╚═╝ ╚═════╝ ╚═╝  ╚═══╝
*/
IPFS::IPFS() {
    setMaxParallels(_maxParallel);
}

IPFS::~IPFS() {
}


/*
████████╗██╗  ██╗██████╗ ███████╗ █████╗ ██████╗
╚══██╔══╝██║  ██║██╔══██╗██╔════╝██╔══██╗██╔══██╗
   ██║   ███████║██████╔╝█████╗  ███████║██║  ██║
   ██║   ██╔══██║██╔══██╗██╔══╝  ██╔══██║██║  ██║
   ██║   ██║  ██║██║  ██║███████╗██║  ██║██████╔╝
   ╚═╝   ╚═╝  ╚═╝╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝╚═════╝
*/
void IPFS::mainFunction() {
    //check if we should be running at all
    if (_processOff) {
        //not using thread so just let it do nothing for half a second
        std::chrono::milliseconds dura(500);
        std::this_thread::sleep_for(dura);
        return;
    }

    //get next job if there is one
    Database* db = Database::GetInstance();
    unsigned int jobIndex;
    string cid;
    string sync;
    string extra;
    unsigned int maxSleep;  //ms
    IPFSCallbackFunction callback;
    db->getNextIPFSJob(jobIndex, cid, sync, extra, maxSleep, callback);
    if (jobIndex == 0) {
        //no waiting jobs so do nothing for a tenth of a second
        std::chrono::milliseconds dura(500);
        std::this_thread::sleep_for(dura);
        return;
    }

    //download the cid
    string content;
    bool failed = false;
    if (sync == "pin") {
        try {
            //check if max size exceeded
            if (
                    extra.empty() ||                            //no restrictions
                    (getSize(cid) < stoi(extra))            //within restrictions
                    ) {
                _command("pin/add/" + cid, {}, _timeoutPin * 1000);
            }
        } catch (const exceptionTimeout& e) {
            //don't worry about failed pin
        }
    } else {
        //figure out what the max time we should try to download the file for is
        unsigned int timeout = _timeoutDownload * 1000;
        bool lastTry = false;
        if (maxSleep < timeout) {
            timeout = maxSleep;
            lastTry = true;
        }

        //download the file
        try {
            content = _command("cat/" + cid, {}, timeout);
        } catch (const exceptionTimeout& e) {
            if (!lastTry) {
                db->pauseIPFSSync(jobIndex, sync, _timeoutRetry * 1000);
                return; //don't remove job or make callback
            }
            failed = true;
        }
    }

    //remove job
    db->removeIPFSJob(jobIndex, sync);

    //execute that functions call back
    callback(cid, extra, content, failed);
}


/*
██╗  ██╗███████╗██╗     ██████╗ ███████╗██████╗ ███████╗
██║  ██║██╔════╝██║     ██╔══██╗██╔════╝██╔══██╗██╔════╝
███████║█████╗  ██║     ██████╔╝█████╗  ██████╔╝███████╗
██╔══██║██╔══╝  ██║     ██╔═══╝ ██╔══╝  ██╔══██╗╚════██║
██║  ██║███████╗███████╗██║     ███████╗██║  ██║███████║
╚═╝  ╚═╝╚══════╝╚══════╝╚═╝     ╚══════╝╚═╝  ╚═╝╚══════╝
 */


/**
 * Converts a SHA256 hash to IPFS cid
 * This works for data that was encoded in raw mode only which has a file size limit of 2MB
 * @param hash
 * @return
 */
string IPFS::sha256ToCID(BitIO& hash) {
    if (hash.getLength() != 256) throw out_of_range("Invalid Hash Size");
    const char chars[33] = "abcdefghijklmnopqrstuvwxyz234567";

    //encode binary data
    BitIO data;
    data.appendBits(0x01551220, 32); //header
    data.appendBits(hash);
    data.appendZeros(2);
    data.movePositionToBeginning();

    //encode in base 32
    string output = "b"; //b means base 32
    for (size_t i = 0; i < 58; i++) output.push_back(chars[data.getBits(5)]);
    return output;
}

/**
 * Converts a SHA256 hash to IPFS cid
 * This works for data that was encoded in raw mode only which has a file size limit of 2MB
 * @param hash
 * @return
 */
string IPFS::sha256ToCID(const string& hash) {
    BitIO data = BitIO::makeHexString(hash);
    return sha256ToCID(data);
}


size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*) userp)->append((char*) contents, size * nmemb);
    return size * nmemb;
}

/**
 * Sends a command to the IPFS node and return result
 * @param command - should be in the format cat/cid  should not have a / at beginning
 * @param data
 * @param timeout - max number of mc to allow timeout
 * @return
 */
std::string IPFS::_command(const string& command, const map<std::string, std::string>& data, unsigned int timeout) {
    //get inputs in correct format
    string url = _nodePrefix + command;
    string postData;
    for (const auto& entry: data) {
        if (!postData.empty()) {
            postData += "&";
        }
        postData += entry.first + "=" + entry.second;
    }

    //make post request
    CURL* curl;
    CURLcode res;
    std::string htmlContent;
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &htmlContent);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
        if (timeout > 0) curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout);
        res = curl_easy_perform(curl);
        if (res == CURLE_OPERATION_TIMEDOUT) throw exceptionTimeout();
        if (res == CURLE_COULDNT_CONNECT) throw exceptionNoConnection();
        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            throw exception();
        }

        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();

    // Display the fetched HTML content
    return htmlContent;
}

/*
 ██████╗ █████╗ ██╗     ██╗     ██████╗  █████╗  ██████╗██╗  ██╗███████╗
██╔════╝██╔══██╗██║     ██║     ██╔══██╗██╔══██╗██╔════╝██║ ██╔╝██╔════╝
██║     ███████║██║     ██║     ██████╔╝███████║██║     █████╔╝ ███████╗
██║     ██╔══██║██║     ██║     ██╔══██╗██╔══██║██║     ██╔═██╗ ╚════██║
╚██████╗██║  ██║███████╗███████╗██████╔╝██║  ██║╚██████╗██║  ██╗███████║
 ╚═════╝╚═╝  ╚═╝╚══════╝╚══════╝╚═════╝ ╚═╝  ╚═╝ ╚═════╝╚═╝  ╚═╝╚══════╝
 */

void IPFS::registerCallback(const string& callbackSymbol, const IPFSCallbackFunction& callback) {
    Database* db = Database::GetInstance();
    db->registerIPFSCallback(callbackSymbol, callback);
}


/*
██╗███╗   ██╗████████╗███████╗██████╗ ███████╗ █████╗  ██████╗███████╗
██║████╗  ██║╚══██╔══╝██╔════╝██╔══██╗██╔════╝██╔══██╗██╔════╝██╔════╝
██║██╔██╗ ██║   ██║   █████╗  ██████╔╝█████╗  ███████║██║     █████╗
██║██║╚██╗██║   ██║   ██╔══╝  ██╔══██╗██╔══╝  ██╔══██║██║     ██╔══╝
██║██║ ╚████║   ██║   ███████╗██║  ██║██║     ██║  ██║╚██████╗███████╗
╚═╝╚═╝  ╚═══╝   ╚═╝   ╚══════╝╚═╝  ╚═╝╚═╝     ╚═╝  ╚═╝ ╚═════╝╚══════╝
*/
/**
 * Function to download data from IPFS and run a pre registered callback when done.
 * If sync is "" call back may be executed immediately if data already downloaded.
 * Is sync provided will always execute all values with the same sync value in order.
 * @param cid
 * @param sync - "" to specify order execution does not matter.  all values of same sync value otherwise executed in order added
 * @param extra - any value you want passed to callback
 * @param callbackRegistry - preregistered value with IPFS::registerCallback function
 */
void IPFS::callOnDownload(const string& cid, const std::string& sync, const string& extra,
                          const string& callbackRegistry, unsigned int maxTime) {
    //check if no cid
    if (cid.empty()) return;

    Database* db = Database::GetInstance();

    //check if we can do synchronously quickly
    if (sync.empty() && isPinned(cid)) {
        string content = _command("cat/" + cid);
        db->getIPFSCallback(callbackRegistry)(cid, extra, content, false);
        return;
    }

    //add type download to database
    db->addIPFSJob(cid, sync, extra, maxTime, callbackRegistry);
}

promise<string> IPFS::callOnDownloadPromise(const string& cid, const string& sync, unsigned int maxTime) {
    //check if we can do synchronously quickly
    if (sync.empty() && isPinned(cid)) {
        string content = _command("cat/" + cid);
        promise<string> result;
        result.set_value(content);
        return result;
    }

    //run asynchronously
    Database* db = Database::GetInstance();
    return db->addIPFSJobPromise(cid, sync, maxTime);
}

std::string IPFS::callOnDownloadSync(const string& cid, const string& sync, unsigned int maxTime) {
    //check if we can do synchronously quickly
    if (sync.empty() && isPinned(cid)) {
        string content = _command("cat/" + cid);
        promise<string> result;
        result.set_value(content);
        return result.get_future().get();
    }

    //run asynchronously
    Database* db = Database::GetInstance();
    return db->addIPFSJobPromise(cid, sync, maxTime).get_future().get();
}

/**
 * Pins a file
 * @param cid
 * @param maxSize - optional 0=don't pin, 1=pin no matter size, >1 pin only if up to that size(0 an option to allow easy config disabling of download)
 */
void IPFS::pin(const string& cid, unsigned int maxSize) {
    //check if no cid
    if (cid.empty()) return;
    if (maxSize == 0) return;   //skip because set to not pin

    //compute extra
    string extra = (maxSize == 1) ? "" : to_string(maxSize);    //if max size is 1 than we don't process the size

    //add type download to database
    Database* db = Database::GetInstance();
    db->addIPFSJob(cid, "pin", extra);
}


bool IPFS::isPinned(const string& cid) const {
    string results = _command("pin/ls/" + cid);
    return (results.find("is not pinned") == string::npos);
}

unsigned int IPFS::getSize(const string& cid) const {
    string stats = _command("object/stat?arg=" + cid);
    Json::Value json;
    Json::CharReaderBuilder rbuilder;
    istringstream s(stats);
    string errs;
    Json::parseFromStream(rbuilder, s, &json, &errs);

    if (json.isMember("DataSize") && json["DataSize"].isInt()) {
        return json["DataSize"].asUInt();
    }
    // Handle error case
    throw out_of_range("No size data found");
}
