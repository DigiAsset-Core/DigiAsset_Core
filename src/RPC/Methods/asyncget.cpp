//
// Created by mctrivia on 08/04/24.
//

#include "AppMain.h"
#include "RPC/Response.h"
#include "RPC/Server.h"
#include "crypto/SHA256.h"
#include "utils.h"
#include <fstream>
#include <iostream>
#include <jsoncpp/json/value.h>

namespace RPC {
    namespace Methods {
        /**
         * Gets the results of an asyncstart command with the same parameters
         *
         * Input Parameters:
         *  params[0] - method name
         *  params[1] - param1
         *  params[2] - param2
         *  ...
         *
         * return format is same as method called but with an added cacheTime parameter if the output is an object
         */
        extern const Response asyncget(const Json::Value& params) {
            if ((params.size() < 1) || (!params[0].isString())) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");

            //compute file name
            string method = params[0].asString();
            Json::FastWriter writer;
            string key = writer.write(params);
            SHA256 sha256;
            sha256.update(key);
            auto sha256Digest = sha256.digest();
            std::string filename = "cache/" + method + "_" + SHA256::toString(sha256Digest) + ".json";

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

    } // namespace Methods
} // namespace RPC