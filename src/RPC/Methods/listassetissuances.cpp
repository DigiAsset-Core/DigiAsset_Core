//
// Created by RenzoDD on 05/04/24.
//

#include "AppMain.h"
#include "RPC/Response.h"
#include "RPC/Server.h"
#include <jsoncpp/json/value.h>

namespace RPC {
    namespace Methods {
        /**
        * Returns a list of all asset issuances on basic format
        *
        * @return array of unsigned ints
        */
        extern const Response listassetissuances(const Json::Value& params) {
            if (params.size() != 1) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
            if (!params[0].isString()) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");

            //look up holders
            Database* db = AppMain::GetInstance()->getDatabase();
            std::vector<IssuanceBasics> results= db->getAssetIssuanceTXIDs(params[0].asString());

            //convert to json
            Value jsonArray=Json::arrayValue;
            for (const auto& result : results) {
                Json::Value assetJSON(Json::objectValue);
                assetJSON["assetIndex"] = static_cast<Json::UInt64>(result.assetIndex);
                assetJSON["txid"] = result.txid;
                assetJSON["amount"] = static_cast<Json::UInt64>(result.amount);
                assetJSON["height"] = result.height;
                assetJSON["cid"] = result.cid;
                jsonArray.append(assetJSON);
            }

            //return response
            Response response;
            response.setResult(jsonArray);
            return response;
        }

    }
}