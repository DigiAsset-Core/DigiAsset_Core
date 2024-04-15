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
        * Returns an object containing all addresses(keys) holding an asset and how many they have(values)
        *  params[0] - assetIndex(unsigned int) or assetId(string)
        */
        extern const Response getassetholders(const Json::Value& params) {
            if (params.size() != 1) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");

            //look up holders
            Database* db = AppMain::GetInstance()->getDatabase();
            vector<AssetHolder> holders;
            if (params[0].isUInt()) {
                holders=db->getAssetHolders(params[0].asUInt());
            } else if (params[0].isString()) {
                holders=db->getAssetHolders(params[0].asString());
            } else {
                throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
            }

            //convert to an object
            Value result=Json::objectValue;
            for (const auto& holder : holders) {
                result[holder.address] = Json::Value::UInt64(holder.count);
            }

            //return response
            Response response;
            response.setResult(result);
            return response;
        }

    }
}