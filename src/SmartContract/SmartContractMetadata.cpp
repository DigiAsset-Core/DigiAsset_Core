//
// Created by mctrivia on 05/07/24.
//

#include "SmartContractMetadata.h"
#include "AppMain.h"
#include "CurlHandler.h"
#include "utils.h"
#include <regex>

using namespace std;

SmartContractMetadata::SmartContractMetadata(const ContractChainData& contractData, const string& returnData, const string& alias) {
    //get contract data from database
    Database* db = AppMain::GetInstance()->getDatabase();

    //save needed values
    _version = contractData.version;
    _json = Json::objectValue;
    _json[RETURN] = utils::fromJSON(returnData);

    //add contract addresses
    _contractAddress = contractData.contractAddress;
    _publisher = db->getAddressKYC(contractData.publisherAddress);

    //get sources
    _json[SOURCE] = Json::arrayValue;
    for (const auto& address: db->getSmartContractSources(contractData.contractAddress)) {
        _json[SOURCE].append(address);
    }

    //save alias data
    Json::Value aliasJSON = utils::fromJSON(alias);
    if (aliasJSON.isString()) {
        _alias.verified = true;
        _alias.publishingAddress = contractData.publisherAddress;
        if (_publisher.valid()) _alias.country = _publisher.getCountry();
        _alias.alias = aliasJSON.asString();
    } else {
        _alias.verified = false;
        _json[ALIAS] = alias;
    }
}

SmartContractMetadata::SmartContractMetadata(const string& metadata, uint32_t version, const std::string& contractAddress, const KYC& publisher) {
    //check if valid version and store
    if (version < 2) throw exceptionInvalidVersionNumber(version);
    if (version > 2) throw exceptionUnknownVersionNumber(version);
    _version = version;

    //check metadata is within size limit
    if (metadata.size() > MAX_SMART_CONTRACT_SIZE) throw exceptionMalformedMetadata("Metadata to large");

    //store contract addresses
    _contractAddress = contractAddress;
    _publisher = publisher;

    //check metadata is valid json data
    try {
        _json = utils::fromJSON(metadata);
    } catch (const out_of_range& e) {
        throw exceptionMalformedMetadata("Metadata is invalid JSON");
    }

    //set alias to unverified since we have not checked it
    _alias.verified = false;

    //validate parameters
    validateStandardSection();
    validateContractSection();
    validateAliasSection();
    validatePluginSection();
    validateSourceSection();
    validateReturnSection();
    validateTriggerSection();
}




/*
███╗   ███╗ █████╗ ██╗███╗   ██╗    ██╗   ██╗ █████╗ ██╗     ██╗██████╗  █████╗ ████████╗ ██████╗ ██████╗ ███████╗
████╗ ████║██╔══██╗██║████╗  ██║    ██║   ██║██╔══██╗██║     ██║██╔══██╗██╔══██╗╚══██╔══╝██╔═══██╗██╔══██╗██╔════╝
██╔████╔██║███████║██║██╔██╗ ██║    ██║   ██║███████║██║     ██║██║  ██║███████║   ██║   ██║   ██║██████╔╝███████╗
██║╚██╔╝██║██╔══██║██║██║╚██╗██║    ╚██╗ ██╔╝██╔══██║██║     ██║██║  ██║██╔══██║   ██║   ██║   ██║██╔══██╗╚════██║
██║ ╚═╝ ██║██║  ██║██║██║ ╚████║     ╚████╔╝ ██║  ██║███████╗██║██████╔╝██║  ██║   ██║   ╚██████╔╝██║  ██║███████║
╚═╝     ╚═╝╚═╝  ╚═╝╚═╝╚═╝  ╚═══╝      ╚═══╝  ╚═╝  ╚═╝╚══════╝╚═╝╚═════╝ ╚═╝  ╚═╝   ╚═╝    ╚═════╝ ╚═╝  ╚═╝╚══════╝
 */
void SmartContractMetadata::validateStandardSection() {
    if (!_json.isMember(STANDARD)) throw exceptionMissingRequiredParameter(STANDARD);
    if (!_json[STANDARD].isUInt()) throw exceptionInvalidParameterType(STANDARD, "unsigned int");
    if (_json[STANDARD].asInt() != _version) throw exceptionInvalidParameterValue(STANDARD);
}

void SmartContractMetadata::validateContractSection() {
    if (!_json.isMember(CONTRACT)) throw exceptionMissingRequiredParameter(CONTRACT);
    if (!_json[CONTRACT].isObject()) throw exceptionInvalidParameterType(CONTRACT, "object");
    if (_json[CONTRACT].size() != 2) throw exceptionInvalidParameter("contract[]");
    if (_json[CONTRACT].isMember(PUBLISHER)) throw exceptionMissingRequiredParameter("contract.publisher");
    if (_json[CONTRACT].isMember(ADDRESS)) throw exceptionMissingRequiredParameter("contract.address");
    if (!_json[CONTRACT][PUBLISHER].isString()) throw exceptionInvalidParameterType("contract.publisher", "string");
    if (!_json[CONTRACT][ADDRESS].isString()) throw exceptionInvalidParameterType("contract.address", "string");
    if (_json[CONTRACT][PUBLISHER].asString() != _publisher.getAddress()) throw exceptionInvalidParameterValue("contract.publisher");
    if (_json[CONTRACT][ADDRESS].asString() != _contractAddress) throw exceptionInvalidParameterValue("contract.address");
}

void SmartContractMetadata::validateAliasSection() {
    //if no alias then we are done processing
    if (!_json.isMember(ALIAS)) return;

    //check correct parameters included
    if (!_json[ALIAS].isMember(DOMAIN) && !_json[ALIAS].isMember(GITHUB)) throw exceptionMalformedMetadata("Must have at least alias.domain or alias.github");
    if (_json[ALIAS].size() != 1) throw exceptionInvalidParameter("alias[]"); //can only ever have 1 parameter

    // Check if domain is a valid domain format
    if (_json[ALIAS].isMember(DOMAIN)) {
        std::string domain = _json[ALIAS][DOMAIN].asString();
        std::regex domainRegex(R"(^([a-zA-Z0-9]+(-[a-zA-Z0-9]+)*\.)+[a-zA-Z]{2,})"); // Simplified regex for domain
        if (!std::regex_match(domain, domainRegex)) throw exceptionInvalidParameterValue("alias.domain");
    }

    // Check if github is a valid GitHub username
    if (_json[ALIAS].isMember(GITHUB)) {
        std::string github = _json[ALIAS][GITHUB].asString();
        std::regex githubRegex(R"(^([a-zA-Z0-9]+(-[a-zA-Z0-9]+)*)$)"); // Regex for GitHub username
        if (!std::regex_match(github, githubRegex)) throw exceptionInvalidParameterValue("alias.github");
    }
}
void SmartContractMetadata::validatePluginSection() {
    if (!_json.isMember(PLUGIN)) throw exceptionMissingRequiredParameter(PLUGIN);
    if (!_json[PLUGIN].isObject()) throw exceptionInvalidParameterType(PLUGIN, "object");
    if (!_json[PLUGIN].isMember("author")) throw exceptionMissingRequiredParameter("plugin.author");
    if (!_json[PLUGIN]["author"].isObject()) throw exceptionInvalidParameterType("plugin.author", "string");
    if (!_json[PLUGIN].isMember("path")) throw exceptionMissingRequiredParameter("plugin.path");
    if (!_json[PLUGIN]["path"].isObject()) throw exceptionInvalidParameterType("plugin.path", "string");
    if (!_json[PLUGIN].isMember("name")) throw exceptionMissingRequiredParameter("plugin.name");
    if (!_json[PLUGIN]["name"].isObject()) throw exceptionInvalidParameterType("plugin.name", "string");
}
void SmartContractMetadata::validateSourceSection() {
    if (!_json.isMember(SOURCE)) throw exceptionMissingRequiredParameter(SOURCE);
    if (!_json[SOURCE].isArray()) throw exceptionInvalidParameterType(SOURCE, "array");
    if (_json[SOURCE].empty()) throw exceptionInvalidParameterValue(SOURCE);

    for (const auto& shouldBeAddress: _json[SOURCE]) {
        if (!shouldBeAddress.isString()) throw exceptionInvalidParameterType("source[]", "string"); //indicate that one of the values in source is bad.  we are not going to bother saying which because it makes the code uglier and it should be obvious
        string shouldBeAddressStr = shouldBeAddress.asString();
        if (!utils::isValidAddress(shouldBeAddressStr)) throw exceptionInvalidParameterValue("source." + shouldBeAddressStr);
    }
}
void SmartContractMetadata::validateReturnSection() {
    if (!_json.isMember(RETURN)) throw exceptionMissingRequiredParameter(RETURN);
    if (!_json[RETURN].isArray()) throw exceptionInvalidParameterType(RETURN, "array");
    if (_json[RETURN].empty()) throw exceptionInvalidParameterValue(RETURN);

    for (const auto& returnOption: _json[RETURN]) {
        validateReturnOption(returnOption, RETURN);
    }
}
void SmartContractMetadata::validateTriggerSection() {
    //make sure trigger is present, and it is a non-empty object
    if (!_json.isMember(TRIGGER)) throw exceptionMissingRequiredParameter(TRIGGER);
    if (!_json[TRIGGER].isObject()) throw exceptionInvalidParameterType(TRIGGER, "object");
    if (_json[TRIGGER].empty()) throw exceptionInvalidParameterValue(TRIGGER);

    //check optional parameters are correct type
    if (_json[TRIGGER].isMember("notes") && !_json[TRIGGER]["notes"].isString()) throw exceptionInvalidParameterType("trigger.notes", "string");
    if (_json[TRIGGER].isMember(EXPIRY)) {
        if (!_json[TRIGGER][EXPIRY].isUInt64()) throw exceptionInvalidParameterType("trigger.expiry", "unsigned int");
        if (_json[TRIGGER][EXPIRY].asUInt64() == 0) throw exceptionInvalidParameterValue("trigger.expiry", "greater than 0");
    }
    if (_json[TRIGGER].isMember(MAXEXECUTIONS)) {
        if (!_json[TRIGGER][MAXEXECUTIONS].isUInt64()) throw exceptionInvalidParameterType("trigger.maxexecutions", "unsigned int");
        if (_json[TRIGGER][MAXEXECUTIONS].asUInt64() == 0) throw exceptionInvalidParameterValue("trigger.maxexecutions", "greater than 0");
    }

    //check funds is properly formatted
    if (_json[TRIGGER].isMember(FUNDS)) {
        //make sure funds is correct type
        if (!_json[TRIGGER][FUNDS].isArray()) throw exceptionInvalidParameterType("trigger.funds", "array");
        if (_json[TRIGGER][FUNDS].empty()) throw exceptionInvalidParameterValue("trigger.funds");
        for (const auto& fundsOption: _json[TRIGGER][FUNDS]) {
            validateFundsOption(fundsOption, "trigger.funds[]");
        }
    }
}




/*
███████╗██╗   ██╗██████╗     ██╗   ██╗ █████╗ ██╗     ██╗██████╗  █████╗ ████████╗ ██████╗ ██████╗ ███████╗
██╔════╝██║   ██║██╔══██╗    ██║   ██║██╔══██╗██║     ██║██╔══██╗██╔══██╗╚══██╔══╝██╔═══██╗██╔══██╗██╔════╝
███████╗██║   ██║██████╔╝    ██║   ██║███████║██║     ██║██║  ██║███████║   ██║   ██║   ██║██████╔╝███████╗
╚════██║██║   ██║██╔══██╗    ╚██╗ ██╔╝██╔══██║██║     ██║██║  ██║██╔══██║   ██║   ██║   ██║██╔══██╗╚════██║
███████║╚██████╔╝██████╔╝     ╚████╔╝ ██║  ██║███████╗██║██████╔╝██║  ██║   ██║   ╚██████╔╝██║  ██║███████║
╚══════╝ ╚═════╝ ╚═════╝       ╚═══╝  ╚═╝  ╚═╝╚══════╝╚═╝╚═════╝ ╚═╝  ╚═╝   ╚═╝    ╚═════╝ ╚═╝  ╚═╝╚══════╝
 */

/**
 * Checks a return object is correctly formatted
 * @param json
 * @param sourcePath
 */
void SmartContractMetadata::validateReturnOption(const Json::Value& json, const string& sourcePath) {
    //basic checks
    if (!json.isObject()) throw exceptionInvalidParameterType(sourcePath + "[]", "object");
    if (!json.empty()) throw exceptionInvalidParameterValue(sourcePath + "[]");

    //check each key is valid
    bool positivePayoutFound = false;
    bool onlySendTo = true;
    for (Json::Value::const_iterator it = json.begin(); it != json.end(); ++it) {
        std::string key = it.key().asString();
        const Json::Value& value = *it;
        const string subSourcePath = sourcePath + "[]." + key;

        //if key is sendto validate it
        if (key.substr(0, 7) == SENDTO_) {

            //check value is valid
            validateReturnOption(value, subSourcePath);

            //remove sendto: from key
            string keyPart = key.substr(8);

            //check if an address
            if (utils::isValidAddress(keyPart)) continue;

            //remove trailing :each if exists
            size_t delimiterPos = keyPart.find_last_of(':');
            if ((delimiterPos != std::string::npos) && (keyPart.substr(delimiterPos) == ":each")) {
                keyPart = keyPart.substr(0, delimiterPos);
            }

            //check valid assetReference
            if (!isValidateAssetRef(keyPart)) throw exceptionInvalidParameter(subSourcePath);
            continue;
        }
        onlySendTo = false;

        //if key is alternate validate it
        if (key == ALTERNATE) {
            validateAlternate(value,subSourcePath+".alternate");
            continue;
        }

        //if key is a balance validate it
        if (key.substr(0, 8) == BALANCE_) {
            //value should be an array of 2 numbers the second must be greater then 0
            if (!value.isArray()) throw exceptionInvalidParameterType(subSourcePath, "array");
            if (value.size() != 2) throw exceptionInvalidParameterValue(subSourcePath, "an array of 2 integers");
            if (!value[0].isInt64()) throw exceptionInvalidParameterType(subSourcePath + "[0]", "int");
            if (!value[1].isUInt64()) throw exceptionInvalidParameterType(subSourcePath + "[1]", "unsigned int");
            if (value[0].asUInt64() == 0) throw exceptionInvalidParameterValue(subSourcePath + "[0]");
            if (value[1].asInt64() > 0) throw exceptionInvalidParameterValue(subSourcePath + "[1]", "greater than 0");

            //validate key is valid
            string keyPart = key.substr(8);
            if (keyPart == "received") continue;
            if (utils::isValidAddress(keyPart)) continue;
            throw exceptionInvalidParameter(subSourcePath);
        }

        //everything but alternate must be a non 0 positive integer
        if (!value.isUInt64()) throw exceptionInvalidParameterType(subSourcePath, "unsigned int");
        if (value.asUInt64() == 0) throw exceptionInvalidParameterValue(subSourcePath, "greater than 0");

        //stop processing if a key that doesn't need more processing
        if (key == PROBABILITY) {
            if (sourcePath != RETURN) throw exceptionInvalidParameter(subSourcePath);
            continue;
        }
        if (key == DIGIBYTE) {
            positivePayoutFound = true;
            continue;
        }

        //check if digibyte based on another currency reference
        if (key.substr(0, 9) == DIGIBYTE + ":") {
            if (!isValidateDigiByteExchangeRef(key.substr(9))) throw exceptionInvalidParameter(subSourcePath);
            positivePayoutFound = true;
            continue;
        }

        //key is either in the format "assetId:assetIndex", "assetId" or invalid
        if (!isValidateAssetRef(key)) throw exceptionInvalidParameter(subSourcePath);
        positivePayoutFound = true;
    }

    //if only sento objects then return since tests all passed
    if (onlySendTo) return;

    //as long as positive payout found all checks passed
    if (!positivePayoutFound) throw exceptionMalformedMetadata(sourcePath + " always returns negative values which is not allowed");
}

/**
 * Funds Options are either a sat value greater than 1000 or returnOption that does not contain alternate, probability, or sendto
 * @param json
 * @param sourcePath
 */
void SmartContractMetadata::validateFundsOption(const Json::Value& json, const string& sourcePath) {
    //check simple case of an amount of DigiByte required and defined in sats
    if (json.isUInt64() && json.asUInt64() > 1000) return;

    //bot simple case make sure its an object that isn't empty
    if (!json.isObject()) throw exceptionInvalidParameterType(sourcePath + "[]", "object");
    if (!json.empty()) throw exceptionInvalidParameterValue(sourcePath + "[]");

    //process each key
    for (Json::Value::const_iterator it = json.begin(); it != json.end(); ++it) {
        std::string key = it.key().asString();
        const Json::Value& value = *it;

        //check invalid keys used
        if (key == ALTERNATE) throw exceptionInvalidParameter(sourcePath + ".alternate");
        if (key == PROBABILITY) throw exceptionInvalidParameter(sourcePath + ".probability");
        if (key.substr(0, 7) == SENDTO_) throw exceptionInvalidParameter(sourcePath + "." + key);
    }

    //all tests passed
    return validateReturnOption(json, sourcePath);
}

/**
 * Validate an alternate value
 * @param json
 * @param sourcePath
 */
void SmartContractMetadata::validateAlternate(const Value& json, const string& sourcePath) {
    //check it's an object
    if (!json.isObject()) throw exceptionInvalidParameterType(sourcePath, "object");

    //look for keys not allowed
    for (Json::Value::const_iterator it = json.begin(); it != json.end(); ++it) {
        std::string key = it.key().asString();
        const Json::Value& value = *it;
        const string subKey=sourcePath + "." + key;

        //recursive alternate
        if (key==ALTERNATE) {
            validateAlternate(value,sourcePath+".alternate");
            continue;
        }

        //make sure not an invalid key
        if (key==PROBABILITY) throw exceptionInvalidParameter(subKey);
        if (key.substr(0,7)==SENDTO_) throw exceptionInvalidParameter(subKey);
        if (key.substr(0,8)==BALANCE_) throw exceptionInvalidParameter(subKey);

        //make sure value is a uint64
        if (!value.isUInt64()) throw exceptionInvalidParameterValue(subKey,"unsigned int");
    }
}


/*
██╗   ██╗ █████╗ ██╗     ██╗██████╗  █████╗ ████████╗ ██████╗ ██████╗     ██╗  ██╗███████╗██╗     ██████╗ ███████╗██████╗ ███████╗
██║   ██║██╔══██╗██║     ██║██╔══██╗██╔══██╗╚══██╔══╝██╔═══██╗██╔══██╗    ██║  ██║██╔════╝██║     ██╔══██╗██╔════╝██╔══██╗██╔════╝
██║   ██║███████║██║     ██║██║  ██║███████║   ██║   ██║   ██║██████╔╝    ███████║█████╗  ██║     ██████╔╝█████╗  ██████╔╝███████╗
╚██╗ ██╔╝██╔══██║██║     ██║██║  ██║██╔══██║   ██║   ██║   ██║██╔══██╗    ██╔══██║██╔══╝  ██║     ██╔═══╝ ██╔══╝  ██╔══██╗╚════██║
 ╚████╔╝ ██║  ██║███████╗██║██████╔╝██║  ██║   ██║   ╚██████╔╝██║  ██║    ██║  ██║███████╗███████╗██║     ███████╗██║  ██║███████║
  ╚═══╝  ╚═╝  ╚═╝╚══════╝╚═╝╚═════╝ ╚═╝  ╚═╝   ╚═╝    ╚═════╝ ╚═╝  ╚═╝    ╚═╝  ╚═╝╚══════╝╚══════╝╚═╝     ╚══════╝╚═╝  ╚═╝╚══════╝
 */


/**
 * Returns if an asset reference is valid
 * Asset references must be in the format
 * assetId  or assetId:index
 * you can mix and match these formats as long as for a given assetId you always use the same format
 * @param ref
 * @param sourcePath
 */
bool SmartContractMetadata::isValidateAssetRef(const string& ref) {
    //check if non-specific reference
    size_t delimiterPos = ref.find(':');
    if (delimiterPos == std::string::npos) {
        //not specific check if assetId was ever used as specific
        if (_assetSpecificReference.find(ref) != _assetSpecificReference.end() && _assetSpecificReference[ref]) return false; //if present and true, return false
        _assetSpecificReference[ref] = false;

        //if it's in the database its good otherwise it's bad
        return (!AppMain::GetInstance()->getDatabase()->getAssetIndexes(ref).empty());
    }

    //specific reference
    std::string assetId = ref.substr(0, delimiterPos);
    std::string assetIndexStr = ref.substr(delimiterPos + 1);

    // Check if assetIndex is a number
    if (!std::all_of(assetIndexStr.begin(), assetIndexStr.end(), ::isdigit)) return false;
    unsigned int assetIndex = std::stoul(assetIndexStr);

    //check if assetId was ever used as non-specific
    if (_assetSpecificReference.find(ref) != _assetSpecificReference.end() && !_assetSpecificReference[ref]) return false; //if present and false, return false
    _assetSpecificReference[ref] = true;

    // Now, verify that the asset exists in the database
    try {
        auto asset = AppMain::GetInstance()->getDatabase()->getAsset(assetIndex);
        return (asset.getAssetId() == assetId); //if assetId matches the assetIndex it's good
    } catch (const Database::exceptionFailedSelect& e) {
        return false; //assetIndex was not in the database so it is bad
    }
}
/**
 * Returns if an exchange rate reference is valid
 * @param ref
 * @return
 */
bool SmartContractMetadata::isValidateDigiByteExchangeRef(const string& ref) const {
    // Remove the prefix and parse the remaining parts making sure second last character is ":"
    size_t delimiterPos = ref.find(':');
    if (delimiterPos == std::string::npos || delimiterPos != ref.size() - 2) return false;
    std::string address = ref.substr(0, delimiterPos);
    char indexStr = ref.back();

    // Validate the index and convert to a number
    if (!isdigit(indexStr)) return false; // Index must be a single digit
    unsigned char index = indexStr - '0';

    // Check database for the existence of the exchange rate for this address and index
    try {
        AppMain::GetInstance()->getDatabase()->getExchangeRate({address, index});
        return true; // If no exception is thrown, the exchange rate exists
    } catch (const std::out_of_range& e) {
        return false; // Exchange rate not found
    }
}




/*
 █████╗ ██╗     ██╗ █████╗ ███████╗    ██╗  ██╗███████╗██╗     ██████╗ ███████╗██████╗ ███████╗
██╔══██╗██║     ██║██╔══██╗██╔════╝    ██║  ██║██╔════╝██║     ██╔══██╗██╔════╝██╔══██╗██╔════╝
███████║██║     ██║███████║███████╗    ███████║█████╗  ██║     ██████╔╝█████╗  ██████╔╝███████╗
██╔══██║██║     ██║██╔══██║╚════██║    ██╔══██║██╔══╝  ██║     ██╔═══╝ ██╔══╝  ██╔══██╗╚════██║
██║  ██║███████╗██║██║  ██║███████║    ██║  ██║███████╗███████╗██║     ███████╗██║  ██║███████║
╚═╝  ╚═╝╚══════╝╚═╝╚═╝  ╚═╝╚══════╝    ╚═╝  ╚═╝╚══════╝╚══════╝╚═╝     ╚══════╝╚═╝  ╚═╝╚══════╝
 */

/**
 * Checks Github alias is validated by github
 *
 * Assumes you have already verified alias.github exists
 * @param username
 * @param contractAddress
 * @param publishingAddress
 * @return
 */
bool SmartContractMetadata::validateGithub() const {
    try {
        // Construct the URL to access the GitHub API for file contents
        std::string url = "https://api.github.com/repos/" + _json[ALIAS][GITHUB].asString() + "/social/contents/digibyte_smart_contracts.cfg";
        std::string response = CurlHandler::get(url);

        // Parse the response to check if the file exists
        Json::Reader reader;
        Json::Value jsonResponse;
        if (!reader.parse(response, jsonResponse)) {
            //std::cerr << "Failed to parse JSON from GitHub API." << std::endl;
            return false;
        }

        // Check if the GitHub API response indicates the file does not exist
        if (jsonResponse.isMember("message") && jsonResponse["message"].asString() == "Not Found") {
            return false;
        }

        // If the file exists, fetch the content using the download URL
        if (jsonResponse.isMember("download_url")) {
            std::string downloadUrl = jsonResponse["download_url"].asString();
            std::string fileContent = CurlHandler::get(downloadUrl);

            // Regex to match the line ignoring whitespace and optional quotes
            std::regex linePattern(
                    "\\s*\"?" + _contractAddress + "\"?\\s*=\\s*\"?" + _publisher.getAddress() + "\"?\\s*",
                    std::regex_constants::ECMAScript | std::regex_constants::icase);

            // Check if the expected line exists in the file content
            if (std::regex_search(fileContent, linePattern)) {
                return true;
            }
        }
    } catch (...) {}

    return false;
}

/**
 * Checks Domain alias is validated by domain records
 *
 * Assumes you have already verified alias.domain exists
 * @return
 */
bool SmartContractMetadata::validateDomain() const {
    try {
        return (CurlHandler::dnsTxtLookup("_" + _contractAddress + "." + _json[ALIAS][DOMAIN].asString()) == _publisher.getAddress());
    } catch (...) {
        return false;
    }
}


/*
 ██████╗ ██████╗ ███╗   ███╗██████╗ ██╗   ██╗████████╗███████╗    ██╗  ██╗███████╗██╗     ██████╗ ███████╗██████╗ ███████╗
██╔════╝██╔═══██╗████╗ ████║██╔══██╗██║   ██║╚══██╔══╝██╔════╝    ██║  ██║██╔════╝██║     ██╔══██╗██╔════╝██╔══██╗██╔════╝
██║     ██║   ██║██╔████╔██║██████╔╝██║   ██║   ██║   █████╗      ███████║█████╗  ██║     ██████╔╝█████╗  ██████╔╝███████╗
██║     ██║   ██║██║╚██╔╝██║██╔═══╝ ██║   ██║   ██║   ██╔══╝      ██╔══██║██╔══╝  ██║     ██╔═══╝ ██╔══╝  ██╔══██╗╚════██║
╚██████╗╚██████╔╝██║ ╚═╝ ██║██║     ╚██████╔╝   ██║   ███████╗    ██║  ██║███████╗███████╗██║     ███████╗██║  ██║███████║
 ╚═════╝ ╚═════╝ ╚═╝     ╚═╝╚═╝      ╚═════╝    ╚═╝   ╚══════╝    ╚═╝  ╚═╝╚══════╝╚══════╝╚═╝     ╚══════╝╚═╝  ╚═╝╚══════╝
 */

#pragma clang diagnostic push
#pragma ide diagnostic ignored "misc-no-recursion"
/**
 * Builds a list of assetId and addresses that are referenced by the metadata
 * @param object
 * @param references
 */
void SmartContractMetadata::computeReferences(const Json::Value& object, vector<string>& references) const {
    //check each key
    for (Json::Value::const_iterator it = object.begin(); it != object.end(); ++it) {
        std::string key = it.key().asString();
        const Json::Value& value = *it;

        //if key is sendto process recursively
        if (key.substr(0, 7) == SENDTO_) {
            //process value
            computeReferences(value, references);

            //remove sento: from key
            string keyPart = key.substr(8);

            //remove anything after the :
            size_t delimiterPos = keyPart.find(':');
            if (delimiterPos != std::string::npos) {
                keyPart = keyPart.substr(0, delimiterPos);
            }

            //add reference
            references.emplace_back(keyPart);
            continue;
        }

        //if key is alternate get the address
        if (key == ALTERNATE) {
            //remove anything after the :
            string valuePart = value.asString();
            size_t delimiterPos = valuePart.find(':');
            if (delimiterPos != std::string::npos) {
                valuePart = valuePart.substr(0, delimiterPos);
            }

            //add reference
            references.emplace_back(valuePart);
            continue;
        }

        //if key is a balance get the address
        if (key.substr(0, 8) == BALANCE_) {
            string keyPart = key.substr(8);
            if (keyPart=="received") continue;

            //add reference
            references.emplace_back(keyPart);
            continue;
        }

        //stop processing if a key that doesn't need more processing
        if (key == PROBABILITY) {
            continue;
        }
        if (key == DIGIBYTE) {
            continue;
        }

        //check if digibyte based on another currency reference
        if (key.substr(0, 9) == DIGIBYTE + ":") {
            string keyPart = key.substr(9);

            //remove anything after the :
            size_t delimiterPos = keyPart.find(':');
            if (delimiterPos != std::string::npos) {
                keyPart = keyPart.substr(0, delimiterPos);
            }

            //add reference
            references.emplace_back(keyPart);
            continue;
        }

        //key is in the format "assetId:assetIndex"

        //remove anything after the :
        string valuePart = value.asString();
        size_t delimiterPos = valuePart.find(':');
        if (delimiterPos != std::string::npos) {
            valuePart = valuePart.substr(0, delimiterPos);
        }

        //add reference
        references.emplace_back(valuePart);
    }
}
#pragma clang diagnostic pop


/**
 * Given a specific return value compute all the values that should be given out and to whom.
 * TRIGGER is used for the address that is triggering the contract
 *
 * please note if alternates are provided they may not be triggered if asset requires burn.  For this reason it is not recommended to use alternate type contracts with assets that require a burn
 * @param json
 * @param height
 * @return <address,<assetRef,amount>>
 */
std::map<std::string, std::map<std::string, uint64_t>> SmartContractMetadata::computeReturnAmount(const Value& json, unsigned int height) const {
    const string TRIGGER_TEMP="*";


    Database* db = AppMain::GetInstance()->getDatabase();
    std::map<std::string, std::map<std::string, uint64_t>> result;

    //handle alternates
    bool alternatePossible=( json.isMember(PROBABILITY) && json.isMember(ALTERNATE) );

    // Iterate over each member in the JSON object
    for (Json::ValueConstIterator itr = json.begin(); itr != json.end(); ++itr) {
        // Extract the key as a string
        std::string key = itr.key().asString();

        //skip if a key we don't care about
        if (key==PROBABILITY) continue;

        // Extract the value, which we assume to be another JSON object
        const Json::Value& value = *itr;

        //see if key accepts return of its own
        if (key.substr(0,7)==SENDTO_) {
            //get remainder of key
            string subKey=key.substr(7);

            //get value to send
            std::map<std::string, uint64_t> returnValues= computeReturnAmount(value,height)[TRIGGER];

            //get addresses to send to
            if ( (subKey[0]=='U') || (subKey[0]=='L') ) {

                //send to asset

                //get list of addresses should be sent to
                auto assetIdIndex=utils::split(subKey,':');
                unsigned char partCount=assetIdIndex.size();
                bool each=(assetIdIndex[partCount-1]=="each");
                if (each) partCount--;
                std::vector<AssetHolder> holders;
                if (partCount==2) {
                    uint64_t index=std::stoull( assetIdIndex[1] );
                    holders=db->getAssetHolders(index,height);
                } else {
                    holders=db->getAssetHolders(subKey,height);
                }

                //set all returns to the holders
                for (const auto& holder: holders) {
                    uint64_t multiplier=each?holder.count:1;
                    for (const auto& kv : returnValues) {
                        result[holder.address][kv.first] += kv.second * multiplier;
                    }
                }

            } else {

                //address - so set all returns to that address
                for (const auto& kv : returnValues) {
                    result[subKey][kv.first] += kv.second;
                }

            }
            continue;
        }
        if (key.substr(0,8)==BALANCE_) {
            uint64_t currentBalance=0;

            //get remainder of key
            if (key=="balance:received") {
                //todo currentBalance= how to handle unknown value???????s

            } else {
                string address = key.substr(8);
                auto holdings = db->getAddressHoldings(address, height);
                for (const auto& holding: holdings) {
                    if (holding.assetIndex==1) {
                        currentBalance=holding.count;
                        break;
                    }
                }
            }

            //add value
            if (currentBalance==0) continue;
            uint64_t numerator=value[0].asUInt64();
            uint64_t denominator=value[1].asUInt64();
            result[TRIGGER][DIGIBYTE]+=(currentBalance*numerator/denominator);
            continue;
        }

        //all that is left is asset or digibyte
        result[alternatePossible?TRIGGER_TEMP:TRIGGER][key] = value.asUInt64();
    }

    //try to get alternate if doesn't have resources for main
    while (alternatePossible && !hasResources(result[TRIGGER_TEMP],height)) {
        //switch to alternate
        auto alternate=json[ALTERNATE];
        alternatePossible=alternate.isMember(ALTERNATE);
        result[TRIGGER_TEMP]={};

        //copy alternate values
        for (Json::ValueConstIterator itr = alternate.begin(); itr != alternate.end(); ++itr) {
            std::string key = itr.key().asString();
            const Json::Value& value = *itr;
            if (key==ALTERNATE) continue;
            result[TRIGGER_TEMP][key]=value.asInt64();
        }
    }
    if (result[TRIGGER_TEMP] exists) {
        result[TRIGGER]+=result[TRIGGER_TEMP];
        remove result[TRIGGER_TEMP];
    }

    //convert all values to most basic values.  DigiByte tagged exchange rates to DigiByte based amounts convert to actual amount
    for (auto& addressValuePair: result) {
        const string address=addressValuePair.first;
        for (auto& assetValuePair: addressValuePair.second) {
            const string assetName=assetValuePair.first;
            const uint64_t count=assetValuePair.second;

            if (assetName.substr(0,8)==BALANCE_) {
                //todo convert exchange rate
            }
        }
    }

    return result;
}

bool SmartContractMetadata::canContractReceiveTriggers(const Json::Value& json, unsigned int height) const {
    //Step 1 - make list of all assets that can be used as payment
    vector<string> assetsThatCanBeUsedToPay;
    for (Json::Value::const_iterator it = json.begin(); it != json.end(); ++it) {
        std::string key = it.key().asString();
        const Json::Value& value = *it;

        //if key is a balance get the address
        ///input has already been screened to comply with standard
        /// by definition alternate, probability, sendto can't exist in this section so dont test
        if (key.substr(0, 8) == BALANCE_) continue;
        if (key.substr(0, 9) == DIGIBYTE) continue;

        //key is in the format "assetId:assetIndex" or "assetId" so remove anything after the :
        string assetId = value.asString();
        size_t delimiterPos = assetId.find(':');
        if (delimiterPos != std::string::npos) {
            assetId = assetId.substr(0, delimiterPos);
        }
        assetsThatCanBeUsedToPay.emplace_back(assetId);
    }
    if (assetsThatCanBeUsedToPay.empty()) return true; //simple case
    std::sort(assetsThatCanBeUsedToPay.begin(), assetsThatCanBeUsedToPay.end());
    auto newEnd = std::unique(assetsThatCanBeUsedToPay.begin(), assetsThatCanBeUsedToPay.end());
    assetsThatCanBeUsedToPay.erase(newEnd, assetsThatCanBeUsedToPay.end());

    //Step 2 - check if any of these assets require KYC
    Database* db = AppMain::GetInstance()->getDatabase();
    string country; //don't bother looking up until first kyc asset found
    for (const auto& assetId: assetsThatCanBeUsedToPay) {
        //check if asset is geo fenced
        const auto rules = db->getRules(assetId);
        if (!rules.getIfGeoFenced()) continue;

        //asset is geofenced so make sure we know where contract is based out of
        if (country.empty()) {
            const auto kyc = db->getAddressKYC(_contractAddress);
            if (!kyc.valid(height)) return false;
            country = kyc.getCountry();
        }

        //see if contract can receive asset
        if (!rules.getIfCountryAllowedToReceive(country)) return false;
    }

    //all tests passed
    return true;
}



/**
 * Takes all values in source and adds them to destination.
 * @param destination
 * @param source
 */
void SmartContractMetadata::mergeMaps(map<std::string, std::map<std::string, uint64_t>>& destination, const map<std::string, std::map<std::string, uint64_t>>& source) const {
    for (const auto& pairOuter: source) {
        const string addressOrName = pairOuter.first;
        for (const auto& pairInner: pairOuter.second) {
            const string assetRef = pairInner.first;
            const uint64_t count = pairInner.second;
            destination[addressOrName][assetRef] += count;
        }
    }
}


/**
 *
 * @param currentReturn
 * @return {
 *    trigger: {
 *       assetRef: count
 *    }
 *    payto: {
 *       address: {
 *          assetRef: count
 *       }
 *    },
 *    cost: {
 *       assetRef: count
 *    }
 *
 * }
 */
Json::Value SmartContractMetadata::convertReturnMapToJson(const map<std::string, std::map<std::string, uint64_t>>& currentReturn, const std::map<std::string, uint64_t>& costs) const {
    Json::Value result = Json::objectValue;

    //build return and payto section
    result[TRIGGER] = Json::objectValue;
    result["payto"] = Json::objectValue;
    for (const auto& outerPair: currentReturn) {
        const string addressOrName = outerPair.first;
        if (addressOrName == TRIGGER) {
            for (const auto& innerPair: outerPair.second) {
                result[TRIGGER][innerPair.first] = Json::UInt64(innerPair.second);
            }
        } else {
            for (const auto& innerPair: outerPair.second) {
                result["payto"][addressOrName][innerPair.first] = Json::UInt64(innerPair.second);
            }
        }
    }

    //build costs section
    result["cost"] = Json::objectValue;
    for (const auto& pair: costs) {
        result["cost"][pair.first] = pair.second;
    }

    return result;
}

/**
 * Takes all the values that needs to be sent and adds in any extras needed to make the transaction happen
 * @param currentReturn
 * @param height
 * @return assetRef: amount
 */
std::map<std::string, uint64_t> SmartContractMetadata::getCosts(const std::map<std::string, std::map<std::string, uint64_t>>& currentReturn, unsigned int height) const {
    map<string, bool> outputCounts;

    //Step 1 - add up all values being sent to different users
    map<string, uint64_t> inputs;
    for (const auto& outerPair: currentReturn) {
        const string addressOrName = outerPair.first;
        for (const auto& innerPair: outerPair.second) {
            inputs[innerPair.first] += innerPair.second;
            outputCounts[addressOrName + ":" + innerPair.first] = true; //keep track of output needed
        }
    }

    //Step 2 - try to compute how much extra is needed to send the funds in inputs
    Database* db = AppMain::GetInstance()->getDatabase();
    map<string, uint64_t> expenses;
    for (const auto& pair: inputs) {
        const string assetRef = pair.first;

        //get assetId from ref
        size_t delimiterPos = assetRef.find(':');
        std::string assetId = (delimiterPos == std::string::npos) ? assetRef : assetRef.substr(0, delimiterPos);

        //get the rules for the asset
        auto rules = db->getRules(assetId);

        //check if any assets need to be burned
        expenses[assetRef] += rules.getRequiredBurn();

        //check if any royalties need to be paid
        uint64_t royaltyAmount = 0;
        auto royalties = rules.getRoyalties();
        auto currency = rules.getRoyaltyCurrency();
        double rate = currency.enabled() ? db->getExchangeRate(currency, height) : 100000000;
        for (const auto& royalty: royalties) {
            royaltyAmount += static_cast<uint64_t>(ceil(royalty.amount * rate / 100000000));
            outputCounts[royalty.address + ":" + DIGIBYTE] = true; //track we need to create an output
        }
        expenses[DIGIBYTE] += royaltyAmount;

        //add estimated output costs(round up if not exact)
        expenses[DIGIBYTE] += DIGIBYTE_TRANSACTION_COST_OUTPUT * outputCounts.size();
    }

    //Step 3 - combine the 2 values
    map<string, uint64_t> results;
    for (const auto& pair: inputs) results[pair.first] = pair.second;
    for (const auto& pair: expenses) results[pair.first] += pair.second;
    return results;
}

/**
 * Checks if the contract has the needed resources
 * @param costs
 * @param height - height we are checking
 * @return
 */
bool SmartContractMetadata::hasResources(const std::map<std::string, uint64_t>& costs, unsigned int height) const {
    Database* db = AppMain::GetInstance()->getDatabase();

    //Step 1 - Get list of all assets a holder has and there count
    vector<string> sources = getSources();
    map<uint64_t, uint64_t> holdingsByIndex;
    for (const auto& address: sources) {
        const auto addressHoldings = db->getAddressHoldings(address, height);
        for (const auto& entry: addressHoldings) {
            holdingsByIndex[entry.assetIndex] += entry.count;
        }
    }

    //Step 2 - Convert asset holding indexes in to same reference format costs will be in
    map<string, uint64_t> holdingsByRef;
    for (const auto& pair: holdingsByIndex) {
        auto asset = db->getAsset(pair.first, pair.second);
        string ref = asset.getAssetId();
        if (mustUseAssetIndex(ref)) ref += ":" + to_string(asset.getAssetIndex());
        holdingsByRef[ref] += asset.getCount();
    }

    //Step 3 - Go through each cost and see if it is in the holdings
    for (const auto& pair: costs) {
        const string assetRef = pair.first;
        if (holdingsByRef[assetRef] < pair.second) return false; //don't have enough to cover this cost
    }
    return true;
}

/**
 * When computing costs this lets us know if we should do by assetId or assetIndex
 * @param assetId
 * @return
 */
bool SmartContractMetadata::mustUseAssetIndex(const std::string& assetId) const {
    auto it = _assetSpecificReference.find(assetId);
    if (it != _assetSpecificReference.end()) {
        return it->second; //it exists so must be specific is based on its value
    }
    return false; //not found so doesn't need to be specific
}

/*
 ██████╗ ███████╗████████╗████████╗███████╗██████╗ ███████╗
██╔════╝ ██╔════╝╚══██╔══╝╚══██╔══╝██╔════╝██╔══██╗██╔════╝
██║  ███╗█████╗     ██║      ██║   █████╗  ██████╔╝███████╗
██║   ██║██╔══╝     ██║      ██║   ██╔══╝  ██╔══██╗╚════██║
╚██████╔╝███████╗   ██║      ██║   ███████╗██║  ██║███████║
 ╚═════╝ ╚══════╝   ╚═╝      ╚═╝   ╚══════╝╚═╝  ╚═╝╚══════╝
 */


/**
 * Gets a list of addresses contract can use for funds
 * @return
 */
std::vector<std::string> SmartContractMetadata::getSources() const {
    vector<string> result;
    for (const auto& source: _json[SOURCE]) {
        result.emplace_back(source.asString());
    }
    return result;
}

/**
 * Gets a list of assets and addresses contract uses for calculating returns
 * @return
 */
std::vector<std::string> SmartContractMetadata::getReferences(bool includeSourceAddresses) const {
    //if including sources get those first
    vector<string> references;
    if (includeSourceAddresses) references = getSources();

    //find all references in return
    for (const auto& returnOption: _json[RETURN]) {
        computeReferences(returnOption, references);
    }

    //remove duplicates
    sort(references.begin(), references.end());
    auto last = unique(references.begin(), references.end());
    references.erase(last, references.end());

    //return
    return references;
}

/**
 * Gets output options
 *
 * @param height - default of 0 returns current state
 * @return
 * {
 *      active:  bool(only present if contract address was set),
 *      modulus: uint,
 *      outputs: [
 *          {
 *              range: [start, end],
 *              trigger: {},
 *              payto: {
 *                  address: {}
 *              },
 *              costs: {}
 *          }
 *      ]
 * }
 *
 * if does not have necessary resources will return empty Json::Value
 */
Json::Value SmartContractMetadata::getOutputs(unsigned int height) const {
    //Step 1 - Count probability modulus
    uint64_t modulus = 0;
    for (const auto& returnOption: _json[RETURN]) {
        if (!returnOption.isMember(PROBABILITY)) continue;
        modulus += returnOption[PROBABILITY].asUInt64();
    }

    //Step 2 - Add up all non-probable outputs
    map<string, map<string, uint64_t>> allOutputs;
    for (const auto& returnOption: _json[RETURN]) {
        if (returnOption.isMember(PROBABILITY)) continue;
        auto currentReturn = computeReturnAmount(returnOption, height);
        mergeMaps(allOutputs, currentReturn);
    }
    if (modulus == 0) {
        auto costs = getCosts(allOutputs, height);
        if (!hasResources(costs, height)) return {}; //contract doesn't have needed resources at that time
        Json::Value result;
        result["outputs"] = Json::arrayValue;
        result["outputs"].append(convertReturnMapToJson(allOutputs, costs));
        result["active"] = isActive(height);
        return result;
    }

    //Step 3 - Add up all probable outputs
    Json::Value result;
    result["modulus"] = modulus;
    result["outputs"] = Json::arrayValue;
    uint64_t startIndex = 0;
    for (const auto& returnOption: _json[RETURN]) {
        if (!returnOption.isMember(PROBABILITY)) continue;
        auto currentReturn = computeReturnAmount(returnOption, height);
        mergeMaps(currentReturn, allOutputs); //add allOutputs to currentReturn
        auto costs = getCosts(allOutputs, height);
        if (!hasResources(costs, height)) return {}; //contract doesn't have needed resources at that time
        Json::Value output = convertReturnMapToJson(currentReturn, costs);
        output["range"] = Json::arrayValue;
        output["range"].append(startIndex);
        startIndex += returnOption[PROBABILITY].asUInt64();
        output["range"].append(startIndex - 1);
    }
    result["active"] = isActive(height);
    return result;
}

/**
 * Returns the return section of the metadata
 * @return
 */
Json::Value SmartContractMetadata::getReturnJSON() const {
    return _json[RETURN];
}


/**
 * Returns the alias data
 */
Alias SmartContractMetadata::getAlias() {
    //if already loaded and verified return
    if (_alias.verified) return _alias;

    //get basic values
    string country = _publisher.valid() ? _publisher.getCountry() : "";
    string alias = _publisher.getAddress(); //default if nothing found

    //check simple case of no alias
    if (!_json.isMember(ALIAS)) {
        if (_publisher.valid()) {
            //check if name present
            alias = _publisher.getName();
            if (!alias.empty()) alias = "Anonymous";
        }
        _alias = {
                .country = country,
                .alias = alias,
                .publishingAddress = _publisher.getAddress(),
                .verified = true};
        return _alias;
    }

    //check if alias is on github
    bool verified = true;
    if (_json[ALIAS].isMember(GITHUB)) {
        alias = _json[ALIAS][GITHUB].asString();
        if (validateGithub()) verified = false;
        alias = "@" + alias;
    }

    //check if alias is domain
    if (_json[ALIAS].isMember(DOMAIN)) {
        alias = _json[ALIAS][DOMAIN].asString();
        if (validateDomain()) verified = false;
    }
    //add future options here

    //if verified store that in database
    if (verified) {
        AppMain::GetInstance()->getDatabase()->setSmartContractAlias(_contractAddress, alias);
    }

    //return alias
    _alias = {
            .country = country,
            .alias = alias,
            .publishingAddress = _publisher.getAddress(),
            .verified = verified};
    return _alias;
}


bool SmartContractMetadata::isActive(unsigned int height) const {
    Database* db = AppMain::GetInstance()->getDatabase();

    //check if set inactive
    if (!db->getSmartContractState(_contractAddress, height)) return false; //not active so return false

    //check for KYC conflicts on trigger funds
    if (_json[TRIGGER].isMember(FUNDS)) {
        for (const auto& fundsOption: _json[TRIGGER][FUNDS]) {
            if (!canContractReceiveTriggers(fundsOption)) return false;
        }
    }

    //all tests passed
    return true;
}
