//
// Created by DigiAsset Core on 14/07/26.
//

#include "AssetWallet.h"
#include "AppMain.h"
#include "Database.h"
#include "DigiAssetRules.h"
#include "DigiByteCore.h"
#include "RPC/Server.h" //for the RPC_ error code constants
#include <algorithm>
#include <chrono>
#include <cmath>
#include <thread>

using namespace std;

namespace AssetWallet {

    vector<AssetUTXO> getWalletUTXOs(int minconf) {
        AppMain* main = AppMain::GetInstance();
        DigiByteCore* dgb = main->getDigiByteCore();
        Database* db = main->getDatabase();

        vector<AssetUTXO> results;
        vector<unspenttxout_t> unspent = dgb->listUnspent(minconf);
        results.reserve(unspent.size());
        for (const unspenttxout_t& utxo: unspent) {
            AssetUTXO entry;
            try {
                entry = db->getAssetUTXO(utxo.txid, utxo.n);
            } catch (const Database::exceptionDataPruned& e) {
                //normal for every plain coin when storenonassetutxo=0, and possible for a recent
                //coin the analyzer has not reached yet.  Treating the unknown as asset free is the
                //cautious answer here - it only means a balance is under reported or an input is
                //not picked, and both fix themselves once the analyzer catches up.  fundSignSend
                //makes the opposite call because there the unknown coin would be spent as a fee
                entry.txid = utxo.txid;
                entry.vout = utxo.n;
                entry.assets.clear();
            }
            //db may not know address/value if not storing non asset utxos so use wallet values
            entry.address = utxo.address;
            entry.digibyte = static_cast<uint64_t>(llround(utxo.amount * 100000000.0));
            results.push_back(entry);
        }
        return results;
    }

    void assertTransferableAsset(const DigiAsset& asset) {
        //getIfExpired() compares against seconds; the expiry itself is stored in ms
        Database* db = AppMain::GetInstance()->getDatabase();
        uint64_t nowSeconds = chrono::duration_cast<chrono::seconds>(
                                      chrono::system_clock::now().time_since_epoch())
                                      .count();
        assertTransferableAsset(asset, db->getBlockHeight(), nowSeconds);
    }

    void assertTransferableAsset(const DigiAsset& asset, unsigned int chainHeight, uint64_t nowSeconds) {
        DigiAssetRules rules = asset.getRules();
        if (rules.empty()) return;

        //Rules that need extra outputs the transfer builder never adds.  addRuleOutputs() is
        //called from issueasset only, so there is no code path that could satisfy these.
        const string lead = "This asset has a ";
        const string tail =
                " rule.  A wallet built transfer cannot add the output that rule requires, so the "
                "transfer would fail validation and every unit in the transaction would be "
                "destroyed, including the change.  Refusing.";
        if (rules.getIfRequiresRoyalty()) throw DigiByteException(RPC_MISC_ERROR, lead + "royalty" + tail);
        if (rules.getRequiredBurn() > 0) throw DigiByteException(RPC_MISC_ERROR, lead + "required burn" + tail);
        if (rules.getRequiredSignerWeight() > 0) {
            throw DigiByteException(RPC_MISC_ERROR,
                                    "This asset has a required signer rule.  A wallet built transfer cannot "
                                    "collect the signatures that rule requires, so the transfer would fail "
                                    "validation and every unit in the transaction would be destroyed, "
                                    "including the change.  Refusing.");
        }

        //An expired asset is a different shape of the same trap: no transfer can ever pass again,
        //so building one only destroys what is left.
        if (rules.getIfExpired(chainHeight, nowSeconds)) {
            throw DigiByteException(RPC_MISC_ERROR,
                                    "This asset has expired.  No transfer of it can pass validation any more, "
                                    "and attempting one would destroy every unit in the transaction, including "
                                    "the change.  Refusing.");
        }
    }

    vector<AssetUTXO> selectAssetInputs(uint64_t assetIndex, uint64_t amount) {
        vector<AssetUTXO> candidates;
        for (const AssetUTXO& utxo: getWalletUTXOs(1)) {
            if (utxo.assets.empty()) continue;
            for (const DigiAsset& asset: utxo.assets) {
                if (asset.getAssetIndex() == assetIndex) {
                    //refuse before any input is chosen - see assertTransferableAsset()
                    assertTransferableAsset(asset);
                    candidates.push_back(utxo);
                    break;
                }
            }
        }

        //prefer utxos that hold only the wanted asset, then largest amount first so we use the
        //fewest inputs possible
        auto countOf = [assetIndex](const AssetUTXO& utxo) {
            uint64_t total = 0;
            for (const DigiAsset& asset: utxo.assets) {
                if (asset.getAssetIndex() == assetIndex) total += asset.getCount();
            }
            return total;
        };
        sort(candidates.begin(), candidates.end(), [&](const AssetUTXO& a, const AssetUTXO& b) {
            bool aPure = (a.assets.size() == 1);
            bool bPure = (b.assets.size() == 1);
            if (aPure != bPure) return aPure;
            return countOf(a) > countOf(b);
        });

        //select until we have enough
        vector<AssetUTXO> selected;
        uint64_t total = 0;
        for (const AssetUTXO& utxo: candidates) {
            if (total >= amount) break;
            selected.push_back(utxo);
            total += countOf(utxo);
        }
        if (total < amount) throw DigiByteTransaction::exceptionNotEnoughFunds();
        return selected;
    }

    uint64_t parseAssetAmount(const Json::Value& amount, uint8_t decimals) {
        const uint64_t multiplier = BitIO::pow10(decimals);

        if (amount.isIntegral()) {
            if (amount.asInt64() <= 0) throw out_of_range("Amount must be positive");
            return static_cast<uint64_t>(amount.asInt64()) * multiplier;
        }

        //parse doubles and strings as a decimal string to avoid floating point surprises
        string str = amount.isDouble() ? to_string(amount.asDouble()) : amount.asString();
        if (str.empty()) throw out_of_range("Invalid amount");

        size_t decimalPos = str.find('.');
        string whole = (decimalPos == string::npos) ? str : str.substr(0, decimalPos);
        string frac = (decimalPos == string::npos) ? "" : str.substr(decimalPos + 1);

        //trim trailing zeros from the fraction then check it fits in the asset's decimals
        while (!frac.empty() && (frac.back() == '0')) frac.pop_back();
        if (frac.length() > decimals) throw out_of_range("Amount has more decimals than the asset allows");
        while (frac.length() < decimals) frac.push_back('0');

        //make sure everything left is digits
        if (whole.empty() && frac.empty()) throw out_of_range("Invalid amount");
        for (char c: whole + frac) {
            if (!isdigit(c)) throw out_of_range("Invalid amount");
        }

        uint64_t result = frac.empty() ? 0 : stoull(frac);
        if (!whole.empty()) result += stoull(whole) * multiplier;
        if (result == 0) throw out_of_range("Amount must be positive");
        return result;
    }

    string satsToDecimal(uint64_t sats) {
        string whole = to_string(sats / 100000000);
        string frac = to_string(sats % 100000000);
        while (frac.length() < 8) frac.insert(frac.begin(), '0');
        return whole + "." + frac;
    }

    uint64_t estimateMinerFee(const DigiByteTransaction& tx) {
        uint64_t feeRate = 100000; //sats per kB fallback(the v8.22 min relay rate)
        try {
            Json::Value feeParams = Json::arrayValue;
            feeParams.append(6);
            Json::Value est = AppMain::GetInstance()->getDigiByteCore()->sendcommand("estimatesmartfee", feeParams);
            if (est.isMember("feerate") && est["feerate"].isNumeric() && (est["feerate"].asDouble() > 0)) {
                feeRate = static_cast<uint64_t>(est["feerate"].asDouble() * 100000000);
            }
        } catch (...) {} //fallback rate already set
        size_t estimatedVSize = 200 + (tx.encodeAssetOpReturn().length() / 2) +
                                (tx.getOutputCount() * 35) + (tx.getInputCount() * 70) + 150;
        return feeRate * estimatedVSize / 1000;
    }

    string fundSignSend(const DigiByteTransaction& tx, string* signedHex) {
        AppMain* main = AppMain::GetInstance();
        DigiByteCore* dgb = main->getDigiByteCore();
        Database* db = main->getDatabase();

        //build createrawtransaction params.  Outputs use the array form so their order is
        //preserved exactly(transfer instructions reference outputs by index)
        Json::Value inputs = Json::arrayValue;
        for (size_t i = 0; i < tx.getInputCount(); i++) {
            Json::Value input = Json::objectValue;
            input["txid"] = tx.getInput(i).txid;
            input["vout"] = tx.getInput(i).vout;
            inputs.append(input);
        }
        Json::Value outputs = Json::arrayValue;
        for (size_t i = 0; i < tx.getOutputCount(); i++) {
            Json::Value output = Json::objectValue;
            output[tx.getOutput(i).address] = satsToDecimal(tx.getOutput(i).digibyte);
            outputs.append(output);
        }
        Json::Value dataOutput = Json::objectValue;
        dataOutput["data"] = tx.encodeAssetOpReturn();
        outputs.append(dataOutput);

        Json::Value createParams = Json::arrayValue;
        createParams.append(inputs);
        createParams.append(outputs);
        string rawHex = dgb->sendcommand("createrawtransaction", createParams).asString();

        //a transaction broadcast moments ago can take a beat to register in the wallet's
        //unspent view; until then fundrawtransaction can select coins that tx already spent
        //and the broadcast fails with a mempool conflict.  Retry funding when that happens
        for (int attempt = 0;; attempt++) {

            //protect all wallet UTXOs that carry assets or are unconfirmed(the local database can't
            //know about unconfirmed assets yet) so fundrawtransaction can't select them for fees.
            //listunspent never returns coins that are already locked, so the unlock at the end of
            //the attempt can only ever release locks placed here - never one the operator set
            unsigned int chainHeight = dgb->getBlockCount();
            vector<txout_t> toLock;
            bool lockedUnconfirmed = false; //true if any coin we locked is still unconfirmed
            for (const unspenttxout_t& utxo: dgb->listUnspent(0)) {
                //skip utxos that are already explicit inputs
                bool isInput = false;
                for (size_t i = 0; i < tx.getInputCount(); i++) {
                    if ((tx.getInput(i).txid == utxo.txid) && (tx.getInput(i).vout == utxo.n)) {
                        isInput = true;
                        break;
                    }
                }
                if (isInput) continue;

                //height the coin was created at, or 0 while it is unconfirmed(or conflicted)
                unsigned int utxoHeight = 0;
                if ((utxo.confirmations > 0) && (static_cast<unsigned int>(utxo.confirmations) <= chainHeight)) {
                    utxoHeight = chainHeight - static_cast<unsigned int>(utxo.confirmations) + 1;
                }

                bool needsLock = (utxoHeight == 0);
                if (!needsLock) {
                    try {
                        needsLock = !db->getAssetUTXO(utxo.txid, utxo.n).assets.empty();
                    } catch (const Database::exceptionDataPruned& e) {
                        //A plain coin never gets a utxos row when storenonassetutxo=0, so "no row"
                        //is the normal answer for every fee coin under that config, not a gap in
                        //what we know.  Pruning only ever deletes rows of outputs that have been
                        //spent and these coins come from listunspent, so an unspent coin holding
                        //assets always has a row once the analyzer has passed its height - which
                        //makes a missing row proof of "no assets" for exactly these coins.  A
                        //height the analyzer has not reached yet is still unknown, so it stays
                        //locked.  Locking everything would be safe for the assets but leaves
                        //nothing to pay the fee with, which is why this can't just say true
                        needsLock = !db->isHeightIndexed(utxoHeight);
                    }
                }
                if (!needsLock) {
                    //the wallet's unspent view can lag the mempool by a few seconds after a
                    //broadcast.  gettxout is mempool aware: null means some mempool tx already
                    //spends this coin, so selecting it would guarantee a mempool conflict
                    Json::Value txoutParams = Json::arrayValue;
                    txoutParams.append(utxo.txid);
                    txoutParams.append(utxo.n);
                    txoutParams.append(true);
                    needsLock = dgb->sendcommand("gettxout", txoutParams).isNull();
                }
                if (needsLock) {
                    toLock.push_back(txout_t{utxo.txid, utxo.n});
                    if (utxo.confirmations == 0) lockedUnconfirmed = true;
                }
            }

            string fundedHex;
            if (!toLock.empty()) dgb->lockunspent(false, toLock);
            try {
                Json::Value fundOptions = Json::objectValue;
                fundOptions["changePosition"] = static_cast<Json::UInt>(outputs.size()); //append change after all outputs
                Json::Value fundParams = Json::arrayValue;
                fundParams.append(rawHex);
                fundParams.append(fundOptions);
                fundedHex = dgb->sendcommand("fundrawtransaction", fundParams)["hex"].asString();
            } catch (const DigiByteException& e) {
                if (!toLock.empty()) dgb->lockunspent(true, toLock);
                //when everything spendable is sitting in still-unconfirmed change(which we
                //lock because its asset content can't be verified yet), funding fails with
                //insufficient funds.  A confirmation fixes that, so wait out a block interval
                if ((e.getMessage().find("nsufficient") == string::npos) || !lockedUnconfirmed || (attempt >= 4)) throw;
                this_thread::sleep_for(chrono::seconds(5));
                continue;
            } catch (...) {
                if (!toLock.empty()) dgb->lockunspent(true, toLock);
                throw;
            }
            if (!toLock.empty()) dgb->lockunspent(true, toLock);

            //sign(signrawtransactionwithwallet on modern cores, signrawtransaction on old ones)
            Json::Value signParams = Json::arrayValue;
            signParams.append(fundedHex);
            Json::Value signResult;
            //-13 = RPC_WALLET_UNLOCK_NEEDED.  Surface it clearly instead of falling through
            //to the legacy sign call whose "Method not found" would mask the real problem
            auto throwIfWalletLocked = [](const DigiByteException& e) {
                if ((e.getCode() == -13) || (e.getMessage().find("walletpassphrase") != string::npos)) {
                    throw DigiByteException(RPC_MISC_ERROR, "Wallet is encrypted and locked.  Unlock it first with: walletpassphrase \"<passphrase>\" <seconds>");
                }
            };
            try {
                signResult = dgb->sendcommand("signrawtransactionwithwallet", signParams);
            } catch (const DigiByteException& e) {
                throwIfWalletLocked(e);
                try {
                    signResult = dgb->sendcommand("signrawtransaction", signParams);
                } catch (const DigiByteException& e2) {
                    throwIfWalletLocked(e2);
                    throw;
                }
            }
            if (!signResult["complete"].asBool()) {
                throw DigiByteTransaction::exception("Wallet could not fully sign the transaction.  Is the wallet unlocked?");
            }
            string hex = signResult["hex"].asString();
            if (signedHex != nullptr) *signedHex = hex;

            //make sure funding didn't displace our first input.  Asset transfer instructions
            //assume input consumption order, and reissuances derive the assetId from vin[0],
            //so a reordered input list would corrupt the asset side of the transaction
            if (tx.getInputCount() > 0) {
                decoderawtransaction_t decoded = dgb->decoderawtransaction(hex);
                if ((decoded.vin[0].txid != tx.getInput(0).txid) ||
                    (decoded.vin[0].n != tx.getInput(0).vout)) {
                    throw DigiByteTransaction::exception("Wallet changed the transaction input order while funding");
                }
            }

            //broadcast
            Json::Value sendParams = Json::arrayValue;
            sendParams.append(hex);
            try {
                return dgb->sendcommand("sendrawtransaction", sendParams).asString();
            } catch (const DigiByteException& e) {
                if ((e.getMessage().find("onflict") == string::npos) || (attempt >= 4)) throw;
                //wait out at least one block interval - a confirmation reliably refreshes the
                //wallet's view of which of its coins are already spent
                this_thread::sleep_for(chrono::seconds(5));
            }
        }
    }

} // namespace AssetWallet
