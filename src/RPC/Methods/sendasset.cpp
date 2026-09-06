//
// Created by DigiAsset Core on 14/07/26.
//

#include "AppMain.h"
#include "AssetWallet.h"
#include "DigiByteDomain.h"
#include "DigiByteTransaction.h"
#include "RPC/Response.h"
#include "RPC/Server.h"
#include <jsoncpp/json/value.h>

namespace RPC {
    namespace Methods {
        /**
        * Sends DigiAssets from the wallet to an address.  The wallet picks which asset UTXOs
        * to spend, adds asset change back to the wallet and pays the transaction fee in DigiByte.
        *
        * params[0] - address(string) - recipient address or DigiByte domain
        * params[1] - asset(string assetId or integer assetIndex) - asset to send.  Assets with
        *             more than 1 index(unlocked and not starting with "Ua") must use assetIndex
        * params[2] - amount(integer, decimal number or numeric string) - amount to send in display
        *             units.  So sending 1.5 of an asset with 2 decimals sends 150 of the smallest unit
        * params[3] - options(object optional):
        *             "changeAddress"(string) - address asset change should go to.  Defaults to a
        *                                       new wallet change address
        *             "dryrun"(bool default false) - build the transaction and return what it
        *                                       would cost WITHOUT broadcasting anything
        *
        * @return txid(string) of the broadcast transaction, or when dryrun is set an object:
        *   "outputs", "estimatedMinerFee", "estimatedTotal"(decimal DGB strings), "sats"{...}
        */
        extern const Response sendasset(const Json::Value& params) {
            if (params.size() < 3 || params.size() > 4) {
                throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
            }
            if (!params[0].isString()) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");

            AppMain* main = AppMain::GetInstance();
            Database* db = main->getDatabase();

            //resolve recipient(may be a DigiByte domain)
            std::string recipient = params[0].asString();
            if (DigiByteDomain::isDomain(recipient)) {
                recipient = DigiByteDomain::getAddress(recipient);
            }

            //look up the asset
            uint64_t assetIndex;
            try {
                if (params[1].isString()) {
                    assetIndex = db->getAssetIndex(params[1].asString());
                } else if (params[1].isInt()) {
                    assetIndex = params[1].asInt();
                } else {
                    throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
                }
            } catch (const Database::exceptionFailedSelect& e) {
                throw DigiByteException(RPC_MISC_ERROR, "Asset doesn't exist");
            }
            DigiAsset asset;
            try {
                asset = db->getAsset(assetIndex);
            } catch (const Database::exceptionFailedSelect& e) {
                throw DigiByteException(RPC_MISC_ERROR, "Asset doesn't exist");
            }
            if (assetIndex == 1) throw DigiByteException(RPC_INVALID_PARAMS, "Use sendtoaddress to send DigiByte");

            //parse amount in to smallest divisible units
            uint64_t amount;
            try {
                amount = AssetWallet::parseAssetAmount(params[2], asset.getDecimals());
            } catch (const std::out_of_range& e) {
                throw DigiByteException(RPC_INVALID_PARAMS, e.what());
            }

            //get options
            std::string changeAddress;
            bool dryrun = false;
            if (params.size() == 4) {
                if (!params[3].isObject()) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
                if (params[3].isMember("changeAddress")) {
                    changeAddress = params[3]["changeAddress"].asString();
                    if (DigiByteDomain::isDomain(changeAddress)) {
                        changeAddress = DigiByteDomain::getAddress(changeAddress);
                    }
                }
                if (params[3].isMember("dryrun")) {
                    if (!params[3]["dryrun"].isBool()) throw DigiByteException(RPC_INVALID_PARAMS, "dryrun must be a bool");
                    dryrun = params[3]["dryrun"].asBool();
                }
            }

            //pick the wallet UTXOs that will fund the send
            std::vector<AssetUTXO> inputs;
            try {
                inputs = AssetWallet::selectAssetInputs(assetIndex, amount);
            } catch (const DigiByteTransaction::exceptionNotEnoughFunds& e) {
                throw DigiByteException(RPC_WALLET_INSUFFICIENT_FUNDS, "Wallet does not hold enough of this asset");
            }

            //figure out what is left over(change).  Selected inputs may carry more of the sent
            //asset and possibly other assets that all need to be routed back to the wallet
            uint64_t changeAmount = 0;
            std::vector<DigiAsset> otherAssets;
            for (const AssetUTXO& input: inputs) {
                for (const DigiAsset& inputAsset: input.assets) {
                    if (inputAsset.getAssetIndex() == assetIndex) {
                        changeAmount += inputAsset.getCount();
                    } else {
                        //merge same assets together
                        bool found = false;
                        for (DigiAsset& other: otherAssets) {
                            if (other.getAssetIndex() == inputAsset.getAssetIndex()) {
                                other.addCount(inputAsset.getCount());
                                found = true;
                                break;
                            }
                        }
                        if (!found) otherAssets.push_back(inputAsset);
                    }
                }
            }
            changeAmount -= amount;

            //build the transaction
            DigiByteTransaction tx;
            for (const AssetUTXO& input: inputs) tx.addInput(input);

            DigiAsset sendAsset = asset;
            sendAsset.setCount(amount);
            tx.addDigiAssetOutput(recipient, std::vector<DigiAsset>{sendAsset});

            if ((changeAmount > 0) || (!otherAssets.empty())) {
                if (changeAddress.empty()) {
                    changeAddress = main->getDigiByteCore()->getrawchangeaddress();
                }
                std::vector<DigiAsset> changeAssets = otherAssets;
                if (changeAmount > 0) {
                    DigiAsset changeAsset = asset;
                    changeAsset.setCount(changeAmount);
                    changeAssets.insert(changeAssets.begin(), changeAsset);
                }
                tx.addDigiAssetOutput(changeAddress, changeAssets);
            }

            //dry run stops here: report the cost without broadcasting anything
            if (dryrun) {
                uint64_t outputSats = 0;
                for (unsigned int i = 0; i < tx.getOutputCount(); i++) outputSats += tx.getOutput(i).digibyte;
                uint64_t minerFeeSats = AssetWallet::estimateMinerFee(tx);

                Json::Value result = Json::objectValue;
                result["outputs"] = AssetWallet::satsToDecimal(outputSats);
                result["estimatedMinerFee"] = AssetWallet::satsToDecimal(minerFeeSats);
                result["estimatedTotal"] = AssetWallet::satsToDecimal(outputSats + minerFeeSats);
                result["sats"] = Json::objectValue;
                result["sats"]["outputs"] = static_cast<Json::UInt64>(outputSats);
                result["sats"]["estimatedMinerFee"] = static_cast<Json::UInt64>(minerFeeSats);
                Response response;
                response.setResult(result);
                response.setBlocksGoodFor(-1); //do not cache
                return response;
            }

            //fund, sign and broadcast
            std::string txid = AssetWallet::fundSignSend(tx);

            //return response
            Response response;
            response.setResult(txid);
            response.setBlocksGoodFor(-1); //do not cache
            return response;
        }

    } // namespace Methods
} // namespace RPC
