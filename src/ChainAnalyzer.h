//
// Created by mctrivia on 02/02/23.
//

#ifndef DIGIBYTECORE_CHAINANALYZER_H
#define DIGIBYTECORE_CHAINANALYZER_H

#define CHAINANALYZER_CALLBACK_NEWMETADATA_ID  "ChainAnalyzer::_callbackNewMetadata"
#define DIGIBYTE_BLOCK1_HASH "4da631f2ac1bed857bd968c67c913978274d8aabed64ab2bcebc1665d7f4d3a0";

#define CHAINANALYZER_DEFAULT_PINASSETICON 100000
#define CHAINANALYZER_DEFAULT_PINASSETDESCRIPTION 100000
#define CHAINANALYZER_DEFAULT_PINASSETEXTRA 0
#define CHAINANALYZER_DEFAULT_PINPERMANENT 1

#define PRUNE_INTERVAL_DIVISOR 8
//higher numbers will prune more often.  Do not set higher then 100

#include <cmath>
#include <thread>
#include "Database.h"
#include "DigiByteCore.h"
#include "Threaded.h"

class ChainAnalyzer : public Threaded {
    static std::string _lastErrorMessage;

public:
    //constructor/destructor
    explicit ChainAnalyzer(DigiByteCore& digibyteCore);
    ~ChainAnalyzer();

    //load/save config
    void setFileName(const std::string& fileName);
    void saveConfig();
    void loadConfig();

    //set config values
    void setPruneAge(int age);  //-1 disable pruning
    void setPruneExchangeHistory(bool shouldPrune);
    void setPruneUTXOHistory(bool shouldPrune);
    void setPruneVoteHistory(bool shouldPrune);
    void setStoreNonAssetUTXO(bool shouldStore);

    //running state modifiers
    void restart(); //erases all data and starts syncing over

    //SYNCING is anything <0 where the number is how many blocks behind it is
    static const int SYNCED = 0;
    static const int STOPPED = 1;
    static const int INITIALIZING = 2;
    static const int REWINDING = 3;


    void startupFunction() override;
    void mainFunction() override;
    void shutdownFunction() override;

    ///public because needs to be but should only be used by DigiByteDomain.cpp
    static void
    _callbackNewMetadata(const std::string& cid, const std::string& extra, const std::string& content, bool failed);

private:
    DigiByteCore* _dgb;
    std::string _configFileName = "config.cfg";

    //config functions
    void resetConfig();
    unsigned int pruneMax(unsigned int height);
    bool shouldPruneExchangeHistory() const;
    bool shouldPruneUTXOHistory() const;
    bool shouldPruneVoteHistory() const;
    bool shouldStoreNonAssetUTXO() const;

    //state(for defaults see resetConfig() )
    int _state = STOPPED;
    int _height;
    std::string _nextHash;

    //config variables(chain data)
    int _pruneAge;           //number of blocks to keep for roll back protection(-1 don't prune, default is 1 day)
    int _pruneInterval;
    bool _pruneExchangeHistory; //if true prune "exchange"
    bool _pruneUTXOHistory;     //if true prune "utxos"
    bool _pruneVoteHistory;     //if true prune "votes
    bool _storeNonAssetUTXOs;   //if false won't bother storing NonAsset UTXOS

    //config variable(meta data) - need to be static or make entire thing singleton.  decided to make static
    static unsigned int _pinAssetIcon;
    static unsigned int _pinAssetDescription;
    static unsigned int _pinAssetExtra;
    static unsigned int _pinAssetPermanent;
    static std::map<std::string, int> _pinAssetExtraMimeTypes;

    //phases functions
    void phaseRewind();
    void phaseSync();
    void phasePrune();

    //process sub functions
    void processTX(const std::string& txid, unsigned int height);

    //helper function
    static unsigned int configSizeToInt(unsigned int value);
    static bool isIPFSLink(const std::string& url);
    static unsigned int extraFileLengthByMimeType(const std::string& mimeType);

public:
    /*
   ███████╗██████╗ ██████╗  ██████╗ ██████╗ ███████╗
   ██╔════╝██╔══██╗██╔══██╗██╔═══██╗██╔══██╗██╔════╝
   █████╗  ██████╔╝██████╔╝██║   ██║██████╔╝███████╗
   ██╔══╝  ██╔══██╗██╔══██╗██║   ██║██╔══██╗╚════██║
   ███████╗██║  ██║██║  ██║╚██████╔╝██║  ██║███████║
   ╚══════╝╚═╝  ╚═╝╚═╝  ╚═╝ ╚═════╝ ╚═╝  ╚═╝╚══════╝
    */
    class exception : public std::exception {
    private:
        std::string _message;
    public:
        exception(const std::string& message = "unknown error") {
            _message = "Something went wrong with chain analyzer: " + message;
        }

        char* what() {
            _lastErrorMessage = _message;
            return const_cast<char*>(_lastErrorMessage.c_str());
        }
    };

    class exceptionConfigFileMissing : public exception {
    public:
        char* what() {
            _lastErrorMessage = "The config file could not be found";
            return const_cast<char*>(_lastErrorMessage.c_str());
        }
    };

    class exceptionCorruptConfigFile : public exception {
    public:
        char* what() {
            _lastErrorMessage = "The config file is corrupt";
            return const_cast<char*>(_lastErrorMessage.c_str());
        }
    };

    class exceptionAlreadyPruned : public exception {
    public:
        char* what() {
            _lastErrorMessage = "Pruning can't be disabled when data has already been pruned";
            return const_cast<char*>(_lastErrorMessage.c_str());
        }
    };
};


#endif //DIGIBYTECORE_CHAINANALYZER_H
