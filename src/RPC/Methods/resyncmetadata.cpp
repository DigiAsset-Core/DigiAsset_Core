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
        * pins all ipfs meta data
        * returns true - does not mean they are all downloaded yet.  Will likely take a while to finish
        */
        extern const Response resyncmetadata(const Json::Value& params) {
            AppMain* main = AppMain::GetInstance();
            PermanentStoragePoolList* pools = main->getPermanentStoragePoolList();
            for (const auto& pool: *pools) {
                if (!pool->subscribed()) continue;
                pool->repinAllFiles();
            }

            //return response
            Response response;
            response.setResult(true);
            response.setBlocksGoodFor(-1); //do not cache
            return response;
        }

    }
}