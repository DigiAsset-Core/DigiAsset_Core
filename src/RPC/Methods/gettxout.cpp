//
// Created by RenzoDD on 16/04/24.
//

#include "AppMain.h"
#include "RPC/Response.h"
#include "RPC/Server.h"
#include <jsoncpp/json/value.h>

namespace RPC {
    namespace Methods {
        /**
        * params[0] - txid(string)
        * params[1] - vout(unsigned int)
        * params[2] - mempool(optional bool default true)
        *
        * Returns same as before but now extra fields form DigiAsset::toJSON are now present
        */
        extern const Response gettxout(const Json::Value& params) {
            if (params.size() < 1 || params.size() > 3) {
                throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
            }
            if (!params[0].isString() || (params[0].asString().length()!=64)) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
            if (!params[1].isInt() || (params[1].asInt()<0)) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
            if (params.size() > 2 && !params[2].isBool()) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
            
            try {
                Response response;
                //get what core wallet has to say
                Json::Value coreInputData = AppMain::GetInstance()->getDigiByteCore()->sendcommand("gettxout", params);
                if (coreInputData.empty()) {
                    response.setResult(coreInputData);
                    return response;
                }

                Database* db = AppMain::GetInstance()->getDatabase();
                AssetUTXO assetInputData = db->getAssetUTXO(params[0].asString(), params[1].asInt());
                coreInputData["digibyte"] = assetInputData.digibyte;

                Value jsonArray=Json::arrayValue;
                for (const auto& asset: assetInputData.assets) {
                    jsonArray.append(asset.toJSON(true));
                }
                coreInputData["assets"] =  jsonArray;

                response.setResult(coreInputData);
                return response;
            } catch (const Database::exceptionDataPruned& e) {
                throw DigiByteException(RPC_MISC_ERROR, "Desired data has been pruned");
            }
        }

    }
}