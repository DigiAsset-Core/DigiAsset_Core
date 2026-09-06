//
// Created by DigiAsset Core on 19/07/26.
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
        * Permanently destroys DigiAssets held by the wallet.  The burn is written on chain
        * (a transfer instruction targeting the burn marker) so total supply verifiably drops.
        *
        * params[0] - asset(string assetId or integer assetIndex) - asset to burn.  Assets with
        *             more than 1 index(unlocked and not starting with "Ua") must use assetIndex
        * params[1] - amount(integer, decimal number or numeric string) - amount to burn in
        *             display units
        * params[2] - options(object optional):
        *             "changeAddress"(string) - address any asset change should go to.  Defaults
        *                                       to a new wallet change address
        *             "dryrun"(bool default false) - build the transaction and return what it
        *                                       would cost WITHOUT broadcasting anything
        *
        * @return txid(string) of the broadcast transaction, or when dryrun is set an object:
        *   "outputs", "estimatedMinerFee", "estimatedTotal"(decimal DGB strings), "sats"{...}
        */
        extern const Response burnasset(const Json::Value& params) {
            if (params.size() < 2 || params.size() > 3) {
                throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
            }

            AppMain* main = AppMain::GetInstance();
            Database* db = main->getDatabase();

            //look up the asset
            uint64_t assetIndex;
            try {
                if (params[0].isString()) {
                    assetIndex = db->getAssetIndex(params[0].asString());
                } else if (params[0].isInt()) {
                    assetIndex = params[0].asInt();
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
            if (assetIndex == 1) throw DigiByteException(RPC_INVALID_PARAMS, "DigiByte can not be burned");

            //parse amount in to smallest divisible units
            uint64_t amount;
            try {
                amount = AssetWallet::parseAssetAmount(params[1], asset.getDecimals());
            } catch (const std::out_of_range& e) {
                throw DigiByteException(RPC_INVALID_PARAMS, e.what());
            }

            //get options
            std::string changeAddress;
            bool dryrun = false;
            if (params.size() == 3) {
                if (!params[2].isObject()) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
                if (params[2].isMember("changeAddress")) {
                    changeAddress = params[2]["changeAddress"].asString();
                    if (DigiByteDomain::isDomain(changeAddress)) {
                        changeAddress = DigiByteDomain::getAddress(changeAddress);
                    }
                }
                if (params[2].isMember("dryrun")) {
                    if (!params[2]["dryrun"].isBool()) throw DigiByteException(RPC_INVALID_PARAMS, "dryrun must be a bool");
                    dryrun = params[2]["dryrun"].asBool();
                }
            }

            //pick the wallet UTXOs holding the assets being burned
            std::vector<AssetUTXO> inputs;
            try {
                inputs = AssetWallet::selectAssetInputs(assetIndex, amount);
            } catch (const DigiByteTransaction::exceptionNotEnoughFunds& e) {
                throw DigiByteException(RPC_WALLET_INSUFFICIENT_FUNDS, "Wallet does not hold enough of this asset");
            }

            //whatever the selected inputs carry beyond the burn amount goes back as change
            uint64_t changeAmount = 0;
            std::vector<DigiAsset> otherAssets;
            for (const AssetUTXO& input: inputs) {
                for (const DigiAsset& inputAsset: input.assets) {
                    if (inputAsset.getAssetIndex() == assetIndex) {
                        changeAmount += inputAsset.getCount();
                    } else {
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

            DigiAsset burnAsset = asset;
            burnAsset.setCount(amount);
            tx.addAssetBurn(std::vector<DigiAsset>{burnAsset});

            if ((changeAmount > 0) || (!otherAssets.empty())) {
                if (changeAddress.empty()) {
                    changeAddress = main->getDigiByteCore()->getrawchangeaddress();
                }
                std::vector<DigiAsset> changeAssets = otherAssets;
                if (changeAmount > 0) {
                    DigiAsset changePart = asset;
                    changePart.setCount(changeAmount);
                    changeAssets.insert(changeAssets.begin(), changePart);
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

            Response response;
            response.setResult(txid);
            response.setBlocksGoodFor(-1); //do not cache
            return response;
        }

    } // namespace Methods
} // namespace RPC
