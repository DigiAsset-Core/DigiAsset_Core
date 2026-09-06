//
// Created by mctrivia on 17/03/24.
//

#include "AppMain.h"
#include "Log.h"
#include "RPC/Response.h"
#include "RPC/Server.h"
#include <csignal>
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

            //hand the rest of the shutdown(stop RPC server, flush WAL, close db) to the
            //main thread - same path as ctrl-c.  The response still goes out because the
            //RPC worker pool is drained before the sockets close
            std::raise(SIGTERM);

            //return response
            Response response;
            response.setResult(true);
            response.setBlocksGoodFor(-1); //do not cache
            return response;
        }

    }
}