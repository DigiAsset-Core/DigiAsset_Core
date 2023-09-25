//
// Created by mctrivia on 17/06/23.
//

#ifndef DIGIASSET_CORE_DIGIBYTETRANSACTION_H
#define DIGIASSET_CORE_DIGIBYTETRANSACTION_H

//todo check if min length is longer (OP_RETURN - 8bit)(OP_RETURN LENGTH - 8 bit)(DigiAsset Header - 16 bit)(version - 8 bit)(OP_CODE - 8bit)?
#define DIGIASSET_MIN_POSSIBLE_LENGTH 48


#include "DigiAsset.h"
#include <jsonrpccpp/server.h>
#include "Database.h"
#include "DigiAssetTypes.h"

struct UTXO {
    std::string txid;
    unsigned int vout;
};

class DigiByteTransaction {
    const static unsigned int STANDARD = 0;
    const static unsigned int DIGIASSET_ISSUANCE = 1;
    const static unsigned int DIGIASSET_TRANSFER = 2;
    const static unsigned int DIGIASSET_BURN = 3;
    const static unsigned int KYC_ISSUANCE = 10;
    const static unsigned int KYC_REVOKE = 11;
    const static unsigned int EXCHANGE_PUBLISH = 20;


    std::vector<AssetUTXO> _inputs;
    std::vector<AssetUTXO> _outputs;
    DigiAsset _newAsset;
    unsigned char _txType = STANDARD;   //must default to STANDARD since not set in code if STANDARD
    bool _assetFound;
    bool _unintentionalBurn = false;
    unsigned int _height;
    std::string _txid;      //if set tx is not writable(existing)
    std::string _blockHash;
    uint64_t _time;  //internal use only it will only be correct for transactions built from tx data or new constructed transactions         //todo

    //type KYC_* Only
    KYC _kycData;

    //type Exchange_PUBLISH Only
    std::vector<double> _exchangeRate;

    //type DigiAsset_* Only
    unsigned char _assetTransactionVersion;


    //tx process TestHelpers
    void processAssetTX(const getrawtransaction_t& txData);
    bool processExchangeRate(const getrawtransaction_t& txData);
    bool processKYC(const getrawtransaction_t& txData);

    //asset process TestHelpers
    void processAssetTransfer(BitIO& dataStream, const std::vector<AssetUTXO>& inputAssets, uint8_t type);
    void checkRulesPass() const;
    void addAssetToOutput(size_t output, const DigiAsset& asset);

public:

    explicit DigiByteTransaction();
    DigiByteTransaction(const string& txid, DigiByteCore& core, unsigned int height = 0);

    void addToDatabase(const std::string& optionalMetaCallbackSymbol = "");

    bool isStandardTransaction() const;

    bool isNonAssetTransaction() const;
    bool isIssuance() const;
    bool isTransfer(bool includeIntentionalBurn = false) const;
    bool isBurn(bool includeUnintentionalBurn = false) const;
    bool isUnintentionalBurn() const;
    DigiAsset getIssuedAsset() const;

    bool isKYCTransaction() const;
    bool isKYCRevoke() const;
    bool isKYCIssuance() const;
    KYC getKYC() const;

    bool isExchangeTransaction() const;
    size_t getExchangeRateCount() const;
    double getExchangeRate(uint8_t i) const;
    ExchangeRate getExchangeRateName(uint8_t i) const;


    AssetUTXO getInput(size_t n) const;
    AssetUTXO getOutput(size_t n) const;

    Value toJSON(const Value& original = Json::objectValue) const;

};


#endif //DIGIASSET_CORE_DIGIBYTETRANSACTION_H
