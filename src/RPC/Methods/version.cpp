//
// Created by mctrivia on 17/03/24.
//

#include "Version.h"
#include "AppMain.h"
#include "RPC/Response.h"
#include "RPC/Server.h"
#include <jsoncpp/json/value.h>

namespace RPC {
    namespace Methods {
        /**
        * Returns the current version number
        */
        extern const Response version(const Json::Value& params) {
            //return response
            Response response;
            response.setResult(getVersionString());
            response.setBlocksGoodFor(5760); //day
            return response;
        }

    }
}