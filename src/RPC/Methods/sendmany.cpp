//
// Created by mctrivia on 17/03/24.
//

#include "AppMain.h"
#include "DigiByteDomain.h"
#include "RPC/Response.h"
#include "RPC/Server.h"
#include <jsoncpp/json/value.h>

namespace RPC {
    namespace Methods {
        /**
        * params - see https://developer.bitcoin.org/reference/rpc/sendmany.html
        * only difference is we now accept domains
        */
        extern const Response sendmany(const Json::Value& params) {
            if (params.size() < 2 || params.size() > 9) {
                throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
            }
            if (!params[1].isObject()) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");

            //check if any domains in outputs
            std::vector<std::string> keysToRemove;
            Value newParams = params;
            for (auto it = newParams[1].begin(); it != newParams[1].end(); ++it) {
                std::string key = it.name();
                Json::Value value = *it;

                if (DigiByteDomain::isDomain(key)) {
                    //change the domain into an address
                    string newKey = DigiByteDomain::getAddress(key);
                    if (newParams[1].isMember(newKey)) {
                        newParams[1][newKey] = newParams[1][newKey].asDouble() + value.asDouble();
                    } else {
                        newParams[1][newKey] = value;
                    }

                    // Mark the old key for removal
                    keysToRemove.push_back(key);
                }
            }

            // Remove the old keys
            for (const auto& key: keysToRemove) {
                newParams[1].removeMember(key);
            }

            //send modified params to wallet
            Json::Value result=AppMain::GetInstance()->getDigiByteCore()->sendcommand("sendmany", newParams);

            //return response
            Response response;
            response.setResult(result);
            response.setBlocksGoodFor(-1); //do not cache
            return response;
        }

    }
}