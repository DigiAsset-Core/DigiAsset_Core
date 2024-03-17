//
// Created by mctrivia on 17/03/24.
//

#include "AppMain.h"
#include "BitcoinRpcServer.h"
#include "Log.h"
#include <jsoncpp/json/value.h>

namespace RPCMethods {
    /**
     * description of method
     */
    extern const Json::Value shutdown(const Json::Value& params) {
        AppMain* main=AppMain::GetInstance();
        main->getChainAnalyzer()->stop();
        main->getIPFS()->stop();
        Log* log = Log::GetInstance();
        log->addMessage("Safe to shut down", Log::CRITICAL);
        return true;
    }
}