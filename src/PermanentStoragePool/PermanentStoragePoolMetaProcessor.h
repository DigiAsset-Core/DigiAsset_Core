//
// Created by mctrivia on 04/11/23.
//

#ifndef DIGIASSET_CORE_PERMANENTSTORAGEPOOLMETAPROCESSOR_H
#define DIGIASSET_CORE_PERMANENTSTORAGEPOOLMETAPROCESSOR_H


#include <string>


class PermanentStoragePoolMetaProcessor {
protected:
    unsigned int _poolIndex;
    virtual bool _shouldPinFile(const std::string& name, const std::string& mimeType, const std::string& cid) = 0; //called for each file included in asset returns if file should be pinned
public:
    bool shouldPinFile(const std::string& name, const std::string& mimeType, const std::string& cid); //called for each file included in asset returns if file should be pinned
    explicit PermanentStoragePoolMetaProcessor(unsigned int poolIndex);
};



#endif //DIGIASSET_CORE_PERMANENTSTORAGEPOOLMETAPROCESSOR_H
