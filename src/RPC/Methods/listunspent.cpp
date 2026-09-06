//
// Created by mctrivia on 17/03/24.
//

#include "AppMain.h"
#include "RPC/Response.h"
#include "RPC/Server.h"
#include <jsoncpp/json/value.h>
#include <numeric>

namespace RPC {
    namespace Methods {
        /**
        * Returns a list of utxos for the wallet or provided addresses
        *  params[0] - min confirms(optional default 1)
        *  params[1] - max confirms(optional default infinity)
        *  params[2] - address list(optional all wallet addresses)
        *  params[3] - include unsafe(optional - always true no matter what is put in here)
        *  params[4] - query options(optional object)
        *     {
        *       "minimumAmount": amount,       (numeric or string, optional, default=0) Minimum value of **EACH** UTXO in DGB
        *       "maximumAmount": amount,       (numeric or string, optional, default=unlimited) Maximum value of **EACH** UTXO in DGB
        *       "maximumCount": n,             (numeric, optional, default=unlimited) Maximum number of UTXOs
        *       "minimumSumAmount": amount,    (numeric or string, optional, default=unlimited) Minimum sum value of all UTXOs in DGB(stops processing once reached will still return what is found if not reached)
        *       "includeAsset": index,         (numeric, bool, or string, default=true) if as string returns only asset utxo with that assetId,
        *                                                                               if as integer returns only asset utxo with that assetIndex(faster),
        *                                                                               if true returns all utxo
        *                                                                               if false returns only funds utxos
        *       "detailedAssetData": bool      (bool, optional=false) if true includes detailed asset data.  if false use getassetdata to get
        *     }
        *
        *
        * if addresses provided that are not part of wallet it will only return asset utxo unless storenonassetutxo is true                     *
        */
        extern const Response listunspent(const Json::Value& params) {
            if (params.size() >5) {
                throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
            }

            //get min confirms default is 1
            unsigned int minConfirm=1;
            if (params.size()>0) {
                if (params[0].isUInt()) {
                    minConfirm = params[0].asUInt();
                } else if (!params[0].isNull()) {
                    throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
                }
            }

            //get max confirms default is infinity
            unsigned int maxConfirm=std::numeric_limits<unsigned int>::max();
            if (params.size()>1) {
                if (params[1].isUInt()) {
                    maxConfirm = params[1].asUInt();
                } else if (!params[1].isNull()) {
                    throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
                }
            }

            //get addresses
            vector<string> addresses;
            if ((params.size()>2)&&(!params[2].isNull())) {
                if (!params[2].isArray()) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");

                //use provided list
                for (const auto& address: params[2]) {
                    addresses.emplace_back(address.asString());
                }
            } else {
                //get list from wallet
                DigiByteCore* dgb=AppMain::GetInstance()->getDigiByteCore();
                auto labels=dgb->listlabels();
                for (const auto& label: labels) {
                    auto addressList=dgb->getaddressesbylabel(label);
                    for (const auto& address: addressList) {
                        addresses.emplace_back(address);
                    }
                }
            }

            //get query options
            uint64_t minAmount=0;
            uint64_t maxAmount=std::numeric_limits<uint64_t>::max();
            unsigned int maxCount=std::numeric_limits<unsigned int>::max();
            double minSumAmount=INFINITY;
            bool returnDigiAssets=true;
            uint64_t restrictDigiAssetIndex=0;
            string restrictDigiAsset;
            bool detailedAssetData=false;
            if (params.size()==5) {
                if (!params[4].isObject()) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");

                if (params[4].isMember("minimumAmount")) {
                    if (params[4]["minimumAmount"].isDouble()) {
                        minAmount=floor(params[4]["minimumAmount"].asDouble()*100000000);
                    } else {
                        throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
                    }
                }

                if (params[4].isMember("maximumAmount")) {
                    if (params[4]["maximumAmount"].isDouble()) {
                        maxAmount=ceil(params[4]["maximumAmount"].asDouble()*100000000);
                    } else {
                        throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
                    }
                    if (maxAmount<minAmount) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
                }

                if (params[4].isMember("minimumSumAmount")) {
                    if (params[4]["minimumSumAmount"].isDouble()) {
                        minSumAmount=params[4]["minimumSumAmount"].asDouble();
                    } else {
                        throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
                    }
                }

                if (params[4].isMember("includeAsset")) {
                    if (params[4]["includeAsset"].isUInt64()) {
                        restrictDigiAssetIndex=params[4]["includeAsset"].asUInt64();
                    } else if (params[4]["includeAsset"].isString()) {
                        restrictDigiAsset = params[4]["includeAsset"].asString();
                    } else if (params[4]["includeAsset"].isBool()) {
                        returnDigiAssets = params[4]["includeAsset"].asBool();
                    } else {
                        throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
                    }
                }

                if (params[4].isMember("detailedAssetData")) {
                    if (!params[4]["detailedAssetData"].isBool()) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
                    detailedAssetData = params[4]["detailedAssetData"].asBool();
                }
            }

            //get desired utxos
            std::vector<AssetUTXO> utxos;
            Database* db=AppMain::GetInstance()->getDatabase();
            for (const auto& address: addresses) {
                auto addressUTXOs=db->getAddressUTXOs(address,minConfirm,maxConfirm);

                //filter out undesired utxos
                for (const auto& utxo : addressUTXOs) {

                    // Check if the UTXO meets the minimum and maximum amount criteria
                    if (utxo.assets.empty()) {

                        //if DigiByte UTXO check balance in requested range
                        if (utxo.digibyte < minAmount || utxo.digibyte > maxAmount) {
                            continue; // Skip UTXOs outside the desired amount range
                        }

                    } else if (returnDigiAssets) {

                        //is an asset utxo and returning at least some asset utxo
                        bool assetMatchFound = false;
                        for (const auto& asset : utxo.assets) {
                            if (!restrictDigiAsset.empty()) {

                                //match based on assetId
                                if (asset.getAssetId() == restrictDigiAsset) {
                                    assetMatchFound = true;
                                    break;
                                }

                            } else if (restrictDigiAssetIndex != 0) {

                                //match based on assetIndex
                                if (asset.getAssetIndex() == restrictDigiAssetIndex) {
                                    assetMatchFound = true;
                                    break;
                                }

                            } else {

                                //what asset not filtered
                                assetMatchFound = true;

                            }
                        }
                        if (!assetMatchFound) continue;

                    } else {

                        //digiasset utxo but not wanting digiasset utxos
                        continue;

                    }

                    // Add the UTXO to the list if it passed all filters
                    utxos.push_back(utxo);

                    // Check if we've collected enough UTXOs to meet the 'maximumCount' or 'minimumSumAmount' criteria
                    if (utxos.size() >= maxCount) goto end_loop;
                    double totalAmount = std::accumulate(utxos.begin(), utxos.end(), 0.0,
                                                         [](double sum, const AssetUTXO& utxo) { return sum + utxo.digibyte; });
                    if (totalAmount >= minSumAmount) goto end_loop;
                }
            }

            //convert results to json
        end_loop:
            Json::Value result=Json::arrayValue;
            for (const auto& utxo: utxos) {
                Json::Value temp=utxo.toJSON(!detailedAssetData);

                //fill in some values normally in listunspent
                temp["amount"]=static_cast<double>(utxo.digibyte)/100000000;
                result.append(temp);
            }

            //return response
            Response response;
            response.setResult(result);
            response.setBlocksGoodFor(5760); //day
            for (string address: addresses) {
                response.addInvalidateOnAddressChange(address);
            }
            return response;
        }

    }
}