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
        * Enables a contract that is  disabled(if already enabled does nothing)
        * Note: only works on your contracts
        * params[0] - pluginName(string)
        * params[1] - contractAddress(string)
        */
        extern const Response enablemycontract(const Json::Value& params) {
            if (params.size() != 2) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
            if (!params[0].isString()) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
            if (!params[1].isString()) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");

            //load contract
            shared_ptr<SmartContractHandler> contract;
            try {
                contract = AppMain::GetInstance()->getSmartContracts()->getContract(params[0].asString());
            } catch (const std::out_of_range& e) {
                Response response;
                response.setError("Invalid plugin name");
                response.setBlocksGoodFor(0);
                return response;
            }

            //get list of plugins
            bool success = contract->enableContract(params[1].asString());

            //generate response
            Response response;
            response.setResult(success);
            response.setBlocksGoodFor(0);
            return response;
        }

    } // namespace Methods
} // namespace RPC
