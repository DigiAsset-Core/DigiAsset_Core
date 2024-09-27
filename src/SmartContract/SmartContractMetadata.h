//
// Created by mctrivia on 05/07/24.
//

#ifndef DIGIASSET_CORE_SMARTCONTRACTMETADATA_H
#define DIGIASSET_CORE_SMARTCONTRACTMETADATA_H


#define MAX_SMART_CONTRACT_SIZE 500000



#include "KYC.h"
#include "SmartContractList.h"
#include <cstdint>
#include <jsoncpp/json/value.h>
#include <string>

struct Alias {
    std::string country;    //empty if not kyc verified
    std::string alias;      //name they wish to be called[in order: alias if defined, (full name or Anonymous) if kyc verified, publishing address
    std::string publishingAddress;
    bool verified;          //if the alias was verified(alias=publishingAddress is always true) //todo check if false is default
};

class SmartContractMetadata {
    const std::string EXPIRY="expiry";
    const std::string MAXEXECUTIONS="maxexecutions";
    const std::string TRIGGER="trigger";
    const std::string FUNDS="funds";
    const std::string RETURN="return";
    const std::string SOURCE="source";
    const std::string ALIAS="alias";
    const std::string STANDARD="standard";
    const std::string CONTRACT="contract";
    const std::string PUBLISHER="publisher";
    const std::string ADDRESS="address";
    const std::string DOMAIN="domain";
    const std::string GITHUB="github";
    const std::string PLUGIN="plugin";
    const std::string ALTERNATE="alternate";
    const std::string PROBABILITY="probability";
    const std::string DIGIBYTE="digibyte";

    const std::string SENDTO_="sendto:";
    const std::string BALANCE_="balance:";


    uint32_t _version;
    std::string _contractAddress;
    KYC _publisher;
    Alias _alias;
    std::map<std::string, bool> _assetSpecificReference;    //assetId: specific(true), generic(false)
    Json::Value _json;


    //main level validators
    void validateStandardSection();
    void validateContractSection();
    void validateAliasSection();
    void validatePluginSection();
    void validateSourceSection();
    void validateReturnSection();//todo check if output to be impossible, do to output being to big
    void validateTriggerSection();

    //sub level validators
    void validateReturnOption(const Json::Value& json, const std::string& sourcePath);
    void validateFundsOption(const Json::Value& json, const std::string& sourcePath);
    void validateAlternate(const Json::Value& json, const std::string& sourcePath);

    //validator helpers
    bool isValidateAssetRef(const std::string& ref);
    bool isValidateDigiByteExchangeRef(const std::string& ref) const;

    //alias helpers
    bool validateGithub() const;
    bool validateDomain() const;

    //helper functions for computing values
    void computeReferences(const Json::Value& json, std::vector<std::string>& references) const;
    std::map<std::string,uint64_t> getCosts(const std::map<std::string, std::map<std::string, uint64_t>>& currentReturn, unsigned int height=0) const;
    std::map<std::string, std::map<std::string, uint64_t>> computeReturnAmount(const Json::Value& json, unsigned int height=0) const;

    void mergeMaps(std::map<std::string, std::map<std::string, uint64_t>>& destination, const std::map<std::string, std::map<std::string, uint64_t>>& source) const;
    Json::Value convertReturnMapToJson(const std::map<std::string, std::map<std::string, uint64_t>>& currentReturn, const std::map<std::string,uint64_t>& costs) const;
    bool hasResources(const std::map<std::string,uint64_t>& currentReturn, unsigned int height=0) const;
    bool mustUseAssetIndex(const std::string& assetId) const;
    bool canContractReceiveTriggers(const Json::Value& json, unsigned int height=0) const;


public:
    SmartContractMetadata() = delete;

    SmartContractMetadata(const std::string& metadata, uint32_t version, const std::string& contractAddress, const KYC& publisher);
    SmartContractMetadata(const ContractChainData& contractData, const std::string& returnData, const std::string& alias);   //todo make private

    std::vector<std::string> getSources() const;
    std::vector<std::string> getReferences(bool includeSourceAddresses=false) const;
    Json::Value getOutputs(unsigned int height=0) const;
    Json::Value getReturnJSON() const;
    Alias getAlias();
    bool isActive(unsigned int height=0) const;

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
        exception(const std::string& message = "Unknown") : _lastErrorMessage(message) {}

        virtual const char* what() const noexcept override {
            _fullErrorMessage = "SmartContract Metadata Exception: " + _lastErrorMessage;
            return _fullErrorMessage.c_str();
        }
    };

    //Versions 0 and 1 are not allowed by specs.  This error means a value that is not allowed was used.
    class exceptionAliasCouldNotBeValidated : public exception {
    public:
        explicit exceptionAliasCouldNotBeValidated()
            : exception("Alias could not be validated") {}
    };

    //Versions 0 and 1 are not allowed by specs.  This error means a value that is not allowed was used.
    class exceptionInvalidVersionNumber : public exception {
    public:
        explicit exceptionInvalidVersionNumber(uint32_t versionNumber)
            : exception(std::to_string(versionNumber) + " is not valid version number") {}
    };

    //This error means it is an unknown version and should be ignored until code upgrade
    class exceptionUnknownVersionNumber : public exception {
    public:
        explicit exceptionUnknownVersionNumber(uint32_t versionNumber)
            : exception(std::to_string(versionNumber) + " is unknown upgrade may be required") {}
    };

    class exceptionMalformedMetadata : public exception {
    public:
        explicit exceptionMalformedMetadata(const std::string& error)
            : exception(error) {}
    };

    class exceptionMissingRequiredParameter : public exceptionMalformedMetadata {
    public:
        explicit exceptionMissingRequiredParameter(const std::string& parameter)
            : exceptionMalformedMetadata("Missing required parameter " + parameter) {}
    };

    class exceptionInvalidParameter : public exceptionMalformedMetadata {
    public:
        explicit exceptionInvalidParameter(const std::string& parameter)
            : exceptionMalformedMetadata("Parameter " + parameter + " is not allowed") {}
    };

    class exceptionInvalidParameterType : public exceptionMalformedMetadata {
    public:
        explicit exceptionInvalidParameterType(const std::string& parameter, const std::string& type)
            : exceptionMalformedMetadata("Parameter " + parameter + " must be a " + type) {}
    };

    class exceptionInvalidParameterValue : public exceptionMalformedMetadata {
    public:
        explicit exceptionInvalidParameterValue(const std::string& parameter)
            : exceptionMalformedMetadata("Parameter " + parameter + " is invalid") {}
        explicit exceptionInvalidParameterValue(const std::string& parameter, const std::string& wordsToPutAfterMustBe)
            : exceptionMalformedMetadata("Parameter " + parameter + " is invalid it must be " + wordsToPutAfterMustBe) {}
    };
};



#endif //DIGIASSET_CORE_SMARTCONTRACTMETADATA_H
