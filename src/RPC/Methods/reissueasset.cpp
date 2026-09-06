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
        * Issues more of an existing UNLOCKED asset.  The wallet must control the address the
        * asset was originally issued from, and that address must hold at least one DigiByte
        * only UTXO(the assetId is derived from the first input, so the reissuance must spend
        * from the issuer address.  Send a little DGB to it first if it has no coins).
        *
        * The reissuance reuses the existing metadata and does not change the asset's rules
        * (the chain keeps the old rules when an issuance carries none).
        *
        * params[0] - asset(string assetId or integer assetIndex) - the unlocked asset
        * params[1] - amount(integer, decimal number or numeric string) - additional assets to
        *             create in display units
        * params[2] - options(object optional):
        *             "toAddress"(string) - address or domain the new assets go to.  Defaults to
        *                                   a new wallet address
        *             "dryrun"(bool default false) - build the transaction and return what it
        *                                   would cost WITHOUT broadcasting anything
        *
        * @return object:
        *   "txid"(string) - txid of the reissuance transaction
        *   "assetId"(string) - id of the asset(same as the original)
        * or when dryrun is set:
        *   "outputs", "estimatedMinerFee", "estimatedTotal"(decimal DGB strings), "sats"{...}
        */
        extern const Response reissueasset(const Json::Value& params) {
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
            DigiAsset existing;
            try {
                existing = db->getAsset(assetIndex);
            } catch (const Database::exceptionFailedSelect& e) {
                throw DigiByteException(RPC_MISC_ERROR, "Asset doesn't exist");
            }
            if (assetIndex == 1) throw DigiByteException(RPC_INVALID_PARAMS, "DigiByte is not an asset");
            if (existing.isLocked()) {
                throw DigiByteException(RPC_INVALID_PARAMS, "Asset is locked - no more can ever be issued");
            }

            //parse amount in to smallest divisible units
            uint64_t amount;
            try {
                amount = AssetWallet::parseAssetAmount(params[1], existing.getDecimals());
            } catch (const std::out_of_range& e) {
                throw DigiByteException(RPC_INVALID_PARAMS, e.what());
            }

            //get options
            std::string toAddress;
            bool dryrun = false;
            if (params.size() == 3) {
                if (!params[2].isObject()) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
                if (params[2].isMember("toAddress")) {
                    toAddress = params[2]["toAddress"].asString();
                    if (DigiByteDomain::isDomain(toAddress)) {
                        toAddress = DigiByteDomain::getAddress(toAddress);
                    }
                }
                if (params[2].isMember("dryrun")) {
                    if (!params[2]["dryrun"].isBool()) throw DigiByteException(RPC_INVALID_PARAMS, "dryrun must be a bool");
                    dryrun = params[2]["dryrun"].asBool();
                }
            }
            if (toAddress.empty() && !dryrun) toAddress = main->getDigiByteCore()->getnewaddress();

            //the assetId is derived from the first input's address, so the reissuance MUST
            //spend a UTXO sitting on the original issuer address.  It also must not carry
            //assets(issuances would burn them)
            std::string issuerAddress = existing.getIssuer().getAddress();
            if (issuerAddress.empty()) {
                throw DigiByteException(RPC_MISC_ERROR, "Could not determine the asset's issuer address");
            }
            AssetUTXO issuerInput;
            bool foundInput = false;
            for (const AssetUTXO& utxo: AssetWallet::getWalletUTXOs(1)) {
                if ((utxo.address == issuerAddress) && (utxo.assets.empty())) {
                    issuerInput = utxo;
                    foundInput = true;
                    break;
                }
            }
            if (!foundInput) {
                throw DigiByteException(RPC_WALLET_INSUFFICIENT_FUNDS,
                                        "Wallet holds no spendable DigiByte on the issuer address " + issuerAddress +
                                                " - send some DGB there first(the reissuance must spend from that address)");
            }

            //rebuild the asset with the same identity determining fields(cid, decimals,
            //aggregation, unlocked) and the additional amount.  No rules are encoded so the
            //chain keeps whatever rules the asset already has
            unsigned char aggregation = DigiAsset::AGGREGABLE;
            if (existing.isHybrid()) aggregation = DigiAsset::HYBRID;
            if (existing.isDispersed()) aggregation = DigiAsset::DISPERSED;
            DigiAsset addition;
            try {
                addition = DigiAsset(existing.getCID(), amount, existing.getDecimals(), false, aggregation);
            } catch (const std::out_of_range& e) {
                throw DigiByteException(RPC_INVALID_PARAMS, e.what());
            }

            DigiByteTransaction tx;
            tx.addInput(issuerInput);
            tx.setIssuance(addition);
            //a dry run with no explicit toAddress uses the issuer address as a stand in
            //instead of consuming a fresh wallet address(cost is identical)
            tx.addDigiAssetOutput(toAddress.empty() ? issuerAddress : toAddress, std::vector<DigiAsset>{addition});

            //dry run stops here: report the cost without broadcasting anything
            if (dryrun) {
                uint64_t outputSats = 0;
                for (unsigned int i = 0; i < tx.getOutputCount(); i++) outputSats += tx.getOutput(i).digibyte;
                //the issuer input's DGB comes back out(minus fee), so cost is just the fee
                //plus any dust the new asset output needs beyond that input
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

            //fund, sign and broadcast(fundSignSend verifies the first input keeps its place)
            std::string signedHex;
            std::string txid = AssetWallet::fundSignSend(tx, &signedHex);

            //confirm the derived assetId matches(it must - first input is on the issuer address)
            std::string newAssetId;
            try {
                decoderawtransaction_t decoded = main->getDigiByteCore()->decoderawtransaction(signedHex);
                newAssetId = addition.calculateAssetId(decoded.vin[0], addition.getIssuanceFlags());
            } catch (const std::exception& e) {
                newAssetId = "";
            }

            Json::Value result = Json::objectValue;
            result["txid"] = txid;
            result["assetId"] = newAssetId.empty() ? existing.getAssetId() : newAssetId;
            if ((!newAssetId.empty()) && (newAssetId != existing.getAssetId())) {
                //should be impossible - surface loudly if it ever happens
                result["warning"] = "derived assetId " + newAssetId + " does not match " + existing.getAssetId();
            }
            Response response;
            response.setResult(result);
            response.setBlocksGoodFor(-1); //do not cache
            return response;
        }

    } // namespace Methods
} // namespace RPC
