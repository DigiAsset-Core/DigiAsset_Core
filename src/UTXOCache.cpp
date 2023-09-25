//
// Created by mctrivia on 13/09/23.
//

#include <cstring>
#include <ios>
#include <sstream>
#include "UTXOCache.h"

using namespace std;

UTXOCache::UTXOCache(size_t capacity) : capacity_(capacity), size_(0), head_(0) {
    values_.resize(capacity_);
}

void UTXOCache::add(const string& txid, unsigned int vout) {
    //convert txid and vout to binary
    unsigned char value[36];
    makeUnsignedChar(txid, vout, value);

    // Remove the oldest value if the cache is full
    if (size_ >= capacity_) {
        removeOldest();
    }

    // Copy the new value to the cache
    size_t index = (head_ + size_) % capacity_;
    memcpy(&values_[index], value, ValueSize);

    // Add the value to the set and store its index
    valueSet_.insert(value);
    valueIndices_[value] = index;
    size_++;
}

bool UTXOCache::exists(const string& txid, unsigned int vout) {
    //convert txid and vout to binary
    unsigned char value[36];
    makeUnsignedChar(txid, vout, value);

    //find if exists
    auto it = valueSet_.find(value);
    if (it != valueSet_.end()) {
        size_t index = valueIndices_[*it];

        //existed so remove
        removeAtIndex(index);
        return true;
    }
    return false;
}

void UTXOCache::removeOldest() {
    size_--;
    head_ = (head_ + 1) % capacity_;
}

void UTXOCache::removeAtIndex(size_t index) {
    size_--;
    const unsigned char* valueToRemove = &values_[index];

    valueSet_.erase(valueToRemove);
    valueIndices_.erase(valueToRemove);

    if (index == head_) {
        head_ = (head_ + 1) % capacity_;
    } else {
        size_t tail = (head_ + size_) % capacity_;
        if (index < tail) {
            memmove(&values_[index], &values_[index + 1], (tail - index) * ValueSize);
        } else {
            memmove(&values_[head_], &values_[head_ + 1], (index - head_) * ValueSize);
            head_ = (head_ + 1) % capacity_;
        }
    }
}

void UTXOCache::makeUnsignedChar(const std::string& txid, unsigned int vout, unsigned char* out) {
    if (txid.size() != 64) {
        // Handle invalid input (txid should be a 32-byte hex string)
        throw std::invalid_argument("Invalid txid length");
    }

    // Convert the hex string to a 256-bit value (32 bytes)
    uint8_t hexValue[32];
    for (int i = 0; i < 32; ++i) {
        std::string byteStr = txid.substr(i * 2, 2);
        std::istringstream converter(byteStr);
        int byteValue;
        converter >> std::hex >> byteValue;
        hexValue[i] = static_cast<uint8_t>(byteValue);
    }

    // Convert the integer to a 32-bit value
    uint8_t intBytes[4];
    intBytes[0] = static_cast<uint8_t>((vout >> 24) & 0xFF);
    intBytes[1] = static_cast<uint8_t>((vout >> 16) & 0xFF);
    intBytes[2] = static_cast<uint8_t>((vout >> 8) & 0xFF);
    intBytes[3] = static_cast<uint8_t>(vout & 0xFF);

    // Combine the hex value and integer value to get a 288-bit value (36 bytes)
    std::memcpy(out, hexValue, 32);
    std::memcpy(out + 32, intBytes, 4);
}
