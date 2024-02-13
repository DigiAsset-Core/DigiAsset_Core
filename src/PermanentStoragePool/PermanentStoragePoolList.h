//
// Created by mctrivia on 04/11/23.
//

#ifndef DIGIASSET_CORE_PERMANENTSTORAGEPOOLLIST_H
#define DIGIASSET_CORE_PERMANENTSTORAGEPOOLLIST_H

#define PSP_CALLBACK_NEWMETADATA_ID "PermanentStoragePoolList::_callbackNewMetadata"
#define PSP_PIN_METADATA_LIMIT 2000000

#include "PermanentStoragePool.h"
class PermanentStoragePoolList {
private:
    std::vector<std::unique_ptr<PermanentStoragePool>> _pools;
    void addPool(std::unique_ptr<PermanentStoragePool> pool, const Config& config);

public:
    PermanentStoragePoolList(const std::string& configFile);
    ~PermanentStoragePoolList() = default;


    PermanentStoragePool* getRandomPool();
    PermanentStoragePool* getPool(unsigned int poolIndex);
    unsigned int getPoolCount();
    void processNewMetaData(const DigiByteTransaction& tx, unsigned int assetIndex, const std::string& cid);
    ///public because needs to be but should only be used by PermanentStoragePoolList.cpp
    static void
    _callbackNewMetadata(const std::string& cid, const std::string& extra, const std::string& content, bool failed);


    /**
     * Implement Iterator to allow list to be in foreach loop
     */
    using iterator = std::vector<std::unique_ptr<PermanentStoragePool>>::iterator;
    using const_iterator = std::vector<std::unique_ptr<PermanentStoragePool>>::const_iterator;

    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;
    /**
     * End of Iterator
     */
};




#endif //DIGIASSET_CORE_PERMANENTSTORAGEPOOLLIST_H
