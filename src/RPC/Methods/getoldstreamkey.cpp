//
// Created by mctrivia on 17/03/24.
//

#include "AppMain.h"
#include "OldStream.h"
#include "RPC/Response.h"
#include "RPC/Server.h"
#include "utils.h"
#include <fstream>
#include <iostream>
#include <jsoncpp/json/value.h>

namespace RPC {
    namespace Methods {
        namespace {
            UniqueTaskQueue _taskQueue;
            std::atomic<bool> _processingThreadStarted{false};
        }

        /**
        * This function will be removed eventually and should not be used for new projects
        * Simulates old DigiAsset Stream
        *
        *  params[0] - key(string)
        *
        *  returns the number of items in the job que
        */
        extern const Response getoldstreamkey(const Json::Value& params) {
            if (params.size() != 1) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");

            string key;
            if (params[0].isInt()) {
                key= to_string(params[0].asInt());
            } else if (params[0].isString()){
                key = params[0].asString();
            } else {
                throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
            }

            //process request
            return OldStream::getKey(key);
        }

    }
}