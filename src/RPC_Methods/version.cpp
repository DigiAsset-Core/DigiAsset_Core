//
// Created by mctrivia on 17/03/24.
//

#include "AppMain.h"
#include "BitcoinRpcServer.h"
#include <jsoncpp/json/value.h>
#include "Version.h"

namespace RPCMethods {
    /**
     * Returns the current version number
     */
    extern const Json::Value version(const Json::Value& params) {
        return getVersionString();
    }
}