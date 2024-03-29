//
// Created by mctrivia on 02/11/23.
//

#ifndef DIGIASSET_CORE_PERMANENTSTORAGEPOOL_H
#define DIGIASSET_CORE_PERMANENTSTORAGEPOOL_H



#include "Config.h"
#include "DigiByteTransaction.h"
#include "PermanentStoragePoolMetaProcessor.h"
#include <string>


class PermanentStoragePool {
private:
    bool _subscribed = true;
    std::string _payoutAddress;
    bool _autoRemoveBad = true;

protected:
    unsigned int _poolIndex;
    virtual void _reportAssetBad(const std::string& assetId);
    virtual void _reportFileBad(const std::string& cid);
    virtual void _setConfig(const Config& config);



public:
    static std::string _lastErrorMessage;

    //called by PoolList
    void setPoolIndexAndInitialize(unsigned int index, const Config& config);

    //called by Chain Analyzer
    virtual std::string serializeMetaProcessor(const DigiByteTransaction& tx) = 0;                                              //if tx is part of PSP returns serialized data for processing metadata if not returns empty
    virtual std::unique_ptr<PermanentStoragePoolMetaProcessor> deserializeMetaProcessor(const std::string& serializedData) = 0; //create object for processing what should be pinned
    void markAssetAsPartOfPool(unsigned int assetIndex);                                                                        //called after verifying all files are part of PSP

    //called by Node Operators that subscribe to PSP(actually usually called automatically when subscribing and unsubscribing or at start if subscribed)
    virtual void start() = 0;
    virtual void stop() = 0;

    //called by api
    bool subscribed() const;                               //returns if currently subscribe to the pool or not
    std::string getPayoutAddress() const;                  //if subscribed returns payout address you subscribed with(returns error if not subscribed)
    bool isAssetPartOfPool(unsigned int assetIndex) const; //returns true if all files are part of PSP
    void repinAllFiles() const;
    unsigned int getPoolIndex() const;
    virtual bool isAssetBad(const std::string& assetId);                        //if not overridden always returns false
    void reportAssetBad(const std::string& assetId, bool internalOnly = false); //if PSP calling set internalOnly so it doesnt call psp
    void reportFileBad(const std::string& cid, bool internalOnly = false);      //if PSP calling set internalOnly so it doesnt call psp
    std::vector<std::string> getFiles();
    Json::Value toJSON();

    //called by asset creator
    virtual void enable(DigiByteTransaction& tx) = 0;            //makes changes to tx to enable psp on that transaction(must be called last before publishing.  Code must allow for 240 block delay in publishing)
    virtual uint64_t getCost(const DigiByteTransaction& tx) = 0; //estimates the cost of using this psp and returns in DGB sats(may not be exact since exchange rates may change)
    virtual std::string getName() = 0;                           //gets the name of the PSP
    virtual std::string getDescription() = 0;                    //gets the description
    virtual std::string getURL() = 0;                            //gets the PSP's website

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
            _lastErrorMessage = "Something went wrong with psp";
            return const_cast<char*>(_lastErrorMessage.c_str());
        }
    };

    class exceptionCantEnablePSP : public exception {
    public:
        char* what() {
            _lastErrorMessage = "Tried to enable PSP on a transaction that wasn't possible to enable";
            return const_cast<char*>(_lastErrorMessage.c_str());
        }
    };

    class exceptionCantLoadPSP : public exception {
    public:
        char* what() {
            _lastErrorMessage = "Couldn't load the PSP";
            return const_cast<char*>(_lastErrorMessage.c_str());
        }
    };

    class exceptionCouldntReport : public exception {
    public:
        char* what() {
            _lastErrorMessage = "Couldn't report to PSP";
            return const_cast<char*>(_lastErrorMessage.c_str());
        }
    };
};




#endif //DIGIASSET_CORE_PERMANENTSTORAGEPOOL_H
