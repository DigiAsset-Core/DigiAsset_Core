//
// Created by mctrivia on 19/05/24.
//

#ifndef DIGIASSET_CORE_SMARTCONTRACTLIST_H
#define DIGIASSET_CORE_SMARTCONTRACTLIST_H


#define SMARTCONTRACT_CALLBACK_NEWMETADATA_ID  "SmartContractList::_callbackNewMetadata"
#define MAX_SMART_CONTRACT_VERSION_SUPPORTED 2
#define SMART_CONTRACT_MAX_IPFS_TIME 600000

#include "DigiAsset.h"
#include "KYC.h"
#include "QueueThread.h"
#include "SmartContractHandler.h"
#include <condition_variable>
#include <jsoncpp/json/value.h>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>


struct ContractChainData {
    std::string publisherAddress;
    std::string contractAddress;
    uint32_t version;
    std::string cid;    //will be empty in case of a state change
};


class SmartContractList {
private:
    std::map<std::string, std::shared_ptr<SmartContractHandler>> _contracts;

    void loadPlugin(const std::string& path);

    //alias lookup que
    static std::function<void(const std::string&)> _smartContractAliasProcessor;
    static QueueThread<std::string> _aliasLookUpQueue;



public:
    SmartContractList();
    void updateAvailableContracts();
    std::shared_ptr<SmartContractHandler> getContract(const std::string& name);
    bool empty();

    static ContractChainData processTX(const getrawtransaction_t& txData, unsigned int height,
                                const std::function<std::string(const std::string&, unsigned int)>& addressGetterFunction);    //return address empty if not

    //for use by contracts only
    static std::function<std::string(const std::string&, const std::string&)> rpcInterface;

    // Iterator support
    std::map<std::string, std::shared_ptr<SmartContractHandler>>::iterator begin();
    std::map<std::string, std::shared_ptr<SmartContractHandler>>::iterator end();
    std::map<std::string, std::shared_ptr<SmartContractHandler>>::const_iterator begin() const;
    std::map<std::string, std::shared_ptr<SmartContractHandler>>::const_iterator end() const;
    //iterator will return .first = name,  .second = contract

    ///public because needs to be but should only be used by SmartContractList.cpp
    static void _callbackNewMetadata(const std::string& cid, const std::string& extra, const std::string& content,
                                              bool failed);
};



#endif //DIGIASSET_CORE_SMARTCONTRACTLIST_H
