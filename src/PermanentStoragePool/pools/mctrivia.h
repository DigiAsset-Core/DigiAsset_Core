//
// Created by mctrivia on 04/11/23.
//

#ifndef DIGIASSET_CORE_MCTRIVIA_H
#define DIGIASSET_CORE_MCTRIVIA_H



#include "PermanentStoragePool/PermanentStoragePool.h"
#include "utils.h"


class mctrivia : public PermanentStoragePool {
private:
    enum ServerCalls {
        KEEP_ALIVE,
        UNSUBSCRIBE,
        REPORT
    };

    std::thread _keepAliveThread;
    std::atomic<bool> _keepRunning;
    std::string _secretCode = utils::generateRandom(8, utils::CodeType::ALPHANUMERIC);
    void keepAliveTask();
    void updateBadList();
    void _callServer(ServerCalls command, const std::string& extra = "");

    std::vector<std::string> _badAssets;
    std::vector<std::string> _badFiles;
    unsigned int _badTime = 0;
    bool _visible = true;

protected:
    void _setConfig(const Config& config) override;
    void _reportAssetBad(const std::string& assetId) override;
    void _reportFileBad(const std::string& cid) override;

public:
    mctrivia();

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

class mctriviaMetaProcessor : public PermanentStoragePoolMetaProcessor {
private:
    uint64_t _spaceLeft = 0;

public:
    mctriviaMetaProcessor(const std::string& serializedData, unsigned int poolIndex);
    bool _shouldPinFile(const std::string& name, const std::string& mimeType, const std::string& cid) override; //called for each file included in asset returns if file should be pinned
};


#endif //DIGIASSET_CORE_MCTRIVIA_H
