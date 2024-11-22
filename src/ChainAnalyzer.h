//
// Created by mctrivia on 02/02/23.
//

#ifndef DIGIBYTECORE_CHAINANALYZER_H
#define DIGIBYTECORE_CHAINANALYZER_H

#define DIGIBYTE_BLOCK1_HASH "4da631f2ac1bed857bd968c67c913978274d8aabed64ab2bcebc1665d7f4d3a0";

#define PRUNE_INTERVAL_DIVISOR 8
//higher numbers will prune more often.  Do not set higher then 100

#include "Database.h"
#include "DigiByteCore.h"
#include "Threaded.h"
#include <cmath>
#include <thread>

class ChainAnalyzer : public Threaded {
public:
    //constructor/destructor
    explicit ChainAnalyzer();
    ~ChainAnalyzer();

    //load/save config
    void setFileName(const std::string& fileName);
    void saveConfig();
    void loadConfig();
    void loadFake(unsigned int databaseHeight, int syncLevel);

    //set config values
    void setPruneAge(int age); //-1 disable pruning
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
    static const int BUSY = 4;

    //get state
    int getSync() const;
    unsigned int getSyncHeight() const;

    std::string printProfilingInfo() {
        long long totalDuration = _processTransactionRunTime;
        unsigned int transactions = _processTransactionRunCount;
        long long avgDuration = transactions > 0 ? totalDuration / transactions : 0;

        std::ostringstream oss;
        oss << std::right << std::setw(30) << "ChainAnalyzer Tx Process"
            << std::setw(20) << totalDuration
            << std::setw(20) << avgDuration
            << std::setw(20) << transactions << std::endl;

        totalDuration = _saveTransactionRunTime;
        transactions = _saveTransactionRunCount;
        avgDuration = transactions > 0 ? totalDuration / transactions : 0;

        oss << std::right << std::setw(30) << "ChainAnalyzer Tx Save"
            << std::setw(20) << totalDuration
            << std::setw(20) << avgDuration
            << std::setw(20) << transactions << std::endl;

        totalDuration = _clearAddressCacheRunTime;
        transactions = _clearAddressCacheRunCount;
        avgDuration = transactions > 0 ? totalDuration / transactions : 0;

        oss << std::right << std::setw(30) << "ChainAnalyzer Cache Clean"
            << std::setw(20) << totalDuration
            << std::setw(20) << avgDuration
            << std::setw(20) << transactions << std::endl;

        return oss.str();
    }

private:
    std::string _configFileName = "config.cfg";

    //thread overrides
    void startupFunction() override;
    void mainFunction() override;
    void shutdownFunction() override;

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
    int _pruneAge; //number of blocks to keep for roll back protection(-1 don't prune, default is 1 day)
    int _pruneInterval;
    bool _pruneExchangeHistory; //if true prune "exchange"
    bool _pruneUTXOHistory;     //if true prune "utxos"
    bool _pruneVoteHistory;     //if true prune "votes
    bool _storeNonAssetUTXOs;   //if false won't bother storing NonAsset UTXOS
    bool _verifyDatabaseWrite;  //if set to false will write without checking
    bool _showAllBlockSyncTime; //if true will not collapse blocks of 100 together when behind

    //config variable(meta data) - need to be static or make entire thing singleton.  decided to make static
    static unsigned int _pinAssetIcon;
    static unsigned int _pinAssetDescription;
    static unsigned int _pinAssetExtra;
    static unsigned int _pinAssetPermanent;
    static std::map<std::string, int> _pinAssetExtraMimeTypes;

    //time stats
    long long _processTransactionRunTime = 0;
    unsigned int _processTransactionRunCount = 0;
    long long _saveTransactionRunTime = 0;
    unsigned int _saveTransactionRunCount = 0;
    long long _clearAddressCacheRunTime = 0;
    unsigned int _clearAddressCacheRunCount = 0;

    //phases functions
    void phaseRewind();
    void phaseSync();
    void phasePrune();

    //process sub functions
    void processTX(const std::string& txid, unsigned int height);

    friend class Database; //so database can modify state
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
    protected:
        std::string _lastErrorMessage;
        mutable std::string _fullErrorMessage;

    public:
        explicit exception(const std::string& message = "Unknown") : _lastErrorMessage(message) {}

        virtual const char* what() const noexcept override {
            _fullErrorMessage = "Chain Analyzer: " + _lastErrorMessage;
            return _fullErrorMessage.c_str();
        }
    };

    class exceptionAlreadyPruned : public exception {
    public:
        explicit exceptionAlreadyPruned()
            : exception("Already been pruned") {}
    };
};


#endif //DIGIBYTECORE_CHAINANALYZER_H
