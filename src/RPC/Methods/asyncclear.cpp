//
// Created by mctrivia on 11/04/24.
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
         * Clears the results of an asyncronous call
         *
         * Input Parameters:
         *  params[0] - method name
         *  params[1] - param1
         *  params[2] - param2
         *  ...
         */
        extern const Response asyncclear(const Json::Value& params) {
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

            //delete file
            std::remove(filename.c_str());
            response.setResult(true);
            return response;
        }

    } // namespace Methods
} // namespace RPC