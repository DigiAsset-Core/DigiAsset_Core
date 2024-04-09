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
        * Returns data about a specific asset
        *
        * Usage 1:
        *  params[0] - assetIndex(integer)
        *  params[1] - exclude IPFS(bool default false)
        *
        * Usage 2:
        *  params[0] - assetId(string)
        *  params[1] - txid(string optional)
        *  params[2] - vout(integer optional)
        *  params[3] - exclude IPFS(bool default false)
        *
        *  txid and vout are for any transaction involving the asset.  These are only needed for assets that
        *  have more than 1 index.  All assets starting with L or Ua have only 1 index
        *
        * @return Json::Value - Returns a Json::Value object that represents the DigiAsset in JSON format.
        *                       Refer to DigiAsset::toJSON for the format of the returned JSON object.
        * ***Note DigiByte locked up in asset utxos is returned as part of DigiByte total.  Actual spendable amount will be less.
        */
        extern const Response getassetdata(const Json::Value& params) {
            if (params.size() < 1 || params.size() > 4) {
                throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
            }

            Database* db = AppMain::GetInstance()->getDatabase();
            DigiAsset asset;

            try {
                if (params.size() >= 3) {
                    //definitely usage 2(all values included)
                    if (
                            !params[0].isString() ||
                            !params[1].isString() || (params[1].asString().length() != 64) ||
                            !params[2].isInt() ||
                            ((params.size() == 4) && (!params[3].isBool()))) {
                        throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
                    }
                    asset = db->getAsset(db->getAssetIndex(
                            params[0].asString(),
                            params[1].asString(),
                            params[2].asInt()));
                } else if (params.size() == 2 && !params[1].isBool()) {
                    throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
                } else if (params[0].isString()) {
                    asset = db->getAsset(db->getAssetIndex(params[0].asString()));
                } else if (params[0].isInt()) {
                    asset = db->getAsset(params[0].asInt());
                } else {
                    throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
                }
            } catch (const Database::exceptionFailedSelect& e) {
                throw DigiByteException(RPC_MISC_ERROR, "Asset Doesn't Exist");
            }

            //look up how many assets exist
            asset.setCount(db->getTotalAssetCount(asset.getAssetIndex()));
            asset.getInitialCount();

            //lookup PSP Membership
            asset.getPspMembership();

            //get simplified default is false
            bool excludeIPFS = false;
            if (params.size() == 2) {
                excludeIPFS = params[1].asBool();
            }
            if (params.size() == 4) {
                excludeIPFS = params[3].asBool();
            }

            //return response
            Response response;
            response.setResult(asset.toJSON(false, excludeIPFS));
            return response;
        }

    } // namespace Methods
} // namespace RPC