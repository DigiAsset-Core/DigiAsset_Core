//
// Created by mctrivia on 17/03/24.
//

#include "AppMain.h"
#include "Log.h"
#include "RPC/Response.h"
#include "RPC/Server.h"
#include <jsoncpp/json/value.h>

namespace RPC {
    namespace Methods {
        /**
        * description of method
        */
        extern const Response shutdown(const Json::Value& params) {
            AppMain* main=AppMain::GetInstance();
            main->getChainAnalyzer()->stop();
            main->getIPFS()->stop();
            Log* log = Log::GetInstance();
            log->addMessage("Safe to shut down", Log::CRITICAL);

            //return response
            Response response;
            response.setResult(true);
            response.setBlocksGoodFor(-1); //do not cache
            return response;
        }

    }
}