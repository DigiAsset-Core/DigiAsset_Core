//
// Created by mctrivia on 12/11/23.
//

#ifndef DIGIASSET_CORE_LOCAL_H
#define DIGIASSET_CORE_LOCAL_H


#define PSP_LOCAL_DB_FILENAME "local.db"

#include "PermanentStoragePool/PermanentStoragePool.h"


class local : public PermanentStoragePool {
private:
    sqlite3* _db = nullptr;
    sqlite3_stmt* _stmtCheckIfPartOfPool = nullptr;
    sqlite3_stmt* _stmtCheckIfBad = nullptr;
    sqlite3_stmt* _stmtEnableInPool = nullptr;
    sqlite3_stmt* _stmtMarkBad = nullptr;
    sqlite3_stmt* _stmtDiableFromPool = nullptr;

    bool localExists() const;
    void loadDB();
    void buildTables();
    void initializeDBValues();

protected:
    void _reportAssetBad(const std::string& assetId) override;
    void _reportFileBad(const std::string& cid) override;

public:
    local() = default;

    //called by Node Operators that subscribe to PSP
    std::string serializeMetaProcessor(const DigiByteTransaction& tx) override;                                              //if tx is part of PSP returns serialized data for processing metadata if not returns empty
    std::unique_ptr<PermanentStoragePoolMetaProcessor> deserializeMetaProcessor(const std::string& serializedData) override; //create object for processing what should be pinned
    void start() override;
    void stop() override;

    //called by API
    bool isAssetBad(const std::string& assetId) override;

    //called by asset creator
    void enable(DigiByteTransaction& tx) override;            //makes changes to tx to enable psp on that transaction(must be called last before publishing)
    uint64_t getCost(const DigiByteTransaction& tx) override; //estimates the cost of using this psp and returns in DGB sats(may not be exact since exchange rates may change)
    std::string getName() override;                           //gets the name of the PSP
    std::string getDescription() override;                    //gets the description
    std::string getURL() override;                            //gets the PSP's website
};

class localMetaProcessor : public PermanentStoragePoolMetaProcessor {
public:
    localMetaProcessor(const std::string& serializedData, unsigned int poolIndex);
    bool _shouldPinFile(const std::string& name, const std::string& mimeType, const std::string& cid) override; //called for each file included in asset returns if file should be pinned
};



#endif //DIGIASSET_CORE_LOCAL_H
