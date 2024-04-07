//
// Created by RenzoDD on 21/03/24.
//

#include "AppMain.h"
#include "RPC/Response.h"
#include "RPC/Server.h"
#include <jsoncpp/json/value.h>

namespace RPC {
    namespace Methods {
        /**
        * Returns a list of assetIDs ordered by issuance height
        *  params[0] - numberOfRecords(unsigned int) - default is infinity
        *  params[2] - firstAsset(unsigned int) - default is infinity
        *  params[3] - basic output - default is true
        *  params[4] - filter object
        *      {
        *        psp:  bool - if true only returns assets that are part of a psp, false only those that are not part of a psp
        *              int  - if int returns only ones that are part of that specific psp
        *      }
        * ** please note filtering may result in the number of items not equaling the number asked for.  It can also result in several pages of empty results
        *
        *
        * In basic mode, the function returns an array of Json::Value objects, each representing basic information about a DigiAsset, including:
        * - assetIndex: The index of the asset(note only valid on this node)
        * - assetId: The ID of the asset.
        * - cid: The CID associated with the asset.
        * - height: The issuance height of the asset.
        *
        * When not in basic mode returns:
        * @return array[Json::Value] - Returns a Json::Value object that represents the DigiAsset in JSON format.
        *                       Refer to DigiAsset::toJSON for the format of the returned JSON object.
        */
        extern const Response listlastassets(const Json::Value& params) {
            if (params.size() > 5) {
                throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
            }

            //get number of records default is infinity
            unsigned int numberOfRecords=std::numeric_limits<unsigned int>::max();
            if (params.size()>0) {
                if (params[0].isUInt()) {
                    numberOfRecords = params[0].asUInt();
                    if (numberOfRecords < 1) {
                        throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
                    }
                } else if (!params[0].isNull()) {
                    throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
                }
            }

            //get firstAsset default is infinity
            int firstAsset=std::numeric_limits<int>::max();
            if (params.size()>1) {
                if (params[1].isUInt()) {
                    firstAsset = params[1].asUInt();
                    if (firstAsset < 1) {
                        throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
                    }
                } else if (!params[1].isNull()) {
                    throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
                }
            }

            //get basic default is true
            bool basic=true;
            if (params.size()>2) {
                if (params[2].isBool()) {
                    basic = params[2].asBool();
                } else if (!params[2].isNull()) {
                    throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
                }
            }

            Json::Value filter=Json::objectValue;
            if (params.size()>3) {
                if (params[3].isObject()) {
                    filter=params[3];
                } else {
                    throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
                }
            }

            //get asset list
            Database* db=AppMain::GetInstance()->getDatabase();
            auto assets=db->getLastAssetsIssued(numberOfRecords, firstAsset);

            Value jsonArray=Json::arrayValue;

            for (const auto& asset: assets) {
                //skip if fails filter
                if (filter.isMember("psp")) {
                    bool skipIfInPool=( filter["psp"].isBool() && !filter["psp"].asBool() );    //equal to true when we want non psp assets
                    if (filter["psp"].isBool()) {
                        if (db->isAssetInPool(asset.assetIndex)==skipIfInPool) continue;    //will skip to next if not desired
                    } else if (filter["psp"].isUInt()){
                        if (db->isAssetInPool(filter["psp"].asUInt(),asset.assetIndex)==skipIfInPool) continue;
                    }
                }

                //output
                if (basic) {
                    jsonArray.append(static_cast<Json::UInt64>(asset.assetIndex));
                } else {
                    Json::Value assetJSON(Json::objectValue);
                    assetJSON["assetIndex"] = static_cast<Json::UInt64>(asset.assetIndex);
                    assetJSON["assetId"] = asset.assetId;
                    assetJSON["cid"] = asset.cid;
                    assetJSON["height"] = asset.height;
                    jsonArray.append(assetJSON);
                }
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