//
// Created by mctrivia on 30/01/23.
//
#ifndef SHA256_LENGTH
#define SHA256_LENGTH   32
#endif

#ifndef STRINGIZE
#define STRINGIZE(x) #x
#define STRINGIZE_VALUE_OF(x) STRINGIZE(x)
#endif




#ifndef DIGIBYTECORE_DATABASECHAIN_H
#define DIGIBYTECORE_DATABASECHAIN_H

#define DIGIBYTECORE_DATABASE_CHAIN_WATCH_MAX 20


#include <sqlite3.h>
#include <string>
#include <vector>
#include "KYC.h"
#include "BitIO.h"
#include "DigiAssetTypes.h"
#include "DigiAssetRules.h"
#include "IPFS.h"
#include "DigiByteCore.h"
#include "UTXOCache.h"
#include <mutex>
#include <future>

/**
 * commands to run on database

CREATE INDEX kyc_height_index ON kyc(height);
DROP TABLE assetMetaHistory;
DROP TABLE assets;
CREATE TABLE "assets" ("assetIndex" INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "assetId" TEXT NOT NULL, "cid" TEXT, "issueAddress" TEXT NOT NULL, "rules" BLOB, "heightCreated" INTEGER NOT NULL, "heightUpdated" INTEGER NOT NULL, "expires" INTEGER, "bad" BOOL);
INSERT INTO "assets" VALUES (1,'DigiByte','QmfSVLAntanDUKrEHUnTXRh53GLUBHFfxk5x6LH4zz9PM4','STANDARD',NULL,1,1,NULL,false);

 *
 */



class Database {
/**
 * Singleton Start
 */
private:
    static Database* _pinstance;
    static std::mutex _mutex;

protected:
    Database() = default;
    ~Database();

public:
    Database(Database& other) = delete;
    void operator=(const Database&) = delete;
    static Database* GetInstance();
    static Database* GetInstance(const std::string& fileName);

/**
 * Singleton End
 */
private:
    sqlite3* _db = nullptr;
    sqlite3_stmt* _stmtCheckFlag = nullptr;
    sqlite3_stmt* _stmtSetFlag = nullptr;
    sqlite3_stmt* _stmtGetBlockHeight = nullptr;
    sqlite3_stmt* _stmtSetBlockHash = nullptr;
    sqlite3_stmt* _stmtGetBlockHash = nullptr;
    sqlite3_stmt* _stmtCreateUTXO = nullptr;
    sqlite3_stmt* _stmtSpendUTXO = nullptr;
    sqlite3_stmt* _stmtIsWatchAddress = nullptr;
    sqlite3_stmt* _stmtAddWatchAddress = nullptr;
    sqlite3_stmt* _stmtGetSpendingAddress = nullptr;
    sqlite3_stmt* _stmtAddExchangeRate = nullptr;
    sqlite3_stmt* _stmtAddKYC = nullptr;
    sqlite3_stmt* _stmtRevokeKYC = nullptr;
    sqlite3_stmt* _stmtPruneUTXOs = nullptr;
    sqlite3_stmt* _stmtExchangeRatesAtHeight = nullptr;
    sqlite3_stmt* _stmtPruneExchangeRate = nullptr;
    sqlite3_stmt* _stmtGetVoteCount = nullptr;
    sqlite3_stmt* _stmtPruneVote = nullptr;
    sqlite3_stmt* _stmtAddVote = nullptr;
    sqlite3_stmt* _stmtGetAssetUTXO = nullptr;
    sqlite3_stmt* _stmtGetAssetHolders = nullptr;
    sqlite3_stmt* _stmtAddAsset = nullptr;
    sqlite3_stmt* _stmtUpdateAsset = nullptr;
    sqlite3_stmt* _stmtGetAssetIndex = nullptr;
    sqlite3_stmt* _stmtGetAssetIndexOnUTXO = nullptr;
    sqlite3_stmt* _stmtGetHeightAssetCreated = nullptr;
    sqlite3_stmt* _stmtGetAssetRules = nullptr;
    sqlite3_stmt* _stmtGetAsset = nullptr;
    sqlite3_stmt* _stmtGetKYC = nullptr;
    sqlite3_stmt* _stmtGetValidExchangeRate = nullptr;
    sqlite3_stmt* _stmtGetCurrentExchangeRate = nullptr;
    sqlite3_stmt* _stmtGetNextIPFSJob = nullptr;
    sqlite3_stmt* _stmtSetIPFSPauseSync = nullptr;
    sqlite3_stmt* _stmtClearNextIPFSJob_a = nullptr;
    sqlite3_stmt* _stmtClearNextIPFSJob_b = nullptr;
    sqlite3_stmt* _stmtInsertIPFSJob = nullptr;
    sqlite3_stmt* _stmtClearIPFSPause = nullptr;
    sqlite3_stmt* _stmtSetIPFSLockSync = nullptr;
    sqlite3_stmt* _stmtSetIPFSLockJob = nullptr;
    sqlite3_stmt* _stmtSetIPFSPauseJob = nullptr;
    sqlite3_stmt* _stmtGetDomainAssetId = nullptr;
    sqlite3_stmt* _stmtAddDomain = nullptr;
    sqlite3_stmt* _stmtRevokeDomain = nullptr;
    sqlite3_stmt* _stmtSetDomainMasterAssetId_a = nullptr;
    sqlite3_stmt* _stmtSetDomainMasterAssetId_b = nullptr;
    sqlite3_stmt* _stmtGetPermanentPaid = nullptr;
    sqlite3_stmt* _stmtRemoveNonReachable = nullptr;
    sqlite3_stmt* _stmtInsertPermanent = nullptr;
    sqlite3_stmt* _stmtRepinAssets = nullptr;
    sqlite3_stmt* _stmtRepinPermanent = nullptr;

    //core wallet pointer
    DigiByteCore* _dgb = nullptr;

    //locks
    std::mutex _mutexGetNextIPFSJob;

    void buildTables(unsigned int dbVersionNumber=0);
    void initializeClassValues();

    //flag table
    int getFlagInt(const std::string& flag);
    void setFlagInt(const std::string& flag, int state);
    std::map<std::string, int> _flagState;

    //exchangeWatch table
    std::vector<std::string> _exchangeWatchAddresses;

    //TestHelpers
    static int defaultCallback(void* NotUsed, int argc, char** argv, char** azColName);

    //change database
    void load(const std::string& newFileName = "chain.db"); //used for testing

    //ipfs ram db values
    std::vector<std::pair<std::string, uint64_t>> _ipfsCurrentlyPaused;
    std::map<std::string, IPFSCallbackFunction> _ipfsCallbacks = {
            {"",
             [](const std::string&, const std::string&, const std::string&, bool) {}}    //generic do nothing callback
    };

    //utxo cache(helps speed up spam sections)
    UTXOCache _recentNonAssetUTXO{100000};

    //DigiBYte Domain ram values
    std::vector<std::string> _masterDomainAssetId = {};
public:
    static std::string _lastErrorMessage;

    //link Core wallet
    void setDigiByteCore(DigiByteCore& core);

    //performance related
    void startTransaction();
    void endTransaction();
    void
    disableWriteVerification(); //on power failure not all commands may be written.  If using need to check at startup

    //reset database
    void reset();   //used in case of roll back exceeding pruned history

    //assets table
    uint64_t addAsset(const DigiAsset& asset);
    DigiAsset getAsset(uint64_t assetIndex, uint64_t amount = 0) const;
    uint64_t getAssetIndex(const std::string& assetId, const std::string& txid = "", unsigned int vout = 0) const;

    //assets table not to be used on assets that may have more than one assetIndex
    DigiAssetRules getRules(const std::string& assetId) const;
    unsigned int getAssetHeightCreated(const std::string& assetId, unsigned int backupHeight, uint64_t& assetIndex);

    //block table
    void setBlockHash(uint height, const std::string& hash);
    std::string getBlockHash(uint height) const;
    uint getBlockHeight() const;
    void clearBlocksAboveHeight(uint height);

    //exchange table
    void addExchangeRate(const std::string& address, unsigned int index, unsigned int height, double exchangeRate);
    void pruneExchange(unsigned int height);
    double getAcceptedExchangeRate(const ExchangeRate& rate, unsigned int height) const;
    double getCurrentExchangeRate(const ExchangeRate& rate) const;

    //exchange watch table
    bool isWatchAddress(const std::string& address) const;
    void addWatchAddress(const std::string& address);

    //flag table
    int getBeenPrunedExchangeHistory();   //-1 = never, above=height which anything below may be pruned
    int getBeenPrunedUTXOHistory();   //-1 = never, above=height which anything below may be pruned
    int getBeenPrunedVoteHistory();   //-1 = never, above=height which anything below may be pruned
    bool getBeenPrunedNonAssetUTXOHistory();
    void setBeenPrunedExchangeHistory(int height);   //-1 = never
    void setBeenPrunedUTXOHistory(int height);   //-1 = never
    void setBeenPrunedVoteHistory(int height);   //-1 = never
    void setBeenPrunedNonAssetUTXOHistory(bool state);

    //kyc table
    void
    addKYC(const std::string& address, const std::string& country, const std::string& name, const std::string& hash,
           unsigned int height);
    void revokeKYC(const std::string& address, unsigned int height);
    KYC getAddressKYC(const std::string& address) const;

    //utxos table
    void createUTXO(const AssetUTXO& value, unsigned int heightCreated);
    void spendUTXO(const std::string& txid, unsigned int vout, unsigned int heightSpent);  //returns total amount spent
    std::string getSendingAddress(const std::string& txid, unsigned int vout);
    void pruneUTXO(unsigned int height);
    AssetUTXO getAssetUTXO(const std::string& txid, unsigned int vout);
    std::vector<AssetHolder> getAssetHolders(uint64_t assetIndex) const;
    unsigned int getPermanentSize(const std::string& txid);

    //vote table
    void addVote(const std::string& address, unsigned int assetIndex, uint64_t count, unsigned int height);
    void pruneVote(unsigned int height);

    //IPFS table
    void registerIPFSCallback(const std::string& callbackSymbol, const IPFSCallbackFunction& callback);
    void getNextIPFSJob(unsigned int& jobIndex, std::string& cid, std::string& sync, std::string& extra,
                        unsigned int& maxSleep, IPFSCallbackFunction& callback);
    void pauseIPFSSync(unsigned int jobIndex, const std::string& sync, unsigned int pauseLengthInSeconds = 3600);
    void removeIPFSJob(unsigned int jobIndex, const std::string& sync);
    unsigned int addIPFSJob(const std::string& cid, const std::string& sync = "pin", const std::string& extra = "",
                            unsigned int maxSleep = 0, const std::string& callbackSymbol = "");
    std::promise<std::string>
    addIPFSJobPromise(const std::string& cid, const std::string& sync = "", unsigned int maxTime = 0);
    IPFSCallbackFunction& getIPFSCallback(const std::string& callbackSymbol);

    //DigiByte Domain table(these should only ever be called by DigiByteDomain.cpp
    void revokeDomain(const std::string& domain);
    void addDomain(const std::string& domain, const std::string& assetId);
    std::string getDomainAssetId(const std::string& domain,
                                 bool returnErrorIfRevoked = true) const;
    std::string getDomainAddress(const std::string& domain) const;
    bool isMasterDomainAssetId(const std::string& assetId) const;
    bool isActiveMasterDomainAssetId(const std::string& assetId) const;
    void setMasterDomainAssetId(const std::string& assetId);
    void setDomainCompromised();
    bool isDomainCompromised() const;

    //Permanent table
    void addToPermanent(const string& cid);
    void repinPermanent();

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
            _lastErrorMessage = "Something went wrong with database";
            return const_cast<char*>(_lastErrorMessage.c_str());
        }
    };

    class exceptionFailedToOpen : public exception {
    public:
        char* what() {
            _lastErrorMessage = "Couldn't open or create the database";
            return const_cast<char*>(_lastErrorMessage.c_str());
        }
    };

    class exceptionFailedSQLCommand : public exception {
        char* what() {
            _lastErrorMessage = "Failed to create sql command";
            return const_cast<char*>(_lastErrorMessage.c_str());
        }
    };


    class exceptionFailedToCreateTable : public exceptionFailedSQLCommand {
    public:
        char* what() {
            _lastErrorMessage = "Failed to create table";
            return const_cast<char*>(_lastErrorMessage.c_str());
        }
    };

    class exceptionFailedInsert : public exceptionFailedSQLCommand {
    public:
        char* what() {
            _lastErrorMessage = "Failed to insert into table";
            return const_cast<char*>(_lastErrorMessage.c_str());
        }
    };

    class exceptionFailedSelect : public exceptionFailedSQLCommand {
    public:
        char* what() {
            _lastErrorMessage = "Failed to select from table";
            return const_cast<char*>(_lastErrorMessage.c_str());
        }
    };

    class exceptionFailedUpdate : public exceptionFailedSQLCommand {
    public:
        char* what() {
            _lastErrorMessage = "Failed to update table";
            return const_cast<char*>(_lastErrorMessage.c_str());
        }
    };

    class exceptionFailedDelete : public exceptionFailedSQLCommand {
    public:
        char* what() {
            _lastErrorMessage = "Failed to delete from table";
            return const_cast<char*>(_lastErrorMessage.c_str());
        }
    };

    class exceptionCreatingStatement : public exceptionFailedSQLCommand {
    public:
        char* what() {
            _lastErrorMessage = "Failed to create statement for table";
            return const_cast<char*>(_lastErrorMessage.c_str());
        }
    };

    class exceptionFailedReset : public exception {
    public:
        char* what() {
            _lastErrorMessage = "Failed to reset table";
            return const_cast<char*>(_lastErrorMessage.c_str());
        }
    };

    class exceptionDataPruned : public exception {
    public:
        char* what() {
            _lastErrorMessage = "The requested data has been pruned";
            return const_cast<char*>(_lastErrorMessage.c_str());
        }
    };
};


#endif //DIGIBYTECORE_DATABASECHAIN_H
