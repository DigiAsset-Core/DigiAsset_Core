//
// Created by RenzoDD on 16/04/24.
//

#include "AppMain.h"
#include "RPC/Response.h"
#include "RPC/Server.h"
#include <cmath>
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
            if (!params[0].isString() || (params[0].asString().length() != 64)) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
            if (!params[1].isUInt()) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
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
                AssetUTXO assetInputData;
                try {
                    assetInputData = db->getAssetUTXO(params[0].asString(), params[1].asUInt());
                } catch (const Database::exceptionDataPruned& e) {
                    //A plain output never gets a utxos row when storenonassetutxo=0, so this is
                    //the normal answer for an ordinary coin rather than a sign the data is gone.
                    //Core answered, so the output is unspent, and pruning only ever removes rows
                    //of outputs that have been spent - so once the analyzer has passed its height
                    //a missing row means "no assets".  Anything above that we genuinely can't say
                    unsigned int utxoHeight = 0;
                    if (coreInputData["confirmations"].isNumeric()) {
                        unsigned int confirmations = coreInputData["confirmations"].asUInt();
                        unsigned int chainHeight = AppMain::GetInstance()->getDigiByteCore()->getBlockCount();
                        if ((confirmations > 0) && (confirmations <= chainHeight)) {
                            utxoHeight = chainHeight - confirmations + 1;
                        }
                    }
                    if (!db->isHeightIndexed(utxoHeight)) throw;
                    assetInputData.txid = params[0].asString();
                    assetInputData.vout = params[1].asUInt();
                    assetInputData.digibyte = static_cast<uint64_t>(llround(coreInputData["value"].asDouble() * 100000000.0));
                    assetInputData.assets.clear();
                }
                coreInputData["digibyte"] = static_cast<Json::UInt64>(assetInputData.digibyte);

                Value jsonArray = Json::arrayValue;
                for (const auto& asset: assetInputData.assets) {
                    jsonArray.append(asset.toJSON(true));
                }
                coreInputData["assets"] = jsonArray;

                response.setResult(coreInputData);
                return response;
            } catch (const Database::exceptionDataPruned& e) {
                throw DigiByteException(RPC_MISC_ERROR, "Desired data has been pruned");
            }
        }

    } // namespace Methods
} // namespace RPC
