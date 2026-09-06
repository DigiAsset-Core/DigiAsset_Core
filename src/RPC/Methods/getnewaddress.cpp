//
// Created by DigiAsset Core on 22/07/26.
//
#include "AppMain.h"
#include "RPC/Response.h"
#include "RPC/Server.h"
#include <jsoncpp/json/value.h>

namespace RPC {
    namespace Methods {
        /**
        * params - see https://developer.bitcoin.org/reference/rpc/getnewaddress.html
        * straight passthrough to the wallet - lets callers(eg the GUI) get an address to
        * default fields such as royalty recipients to a wallet address, the same way
        * issueasset's toAddress defaults to one when left blank
        */
        extern const Response getnewaddress(const Json::Value& params) {
            Json::Value result = AppMain::GetInstance()->getDigiByteCore()->sendcommand("getnewaddress", params);

            Response response;
            response.setResult(result);
            response.setBlocksGoodFor(-1); //do not cache
            return response;
        }

    } // namespace Methods
} // namespace RPC
