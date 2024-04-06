//
// Created by RenzoDD on 06/04/24.
//

#include "AppMain.h"
#include "RPC/Response.h"
#include "RPC/Server.h"
#include <jsoncpp/json/value.h>

namespace RPC {
    namespace Methods {
        /**
        * Returns a list the last blocks processed
        *  params[0] - limit(unsigned int) - number of records
        *
        * @return array of unsigned ints
        */
        extern const Response listlastblocks(const Json::Value& params) {
            if (params.size() != 1) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
            if (!params[0].isInt()) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");

            //look up blocks
            Database* db = AppMain::GetInstance()->getDatabase();
            std::vector<BlockBasics> results= db->getLastBlocks(params[0].asInt());

            //convert to json
            Value jsonArray=Json::arrayValue;
            for (const auto& result : results) {
                Json::Value blockJSON(Json::objectValue);
                blockJSON["height"] = result.height;
                blockJSON["hash"] = result.hash;
                blockJSON["time"] = result.time;
                blockJSON["algo"] = result.algo;
                jsonArray.append(blockJSON);
            }

            //return response
            Response response;
            response.setResult(jsonArray);
            return response;
        }

    }
}