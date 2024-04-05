//
// Created by mctrivia on 07/06/23.
//

#ifndef DIGIASSET_CORE_DIGIASSET_H
#define DIGIASSET_CORE_DIGIASSET_H

#define DIGIASSET_JSON_IPFS_MAX_WAIT 1000

//todo check if min length is longer (OP_RETURN - 8bit)(OP_RETURN LENGTH - 8 bit)(DigiAsset Header - 16 bit)(version - 8 bit)(OP_CODE - 8bit)?
#define DIGIASSET_MIN_POSSIBLE_LENGTH 48

#include "BitIO.h"
#include "Database.h"
#include "DigiAssetRules.h"
#include "DigiAssetTypes.h"
#include "KYC.h"
#include <jsonrpccpp/server.h>
#include <map>
#include <string>

class DigiAsset {
    static std::string _lastErrorMessage;

    bool _existingAsset = false; //set to true if an existing asset
    bool _enableWrite = true;    //set to false if existing asset.  can be turned back to true if unlocked asset

    //asset data
    std::string _assetId;
    std::string _cid;
    DigiAssetRules _rules;
    unsigned char _divisibility;
    bool _locked;
    unsigned char _aggregation;
    unsigned int _heightCreated;
    unsigned int _heightUpdated;

    //database reference
    uint64_t _assetIndex = 0; //0=not yet created

    //issuer data
    KYC _issuer;

    //count(optional to allow including a count of specific asset type)
    uint64_t _count;

    //functions to help process chain data
    bool
    processIssuance(const getrawtransaction_t& txData, unsigned int height, unsigned char version, unsigned char opcode,
                    BitIO& dataStream);
    std::string calculateAssetId(const vin_t& firstVin, uint8_t issuanceFlags) const;
    static std::vector<uint8_t> calcSimpleScriptPubKey(const vin_t& vinData);
    static void insertSRHash(const std::string& dataToHash, std::vector<uint8_t>& result, size_t startIndex);
    static void insertSRHash(std::vector<uint8_t> dataToHash, std::vector<uint8_t>& result, size_t startIndex);
    void handleRulesConflict();

    friend class DigiAsset_calcSimpleScriptPubKey_Test;

    friend class DigiAsset_calculateAssetId_Test;

public:
    //constants
    static const unsigned int EXCHANGE_RATE_LENIENCY = 240; //number of blocks off exchange rate can be and still be excepted
    static const unsigned char AGGREGABLE = 0;
    static const unsigned char HYBRID = 1;
    static const unsigned char DISPERSED = 2;

    static const ExchangeRate standardExchangeRates[];
    static const size_t standardExchangeRatesCount = 20;
    static const std::string standardVoteAddresses[];
    static const size_t standardVoteCount = 50;

    //constructors
    DigiAsset() = default;
    DigiAsset(const getrawtransaction_t& txData, unsigned int height, unsigned char version,
              unsigned char opcode, BitIO& dataStream);

    //helper functions for preprocessing asset
    static void decodeAssetTxHeader(const getrawtransaction_t& txData, unsigned char& version, unsigned char& opcode,
                                    BitIO& dataStream);

    //constructor intended for use by Database only
    DigiAsset(uint64_t assetIndex, const std::string& assetId, const std::string& cid, const KYC& issuer,
              const DigiAssetRules& rules,
              unsigned int heightCreated, unsigned int heightUpdated, uint64_t amount);

    //comparison
    bool operator==(const DigiAsset& rhs) const;
    bool operator!=(const DigiAsset& rhs) const;

    std::string getIssuanceTXID() const;
    uint64_t getOriginalCount() const;
    void removeCount(uint64_t count);
    void setCount(uint64_t count);
    void addCount(uint64_t count);
    uint64_t getCount() const;
    std::string getStrCount() const;
    uint8_t getDecimals() const;

    uint64_t getAssetIndex(bool allowUnknownAssetIndex=false) const;
    bool isAssetIndexSet() const;
    void lookupAssetIndex(const std::string& txid, unsigned int vout);
    void setAssetIndex(uint64_t assetIndex);
    std::string getAssetId() const;
    std::string getCID() const;
    KYC getIssuer() const;
    DigiAssetRules getRules() const;
    unsigned int getHeightCreated() const;
    unsigned int getHeightUpdated() const;
    uint64_t getExpiry() const;
    bool isBad(int poolIndex = -1) const;


    bool isHybrid() const;
    bool isAggregable() const;
    bool isDispersed() const;
    bool isLocked() const;

    //functions that can only be used on assets we own and can be edited(or new assets not on chain)
    void setOwned();
    void setRules(const DigiAssetRules& rules);
    void
    checkRulesPass(const std::vector<AssetUTXO>& inputs, const std::vector<AssetUTXO>& outputs, unsigned int height,
                   uint64_t time) const;

    Value toJSON(bool simplified = false, bool ignoreIpfs = false) const;

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
        exception() = default;

        char* what() {
            _lastErrorMessage = "There was an unknown error involving DigiAsset object";
            return const_cast<char*>(_lastErrorMessage.c_str());
        }
    };

    class exceptionInvalidIssuance : public exception {
    public:
        char* what() {
            _lastErrorMessage = "Invalid Issuance";
            return const_cast<char*>(_lastErrorMessage.c_str());
        }
    };

    class exceptionInvalidTransfer : public exception {
    public:
        char* what() {
            _lastErrorMessage = "Invalid Transfer";
            return const_cast<char*>(_lastErrorMessage.c_str());
        }
    };

    class exceptionWriteProtected : public exception {
    public:
        char* what() {
            _lastErrorMessage = "Asset value is write protected"; //running setOwned may fix problem if it doesn't value can not be changed
            return const_cast<char*>(_lastErrorMessage.c_str());
        }
    };

    class exceptionUnknownAssetIndex : public exception {
    public:
        char* what() {
            _lastErrorMessage = "The asset has either not been added to database yet or have not looked it up yet"; //running setOwned may fix problem if it doesn't value can not be changed
            return const_cast<char*>(_lastErrorMessage.c_str());
        }
    };

    class exceptionInvalidMetaData : public exception {
    public:
        char* what() {
            _lastErrorMessage = "MetaData Invalid";
            return const_cast<char*>(_lastErrorMessage.c_str());
        }
    };

    class exceptionRuleFailed : public exceptionInvalidTransfer {
    private:
        std::string _message;

    public:
        exceptionRuleFailed(const std::string& rule) {
            _message = "Transaction failed because " + rule + " rule failed";
        }

        char* what() {
            _lastErrorMessage = _message;
            return const_cast<char*>(_lastErrorMessage.c_str());
        }
    };
};


#endif //DIGIASSET_CORE_DIGIASSET_H
