//
// Created by mctrivia on 16/03/24.
//

#include "AppMain.h"
#include "BitcoinRpcServer.h"
#include <jsoncpp/json/value.h>

namespace RPCMethods {
    /**
     * params[0] - txid(string)
     * params[1] - verbose(optional bool=false)
     * params[2] - ignored
     *
     * Returns same as before but now extra fields form DigiAsset::toJSON are not present
     */
    extern const Json::Value getrawtransaction(const Json::Value& params) {
        if (params.size() < 1 || params.size() > 3) {
            throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
        }
        if (!params[0].isString() || (params[0].asString().length()!=64)) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");

        //get what core wallet has to say
        Json::Value rawTransactionData = AppMain::GetInstance()->getDigiByteCore()->sendcommand("getrawtransaction", params);
        if ((params.size() == 1) || (params[1].isBool() && params[1].asBool() == false)) {
            return rawTransactionData;
        }

        //load transaction
        try {
            DigiByteTransaction tx{params[0].asString()};

            //convert to a value and return
            tx.lookupAssetIndexes();
            return tx.toJSON(rawTransactionData);
        } catch (const Database::exceptionDataPruned& e) {
            throw DigiByteException(RPC_MISC_ERROR, "Desired data has been pruned");
        }
    }
}