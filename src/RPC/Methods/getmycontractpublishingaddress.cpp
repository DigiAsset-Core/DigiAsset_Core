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
        * Gets the publishing address
        * Note: only works on your contracts
        * params[0] - pluginName(string)
        * params[1] - account(string optional)
        *
        * Return: publishingAddress
        */
        extern const Response getmycontractpublishingaddress(const Json::Value& params) {
            if ((params.size() == 0) || (params.size() >2)) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
            if (!params[0].isString()) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
            string account;
            if (params.size()==2) {
                if (!params[1].isString()) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
                account=params[1].asString();
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
            string publishingAddress = contract->getPublishingAddress(account);

            //generate response
            Response response;
            response.setResult(publishingAddress);
            response.setBlocksGoodFor(5760); //day
            return response;
        }

    } // namespace Methods
} // namespace RPC
