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
#include <map>

namespace RPC {
    namespace Methods {
        /**
        * Sends DigiAssets to several recipients in a single transaction.  The wallet picks
        * which asset UTXOs to spend, sends all asset change back to the wallet and pays the
        * transaction fee in DigiByte.
        *
        * params[0] - sends(array) - one entry per recipient:
        *     {
        *       "address"(string) - recipient address or DigiByte domain
        *       "asset"(string assetId or integer assetIndex) - asset to send.  Assets with
        *                more than 1 index(unlocked and not starting with "Ua") must use index
        *       "amount"(integer, decimal number or numeric string) - amount in display units
        *     }
        *     Up to 31 entries(protocol limit: asset outputs must sit in the first 32 outputs)
        * params[1] - options(object optional):
        *             "changeAddress"(string) - address asset change should go to.  Defaults to
        *                                       a new wallet change address
        *
        * @return txid(string) of the broadcast transaction
        */
        extern const Response sendmanyassets(const Json::Value& params) {
            if (params.size() < 1 || params.size() > 2) {
                throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
            }
            if (!params[0].isArray() || params[0].empty() || (params[0].size() > 31)) {
                throw DigiByteException(RPC_INVALID_PARAMS, "sends must be an array of 1 to 31 {address,asset,amount} objects");
            }

            AppMain* main = AppMain::GetInstance();
            Database* db = main->getDatabase();

            //parse the send list
            struct SendEntry {
                std::string address;
                uint64_t assetIndex;
                uint64_t amount;
                DigiAsset asset;
            };
            std::vector<SendEntry> sends;
            std::map<uint64_t, uint64_t> totalPerAsset;
            for (const Json::Value& entry: params[0]) {
                if (!entry.isObject() || !entry.isMember("address") || !entry["address"].isString() ||
                    !entry.isMember("asset") || !entry.isMember("amount")) {
                    throw DigiByteException(RPC_INVALID_PARAMS, "each send needs address, asset and amount");
                }
                SendEntry send;
                send.address = entry["address"].asString();
                if (DigiByteDomain::isDomain(send.address)) {
                    send.address = DigiByteDomain::getAddress(send.address);
                }
                try {
                    if (entry["asset"].isString()) {
                        send.assetIndex = db->getAssetIndex(entry["asset"].asString());
                    } else if (entry["asset"].isInt()) {
                        send.assetIndex = entry["asset"].asInt();
                    } else {
                        throw DigiByteException(RPC_INVALID_PARAMS, "asset must be an assetId or assetIndex");
                    }
                    send.asset = db->getAsset(send.assetIndex);
                } catch (const Database::exceptionFailedSelect& e) {
                    throw DigiByteException(RPC_MISC_ERROR, "Asset doesn't exist");
                }
                if (send.assetIndex == 1) throw DigiByteException(RPC_INVALID_PARAMS, "Use sendtoaddress to send DigiByte");
                try {
                    send.amount = AssetWallet::parseAssetAmount(entry["amount"], send.asset.getDecimals());
                } catch (const std::out_of_range& e) {
                    throw DigiByteException(RPC_INVALID_PARAMS, e.what());
                }
                totalPerAsset[send.assetIndex] += send.amount;
                sends.push_back(send);
            }

            //get options
            std::string changeAddress;
            if (params.size() == 2) {
                if (!params[1].isObject()) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
                if (params[1].isMember("changeAddress")) {
                    changeAddress = params[1]["changeAddress"].asString();
                    if (DigiByteDomain::isDomain(changeAddress)) {
                        changeAddress = DigiByteDomain::getAddress(changeAddress);
                    }
                }
            }

            //select inputs per asset and merge(one UTXO can serve several assets)
            std::vector<AssetUTXO> inputs;
            for (const auto& needed: totalPerAsset) {
                std::vector<AssetUTXO> selection;
                try {
                    selection = AssetWallet::selectAssetInputs(needed.first, needed.second);
                } catch (const DigiByteTransaction::exceptionNotEnoughFunds& e) {
                    throw DigiByteException(RPC_WALLET_INSUFFICIENT_FUNDS,
                                            "Wallet does not hold enough of asset index " + std::to_string(needed.first));
                }
                for (const AssetUTXO& utxo: selection) {
                    bool duplicate = false;
                    for (const AssetUTXO& existing: inputs) {
                        if ((existing.txid == utxo.txid) && (existing.vout == utxo.vout)) {
                            duplicate = true;
                            break;
                        }
                    }
                    if (!duplicate) inputs.push_back(utxo);
                }
            }

            //whatever the inputs carry beyond what is being sent goes back as change
            std::map<uint64_t, DigiAsset> changeAssets; //assetIndex -> asset with leftover count
            for (const AssetUTXO& input: inputs) {
                for (const DigiAsset& inputAsset: input.assets) {
                    auto it = changeAssets.find(inputAsset.getAssetIndex());
                    if (it == changeAssets.end()) {
                        changeAssets[inputAsset.getAssetIndex()] = inputAsset;
                    } else {
                        it->second.addCount(inputAsset.getCount());
                    }
                }
            }
            for (const auto& needed: totalPerAsset) {
                changeAssets[needed.first].setCount(changeAssets[needed.first].getCount() - needed.second);
            }

            //group sends by recipient - the wallet's createrawtransaction rejects duplicate
            //addresses, and one output can carry several assets anyway
            std::vector<std::string> recipientOrder;
            std::map<std::string, std::vector<DigiAsset>> perRecipient;
            for (const SendEntry& send: sends) {
                if (perRecipient.find(send.address) == perRecipient.end()) recipientOrder.push_back(send.address);
                std::vector<DigiAsset>& list = perRecipient[send.address];
                bool merged = false;
                for (DigiAsset& part: list) {
                    if (part.getAssetIndex() == send.assetIndex) {
                        part.addCount(send.amount);
                        merged = true;
                        break;
                    }
                }
                if (!merged) {
                    DigiAsset part = send.asset;
                    part.setCount(send.amount);
                    list.push_back(part);
                }
            }

            //build the transaction: one output per unique recipient, then one change output
            DigiByteTransaction tx;
            for (const AssetUTXO& input: inputs) tx.addInput(input);
            for (const std::string& recipient: recipientOrder) {
                tx.addDigiAssetOutput(recipient, perRecipient[recipient]);
            }
            std::vector<DigiAsset> leftovers;
            for (const auto& change: changeAssets) {
                if (change.second.getCount() > 0) leftovers.push_back(change.second);
            }
            if (!leftovers.empty()) {
                if (changeAddress.empty()) {
                    changeAddress = main->getDigiByteCore()->getrawchangeaddress();
                }
                tx.addDigiAssetOutput(changeAddress, leftovers);
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
