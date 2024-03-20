//
// Created by mctrivia on 17/03/24.
//
#include "AppMain.h"
#include "BitcoinRpcServer.h"
#include "DigiByteDomain.h"
#include <jsoncpp/json/value.h>

namespace RPCMethods {
    /**
     * params - see https://developer.bitcoin.org/reference/rpc/sendtoaddress.html
     * only difference is we now accept domains
     */
    extern const Json::Value sendtoaddress(const Json::Value& params) {
        if (params.size() < 2 || params.size() > 9) {
            throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
        }
        if (!params[0].isString()) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");

        //check if any domains in outputs
        std::vector<std::string> keysToRemove;
        Value newParams = params;
        if (DigiByteDomain::isDomain(newParams[0].asString())) {
            //change the domain into an address
            newParams[0] = DigiByteDomain::getAddress(newParams[0].asString());
        }

        //send modified params to wallet
        return AppMain::GetInstance()->getDigiByteCore()->sendcommand("sendtoaddress", newParams);
    }
}