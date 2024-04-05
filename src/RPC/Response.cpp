//
// Created by mctrivia on 04/04/24.
//

#include "Response.h"
#include "utils.h"
#include <algorithm>

namespace RPC {
    void Response::setResult(const Json::Value& result) {
        _result = result;
        _error = false;
        _size += utils::estimateJsonMemoryUsage(result);
    }
    void Response::setError(const Json::Value& error) {
        _result = error;
        _error = true;
        _size += utils::estimateJsonMemoryUsage(error);
    }
    void Response::addInvalidateOnAddressChange(const std::string& address) {
        // Check if the address already exists in the vector
        if (std::find(_invalidateOnAddressChange.begin(), _invalidateOnAddressChange.end(), address) == _invalidateOnAddressChange.end()) {
            // Address not found, so add it to the vector
            _invalidateOnAddressChange.emplace_back(address);
            _size += address.size();
        }
    }
    void Response::setBlocksGoodFor(int blocks) {
        _blocksGoodFor = blocks;
    }
    bool Response::empty() const {
        return (_size == sizeof(Response));
    }
    size_t Response::addressChanged(const std::string& address) const {
        // Check if the address is in the _invalidateOnAddressChange vector
        auto it = std::find(_invalidateOnAddressChange.begin(), _invalidateOnAddressChange.end(), address);

        // If the address is found, return the size of the Response object
        if (it != _invalidateOnAddressChange.end()) {
            return _size;
        }

        // If the address is not found, return 0
        return 0;
    }
    size_t Response::newBlockAdded() {
        _blocksGoodFor--;
        if (_blocksGoodFor<0) return _size;
        return 0;
    }
    size_t Response::size() const {
        return _size;
    }

    Json::Value Response::toJSON(const Json::Value& id) const {
        Json::Value json =Json::objectValue;
        if (_error) {
            json["result"]=Json::nullValue;
            json["error"]=_result;
        } else {
            json["result"]=_result;
            json["error"]=Json::nullValue;
        }
        json["id"]=id;
        return json;
    }
}