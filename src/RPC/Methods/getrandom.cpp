//
// Created by mctrivia on 20/05/24.
//

#include "AppMain.h"
#include "RPC/Response.h"
#include "RPC/Server.h"
#include "crypto/SHA256.h"
#include "utils.h"
#include <jsoncpp/json/value.h>

namespace RPC {
    namespace Methods {
        /**
         * Returns a provably fair pseudo random number for a transaction output or a block
         * Warning:  only works if data is not pruned
         *
         * Input Parameters:
         *  params[0] - txid (string) hex value
         *  params[1] - vout (integer)
         *  params[2] - out of (unsigned integer 64bit - default max)
         *
         *  params[0] - block height (integer)
         *  params[1] - out of (integer 64bit - default max)
         *
         * Output Format:
         * unsigned int 64bit
         */
        extern const Response getrandom(const Json::Value& params) {
            //get parameters
            if (params.size() < 1) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
            Database* db = AppMain::GetInstance()->getDatabase();
            uint64_t outOf = numeric_limits<uint64_t>::max();
            std::vector<uint8_t> dataToHash;
            unsigned int height;
            if (params[0].isUInt()) {

                //block request
                height = params[0].asUInt();
                if (params.size() == 2) {
                    if (!params[1].isUInt64()) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
                    outOf = params[1].asUInt64();
                }

            } else if (params[0].isString()) {

                //output request
                string txid = params[0].asString();
                if (txid.size() != 64) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
                if ((params.size() < 2) || (params.size() > 3) || !params[1].isUInt()) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
                uint32_t vout = params[1].asUInt();
                if (params.size() == 3) {
                    if (!params[2].isUInt64()) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
                    outOf = params[2].asUInt64();
                }

                //get height of txid
                try {
                    height = db->getUTXOHeight(txid, vout).first;
                } catch (const Database::exceptionFailedSelect& e) {
                    throw DigiByteException(RPC_MISC_ERROR, "Unknown transaction.  Value pruned or not yet processed");
                }

                //add txid to data to be hashed
                for (unsigned int i = 0; i < 64; i += 2) {
                    std::string byteString = txid.substr(i, 2);
                    uint8_t byte = (uint8_t) strtol(byteString.c_str(), nullptr, 16);
                    dataToHash.push_back(byte);
                }

                //add vout
                dataToHash.push_back((vout >> 24) & 0xFF);
                dataToHash.push_back((vout >> 16) & 0xFF);
                dataToHash.push_back((vout >> 8) & 0xFF);
                dataToHash.push_back(vout & 0xFF);

            } else {
                throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
            }

            //check there are at least 2 confirms
            if (db->getBlockHeight()<height+2) throw DigiByteException(RPC_MISC_ERROR, "Not enough confirms");

            //add last 10 blocks(including current)
            for (size_t bi = 0; bi < 10; bi++) {
                string hash;
                try {
                    hash = db->getBlockHash(height);
                } catch (const Database::exceptionDataPruned& e) {
                    hash = AppMain::GetInstance()->getDigiByteCore()->getBlockHash(height);
                }
                for (unsigned int i = 0; i < 64; i += 2) {
                    std::string byteString = hash.substr(i, 2);
                    uint8_t byte = (uint8_t) strtol(byteString.c_str(), nullptr, 16);
                    dataToHash.push_back(byte);
                }
                height--;
                if (height==0) break;   //unlikely but this allows blocks 1 to 9 to be used still
            }

            //hash the value
            SHA256 sha256;
            sha256.update(dataToHash.data(), dataToHash.size());
            auto sha256Digest = sha256.digest();

            //return the modules of the hash and the out of value
            uint64_t random = utils::mod256by64(sha256Digest, outOf);

            //build and return the response
            Response response;
            response.setResult(static_cast<Json::UInt64>(random));
            response.setBlocksGoodFor(5760); //day
            return response;
        }
    } // namespace Methods
} // namespace RPC