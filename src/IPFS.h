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

    void mainFunction() override;
    static std::string getIP();
    static std::string findPublicAddress(const std::vector<std::string>& addresses, const std::string& ip);
    static std::vector<std::string> extractAddresses(const std::string& idString);

    //TestHelpers
    std::string
    _command(const std::string& command, const std::map<std::string, std::string>& data = {}, unsigned int timeout = 0, const std::string& outputPath = "") const;


public:
    IPFS(const std::string& configFile, bool runStart = true);

    //helpers
    static std::string sha256ToCID(BitIO& hash);
    static std::string sha256ToCID(const std::string& hash);
    static bool isIPFSurl(const std::string& url);
    static std::string getCID(const std::string& url);
    static bool isLostCID(const std::string& cid);
    static bool isValidCID(const std::string& cid);

    //called by initializing code
    static void registerCallback(const std::string& callbackSymbol, const IPFSCallbackFunction& callback);

    //async requests
    void callOnDownload(const std::string& cid, const std::string& sync, const std::string& extra,
                        const std::string& callbackSymbol, unsigned int maxTime = 0);
    std::promise<std::string>
    callOnDownloadPromise(const std::string& cid, const std::string& sync = "", unsigned int maxTime = 0);
    std::string callOnDownloadSync(const std::string& cid, const std::string& sync = "", unsigned int maxTime = 0);
    void pin(const std::string& cid, unsigned int maxSize = 1); //1 is any size
    void unpin(const std::string& cid);

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
    protected:
        std::string _lastErrorMessage;
        mutable std::string _fullErrorMessage;

    public:
        explicit exception(const std::string& message = "Unknown") : _lastErrorMessage(message) {}

        virtual const char* what() const noexcept override {
            _fullErrorMessage = "IPFS Exception: " + _lastErrorMessage;
            return _fullErrorMessage.c_str();
        }
    };

    class exceptionTimeout : public exception {
    public:
        explicit exceptionTimeout()
            : exception("Timeout") {}
    };

    class exceptionInvalidCID : public exception {
    public:
        explicit exceptionInvalidCID(const std::string& cid = "")
            : exception(cid.empty() ? "Invalid CID Provided" : cid + " is not a valid CID") {}
    };

    class exceptionNoConnection : public exception {
    public:
        explicit exceptionNoConnection()
            : exception("IPFS Node Likely Down") {}
    };
};


#endif //DIGIASSET_CORE_IPFS_H
