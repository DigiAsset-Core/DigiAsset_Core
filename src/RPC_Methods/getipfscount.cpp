//
// Created by mctrivia on 17/03/24.
//

#include "AppMain.h"
#include "BitcoinRpcServer.h"
#include <jsoncpp/json/value.h>

namespace RPCMethods {
    /**
     * Returns the number of IPFS jobs in que
     */
    extern const Json::Value getipfscount(const Json::Value& params) {
        Database* db = AppMain::GetInstance()->getDatabase();
        return db->getIPFSJobCount();
    }
}