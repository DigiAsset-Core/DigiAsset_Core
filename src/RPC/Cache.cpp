//
// Created by mctrivia on 05/04/24.
//

#include "Cache.h"
#include "crypto/SHA256.h"

namespace RPC {

    std::array<uint8_t, 32> Cache::generateKey(const std::string& method, const Json::Value& params) {
        // Convert method and params to a single string
        std::string input = method + params.toStyledString();

        // Initialize SHA256 object and update it with the input
        SHA256 sha256;
        sha256.update(input);

        // Compute the digest
        return sha256.digest();
    }

    bool Cache::isCached(const std::string& method, const Json::Value& params, Response& response) {
        std::array<uint8_t, 32> key = generateKey(method, params);
        std::lock_guard<std::mutex> lock(_cacheMutex);
        auto it = _cacheMap.find(key);
        if (it == _cacheMap.end()) {
            return false;
        }
        response = it->second.response;
        return true;
    }

    void Cache::addResponse(const std::string& method, const Json::Value& params, const Response& response) {
        std::array<uint8_t, 32> key = generateKey(method, params);
        std::lock_guard<std::mutex> lock(_cacheMutex);

        // Check if the entry already exists and update it
        auto it = _cacheMap.find(key);
        if (it != _cacheMap.end()) {
            // Update existing entry
            it->second.response = response;
            it->second.timestamp = std::chrono::steady_clock::now();
        } else {
            // Add new entry
            _currentCacheSize += response.size() + _entryOverhead;
            _cacheMap.emplace(key, CacheEntry(response));
        }

        // Ensure cache does not exceed max size
        removeOldestEntries();
    }

    void Cache::addressChanged(const std::string& address) {
        std::lock_guard<std::mutex> lock(_cacheMutex);
        for (auto it = _cacheMap.begin(); it != _cacheMap.end(); ) {
            size_t deleteSize=it->second.response.addressChanged(address);
            if ( deleteSize > 0) {
                _currentCacheSize -= (deleteSize + _entryOverhead);
                it = _cacheMap.erase(it);
            } else {
                ++it;
            }
        }
    }

    void Cache::assetChanged(const std::string& assetId) {
        std::lock_guard<std::mutex> lock(_cacheMutex);
        for (auto it = _cacheMap.begin(); it != _cacheMap.end(); ) {
            size_t deleteSize=it->second.response.assetChanged(assetId);
            if ( deleteSize > 0) {
                _currentCacheSize -= (deleteSize + _entryOverhead);
                it = _cacheMap.erase(it);
            } else {
                ++it;
            }
        }
    }

    void Cache::newBlockAdded() {
        std::lock_guard<std::mutex> lock(_cacheMutex);
        for (auto it = _cacheMap.begin(); it != _cacheMap.end(); ) {
            size_t deleteSize=it->second.response.newBlockAdded();
            if ( deleteSize > 0) {
                _currentCacheSize -= (deleteSize + _entryOverhead);
                it = _cacheMap.erase(it);
            } else {
                ++it;
            }
        }
    }

    void Cache::newAssetIssued() {
        std::lock_guard<std::mutex> lock(_cacheMutex);
        for (auto it = _cacheMap.begin(); it != _cacheMap.end(); ) {
            size_t deleteSize=it->second.response.newAssetIssued();
            if ( deleteSize > 0) {
                _currentCacheSize -= (deleteSize + _entryOverhead);
                it = _cacheMap.erase(it);
            } else {
                ++it;
            }
        }
    }

    void Cache::removeOldestEntries() {
        while (_currentCacheSize > _maxCacheSize && !_cacheMap.empty()) {
            auto oldest = _cacheMap.begin();
            for (auto it = _cacheMap.begin(); it != _cacheMap.end(); ++it) {
                if (it->second.timestamp < oldest->second.timestamp) {
                    oldest = it;
                }
            }
            _currentCacheSize -= (oldest->second.response.size() + _entryOverhead);
            _cacheMap.erase(oldest);
        }
    }
}
