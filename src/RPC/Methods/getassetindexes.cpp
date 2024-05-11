//
// Created by mctrivia on 17/03/24.
//

#include "AppMain.h"
#include "RPC/Response.h"
#include "RPC/Server.h"
#include <jsoncpp/json/value.h>

namespace RPC {
    namespace Methods {
        /**
        * Returns a list of assetIndexes that belong to a specific assetId(most will have only 1)
        *
        * @return array of unsigned ints
        */
        extern const Response getassetindexes(const Json::Value& params) {
            if (params.size() != 1) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
            if (!params[0].isString()) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");

            //look up holders
            Database* db = AppMain::GetInstance()->getDatabase();
            vector<uint64_t> results= db->getAssetIndexes(params[0].asString());

            //convert to json
            Value jsonArray=Json::arrayValue;
            for (const auto& result : results) {
                jsonArray.append(Json::Value::UInt64(result)); // Append each uint64_t value to the Json array
            }

            //return response
            Response response;
            response.setResult(jsonArray);
            response.setBlocksGoodFor(5760);    //day
            response.setInvalidateOnNewAsset();
            return response;
        }

    }
}