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
        * Looks for newly installed plugins
        * Note: only works on your contracts
        */
        extern const Response refreshcontractplugins(const Json::Value& params) {
            if ((params.size() != 0) || (params.size() >2)) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");

            AppMain::GetInstance()->getSmartContracts()->updateAvailableContracts();

            //generate response
            Response response;
            response.setResult(true);
            response.setBlocksGoodFor(0);
            return response;
        }

    } // namespace Methods
} // namespace RPC
