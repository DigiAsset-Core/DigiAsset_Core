//
// Created by mctrivia on 17/03/24.
//

#include "AppMain.h"
#include "BitcoinRpcServer.h"
#include "utils.h"
#include <fstream>
#include <iostream>
#include <jsoncpp/json/value.h>

namespace RPCMethods {
    /**
     * This function will be depricated eventually and should not be used for new projects
     * Simulates old DigiAsset Stream
     *
     *  params[0] - key(string)
     *
     *  return matches https://github.com/digiassetX/digibyte-stream-types as close as possible
     *  returns false if no cache created
     */
    extern const Json::Value getoldstreamkey(const Json::Value& params) {
        if (params.size() != 1) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");

        // Construct the filename from the provided key
        string key;
        if (params[0].isInt()) {
            key= to_string(params[0].asInt());
        } else if (params[0].isString()){
            key = params[0].asString();
        } else {
            throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
        }
        std::string filename = "stream/" + key + ".json";

        // Check if the file exists
        if (!utils::fileExists(filename)) {
            return {false}; // File does not exist
        }

        // Open and read the file
        std::ifstream file(filename);
        if (file.is_open()) {
            Json::Value result;
            file >> result;
            file.close();
            return result; // Return the contents of the file
        } else {
            // If the file exists but cannot be opened, return false
            // This could indicate a permissions issue or a transient file system error
            return {false};
        }
    }
}