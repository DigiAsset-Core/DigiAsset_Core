//
// Created by mctrivia on 18/06/24.
//


#include "AppMain.h"
#include "RPC/Response.h"
#include "RPC/Server.h"
#include <jsoncpp/json/value.h>

namespace RPC {
    namespace Methods {
        /**
        * Returns list of plugins installed
        */
        extern const Response listmycontractplugins(const Json::Value& params) {
            if (params.size() !=0) {
                throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
            }

            //get list of plugins
            auto contracts=AppMain::GetInstance()->getSmartContracts();
            Json::Value json=Json::arrayValue;
            for (const auto& pair: *contracts) {
                json.append(pair.first);
            }

            //generate response
            Response response;
            response.setResult(json);
            response.setBlocksGoodFor(0);
            return response;
        }

    } // namespace Methods
} // namespace RPC
