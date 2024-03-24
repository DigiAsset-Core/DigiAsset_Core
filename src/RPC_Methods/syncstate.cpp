//
// Created by mctrivia on 17/03/24.
//

#include "AppMain.h"
#include "BitcoinRpcServer.h"
#include <jsoncpp/json/value.h>

namespace RPCMethods {
    /**
     * Returns the current DigiByte block height and state of DigiAsset Sync
     * {
     *      count (unsigned int) - height DigiByte Core is currently synced to
     *      sync (int) - negative numbers mean how many blocks behind DigiAsset processing is anything over 120 is unsafe to use
     *                   0 = fully synced
     *                   1 = stopped
     *                   2 = initializing
     *                   3 = rewinding
     *                   4 = optimizing(this state only happens when wallet syncs for the first time.  It optimizes in sections so can go in and out of this state several times.)
     * }
     */
    extern const Json::Value syncstate(const Json::Value& params) {
        Value result = Value(Json::objectValue);
        AppMain* main=AppMain::GetInstance();
        result["count"] = main->getDigiByteCore()->getBlockCount();
        result["sync"] = main->getChainAnalyzer()->getSync();
        return result;
    }
}