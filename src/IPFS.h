//
// Created by mctrivia on 15/06/23.
//

#ifndef DIGIASSET_CORE_IPFS_H
#define DIGIASSET_CORE_IPFS_H

#include <sys/stat.h>
#include <vector>
#include <string>
#include <thread>
#include "DigiAssetRules.h"
#include "Threaded.h"
#include <sqlite3.h>
#include <mutex>
#include <functional>
#include <future>
#include "DigiByteCore_Types.h"

/**
 * cid,extra,content,failed
 */
using IPFSCallbackFunction = std::function<void(const std::string&, const std::string&, const std::string&, bool)>;


class IPFS : public Threaded {
/**
 * Singleton Start
 */
private:
    static IPFS* _pinstance;
    static std::mutex _mutex;

    static std::string _nodePrefix;
    static unsigned int _timeoutPin;
    static unsigned int _timeoutDownload;
    static unsigned int _timeoutRetry;
    static unsigned int _maxParallel;

protected:
    IPFS();
    ~IPFS();

public:
    IPFS(IPFS& other) = delete;
    void operator=(const IPFS&) = delete;
    static IPFS* GetInstance();
    static IPFS* GetInstance(const std::string& configFile);
/**
 * Singleton End
 */

private:

    bool _processOff = false; //used for making bootstrap database only

    void mainFunction() override;

    //TestHelpers
    static std::string
    _command(const std::string& command, const std::map<std::string, std::string>& data = {}, unsigned int timeout = 0);


public:
    static std::string _lastErrorMessage;

    //helpers
    static std::string sha256ToCID(BitIO& hash);
    static std::string sha256ToCID(const std::string& hash);

    //called by initializing code
    static void registerCallback(const std::string& callbackSymbol, const IPFSCallbackFunction& callback);

    //async requests
    void callOnDownload(const std::string& cid, const std::string& sync, const std::string& extra,
                        const std::string& callbackSymbol, unsigned int maxTime = 0);
    std::promise<std::string>
    callOnDownloadPromise(const std::string& cid, const std::string& sync = "", unsigned int maxTime = 0);
    std::string callOnDownloadSync(const std::string& cid, const std::string& sync = "", unsigned int maxTime = 0);
    void pin(const std::string& cid, unsigned int maxSize = 1);   //1 is any size

    //synchronous requests
    bool isPinned(const std::string& cid) const;
    unsigned int getSize(const std::string& cid) const;


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
