//
// Created by RenzoDD on 21/03/24.
//

#include "AppMain.h"
#include "BitcoinRpcServer.h"
#include <jsoncpp/json/value.h>

namespace RPCMethods {
    /**
     * Returns a list of assetIDs ordered by issuance height
     *  params[0] - numberOfRecords(unsigned int) - default is infinity
     *  params[1] - startIndex(unsigned int) - default is 1
     *  params[2] - basic - default is true
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
    extern const Json::Value listassets(const Json::Value& params) {
        if (params.size() > 3) {
            throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
        }

        //get number of records default is infinity
        unsigned int numberOfRecords=std::numeric_limits<unsigned int>::max();;
        if (params.size()>0) {
            if (params[0].isUInt()) {
                numberOfRecords = params[0].asUInt();
            } else if (!params[0].isNull()) {
                throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
            }
        }

        //get start index default is 1
        unsigned int startIndex=1;
        if (params.size()>1) {
            if (params[1].isUInt()) {
                startIndex = params[1].asUInt();
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

        Database* db=AppMain::GetInstance()->getDatabase();
        auto assets=db->getAssetIDsOrderedByIssuanceHeight(numberOfRecords, startIndex);

        Value jsonArray=Json::arrayValue;

        if (basic) {
            for (const auto& asset: assets) {
                Json::Value assetJSON(Json::objectValue);
                assetJSON["assetIndex"] = static_cast<Json::UInt64>(asset.assetIndex);
                assetJSON["assetId"] = asset.assetId;
                assetJSON["cid"] = asset.cid;
                assetJSON["height"] = asset.height;
                jsonArray.append(assetJSON);
            }
        } else {
            for (const auto& asset: assets) {
                jsonArray.append(db->getAsset(asset.assetIndex).toJSON());
            }
        }

        return jsonArray;
    }
}