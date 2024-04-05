//
// Created by mctrivia on 17/03/24.
//

#include "AppMain.h"
#include "RPC/Response.h"
#include "RPC/Server.h"
#include "utils.h"
#include <fstream>
#include <iostream>
#include <jsoncpp/json/value.h>

namespace RPC {
    namespace Methods {
        /**
        * This function will be depricated eventually and should not be used for new projects
        * Simulates old DigiAsset Stream
        *
        *  params[0] - key(string)
        *
        *  return matches https://github.com/digiassetX/digibyte-stream-types as close as possible
        *  returns false if no cache created
        */
        extern const Response getoldstreamkey(const Json::Value& params) {
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
            Response response;
            response.setBlocksGoodFor(-1); //do not cache
            if (!utils::fileExists(filename)) {
                response.setResult(false); // File does not exist
                return response;
            }

            // Open and read the file
            std::ifstream file(filename);
            if (file.is_open()) {
                Json::Value result;
                file >> result;
                file.close();
                response.setResult(result); // Return the contents of the file
            } else {
                // If the file exists but cannot be opened, return false
                // This could indicate a permissions issue or a transient file system error
                response.setResult(false);
            }
            return response;
        }

    }
}