//
// Created by mctrivia on 17/03/24.
//

#include "AppMain.h"
#include "BitcoinRpcServer.h"
#include <jsoncpp/json/value.h>

namespace RPCMethods {
    /**
     * Returns an object containing all addresses(keys) holding an asset and how many they have(values)
     *  params[0] - assetIndex(unsigned int)
     */
    extern const Json::Value getassetholders(const Json::Value& params) {
        if (params.size() != 1) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
        if (!params[0].isUInt()) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");

        //look up holders
        Database* db = AppMain::GetInstance()->getDatabase();
        vector<AssetHolder> holders=db->getAssetHolders(params[0].asUInt());

        //convert to an object
        Value result=Json::objectValue;
        for (const auto& holder : holders) {
            result[holder.address] = Json::Value::UInt64(holder.count);
        }
        return result;
    }
}