//
// Created by mctrivia on 13/09/23.
//

#ifndef DIGIASSET_CORE_UTXOCACHE_H
#define DIGIASSET_CORE_UTXOCACHE_H

using namespace std;

#include <vector>
#include <cstring>  // For memcpy
#include <unordered_set>
#include <unordered_map>
#include <string>

class UTXOCache {
public:
    UTXOCache(size_t capacity);

    void add(const string& txid, unsigned int vout);
    bool exists(const string& txid, unsigned int vout);

private:
    static const size_t ValueSize = 36; // 288 bits / 8 bits per byte = 36 bytes

    size_t capacity_;
    size_t size_;
    size_t head_;
    std::vector<unsigned char> values_;
    std::unordered_set<const unsigned char*> valueSet_;
    std::unordered_map<const unsigned char*, size_t> valueIndices_;
    void makeUnsignedChar(const string& txid, unsigned int vout, unsigned char* out);

    void removeOldest();
    void removeAtIndex(size_t index);
};


#endif //DIGIASSET_CORE_UTXOCACHE_H
