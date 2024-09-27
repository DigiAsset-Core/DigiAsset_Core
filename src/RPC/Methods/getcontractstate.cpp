//
// Created by mctrivia on 18/06/24.
//

#include "AppMain.h"
#include "RPC/Response.h"
#include "RPC/Server.h"
#include <jsoncpp/json/value.h>

//todo create .html file

namespace RPC {
    namespace Methods {
        /**
        * Returns if a contract is active and what the funding state is
        * params[0] - contract address(string)
        * params[1] - height(optional unsigned int - current height)
        *
        * @return object
        *  {
        *     active: bool,
        *     requirements: [
        *        {
        *           assetId,
        *           assetIndex,
        *           need,
        *           have
        *        }
        *     ]
        *  }
        */
        extern const Response getcontractstate(const Json::Value& params) {
            /*
            if ((params.size() == 0) || (params.size() > 2)) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
            if (!params[0].isString()) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
            string contractAddress=params[0].asString();
            unsigned int height=0;
            if (params.size()==2) {
                if (!params[1].isUInt()) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
                height=params[1].asUInt();
            }

            //look up contract resources
            Database* db = AppMain::GetInstance()->getDatabase();
            auto resources= db->getSmartContractResources(contractAddress,height);

            //check if all resources present
            bool active=true;
            for (const auto& resource: resources) {
                if (resource.have<resource.need) active=false;
            }

            //if it has resources then see if the contract is active by creator
            if (active) active=db->getSmartContractState(contractAddress, height);

            //convert to json
            Value json=Json::objectValue;
            json["active"]=active;
            json["requirements"]=Json::arrayValue;
            for (const auto& resource: resources) {
                Json::Value entry=Json::objectValue;
                if (resource.asset=="digibyte") {
                    entry["assetId"]="digibyte";
                    entry["assetIndex"]=1;
                } else {
                    auto pos = resource.asset.find(':');
                    entry["assetId"]=resource.asset.substr(0, pos);
                    entry["assetIndex"]=stoul(resource.asset.substr(pos + 1));
                }
                entry["need"]=resource.need;
                entry["have"]=resource.have;
                json["requirements"].append(entry);
            }

            //build response
            Response response;
            response.setResult(json);
            response.setBlocksGoodFor(5760);    //day

            //set what addresses should invalidate cache
            if (height==0) {
                auto addresses = db->getSmartContractReferences(contractAddress);
                for (const auto& address: addresses) {
                    response.addInvalidateOnAddressChange(address);
                }
            }
            return response;
             */
        }

    }
}