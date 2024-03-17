//
// Created by mctrivia on 17/03/24.
//

#include "AppMain.h"
#include "BitcoinRpcServer.h"
#include <jsoncpp/json/value.h>

namespace RPCMethods {
    /**
     * pins all ipfs meta data
     * returns true - does not mean they are all downloaded yet.  Will likely take a while to finish
     */
    extern const Json::Value resyncmetadata(const Json::Value& params) {
        AppMain* main = AppMain::GetInstance();
        PermanentStoragePoolList* pools = main->getPermanentStoragePoolList();
        for (const auto& pool: *pools) {
            if (!pool->subscribed()) continue;
            pool->repinAllFiles();
        }
        return true;
    }
}