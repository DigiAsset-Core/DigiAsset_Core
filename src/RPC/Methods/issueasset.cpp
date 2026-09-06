//
// Created by DigiAsset Core on 14/07/26.
//

#include "AppMain.h"
#include "AssetWallet.h"
#include "DigiAssetConstants.h"
#include "DigiByteDomain.h"
#include "DigiByteTransaction.h"
#include "IPFS.h"
#include "RPC/Response.h"
#include "RPC/Server.h"
#include <algorithm>
#include <jsoncpp/json/value.h>
#include <jsoncpp/json/writer.h>

namespace RPC {
    namespace Methods {
        /**
        * Creates(issues) a new DigiAsset.  The metadata gets published to IPFS, the issuance
        * transaction is funded, signed and broadcast by the wallet.
        *
        * params[0] - object describing the asset:
        *   "name"(string required) - asset name
        *   "amount"(integer, decimal number or numeric string required) - number of assets to
        *            create in display units("1.5" with 2 decimals creates 150 of the smallest unit)
        *   "decimals"(integer 0-7 optional default 0) - number of decimal places
        *   "locked"(bool optional default true) - true means no more can ever be issued
        *   "aggregation"(string optional default "aggregable") - one of "aggregable", "hybrid", "dispersed"
        *   "description"(string optional) - description stored in the metadata
        *   "urls"(array optional) - array of url objects({"name":..,"url":..,"mimeType":..}) stored in the metadata
        *   "userData"(object optional) - any extra data to store in the metadata
        *   "toAddress"(string optional) - address or domain the new assets should be sent to.
        *                                  Defaults to a new wallet address
        *   "metadata"(object optional) - advanced: full metadata object to publish as is.  When used,
        *                                 name/description/urls/userData are ignored
        *   "psp"(optional default true) - which permanent storage pool(s) store the metadata(and any
        *                                 IPFS urls it references).  The public pool charges $1.20 USD
        *                                 per MB, paid in DGB at the current on-chain exchange rate as
        *                                 an extra output on the issuance.  Accepts:
        *                                    true - the public pool(default)
        *                                    false - no pool.  Issuances that don't pay in to a pool
        *                                            are not recognised by most of the ecosystem, so
        *                                            only use if you know what you are doing
        *                                    integer - a single pool index(see getpsp)
        *                                    array of integers - pay in to several pools
        *
        *   "rules"(object optional) - restrictions encoded permanently with the asset(locked
        *            assets) or changeable later(unlocked assets issued with "changeable":true):
        *      "changeable"(bool default false) - rules may be rewritten by a later issuance.
        *                   Requires "locked":false.  Can not be combined with voting/expiry
        *      "approval": {"required":weight,"approvers":{address:weight,..}} - transfers must
        *                   be approved by signers totalling the required weight
        *      "royalty": {"units":"USD"(or standard rate index, optional),
        *                  "addresses":{address:amount,..}} - every transfer must pay these
        *                   royalties.  Amounts are DGB sats, or units*100000000 when units set
        *      "kyc"(bool) - true means only KYC verified addresses may hold the asset
        *      "geofence": {"allowed":["CAN",..]} or {"denied":[..]} - country restrictions
        *                   (ISO 3166-1 alpha-3, implies KYC)
        *      "voting": {"options":["label",..] or [{"address":..,"label":..},..],
        *                 "restricted":bool} - vote asset.  Label-only options use the standard
        *                   vote addresses(tallied automatically).  restricted:true means the
        *                   asset may ONLY be sent to vote addresses
        *      "expiry"(integer) - block height(<1577836800000) or ms epoch time after which
        *                   the asset can no longer be transferred.  With voting = vote cutoff
        *      "deflation"(integer) - base units that must be burned with every transfer
        *
        *   "dryrun"(bool optional default false) - build everything and return the cost
        *                                 breakdown WITHOUT broadcasting.  The metadata is
        *                                 published to the local IPFS node(needed to price
        *                                 storage) but nothing touches the chain
        *
        * @return object:
        *   "txid"(string) - txid of the issuance transaction
        *   "assetId"(string) - id of the newly created asset
        *   "cid"(string) - IPFS cid of the published metadata
        * or when dryrun is set:
        *   "cid", "assetOutputs", "ruleOutputs", "pspFee", "estimatedMinerFee",
        *   "estimatedTotal"(strings, DGB) and "sats"(object) with the same in sats
        */
        extern const Response issueasset(const Json::Value& params) {
            if ((params.size() != 1) || !params[0].isObject()) {
                throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
            }
            const Json::Value& config = params[0];

            AppMain* main = AppMain::GetInstance();

            //required fields
            if (!config.isMember("name") || !config["name"].isString() || config["name"].asString().empty()) {
                throw DigiByteException(RPC_INVALID_PARAMS, "name is required");
            }
            if (!config.isMember("amount")) throw DigiByteException(RPC_INVALID_PARAMS, "amount is required");

            //optional fields
            unsigned char decimals = 0;
            if (config.isMember("decimals")) {
                if (!config["decimals"].isInt() || (config["decimals"].asInt() < 0) || (config["decimals"].asInt() > 7)) {
                    throw DigiByteException(RPC_INVALID_PARAMS, "decimals must be an integer from 0 to 7");
                }
                decimals = static_cast<unsigned char>(config["decimals"].asInt());
            }
            bool locked = true;
            if (config.isMember("locked")) {
                if (!config["locked"].isBool()) throw DigiByteException(RPC_INVALID_PARAMS, "locked must be a bool");
                locked = config["locked"].asBool();
            }
            unsigned char aggregation = DigiAsset::AGGREGABLE;
            if (config.isMember("aggregation")) {
                std::string aggregationStr = config["aggregation"].asString();
                if (aggregationStr == "aggregable") {
                    aggregation = DigiAsset::AGGREGABLE;
                } else if (aggregationStr == "hybrid") {
                    aggregation = DigiAsset::HYBRID;
                } else if (aggregationStr == "dispersed") {
                    aggregation = DigiAsset::DISPERSED;
                } else {
                    throw DigiByteException(RPC_INVALID_PARAMS, "aggregation must be aggregable, hybrid or dispersed");
                }
            }

            //amount(in display units)
            uint64_t amount;
            try {
                amount = AssetWallet::parseAssetAmount(config["amount"], decimals);
            } catch (const std::out_of_range& e) {
                throw DigiByteException(RPC_INVALID_PARAMS, e.what());
            }

            //parse the rules(same shape getassetdata returns them in)
            DigiAssetRules rules;
            Json::Value votesForMetadata = Json::nullValue; //vote labels that must be published in the metadata
            if (config.isMember("rules")) {
                if (!config["rules"].isObject()) throw DigiByteException(RPC_INVALID_PARAMS, "rules must be an object");
                const Json::Value& rulesJson = config["rules"];

                //reject unknown keys so typos fail loudly instead of issuing a permanent asset without the intended rule
                for (const std::string& key: rulesJson.getMemberNames()) {
                    if ((key != "changeable") && (key != "approval") && (key != "royalty") &&
                        (key != "geofence") && (key != "kyc") && (key != "voting") &&
                        (key != "expiry") && (key != "deflation")) {
                        throw DigiByteException(RPC_INVALID_PARAMS, "unknown rule \"" + key + "\" - valid rules are changeable, approval, royalty, geofence, kyc, voting, expiry, deflation");
                    }
                }

                //changeable(rewritable rules) - only possible on unlocked assets
                if (rulesJson.isMember("changeable")) {
                    if (!rulesJson["changeable"].isBool()) throw DigiByteException(RPC_INVALID_PARAMS, "rules.changeable must be a bool");
                    if (rulesJson["changeable"].asBool()) {
                        if (locked) throw DigiByteException(RPC_INVALID_PARAMS, "changeable rules require \"locked\":false");
                        rules.setRewritable(true);
                    }
                }

                //approval(signers that must approve every transfer)
                if (rulesJson.isMember("approval")) {
                    const Json::Value& approval = rulesJson["approval"];
                    if (!approval.isObject() || !approval.isMember("required") || !approval["required"].isUInt64() ||
                        !approval.isMember("approvers") || !approval["approvers"].isObject() || approval["approvers"].empty()) {
                        throw DigiByteException(RPC_INVALID_PARAMS, "rules.approval needs {\"required\":weight,\"approvers\":{address:weight,...}}");
                    }
                    std::vector<Signer> signers;
                    for (const std::string& address: approval["approvers"].getMemberNames()) {
                        if (!approval["approvers"][address].isUInt64() || (approval["approvers"][address].asUInt64() == 0)) {
                            throw DigiByteException(RPC_INVALID_PARAMS, "approver weights must be positive integers");
                        }
                        signers.push_back(Signer{address, approval["approvers"][address].asUInt64()});
                    }
                    rules.setRequireSigners(approval["required"].asUInt64(), signers);
                }

                //royalty(amounts are in DGB sats, or in units*100000000 when units are given)
                if (rulesJson.isMember("royalty")) {
                    const Json::Value& royalty = rulesJson["royalty"];
                    if (!royalty.isObject() || !royalty.isMember("addresses") || !royalty["addresses"].isObject() ||
                        royalty["addresses"].empty()) {
                        throw DigiByteException(RPC_INVALID_PARAMS, "rules.royalty needs {\"addresses\":{address:amount,...}} and optional \"units\"");
                    }
                    ExchangeRate rate{};
                    if (royalty.isMember("units")) {
                        const Json::Value& units = royalty["units"];
                        bool found = false;
                        for (size_t i = 0; i < DigiAssetConstants::standardExchangeRatesCount; i++) {
                            if ((units.isString() && (DigiAssetConstants::standardExchangeRates[i].name == units.asString())) ||
                                (units.isUInt() && (units.asUInt() == i))) {
                                rate = DigiAssetConstants::standardExchangeRates[i];
                                found = true;
                                break;
                            }
                        }
                        if (!found) throw DigiByteException(RPC_INVALID_PARAMS, "rules.royalty.units must be a standard currency code(e.g. \"USD\") or standard exchange rate index");
                    }
                    std::vector<Royalty> royalties;
                    for (const std::string& address: royalty["addresses"].getMemberNames()) {
                        if (!royalty["addresses"][address].isUInt64() || (royalty["addresses"][address].asUInt64() == 0)) {
                            throw DigiByteException(RPC_INVALID_PARAMS, "royalty amounts must be positive integers(sats, or units*100000000 when units set)");
                        }
                        royalties.push_back(Royalty{address, royalty["addresses"][address].asUInt64()});
                    }
                    rules.setRoyalties(royalties, rate);
                }

                //kyc / geofence
                if (rulesJson.isMember("kyc")) {
                    if (!rulesJson["kyc"].isBool()) throw DigiByteException(RPC_INVALID_PARAMS, "rules.kyc must be a bool");
                    if (rulesJson["kyc"].asBool()) rules.setRequireKYC(); //any KYCd country
                }
                if (rulesJson.isMember("geofence")) {
                    const Json::Value& geofence = rulesJson["geofence"];
                    bool allowed = geofence.isMember("allowed");
                    bool denied = geofence.isMember("denied");
                    if (!geofence.isObject() || (allowed == denied)) {
                        throw DigiByteException(RPC_INVALID_PARAMS, "rules.geofence needs exactly one of \"allowed\" or \"denied\" as an array of ISO 3166-1 alpha-3 country codes");
                    }
                    const Json::Value& list = allowed ? geofence["allowed"] : geofence["denied"];
                    if (!list.isArray()) throw DigiByteException(RPC_INVALID_PARAMS, "geofence country list must be an array");
                    std::vector<std::string> countries;
                    for (const Json::Value& country: list) {
                        if (!country.isString() || (country.asString().length() != 3)) {
                            throw DigiByteException(RPC_INVALID_PARAMS, "country codes must be ISO 3166-1 alpha-3(3 letters)");
                        }
                        countries.push_back(country.asString());
                    }
                    if (countries.empty()) {
                        if (allowed) throw DigiByteException(RPC_INVALID_PARAMS, "geofence.allowed can not be empty(nobody could hold the asset)");
                        rules.setRequireKYC(); //empty deny list = KYC required, any country
                    } else {
                        rules.setRequireKYC(countries, denied);
                    }
                }

                //expiry(block height if below 1577836800000, otherwise ms since epoch)
                uint64_t expiry = DigiAssetRules::EXPIRE_NEVER;
                if (rulesJson.isMember("expiry")) {
                    if (!rulesJson["expiry"].isUInt64() || (rulesJson["expiry"].asUInt64() == 0) ||
                        (rulesJson["expiry"].asUInt64() > (uint64_t) 18014398509481983)) {
                        throw DigiByteException(RPC_INVALID_PARAMS, "rules.expiry must be a block height or a ms epoch time");
                    }
                    expiry = rulesJson["expiry"].asUInt64();
                }

                //voting(labels get published in the metadata, addresses on chain)
                if (rulesJson.isMember("voting")) {
                    const Json::Value& voting = rulesJson["voting"];
                    if (!voting.isObject() || !voting.isMember("options") || !voting["options"].isArray() ||
                        voting["options"].empty() || (voting["options"].size() > 127)) {
                        throw DigiByteException(RPC_INVALID_PARAMS, "rules.voting needs {\"options\":[...1 to 127 entries...]} of label strings(standard vote addresses) or {\"address\":..,\"label\":..} objects");
                    }
                    bool movable = true;
                    if (voting.isMember("restricted")) {
                        if (!voting["restricted"].isBool()) throw DigiByteException(RPC_INVALID_PARAMS, "voting.restricted must be a bool");
                        movable = !voting["restricted"].asBool();
                    }
                    bool stringForm = voting["options"][0].isString();
                    if (stringForm && (voting["options"].size() > DigiAssetConstants::standardVoteCount)) {
                        throw DigiByteException(RPC_INVALID_PARAMS, "at most " + std::to_string(DigiAssetConstants::standardVoteCount) + " label-only vote options(use {address,label} objects for more)");
                    }
                    std::vector<VoteOption> options;
                    votesForMetadata = Json::arrayValue;
                    for (Json::ArrayIndex i = 0; i < voting["options"].size(); i++) {
                        const Json::Value& option = voting["options"][i];
                        if (stringForm) {
                            if (!option.isString()) throw DigiByteException(RPC_INVALID_PARAMS, "vote options must be all strings or all objects");
                            options.push_back(VoteOption{DigiAssetConstants::standardVoteAddresses[i], option.asString()});
                        } else {
                            if (!option.isObject() || !option.isMember("address") || !option["address"].isString() ||
                                !option.isMember("label") || !option["label"].isString()) {
                                throw DigiByteException(RPC_INVALID_PARAMS, "object vote options need address and label strings");
                            }
                            options.push_back(VoteOption{option["address"].asString(), option["label"].asString()});
                        }
                        votesForMetadata.append(option);
                    }
                    rules.setVote(options, expiry, movable);
                } else if (expiry != DigiAssetRules::EXPIRE_NEVER) {
                    rules.setExpiry(expiry);
                }

                //deflation(assets that must be burned per transfer, in base units)
                if (rulesJson.isMember("deflation")) {
                    if (!rulesJson["deflation"].isUInt64() || (rulesJson["deflation"].asUInt64() == 0)) {
                        throw DigiByteException(RPC_INVALID_PARAMS, "rules.deflation must be a positive integer(base units burned per transfer)");
                    }
                    rules.setDeflationary(rulesJson["deflation"].asUInt64());
                }

                if (rules.empty()) throw DigiByteException(RPC_INVALID_PARAMS, "rules object contains no rules");
            }

            //permanent storage pool participation(default: the public pool.  Unstored assets aren't
            //recognised by most of the ecosystem)
            std::vector<unsigned int> pspPools{1}; //pool 1 = public pool
            if (config.isMember("psp")) {
                const Json::Value& pspVal = config["psp"];
                pspPools.clear();
                if (pspVal.isBool()) {
                    if (pspVal.asBool()) pspPools.push_back(1);
                } else if (pspVal.isUInt()) {
                    pspPools.push_back(pspVal.asUInt());
                } else if (pspVal.isArray()) {
                    for (const Json::Value& v: pspVal) {
                        if (!v.isUInt()) throw DigiByteException(RPC_INVALID_PARAMS, "psp array must contain pool indexes");
                        pspPools.push_back(v.asUInt());
                    }
                } else {
                    throw DigiByteException(RPC_INVALID_PARAMS, "psp must be a bool, a pool index or an array of pool indexes");
                }
                std::sort(pspPools.begin(), pspPools.end());
                pspPools.erase(std::unique(pspPools.begin(), pspPools.end()), pspPools.end());
                for (unsigned int poolIndex: pspPools) {
                    if (poolIndex >= main->getPermanentStoragePoolList()->getPoolCount()) {
                        throw DigiByteException(RPC_INVALID_PARAMS, "psp pool index out of range - see getpsp");
                    }
                }
            }

            //dry run: build everything and report the costs without broadcasting
            bool dryrun = false;
            if (config.isMember("dryrun")) {
                if (!config["dryrun"].isBool()) throw DigiByteException(RPC_INVALID_PARAMS, "dryrun must be a bool");
                dryrun = config["dryrun"].asBool();
            }

            //where the new assets should go
            std::string toAddress;
            if (config.isMember("toAddress")) {
                toAddress = config["toAddress"].asString();
                if (DigiByteDomain::isDomain(toAddress)) {
                    toAddress = DigiByteDomain::getAddress(toAddress);
                }
            } else {
                toAddress = main->getDigiByteCore()->getnewaddress();
            }

            //build the metadata document
            Json::Value metadata;
            if (config.isMember("metadata")) {
                if (!config["metadata"].isObject()) throw DigiByteException(RPC_INVALID_PARAMS, "metadata must be an object");
                metadata = config["metadata"];
                if (!votesForMetadata.isNull() && !metadata.isMember("votes")) {
                    throw DigiByteException(RPC_INVALID_PARAMS, "voting rules require a \"votes\" array in the supplied metadata(matching rules.voting.options)");
                }
            } else {
                Json::Value data = Json::objectValue;
                data["assetName"] = config["name"];
                if (config.isMember("description")) data["description"] = config["description"];
                if (config.isMember("urls")) {
                    if (!config["urls"].isArray()) throw DigiByteException(RPC_INVALID_PARAMS, "urls must be an array");
                    data["urls"] = config["urls"];
                } else {
                    data["urls"] = Json::arrayValue; //pools require data.urls to exist to price storage
                }
                if (config.isMember("userData")) data["userData"] = config["userData"];
                metadata = Json::objectValue;
                metadata["data"] = data;
                //vote labels live in the metadata(root level) and are validated against the
                //on-chain addresses by DigiAssetRules::getVoteOptions
                if (!votesForMetadata.isNull()) metadata["votes"] = votesForMetadata;
            }

            //publish the metadata to IPFS
            Json::FastWriter writer;
            std::string cid;
            try {
                cid = main->getIPFS()->addFile(writer.write(metadata));
            } catch (const IPFS::exception& e) {
                throw DigiByteException(RPC_MISC_ERROR, std::string("Failed to publish metadata to IPFS: ") + e.what());
            }

            //build the issuance transaction
            DigiAsset asset;
            try {
                asset = DigiAsset(cid, amount, decimals, locked, aggregation, rules);
            } catch (const std::out_of_range& e) {
                throw DigiByteException(RPC_INVALID_PARAMS, e.what());
            }
            DigiByteTransaction tx;
            tx.setIssuance(asset);
            DigiAsset newAssets = asset; //full amount to the recipient
            tx.addDigiAssetOutput(toAddress, std::vector<DigiAsset>{newAssets});
            unsigned int assetOutputsEnd = tx.getOutputCount();
            try {
                tx.addRuleOutputs(); //signer/royalty/custom-vote outputs(no-op without rules)
            } catch (const DigiAssetRules::exception& e) {
                throw DigiByteException(RPC_INVALID_PARAMS, e.what());
            }
            unsigned int ruleOutputsEnd = tx.getOutputCount();

            //add the permanent storage pool payments(must be the last change to the tx before funding)
            for (unsigned int poolIndex: pspPools) {
                try {
                    main->getPermanentStoragePoolList()->getPool(poolIndex)->enable(tx);
                } catch (const DigiAsset::exceptionInvalidMetaData& e) {
                    throw DigiByteException(RPC_INVALID_PARAMS, "metadata is not compatible with the storage pool(data.urls must be an array and all urls must be on IPFS)");
                } catch (const PermanentStoragePool::exceptionCantEnablePSP& e) {
                    throw DigiByteException(RPC_MISC_ERROR, "could not enable permanent storage on this issuance");
                }
            }

            //dry run stops here: report what the issuance would cost without broadcasting.
            //(the metadata HAS been published to the local IPFS node so its size could be
            //priced - nothing has touched the chain)
            if (dryrun) {
                uint64_t assetOutputsSats = 0;
                uint64_t ruleOutputsSats = 0;
                uint64_t pspFeeSats = 0;
                for (unsigned int i = 0; i < tx.getOutputCount(); i++) {
                    uint64_t sats = tx.getOutput(i).digibyte;
                    if (i < assetOutputsEnd) {
                        assetOutputsSats += sats;
                    } else if (i < ruleOutputsEnd) {
                        ruleOutputsSats += sats;
                    } else {
                        pspFeeSats += sats;
                    }
                }

                uint64_t minerFeeSats = AssetWallet::estimateMinerFee(tx);

                Json::Value result = Json::objectValue;
                result["cid"] = cid;
                result["assetOutputs"] = AssetWallet::satsToDecimal(assetOutputsSats);
                result["ruleOutputs"] = AssetWallet::satsToDecimal(ruleOutputsSats);
                result["pspFee"] = AssetWallet::satsToDecimal(pspFeeSats);
                result["estimatedMinerFee"] = AssetWallet::satsToDecimal(minerFeeSats);
                result["estimatedTotal"] = AssetWallet::satsToDecimal(assetOutputsSats + ruleOutputsSats + pspFeeSats + minerFeeSats);
                result["sats"] = Json::objectValue;
                result["sats"]["assetOutputs"] = static_cast<Json::UInt64>(assetOutputsSats);
                result["sats"]["ruleOutputs"] = static_cast<Json::UInt64>(ruleOutputsSats);
                result["sats"]["pspFee"] = static_cast<Json::UInt64>(pspFeeSats);
                result["sats"]["estimatedMinerFee"] = static_cast<Json::UInt64>(minerFeeSats);
                Response response;
                response.setResult(result);
                response.setBlocksGoodFor(-1); //do not cache
                return response;
            }

            //fund, sign and broadcast
            std::string signedHex;
            std::string txid = AssetWallet::fundSignSend(tx, &signedHex);

            //compute the new assetId from the first input of the signed transaction
            std::string assetId;
            try {
                decoderawtransaction_t decoded = main->getDigiByteCore()->decoderawtransaction(signedHex);
                assetId = asset.calculateAssetId(decoded.vin[0], asset.getIssuanceFlags());
            } catch (const std::exception& e) {
                //asset was issued fine - the id can be looked up once the tx confirms
                assetId = "";
            }

            //return response
            Json::Value result = Json::objectValue;
            result["txid"] = txid;
            result["assetId"] = assetId;
            result["cid"] = cid;
            Response response;
            response.setResult(result);
            response.setBlocksGoodFor(-1); //do not cache
            return response;
        }

    } // namespace Methods
} // namespace RPC
