//
// Created by mctrivia on 18/06/24.
//

#include "AppMain.h"
#include "RPC/Response.h"
#include "RPC/Server.h"
#include <jsoncpp/json/value.h>
#include "utils.h"


namespace RPC {
    namespace Methods {
        /**
        * Gets the construction parameters for a plugin
        * Note: only works on your contracts
        * params[0] - pluginName(string)
        *
        * Return: json parameters based on ConstructorParameters.v*.md
        */
        extern const Response getcontractconstructorparameters(const Json::Value& params) {
            if (params.size() != 1) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
            if (!params[0].isString()) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");

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
            Json::Value json = utils::fromJSON(contract->getContractConstructorParameters());

            //generate response
            Response response;
            response.setResult(json);
            response.setBlocksGoodFor(5760); //day
            return response;
        }

    } // namespace Methods
} // namespace RPC
