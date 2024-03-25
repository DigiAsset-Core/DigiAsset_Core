//
// Created by mctrivia on 22/06/23.
//

#ifndef DIGIASSET_CORE_DIGIASSETTYPES_H
#define DIGIASSET_CORE_DIGIASSETTYPES_H


#include <cstdint>
#include <jsoncpp/json/value.h>
#include <string>

class DigiAsset; //forward declaration

struct AssetBasics {
    std::string assetId;
    uint64_t assetIndex;
    std::string cid;
    unsigned int height;
};

struct AssetHolder {
    std::string address;
    uint64_t count; //in sats
};

struct AssetUTXO {
    std::string txid;
    uint16_t vout;
    std::string address;
    uint64_t digibyte; //in sats
    std::vector<DigiAsset> assets;

    Json::Value toJSON(bool simplified = true) const;
};

struct Signer {
    std::string address;
    uint64_t weight;

    bool operator==(const Signer& b) const {
        if (b.address != address) return false;
        if (b.weight != weight) return false;
        return true;
    }

    bool operator!=(const Signer& b) const {
        return !(
                (b.address == address) &&
                (b.weight == weight));
    }
};

void serialize(std::vector<uint8_t>& serializedData, const Signer& input);
void deserialize(const std::vector<uint8_t>& serializedData, size_t& i, Signer& output);


struct ExchangeRate {
    std::string address;
    uint8_t index;
    std::string name; //not tested in comparison

    bool enabled() const {
        return (!address.empty());
    }

    bool operator==(const ExchangeRate& b) const {
        if (b.address != address) return false;
        if (b.address.empty()) return true; //both disabled
        if (b.index != index) return false;
        return true;
    }

    bool operator!=(const ExchangeRate& b) const {
        if (b.address.empty() && address.empty()) return false; //both disabled
        return !(
                (b.address == address) &&
                (b.index == index));
    }
};

void serialize(std::vector<uint8_t>& serializedData, const ExchangeRate& input);
void deserialize(const std::vector<uint8_t>& serializedData, size_t& i, ExchangeRate& output);


struct Royalty {
    std::string address;
    uint64_t amount;

    bool operator==(const Royalty& b) const {
        if (b.address != address) return false;
        if (b.amount != amount) return false;
        return true;
    }

    bool operator!=(const Royalty& b) const {
        return !(
                (b.address == address) &&
                (b.amount == amount));
    }
};

void serialize(std::vector<uint8_t>& serializedData, const Royalty& input);
void deserialize(const std::vector<uint8_t>& serializedData, size_t& i, Royalty& output);


struct VoteOption {
    std::string address;
    std::string label; //not tested in comparison

    bool operator==(const VoteOption& b) const {
        return (b.address == address);
    }

    bool operator!=(const VoteOption& b) const {
        return (b.address != address);
    }
};

void serialize(std::vector<uint8_t>& serializedData, const VoteOption& input);
void deserialize(const std::vector<uint8_t>& serializedData, size_t& i, VoteOption& output);

#endif //DIGIASSET_CORE_DIGIASSETTYPES_H
