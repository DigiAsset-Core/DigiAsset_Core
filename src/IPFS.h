//
// Created by mctrivia on 15/06/23.
//

#ifndef DIGIASSET_CORE_IPFS_H
#define DIGIASSET_CORE_IPFS_H

#include "DigiAssetRules.h"
#include "DigiByteCore_Types.h"
#include "Threaded.h"
#include <functional>
#include <future>
#include <mutex>
#include <sqlite3.h>
#include <string>
#include <sys/stat.h>
#include <thread>
#include <vector>

/**
 * cid,extra,content,failed
 */
using IPFSCallbackFunction = std::function<void(const std::string&, const std::string&, const std::string&, bool)>;


class IPFS : public Threaded {

private:
    const static std::vector<std::string> _knownLostCID;
    std::string _nodePrefix = "http://localhost:5001/api/v0/";

    ///timeout times are in seconds
    unsigned int _timeoutPin = 1200;
    unsigned int _timeoutDownload = 3600;
    unsigned int _timeoutRetry = 3600;
    unsigned int _maxParallel = 10;

    bool _processOff = false; //used for making bootstrap database only

    void mainFunction() override;
    static std::string getIP();
    static std::string findPublicAddress(const std::vector<std::string>& addresses, const std::string& ip);
    std::vector<std::string> extractAddresses(const std::string& idString) const;
    static bool isLostCID(const std::string& cid);

    //TestHelpers
    std::string
    _command(const std::string& command, const std::map<std::string, std::string>& data = {}, unsigned int timeout = 0, const std::string& outputPath = "") const;


public:
    IPFS(const std::string& configFile, bool runStart = true);
    static std::string _lastErrorMessage;

    //helpers
    static std::string sha256ToCID(BitIO& hash);
    static std::string sha256ToCID(const std::string& hash);
    static bool isIPFSurl(const std::string& url);
    static std::string getCID(const std::string& url);

    //called by initializing code
    static void registerCallback(const std::string& callbackSymbol, const IPFSCallbackFunction& callback);

    //async requests
    void callOnDownload(const std::string& cid, const std::string& sync, const std::string& extra,
                        const std::string& callbackSymbol, unsigned int maxTime = 0);
    std::promise<std::string>
    callOnDownloadPromise(const std::string& cid, const std::string& sync = "", unsigned int maxTime = 0);
    std::string callOnDownloadSync(const std::string& cid, const std::string& sync = "", unsigned int maxTime = 0);
    void pin(const std::string& cid, unsigned int maxSize = 1); //1 is any size

    //synchronous requests
    bool isPinned(const std::string& cid) const;
    unsigned int getSize(const std::string& cid) const;
    void downloadFile(const std::string& cid, const std::string& filePath, bool pinAlso = false);
    std::string getPeerId() const;


    /*
    ███████╗██████╗ ██████╗  ██████╗ ██████╗ ███████╗
    ██╔════╝██╔══██╗██╔══██╗██╔═══██╗██╔══██╗██╔════╝
    █████╗  ██████╔╝██████╔╝██║   ██║██████╔╝███████╗
    ██╔══╝  ██╔══██╗██╔══██╗██║   ██║██╔══██╗╚════██║
    ███████╗██║  ██║██║  ██║╚██████╔╝██║  ██║███████║
    ╚══════╝╚═╝  ╚═╝╚═╝  ╚═╝ ╚═════╝ ╚═╝  ╚═╝╚══════╝
     */
    class exception : public std::exception {
    public:
        char* what() {
            _lastErrorMessage = "Something went wrong with IPFS controller";
            return const_cast<char*>(_lastErrorMessage.c_str());
        }
    };

    class exceptionTimeout : public exception {
    public:
        char* what() {
            _lastErrorMessage = "IPFS Command Timed";
            return const_cast<char*>(_lastErrorMessage.c_str());
        }
    };

    class exceptionNoConnection : public exception {
    public:
        char* what() {
            _lastErrorMessage = "IPFS Node Likely Down";
            return const_cast<char*>(_lastErrorMessage.c_str());
        }
    };
};


#endif //DIGIASSET_CORE_IPFS_H
