//
// Created by mctrivia on 06/06/23.
//

#include "KYC.h"
#include "BitIO.h"
#include <algorithm>
#include <functional>
#include <utility>

using namespace std;




KYC::KYC(const string& address) {
    _address = address;
}

KYC::KYC(const string& address, const string& country, const string& name, const string& hash,
         unsigned int heightCreated, int heightRevoked) {
    _address = address;
    _country = country;
    _name = name;
    _hash = hash;
    _heightCreated = heightCreated;
    _heightRevoked = (heightRevoked > 0) ? heightRevoked : -1;
}

KYC::KYC(const getrawtransaction_t& txData, unsigned int height,
         std::function<std::string(std::string, unsigned int)> addressGetterFunction) {
    processTX(txData, height, std::move(addressGetterFunction));
}

/**
 * Decodes tx data.  Returns 1 of 3 values
 *  KYC::STANDARD - was not a KYC transaction
 *  KYC::VERIFY - was a kyc verification transaction
 *  KYC::REVOKE - was a kyc revoking transaction
 * @param txData - transaction data
 * @param height - height transaction was created at
 * @param addressGetterFunction - a function that takes a utxo's txid and output index as inputs and returns the address that created it
 * @return
 */
unsigned int KYC::processTX(const getrawtransaction_t& txData, unsigned int height,
                            const std::function<std::string(const std::string&, unsigned int)> addressGetterFunction) {
    if (processKYCVerify(txData, height, addressGetterFunction)) return VERIFY;
    if (processKYCRevoke(txData, height, addressGetterFunction)) return REVOKE;
    return NA;
}

bool KYC::processKYCVerify(const getrawtransaction_t& txData, unsigned int height,
                           const std::function<std::string(const std::string&, unsigned int)>& addressGetterFunction) {
    //check there are 3 or 4 outputs
    if ((txData.vout.size() != 3) && (txData.vout.size() != 4)) return false;

    //check output 0 is 600 sat
    if (txData.vout[0].valueS != 600) return false;

    //check output 1 is 0 sat
    if (txData.vout[1].valueS != 0) return false;

    //check output 0 and 2 have only 1 address
    if (txData.vout[0].scriptPubKey.addresses.size() != 1) return false;
    if (txData.vout[2].scriptPubKey.addresses.size() != 1) return false;

    //check encoded data on output 1 has correct header
    if (txData.vout[1].scriptPubKey.type != "nulldata") return false;
    BitIO dataStream = BitIO::makeHexString(txData.vout[1].scriptPubKey.hex);
    if (!dataStream.checkIsBitcoinOpReturn()) return false;                         //not an OP_RETURN
    if (dataStream.getBitcoinDataHeader() != BITIO_BITCOIN_TYPE_DATA) return false; //not data
    dataStream = dataStream.copyBitcoinData();                                      //strip the header out
    if (dataStream.getNumberOfBitLeft() <= 40) {
        return false;
    } //shortest possible data 2 byte header,2 byte country, 1 byte length,name or hash
    if (dataStream.get3B40String(3) != "kyc") return false;

    //check last input is a valid validator
    size_t validatorUTXOInput = txData.vin.size() - 1;
    string validatorAddress = addressGetterFunction(txData.vin[validatorUTXOInput].txid,
                                                    txData.vin[validatorUTXOInput].n);
    if (!isKYCVerifier(validatorAddress, height)) return false;

    //get address that was validated
    string validatedAddress = txData.vout[0].scriptPubKey.addresses[0];

    //get encoded data
    string countryCode;
    string hash;
    string name;
    try {
        countryCode = dataStream.get3B40String(3);
        transform(countryCode.begin(), countryCode.end(), countryCode.begin(),
                  ::toupper); //make upper case since 3B40 is lowercase
        unsigned int length = dataStream.getFixedPrecision();
        if (length == 0) {
            hash = dataStream.getHexString(64); //get 32 byte hex value(64 nibbles)
        } else {
            name = dataStream.getUTF8String(length);
        }
    } catch (const out_of_range& e) {
        return false;
    }
    if (dataStream.getNumberOfBitLeft() != 0) return false;

    //If we get to this line it is a KYC verify record

    //record kyc data
    if ((!_address.empty()) && (_address != validatedAddress)) {
        //object preloaded with data from a different address so remove it
        _heightRevoked = -1;
    }
    _address = validatedAddress;
    _country = countryCode;
    _name = name;
    _hash = hash;
    _heightCreated = height;
    return true;
}

bool KYC::processKYCRevoke(const getrawtransaction_t& txData, unsigned int height,
                           const std::function<std::string(const std::string&, unsigned int)>& addressGetterFunction) {
    //check there are at least 2 outputs
    if (txData.vout.size() < 2) return false;

    //check output 0 is 600 sat
    if (txData.vout[0].valueS != 601) return false;

    //check output 1 is 0 sat
    if (txData.vout[1].valueS != 0) return false;

    //check output 0 has only 1 address
    if (txData.vout[0].scriptPubKey.addresses.size() != 1) return false;

    //check encoded data on output 1 has correct header
    if (txData.vout[1].scriptPubKey.type != "nulldata") return false;
    BitIO dataStream = BitIO::makeHexString(txData.vout[1].scriptPubKey.hex);
    if (!dataStream.checkIsBitcoinOpReturn()) return false;                         //not an OP_RETURN
    if (dataStream.getBitcoinDataHeader() != BITIO_BITCOIN_TYPE_DATA) return false; //not data
    dataStream = dataStream.copyBitcoinData();                                      //strip the header out
    if (dataStream.getNumberOfBitLeft() == 16) return false;                        //should only contain 2 bytes
    if (dataStream.get3B40String(3) != "kyc") return false;

    //check first output is a validator
    if (!isKYCVerifier(txData.vout[0].scriptPubKey.addresses[0], height)) return false;

    //If we get to this line it is a valid KYC revocation

    //get address that was revoked
    string revokedAddress = addressGetterFunction(txData.txid, 0);

    //record kyc revocation
    if ((!_address.empty()) && (_address != revokedAddress)) {
        //object preloaded with data from a different address so remove it
        _name.clear();
        _hash.clear();
        _country.clear();
        _heightCreated = -1;
    }
    _address = revokedAddress;
    _heightRevoked = height;
    return true;
}

/**
 * Place holder function for a time when there is more than 1 verifier
 * @param address - address to be checked
 * @param height - height of transaction being checked
 * @return
 */
bool KYC::isKYCVerifier(const string& address,
                        unsigned int height) { // NOLINT(readability-convert-member-functions-to-static)
    return (address == "dgb1qm3wmsga7tmwlxnphjue2569z05dcj8uxjx69p6");
}

std::string KYC::getAddress() const {
    return _address;
}

std::string KYC::getName() const {
    if (_heightCreated == -1) throw exceptionUnknownValue();
    return _name;
}

std::string KYC::getHash() const {
    if (_heightCreated == -1) throw exceptionUnknownValue();
    return _hash;
}

std::string KYC::getCountry() const {
    if (_heightCreated == -1) throw exceptionUnknownValue();
    return _country;
}

unsigned int KYC::getHeightCreated() const {
    if (_heightCreated == -1) throw exceptionUnknownValue();
    return _heightCreated;
}

int KYC::getHeightRevoked() const {
    return _heightRevoked;
}

bool KYC::valid(unsigned int height) const {
    if (_heightCreated == -1) return false;                       //empty
    if ((height > 0) && (height < _heightCreated)) return false; //before created
    if (_heightRevoked == -1) return true;                        //never revoked
    if (height == 0) return false;                               //was revoked at some point and selected highest
    return (height < _heightRevoked);                             //valid if height before revoke
}

bool KYC::empty() const {
    return (_heightCreated == -1);
}
Json::Value KYC::toJSON() const {
    Json::Value result=Json::objectValue;
    result["address"]=_address;
    if (valid()) {
        result["country"]=_country;
        if (!_name.empty()) {
            result["name"]=_name;
        } else {
            result["hash"]=_hash;
        }
    }
    return result;
}
