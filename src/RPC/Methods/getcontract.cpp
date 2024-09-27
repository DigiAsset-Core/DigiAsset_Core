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
        * Returns if a contract is active and what the funding state is
        * params[0] - contract address(string)
        *
        * @return object
        *  {
        *     publisher: {
        *        address: string,
        *        country: optional string,
        *        name:    optional string,
        *        hash:    optional string,
        *        alias:   optional string
        *     },
        *     contractAddress: string,
        *     cid:     string,
        *     version: integer,
        *     state:   bool
        *  }
        */
        extern const Response getcontract(const Json::Value& params) {
            if (params.size() != 1) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
            if (!params[0].isString()) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
            string contractAddress=params[0].asString();

            //look up contract resources
            Database* db = AppMain::GetInstance()->getDatabase();
            auto contract= db->getSmartContract(contractAddress);

            //convert to json
            Value json=Json::objectValue;
            json["publisher"]=contract.publisher.toJSON();
            if (!contract.publisherAlias.empty()) {
                json["publisher"]["alias"] = contract.publisherAlias;
            }
            json["contractAddress"]=contract.contractAddress;
            json["cid"]=contract.cid;
            json["version"]=contract.version;
            json["state"]=contract.state;

            //build response
            Response response;
            response.setResult(json);
            response.setBlocksGoodFor(5760);    //day

            //set what addresses should invalidate cache
            auto addresses=db->getSmartContractReferences(contractAddress);
            for (const auto& address: addresses) {
                response.addInvalidateOnAddressChange(address);
            }
            return response;
        }

    }
}