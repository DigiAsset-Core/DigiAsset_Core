//
// Created by mctrivia on 14/06/23.
//

#ifndef DIGIASSET_CORE_DIGIASSETRULES_H
#define DIGIASSET_CORE_DIGIASSETRULES_H


#include "DigiAssetTypes.h"
#include "BitIO.h"
#include <jsonrpccpp/server.h>
#include "DigiByteCore_Types.h"

class DigiAssetRules {
    static std::string _lastErrorMessage;

    bool _noRules = true;
    bool _rewritable = false;
    bool _movable = true;
    uint64_t _signersRequired = 0;
    std::vector<Signer> _signers;
    ExchangeRate _exchangeRate;
    std::vector<Royalty> _royalties;
    std::vector<std::string> _countryList;
    bool _countryListIsBan = false;    //false+_countryList.empty()=kyc not required
    //true+_countryList.empty()=kyc required from any country
    //!_countryList.empty()=kyc required limited countries allowed
    uint64_t _expiry = EXPIRE_NEVER;  //584 million years from now = never
    std::vector<VoteOption> _voteOptions;
    uint64_t _deflate = 0;

    std::string _voteLabelsCID;  //empty if labels have been processed already

    void processSigners(const getrawtransaction_t& txData, BitIO& opReturnData);
    void processExchangeRate(const getrawtransaction_t& txData, BitIO& opReturnData);
    void processRoyalties(const getrawtransaction_t& txData, BitIO& opReturnData);
    void processCountryLimits(const getrawtransaction_t& txData, BitIO& opReturnData);
    void processVoteAndExpiry(const getrawtransaction_t& txData, BitIO& opReturnData, const std::string& cid);
    void processDeflate(const getrawtransaction_t& txData, BitIO& opReturnData);

    friend void serialize(std::vector<uint8_t>& serializedData, const DigiAssetRules& input);
    friend void deserialize(const std::vector<uint8_t>& serializedData, size_t& i, DigiAssetRules& output);
public:
    static const unsigned char RULE_SIGNER = 0;
    static const unsigned char RULE_ROYALTIES = 1;
    static const unsigned char RULE_KYC_WHITE_LIST = 2;
    static const unsigned char RULE_KYC_BLACK_LIST = 3;
    static const unsigned char RULE_VOTE = 4;
    static const unsigned char RULE_EXPIRES = 4;  //vote length=0 cutoff!=0
    static const unsigned char RULE_DEFLATE = 5;
    static const unsigned char RULE_EXCHANGE_RATE = 9;

    static const unsigned char RULE_END = 15;

    static const uint64_t MIN_EPOCH_VALUE = 1577836800000;        //expiry bellow this are block height above time in ms
    static const uint64_t EXPIRE_NEVER = std::numeric_limits<uint64_t>::max();


    DigiAssetRules() = default;
    DigiAssetRules(const getrawtransaction_t& txData, BitIO& opReturnData, const std::string& cid,
                   unsigned char opCode);

    void lock();
    bool isRewritable() const;
    bool empty() const;

    uint64_t getExpiry() const;
    bool isExpiryHeight() const;
    bool expires() const;

    //getters
    uint64_t getRequiredSignerWeight() const; //0 if none required
    std::vector<Signer> getSigners() const;
    bool getIfRequiresRoyalty() const;
    ExchangeRate getRoyaltyCurrency() const;
    std::vector<Royalty> getRoyalties() const;
    bool getIfGeoFenced() const;
    bool getIfCountryAllowedToReceive(const std::string& country) const;
    bool getIfExpired(unsigned int height, uint64_t time) const; //time in seconds since epoch
    bool getIfVote() const;
    bool getIfVoteRestricted() const; //returns true if only allowed to send to vote addresses
    std::vector<VoteOption> getVoteOptions();
    bool getIfValidVoteAddress(const std::string& address) const;
    uint64_t getRequiredBurn() const;

    //setters
    void setRewritable(bool state = true);
    void setRequireSigners(uint64_t requiredWeight, const std::vector<Signer>& signers);
    void setRequireKYC();   //sets so can be sent to any KYCd address
    void setDoesNotRequireKYC();
    void setRequireKYC(const std::vector<std::string>& countries,
                       bool banList = false);        //sets countries allowed to hold(optionally countries not allowed)
    void setRoyalties(const std::vector<Royalty>& royalties, const ExchangeRate& rate = {});
    void setVote(const std::vector<VoteOption>& voteOptions, uint64_t expiry = EXPIRE_NEVER);
    void setExpiry(uint64_t expiry);
    void setDeflationary(uint64_t deflateRate);

    //comparators
    bool operator==(const DigiAssetRules& b);       //needed for testing class


    Json::Value toJSON();

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
            _lastErrorMessage = "There was an unknown error involving DigiAsset Rules object";
            return const_cast<char*>(_lastErrorMessage.c_str());
        }
    };

    class exceptionInvalidRule : public exception {
    public:
        char* what() {
            _lastErrorMessage = "Invalid Rule defined";
            return const_cast<char*>(_lastErrorMessage.c_str());
        }
    };

    class exceptionVoteOptionsCorrupt : public exception {
    public:
        char* what() {
            _lastErrorMessage = "Vote option meta data is corrupt or missing";
            return const_cast<char*>(_lastErrorMessage.c_str());
        }
    };

};


#endif //DIGIASSET_CORE_DIGIASSETRULES_H
