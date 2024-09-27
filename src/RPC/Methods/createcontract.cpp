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
        * Creates a smart contract
        * Note: only works on your plugins
        * params[0] - pluginName(string)
        * params[1] - params(json optional - default {})
        * params[2] - account(string optional)
        *
        * Return: contractAddress
        */
        extern const Response createcontract(const Json::Value& params) {
            if ((params.size() == 0) || (params.size() > 3)) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
            if (!params[0].isString()) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
            Json::Value contractParams = Json::objectValue;
            if (params.size() >= 2) {
                if (!params[1].isObject()) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
                contractParams = params[1];
            }
            string account;
            if (params.size() == 3) {
                if (!params[2].isString()) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
                account = params[2].asString();
            }

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
            string contractAddress = contract->createContract(contractParams.toStyledString(), account);

            //generate response
            Response response;
            response.setResult(contractAddress);
            response.setBlocksGoodFor(5760); //day
            return response;
        }

    } // namespace Methods
} // namespace RPC
