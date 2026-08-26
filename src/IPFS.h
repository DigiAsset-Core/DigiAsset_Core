//
// Created by mctrivia on 15/06/23.
//

#ifndef DIGIASSET_CORE_IPFS_H
#define DIGIASSET_CORE_IPFS_H

#include "DigiAssetRules.h"
#include "DigiByteCore_Types.h"
#include "Threaded.h"
#include <atomic>
#include <functional>
#include <future>
#include <mutex>
#include <sqlite3.h>
#include <set>
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

    //cache of cids pinned on the node so isPinned never blocks the callers(the chain
    //analyzer asks for every pool issuance it processes - one HTTP round trip each was
    //enough to stall sync 400x when the ipfs daemon was busy)
    mutable std::set<std::string> _pinnedCache;
    mutable std::mutex _pinnedCacheMutex;
    mutable bool _pinnedCacheLoaded = false;

    ///timeout times are in seconds
    unsigned int _timeoutPin = 1200;
    unsigned int _timeoutDownload = 3600;
    unsigned int _timeoutRetry = 3600;
    ///applies to the requests that only ask the local node a question - pin/ls, id, and reading
    ///content the node has already pinned.  Anything that may have to fetch from the network
    ///asks for _timeoutDownload or _timeoutPin instead.  Without a limit here a wedged daemon
    ///held on to whichever thread called it, and some of those calls are made while an rpc
    ///request is being answered
    unsigned int _timeoutCommand = 30;
    unsigned int _maxParallel = 10;

    ///a node with a dead or wedged ipfs daemon hits these paths once per asset, so the
    ///warnings are throttled - the point is to make the reason visible, not to bury the log
    static const unsigned int WARNING_REPEAT_SECONDS = 60;
    mutable std::atomic<long long> _lastTimeoutWarning{0};
    mutable std::atomic<long long> _lastOfflineWarning{0};
    static bool _shouldWarn(std::atomic<long long>& lastWarning);

    void mainFunction() override;
    static std::string getIP();
    static std::string findPublicAddress(const std::vector<std::string>& addresses, const std::string& ip);
    static std::vector<std::string> extractAddresses(const std::string& idString);

    //TestHelpers
    std::string
    _command(const std::string& command, const std::map<std::string, std::string>& data = {}, unsigned int timeout = 0, const std::string& outputPath = "") const;


public:
    IPFS(const std::string& configFile, bool runStart = true);
    ~IPFS() override;

    ///aborts in flight http requests to the ipfs node before joining the job threads -
    ///a pin or download can legitimately block for many minutes and shutdown shouldn't
    ///wait for them.  Aborted jobs look like timeouts so they stay queued in the
    ///database and resume on next start.
    void stop() override;

    //helpers
    static std::string sha256ToCID(BitIO& hash);
    static std::string sha256ToCID(const std::string& hash);
    static std::string cidToSha256(const std::string& cid);
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
    std::string addFile(const std::string& content, bool pinFile = true) const;
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
