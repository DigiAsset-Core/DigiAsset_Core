//
// Created by mctrivia on 17/03/24.
//

#include "AppMain.h"
#include "BitcoinRpcServer.h"
#include <jsoncpp/json/value.h>

namespace RPCMethods {
    /**
     * Returns data about a specific asset
     *
     * Usage 1:
     *  params[0] - assetIndex(integer)
     *
     * Usage 2:
     *  params[0] - assetId(string)
     *  params[1] - txid(string optional)
     *  params[2] - vout(integer optional)
     *  txid and vout are for any transaction involving the asset.  These are only needed for assets that
     *  have more than 1 index.  All assets starting with L or Ua have only 1 index
     *
     * @return Json::Value - Returns a Json::Value object that represents the DigiAsset in JSON format.
     *                       Refer to DigiAsset::toJSON for the format of the returned JSON object.
     * ***Note DigiByte locked up in asset utxos is returned as part of DigiByte total.  Actual spendable amount will be less.
     */
    extern const Json::Value getassetdata(const Json::Value& params) {
        if (params.size() < 1 || params.size() > 3) {
            throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
        }

        Database* db = AppMain::GetInstance()->getDatabase();
        DigiAsset asset;

        if (params.size() == 3) {
            //definitely usage 2(all values included)
            if (!params[0].isString() || !params[1].isString() || !params[2].isInt()) {
                throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
            }
            asset = db->getAsset(db->getAssetIndex(
                    params[0].asString(),
                    params[1].asString(),
                    params[2].asInt()));
        } else if (params.size() == 2) {
            throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
        } else if (params[0].isString()) {
            asset = db->getAsset(db->getAssetIndex(params[0].asString()));
        } else if (params[0].isInt()) {
            asset = db->getAsset(params[0].asInt());
        } else {
            throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
        }

        //look up how many assets exist
        asset.setCount(db->getTotalAssetCount(asset.getAssetIndex()));

        //return result
        return asset.toJSON();
    }
}