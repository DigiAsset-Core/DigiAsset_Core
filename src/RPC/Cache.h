//
// Created by mctrivia on 05/04/24.
//

#ifndef DIGIASSET_CORE_CACHE_H
#define DIGIASSET_CORE_CACHE_H


#include "Response.h"
#include <chrono>
#include <jsoncpp/json/value.h>
#include <list>
#include <mutex>
#include <string>
#include <unordered_map>
#include <cstring>

namespace RPC {

    struct DirectSizeTHash {
        std::size_t operator()(const std::array<uint8_t, 32>& arr) const {
            static_assert(sizeof(std::size_t) <= 32, "size_t is too small");
            std::size_t hash = 0;
            // Copy the first sizeof(std::size_t) bytes from the array into hash
            memcpy(&hash, arr.data(), sizeof(std::size_t));
            return hash;
        }
    };

    // Equality function for std::array<uint8_t, 32>
    struct ArrayEqual {
        bool operator()(const std::array<uint8_t, 32>& a, const std::array<uint8_t, 32>& b) const {
            return a == b;
        }
    };

    struct CacheEntry {
        Response response;
        std::chrono::steady_clock::time_point timestamp; // Timestamp to track entry age

        CacheEntry(const Response& response)
            : response(response), timestamp(std::chrono::steady_clock::now()) {}
    };

    class Cache {
    private:
        std::unordered_map<std::array<uint8_t, 32>, CacheEntry, DirectSizeTHash, ArrayEqual> _cacheMap;
        size_t _currentCacheSize = 0;
        const size_t _maxCacheSize = 100 * 1024 * 1024; // Example: 100MB max cache size
        std::mutex _cacheMutex;

        const size_t _entryOverhead = sizeof(std::array<uint8_t, 32>) // Key size
                               + sizeof(std::chrono::steady_clock::time_point) // Timestamp size
                               + 16; // Estimated additional overhead (bucketing, etc.)


        // Generates a unique key based on method and params
        std::array<uint8_t, 32> generateKey(const std::string& method, const Json::Value& params);

        // Removes the oldest entries based on timestamp
        void removeOldestEntries();

    public:
        Cache() = default;

        bool isCached(const std::string& method, const Json::Value& params, Response& response);

        void addResponse(const std::string& method, const Json::Value& params, const Response& response);

        void addressChanged(const std::string& address);

        void assetChanged(const std::string& assetId);

        void newBlockAdded();

        void newAssetIssued();
    };
}



#endif //DIGIASSET_CORE_CACHE_H
