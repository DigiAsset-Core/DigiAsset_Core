//
// Created by mctrivia on 16/03/24.
//

#include "AppMain.h"
#include "RPC/Response.h"
#include "RPC/Server.h"
#include <jsoncpp/json/value.h>

namespace RPC {
    namespace Methods {
        /**
        * params[0] - txid(string)
        * params[1] - verbose(optional bool=false)
        * params[2] - ignored
        *
        * Returns same as before but now extra fields form DigiAsset::toJSON are now present
        */
        extern const Response getrawtransaction(const Json::Value& params) {
            if (params.size() < 1 || params.size() > 3) {
                throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
            }
            if (!params[0].isString() || (params[0].asString().length()!=64)) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");

            //get what core wallet has to say
            Json::Value rawTransactionData = AppMain::GetInstance()->getDigiByteCore()->sendcommand("getrawtransaction", params);

            //handle core version 8.22 which dropped some fields
            if (rawTransactionData.isObject()) {
                for (Json::ValueIterator it = rawTransactionData["vout"].begin(); it != rawTransactionData["vout"].end(); it++) {
                    Json::Value& val = *it;
                    Json::Value& spk = val["scriptPubKey"];

                    //8.22 uses singular "address" instead of "addresses" array
                    if (spk.isMember("address")) {
                        Json::Value addresses(Json::arrayValue);
                        addresses.append(spk["address"]);
                        spk["addresses"] = addresses;
                        spk.removeMember("address");
                    }

                    //8.22 dropped reqSigs — reconstruct from script type
                    if (!spk.isMember("reqSigs") && spk.isMember("type")) {
                        std::string type = spk["type"].asString();
                        unsigned int reqSigs = 0;
                        if (type == "pubkeyhash" || type == "scripthash" ||
                            type == "witness_v0_keyhash" || type == "witness_v0_scripthash" ||
                            type == "pubkey") {
                            reqSigs = 1;
                        } else if (type == "multisig" && spk.isMember("asm")) {
                            //multisig ASM starts with "M OP_..." where M is reqSigs count
                            std::string asm_ = spk["asm"].asString();
                            size_t space = asm_.find(' ');
                            if (space != std::string::npos) {
                                try { reqSigs = std::stoul(asm_.substr(0, space)); } catch (...) {}
                            }
                        }
                        spk["reqSigs"] = reqSigs;
                    }
                }
            }

            //convert to response
            Response response;
            response.setBlocksGoodFor(5760); //day
            if ((params.size() == 1) || (params[1].isBool() && params[1].asBool() == false)) {
                response.setResult(rawTransactionData);
                return response;
            }

            //load transaction
            try {
                DigiByteTransaction tx{params[0].asString()};

                //convert to a value and return
                tx.lookupAssetIndexes();
                response.setResult(tx.toJSON(rawTransactionData));
                return response;
            } catch (const Database::exceptionDataPruned& e) {
                throw DigiByteException(RPC_MISC_ERROR, "Desired data has been pruned");
            }
        }

    }
}