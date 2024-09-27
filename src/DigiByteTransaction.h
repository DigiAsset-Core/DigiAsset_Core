//
// Created by mctrivia on 17/06/23.
//

#ifndef DIGIASSET_CORE_DIGIBYTETRANSACTION_H
#define DIGIASSET_CORE_DIGIBYTETRANSACTION_H

//amount of fees to pay for a transaction part(usually max number of bytes it can be)
#define DIGIBYTE_TRANSACTION_COST_HEADERS   44
#define DIGIBYTE_TRANSACTION_COST_INPUT    180
#define DIGIBYTE_TRANSACTION_COST_OUTPUT    34
#define DIGIBYTE_TRANSACTION_COST_DATA      90


#include "Database.h"
#include "DigiAsset.h"
#include "DigiAssetTypes.h"
#include "SmartContract/SmartContractList.h"
#include <jsonrpccpp/server.h>

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
    const static unsigned int ENCRYPTED_KEY = 30;
    const static unsigned int SMARTCONTRACT_DEACTIVATE = 40;
    const static unsigned int SMARTCONTRACT_ACTIVATE = 41;
    const static unsigned int SMARTCONTRACT_PUBLISH = 42;


    std::vector<AssetUTXO> _inputs;
    std::vector<AssetUTXO> _outputs;
    DigiAsset _newAsset;
    unsigned char _txType = STANDARD; //must default to STANDARD since not set in code if STANDARD
    bool _assetFound;
    bool _unintentionalBurn = false;
    unsigned int _height;
    std::string _txid; //if set tx is not writable(existing)
    std::string _blockHash;
    uint64_t _time; //internal use only it will only be correct for transactions built from tx data or new constructed transactions

    //type KYC_* Only
    KYC _kycData;

    //type ENCRYPTED_KEY and STANDARD only
    std::string _opReturnHex;

    //type SmartContract Only
    ContractChainData _smartContractData;

    //type Exchange_PUBLISH Only
    std::vector<double> _exchangeRate;

    //type DigiAsset_* Only
    unsigned char _assetTransactionVersion;


    //tx process TestHelpers
    bool decodeAssetTX(const getrawtransaction_t& txData, int dataIndex);
    bool decodeExchangeRate(const getrawtransaction_t& txData, int dataIndex);
    bool decodeKYC(const getrawtransaction_t& txData, int dataIndex);
    bool decodeEncryptedKeyTx(const getrawtransaction_t& txData, int dataIndex);
    void storeUnknown(const getrawtransaction_t& txData, int dataIndex); //todo need to store locally and then add to database when called
    bool decodeSmartContract(const getrawtransaction_t& txData);

    //asset process TestHelpers
    void decodeAssetTransfer(BitIO& dataStream, const std::vector<AssetUTXO>& inputAssets, uint8_t type);
    void checkRulesPass() const;
    void addAssetToOutput(size_t output, const DigiAsset& asset);

public:
    explicit DigiByteTransaction();
    DigiByteTransaction(const std::string& txid, unsigned int height = 0, bool dontBotherIfNotSpecial = false);

    void addToDatabase();
    void updateDatabase();  //only to be used by Database.cpp upgradeDatabaseContent
    void lookupAssetIndexes();

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

    int isSmartContractStateChange() const; //-1= no, 0=yes disabled, 1=yes enabled
    bool isSmartContractPublishing() const; //true=maybe they published to chain but it is only valid if ipfs data is correct should try to load using db->getSmartContract
    std::string getSmartContractAddress() const; //returns address so you can load from database.


    AssetUTXO getInput(size_t n) const;
    AssetUTXO getOutput(size_t n) const;
    unsigned int getInputCount() const;
    unsigned int getOutputCount() const;
    unsigned int getHeight() const;

    void addDigiByteOutput(const std::string& address, uint64_t amount);
    void addDigiAssetOutput(const std::string& address, const std::vector<DigiAsset>& assets);

    Value toJSON(const Value& original = Json::objectValue) const;



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
            _fullErrorMessage = "DigiByte Transaction Exception: " + _lastErrorMessage;
            return _fullErrorMessage.c_str();
        }
    };

    class exceptionNotEnoughFunds : public exception {
    public:
        explicit exceptionNotEnoughFunds()
            : exception("Not enough funds") {}
    };
};


#endif //DIGIASSET_CORE_DIGIBYTETRANSACTION_H
