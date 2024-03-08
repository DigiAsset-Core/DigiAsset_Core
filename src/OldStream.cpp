//
// Created by mctrivia on 24/02/24.
//

#include "OldStream.h"
#include "AppMain.h"
#include "Database.h"
#include "Log.h"
#include <jsonrpccpp/client.h>
#include <jsonrpccpp/client/connectors/httpclient.h>
#include <set>
#include <unordered_map>

using namespace std;

namespace OldStream {

    /**
     * Retrieves transaction data for a given transaction ID.
     * This is a clone of the getrawtransaction with verbose set to true use getrawtransaction instead.
     *
     *
     * @param txid The transaction ID to fetch data for.
     * @return Json::Value containing TxData with the following structure:
     * {
     *   "txid": string, // The transaction ID
     *   "vin": [ // An array of input objects, each Vin object contains:
     *     {
     *       "sequence": int, // The sequence number of the input
     *       "coinbase": string, // A coinbase transaction identifier (only present in coinbase transactions)
     *       "txid": string, // The transaction ID of the source transaction for this input (not present in coinbase transactions)
     *       "vout": int, // The output index in the source transaction (not present in coinbase transactions)
     *       "scriptSig": { // The scriptSig object contains:
     *         "asm": string, // Assembly notation of the script
     *         "hex": string // Hexadecimal representation of the script
     *       },
     *       "scriptPubKey": { // Optional, the scriptPubKey object related to the input, contains:
     *         "asm": string, // Assembly notation of the script
     *         "hex": string, // Hexadecimal representation of the script
     *         "reqSigs": int, // Number of required signatures
     *         "type": string, // Type of script (e.g., 'pubkeyhash')
     *         "addresses": [string] // Array of addresses involved in the transaction
     *       },
     *       "assets": [ // Optional, present if assets are involved in the input, each AssetCount object contains:
     *         {
     *           "assetId": string, // The asset ID
     *           "amount": string|BigInt, // The amount of the asset
     *           "decimals": int, // Decimals for the asset amount
     *           "cid": string, // Content identifier, present only for non-aggregable assets
     *           "rules": boolean // Rules applied to the asset, true or undefined
     *         }
     *       ]
     *     }
     *   ],
     *   "vout": [ // An array of output objects, each Vout object contains:
     *     {
     *       "value": string|BigInt, // The value of the output in satoshis or asset units
     *       "vout": int, // The index of this output in the transaction
     *       "scriptPubKey": { // The scriptPubKey object contains:
     *         "asm": string, // Assembly notation of the script
     *         "hex": string, // Hexadecimal representation of the script
     *         "reqSigs": int, // Number of required signatures
     *         "type": string, // Type of script (e.g., 'pubkeyhash')
     *         "addresses": [string] // Array of addresses involved in the transaction
     *       },
     *       "spent": int, // The height at which the output was spent (optional)
     *       "assets": [ // Optional, present if assets are involved in the output, same structure as assets in "vin"
     *       ]
     *     }
     *   ],
     *   "blockhash": string, // The hash of the block containing this transaction
     *   "height": int, // The height of the block containing this transaction
     *   "time": int // The time the transaction was included in a block
     * }
     */
    Json::Value getTxData(const std::string& txid) {
        //get what core wallet has to say
        Value params=Json::arrayValue;
        params.append(txid);
        params.append(true);
        Value rawTransactionData = AppMain::GetInstance()->getDigiByteCore()->sendcommand("getrawtransaction", params);

        //load transaction
        DigiByteTransaction tx{txid};

        //convert to a value and return
        tx.lookupAssetIndexes();
        return tx.toJSON(rawTransactionData);
    }

    /**
     * Fetches data for a DigiByte block by its hash
     *
     * @param hash The hash of the block to fetch.
     * @return Json::Value containing DigiByteBlockData with the following structure:
     * {
     *   "hash": string, // The block hash
     *   "strippedsize": int, // The size of the block excluding witness data
     *   "size": int, // The total size of the block
     *   "weight": int, // The block weight as defined in BIP 141
     *   "height": int, // The block height
     *   "version": int, // The block version
     *   "pow_algo_id": int, // The proof-of-work algorithm identifier
     *   "time": int, // The block time in seconds since the epoch
     *   "mediantime": int, // cheated and just copied time value here for backwards compatability
     *   "difficulty": Number, // The difficulty target of the block
     *   "nTx": int, // The number of transactions in the block
     *   "previousblockhash": string, // The hash of the previous block
     *   "nextblockhash": string, // The hash of the next block (optional, may not be present)
     *   "tx": [ // An array of transactions in the block, each TxData object contains:
     *     {
     *       "txid": string, // The transaction ID
     *       "vin": [ // An array of input objects, each Vin object contains:
     *         {
     *           "sequence": int,
     *           "coinbase": string, // Present only for coinbase transactions
     *           "txid": string, // The transaction ID of the source transaction for this input
     *           "vout": int, // The output index in the source transaction
     *           "scriptSig": { // The scriptSig object contains:
     *             "asm": string, // Assembly notation of the script
     *             "hex": string // Hexadecimal representation of the script
     *           },
     *           "scriptPubKey": { // The scriptPubKey object, only present if relevant, contains:
     *             "asm": string,
     *             "hex": string,
     *             "reqSigs": int, // Number of required signatures
     *             "type": string, // Type of script (e.g., 'pubkeyhash')
     *             "addresses": [string] // Array of addresses involved in the transaction
     *           },
     *           "assets": [ // Optional, present if assets are involved in the input
     *             {
     *               "assetId": string, // The asset ID
     *               "amount": string, // The amount of the asset
     *               "decimals": int, // Decimals for the asset amount
     *               "cid": string, // Content identifier, present only for non-aggregable assets
     *               "rules": boolean // Rules applied to the asset, true or undefined
     *             }
     *           ]
     *         }
     *       ],
     *       "vout": [ // An array of output objects, each Vout object contains:
     *         {
     *           "value": string, // The value of the output in satoshis
     *           "vout": int, // The index of this output in the transaction
     *           "scriptPubKey": { // The scriptPubKey object contains:
     *             "asm": string,
     *             "hex": string,
     *             "reqSigs": int,
     *             "type": string,
     *             "addresses": [string]
     *           },
     *           "spent": int, // The height at which the output was spent (optional)
     *           "assets": [ // Optional, present if assets are involved in the output
     *             // Same structure as assets in "vin"
     *           ]
     *         }
     *       ],
     *       "blockhash": string, // The hash of the block containing this transaction
     *       "height": int, // The height of the block containing this transaction
     *       "time": int // The time the transaction was included in a block
     *     }
     *   ]
     * }
     */
    Json::Value getDigiByteBlockData(const string& hash) {
        //get the data core wallet has
        DigiByteCore* dgb=AppMain::GetInstance()->getDigiByteCore();
        blockinfo_t coreData=dgb->getBlock(hash);

        // Fill in the block information
        Json::Value result=Json::objectValue;
        result["hash"] = coreData.hash;
        result["strippedsize"] = coreData.strippedsize;
        result["size"] = coreData.size;
        result["weight"] = coreData.weight;
        result["height"] = coreData.height;
        result["version"] = coreData.version;
        result["pow_algo_id"] = static_cast<int>(coreData.algo);
        result["time"] = coreData.time;
        result["mediantime"] = coreData.time;
        result["difficulty"] = coreData.difficulty;
        result["nTx"] = static_cast<int>(coreData.tx.size()); // Number of transactions
        result["previousblockhash"] = coreData.previousblockhash;
        if (!coreData.nextblockhash.empty()) { // Check if nextblockhash is available
            result["nextblockhash"] = coreData.nextblockhash;
        }
        // Transactions array
        Json::Value txsArray = Json::arrayValue;
        for (const std::string& txid : coreData.tx) {
            Json::Value txData = getTxData(txid);
            txsArray.append(txData);
        }
        result["tx"] = txsArray;

        // Return the fully constructed Json::Value
        return result;
    }

    /**
     * Fetches asset data for a given DigiAsset ID.
     *
     * @param assetId The DigiAsset ID to fetch data for.
     * @return Json::Value containing AssetData with the following structure:
     * {
     *   "assetId": string, // The asset ID
     *   "issuer": string, // The address of the issuer
     *   "locked": boolean, // Whether the asset is locked
     *   "aggregation": string, // Aggregation policy ('aggregatable', 'hybrid', 'dispersed')
     *   "divisibility": int, // The divisibility of the asset
     *   "holders": { // A map of addresses to their respective balances
     *     "address": string|BigInt, // The balance of the asset held by the address
     *     ...divisibility
     *   },
     *   "supply": { // Information about the asset's supply
     *     "initial": string|BigInt, // The initial supply of the asset
     *     "current": string|BigInt, // The current supply of the asset
     *   },
     *   "metadata": [ // An array of metadata objects
     *     {
     *       "txid": string, // The transaction ID where the metadata was recorded
     *       "cid": string, // The content identifier for the metadata
     *     },
     *     ...
     *   ],
     *   "rules": [ // Optional, an array of AssetRules objects if any rules are defined
     *     {
     *       "rewritable": boolean, // Whether the rules are rewritable
     *       "effective": int, // The block height at which the rules became effective
     *       "signers": { // Optional, information about signers if present
     *         "required": int, // The number of required signers
     *         "list": { // A map of signer addresses to their respective weights
     *           "address": int,
     *           ...
     *         },
     *       },
     *       "royalties": { // Optional, a map of addresses to their respective royalty percentages
     *         "address": int|BigInt,
     *         ...
     *       },
     *       "kyc": { // Optional, KYC requirements
     *         "allow": [string], // Allowed countries or addresses
     *         "ban": [string], // Banned countries or addresses
     *       }|boolean, // True if KYC is required, false or undefined otherwise
     *       "vote": { // Optional, information about voting
     *         "options": [ // Voting options
     *           {
     *             "label": string, // The label of the option
     *             "address": string, // The address associated with the option
     *           },
     *           ...
     *         ],
     *         "movable": boolean, // Whether the vote is movable
     *         "cutoff": int|BigInt, // The cutoff block height for the vote
     *       },
     *       "currency": { // Optional, the currency used for transactions
     *         "address": string, // The address of the currency
     *         "index": int, // The index of the currency
     *         "name": string, // The name of the currency
     *       },
     *       "expires": int|BigInt, // Optional, the block height at which the asset expires
     *       "deflate": int|BigInt, // Optional, the deflation rate of the asset
     *     },
     *     ...
     *   ],
     *   "txs": [ // An array of AssetTxRecord objects representing asset transactions
     *     {
     *       "time": int, // The transaction time
     *       "height": int, // The block height of the transaction
     *       "txid": string, // The transaction ID
     *       "type": string, // The type of transaction
     *       "input": { // A map of input assets
     *         "address": string|BigInt,
     *         ...
     *       },
     *       "output": { // A map of output assets
     *         "address": string|BigInt,
     *         ...
     *       },
     *     },
     *     ...
     *   ],
     *   "votes": [ // Optional, an array of vote records if any votes are associated with the asset
     *     {
     *       "label": string, // The label of the vote
     *       "address": string, // The address associated with the vote
     *       "count": int, // The number of votes
     *     },
     *     ...
     *   ],
     *   "firstUsed": int, // The block height at which the asset was first used
     *   "lastUsed": int, // The block height at which the asset was last used
     *   "expired": boolean, // Whether the asset is expired
     *   "kyc": { // Optional, KYC state of the asset
     *     "country": string, // The country code
     *     "name": string, // The name of the individual or entity
     *     "hash": string, // A hash representing the individual or entity
     *     "revoked": int, // The block height at which the KYC status was revoked (optional)
     *   }
     * }
     */
    Json::Value getAssetData(const std::string& assetId) {
        Json::Value results=Json::objectValue;
        results["expired"]=false;   //will set true later if expired

        //get list of assetIndexes this could be
        Database* db = AppMain::GetInstance()->getDatabase();
        vector<uint64_t> assetIndexes = db->getAssetIndexes(assetId);

        //get the data that is common to all assets
        DigiAsset asset=db->getAsset(assetIndexes[0]);
        Json::Value json = asset.toJSON(false);

        //handle basic info
        results["assetId"]=json["assetId"];
        results["issuer"]=json["issuer"]["address"];
        results["locked"]=(assetId[0]=='L');
        switch (assetId[1]) {
            case 'a':
                results["aggregation"]="aggregatable";
                break;
            case 'h':
                results["aggregation"]="hybrid";
                break;
            case 'd':
                results["aggregation"]="dispersed";
                break;
        }
        results["divisibility"]=json["decimals"];

        //handle rules
        if (json.isMember("rules")) {
            Json::Value refRules=json["rules"];

            Json::Value rules=Json::objectValue;
            rules["rewritable"]=refRules["changeable"];
            //rules["effective"]: int, // The block height at which the rules became effective
            if (refRules.isMember("approval")) {
                Json::Value signers=Json::objectValue;
                signers["required"]=refRules["approval"]["required"];
                signers["list"]=refRules["approval"]["approvers"];
                rules["signers"]=signers;
            }
            if (refRules.isMember("royalty")) {
                rules["royalties"]=refRules["royalty"]["addresses"];
                if (refRules["royalty"].isMember("units")) {
                    rules["currency"]=refRules["royalty"]["units"];
                }
            }
            if (refRules.isMember("geofence")) {
                if (refRules["geofence"].isMember("denied")) {
                    if (refRules["geofence"]["denied"].empty()) {
                        rules["kyc"]=true;
                    } else {
                        rules["kyc"] = Json::objectValue;
                        rules["kyc"]["ban"] = refRules["geofence"]["denied"];
                    }
                }
                if (refRules["geofence"].isMember("allowed")) {
                    rules["kyc"]=Json::objectValue;
                    rules["kyc"]["allow"]=refRules["geofence"]["allowed"];
                }
            }
            if (refRules.isMember("voting")) {
                Json::Value vote=Json::objectValue;
                vote["movable"]=refRules["voting"]["restricted"];

                //convert vote options
                Json::Value optionsArray = Json::arrayValue;
                const Json::Value& options = refRules["voting"]["options"];
                for (Json::ValueConstIterator it = options.begin(); it != options.end(); ++it) {
                    Json::Value option = Json::objectValue;
                    option["label"] = it.key().asString(); // Assuming the key itself is the label
                    option["address"] = it->asString(); // The value is the address
                    optionsArray.append(option);
                }
                vote["options"] = optionsArray;

                //check if expires
                if (refRules.isMember("expiry")) {
                    vote["cutoff"]=refRules["expiry"];
                }
                rules["vote"]=vote;
            } else {
                if (refRules.isMember("expiry")) {
                    rules["expires"]=refRules["expiry"];
                }
            }
            if (refRules.isMember("deflation")) {
                rules["deflate"]=refRules["deflation"];
            }
            results["rules"]=rules;

            //check if expired
            if (refRules.isMember("expiry")) {
                unsigned int heightExpires=refRules["expiry"].asUInt();
                unsigned int heightSynced=AppMain::GetInstance()->getChainAnalyzer()->getSyncHeight();
                if (heightExpires<heightSynced) results["expired"]=true;
            }
        }

        //handle txs
        results["txs"]=Json::arrayValue;
        vector<string> txids=db->getAssetTxHistory(assetId);
        for (const string& txid: txids) {
            Json::Value entry=Json::objectValue;
            DigiByteTransaction tx(txid);
            entry["time"]=0;//cheat probably doesn't matter so not bothering  value 0 is obviously wrong
            entry["height"]=tx.getHeight();
            entry["txid"]=txid;
            string type="Transfer";
            if (tx.isBurn(false)) type="Burn";
            if (tx.isUnintentionalBurn()) type="Accidental Burn";
            if (tx.isIssuance()) type="Issuance";
            entry["type"]=type;

            //get inputs
            Json::Value inputs=Json::objectValue;
            for (size_t i=0;i<tx.getInputCount();i++) {
                auto input=tx.getInput(i);
                uint64_t count=0;
                for (const auto& assetInput: input.assets) {
                    if (assetId==assetInput.getAssetId()) {
                        count+=assetInput.getCount();
                    }
                }
                if (count==0) continue;
                if (!inputs.isMember(input.address)) inputs[input.address]=0;
                inputs[input.address]=inputs[input.address].asUInt64()+count;
            }
            entry["input"]=inputs;

            //get outputs
            Json::Value outputs=Json::objectValue;
            for (size_t i=0;i<tx.getOutputCount();i++) {
                auto output =tx.getOutput(i);
                uint64_t count=0;
                for (const auto& assetOutput: output.assets) {
                    if (assetId==assetOutput.getAssetId()) {
                        count+=assetOutput.getCount();
                    }
                }
                if (count==0) continue;
                if (!outputs.isMember(output.address)) outputs[output.address]=0;
                outputs[output.address]=outputs[output.address].asUInt64()+count;
            }
            entry["output"]=outputs;

            results["txs"].append(entry);
        }

        //votes
        map<string,uint64_t> voteMap;
        for (uint64_t assetIndex : assetIndexes) {
            auto votesData = db->getVoteCounts(assetIndex);
            for (const auto& vote: votesData) {
                voteMap[vote.address]+=vote.count;
            }
        }
        if (!voteMap.empty()) {
            Json::Value votes=Json::arrayValue;
            for (const auto& entry : voteMap) {
                Json::Value voteObject(Json::objectValue); // Create a JSON object for each map entry
                voteObject["label"] = "NA"; //hack
                voteObject["address"] = entry.first;  // The map key becomes "label"
                voteObject["count"] = Json::Value::UInt64(entry.second); // The map value becomes "count", ensure to cast to UInt64 if necessary

                votes.append(voteObject); // Append the object to the array
            }
            results["votes"]=votes;
        }

        //handle first and last
        results["firstUsed"]=asset.getHeightCreated();
        results["lastUsed"]=asset.getHeightUpdated();   //cheat not correct value but using it

        //handle kyc
        if (json["issuer"].isMember("country")) {
            Json::Value kyc=Json::objectValue;
            kyc["country"]=json["issuer"]["country"];
            if (json["issuer"].isMember("name")) {
                kyc["name"]=json["issuer"]["name"];
            }
            if (json["issuer"].isMember("hash")) {
                kyc["hash"]=json["issuer"]["hash"];
            }
            results["kyc"]=kyc;
        }

        //handle holders
        unordered_map<std::string, uint64_t> aggregatedHoldings;
        uint64_t totalSupply = 0;
        uint64_t originalSupply = 0;
        for (uint64_t assetIndex : assetIndexes) {
            originalSupply += db->getOriginalAssetCount(assetIndex);
            std::vector<AssetHolder> holders = db->getAssetHolders(assetIndex);
            for (const AssetHolder& holder : holders) {
                aggregatedHoldings[holder.address] += holder.count;
                totalSupply += holder.count;
            }
        }
        Json::Value holdersJson = Json::objectValue;
        for (const auto& pair : aggregatedHoldings) {
            holdersJson[pair.first] = Json::Value(static_cast<Json::UInt64>(pair.second));
        }
        results["holders"] = holdersJson;

        //handle supply
        results["supply"] = Json::objectValue;
        results["supply"]["current"] = Json::Value(static_cast<Json::UInt64>(totalSupply));
        results["supply"]["initial"] = Json::Value(static_cast<Json::UInt64>(originalSupply));

        //handle metadata
        Json::Value metadata=Json::arrayValue;
        for (uint64_t assetIndex : assetIndexes) {
            Json::Value entry=Json::objectValue;
            entry["txid"]="0000000000000000000000000000000000000000000000000000000000000000";//hack obviously fake
            entry["cid"]=db->getAsset(assetIndex).getCID();
            metadata.append(entry);
        }
        results["metadata"]=metadata;

        return results;
    }

    /**
     * Retrieves the current blockchain height.
     *
     * @return Json::Value containing an integer representing the current sync height.
     */
    Json::Value getHeight() {
        return AppMain::GetInstance()->getChainAnalyzer()->getSyncHeight();
    }

    /**
     * Fetches UTXO (Unspent Transaction Outputs) data for a given address.
     * warning will only return non asset utxo if the address is part of your wallet or non asset utxo is being stored.
     *
     * @param address The address to fetch UTXO data for.
     * @return Json::Value containing an array of UTXO objects for the specified address, with each object structured as follows:
     * [
     *   {
     *     "txid": string, // The transaction ID where this UTXO was created
     *     "vout": int, // The output index in the transaction, identifying this specific UTXO
     *     "value": string|BigInt, // The amount of satoshis or asset units this UTXO holds
     *     "scriptPubKey": { // The scriptPubKey object associated with this UTXO, containing:
     *       "asm": string, // Assembly notation of the script
     *       "hex": string, // Hexadecimal representation of the script
     *       "reqSigs": int, // Number of required signatures to spend this UTXO
     *       "type": string, // Type of script (e.g., 'pubkeyhash')
     *       "addresses": [string] // Array of addresses involved in this UTXO
     *     },
     *     "assets": [ // Optional, present if assets are involved in this UTXO, each AssetCount object contains:
     *       {
     *         "assetId": string, // The asset ID
     *         "amount": string|BigInt, // The amount of the asset
     *         "decimals": int, // Decimals for the asset amount
     *         "cid": string, // Content identifier, present only for non-aggregable assets
     *         "rules": boolean // Rules applied to the asset, true or undefined
     *       }
     *     ]
     *   },
     *   ...
     * ]
     *
     * This function returns detailed information about each UTXO held by the specified address, including any associated assets.
     */
    Json::Value getAddressUtxoData(const std::string& address) {
        //get unspent utxo on the current address
        Database* db=AppMain::GetInstance()->getDatabase();
        DigiByteCore* dgb=AppMain::GetInstance()->getDigiByteCore();
        auto utxos=db->getAddressUTXOs(address);

        //convert to old format
        Json::Value results=Json::arrayValue;
        for (const auto& utxo: utxos) {
            //copy basic info
            Json::Value entry=Json::objectValue;
            entry["txid"]=utxo.txid;
            entry["vout"]=utxo.vout;
            entry["value"]=utxo.digibyte;
            entry["scriptPubKey"]=Json::objectValue;
            entry["scriptPubKey"]["hex"]=dgb->getAddressInfo(address).scriptPubKey;
            entry["scriptPubKey"]["asm"]="";//hack
            entry["scriptPubKey"]["reqSigs"]=1;//hack
            entry["scriptPubKey"]["type"]="pubkeyhash";//hack
            entry["scriptPubKey"]["addresses"]=Json::arrayValue;
            entry["scriptPubKey"]["addresses"].append(Json::Value(address));

            //copy assets
            Json::Value assets=Json::arrayValue;
            for (const auto& asset: utxo.assets) {
                Json::Value assetEntry=Json::objectValue;
                assetEntry["assetId"]=asset.getAssetId();
                assetEntry["amount"]=asset.getCount();
                assetEntry["decimals"]=asset.getDecimals();
                if (!asset.isAggregable()) {
                    assetEntry["cid"]=asset.getCID();
                }
                assetEntry["rules"]=!asset.getRules().empty();
                assets.append(assetEntry);
            }
            if (!assets.empty()) {
                entry["assets"]=assets;
            }

            results.append(entry);
        }
        return results;
    }

    /**
     * Fetches comprehensive data associated with a given address.
     * Warning txs will only list DigiAsset transactions if wallet isn't storing non asset utxos
     *
     * @param address The address to fetch data for.
     * @return Json::Value containing AddressData with the following structure:
     * {
     *   "address": string, // The queried address
     *   "index": int, // An index number associated with the address (if applicable)
     *   "group": int, // Group ID the address belongs to (if applicable)
     *   "firstUsed": int, // The block height at which the address was first used
     *   "lastUsed": int, // The block height at which the address was last used
     *   "txs": [ // An array of AddressTxRecord objects, each containing:
     *     {
     *       "assetId": string, // The asset ID involved in the transaction (optional)
     *       "time": int, // The timestamp of the transaction
     *       "height": int, // The block height of the transaction
     *       "txid": string, // The transaction ID
     *       "change": string|BigInt, // The change in balance from the transaction (can be negative)
     *       "balance": string|BigInt, // The new balance of the address after the transaction
     *     },
     *     ...
     *   ],
     *   "deposit": { // Aggregate deposit information, structured as DepositWithdraw:
     *     "min": string|BigInt, // The minimum deposit amount
     *     "max": string|BigInt, // The maximum deposit amount
     *     "sum": string|BigInt, // The total sum of deposits
     *     "count": int, // The count of deposit transactions
     *   },
     *   "withdraw": { // Optional, aggregate withdrawal information, similar structure as deposit
     *     "min": string|BigInt,
     *     "max": string|BigInt,
     *     "sum": string|BigInt,
     *     "count": int,
     *   },
     *   "assets": { // A map of asset IDs to their respective balances held by the address
     *     "assetId": string|BigInt,
     *     ...
     *   },
     *   "kyc": { // Optional, KYC state of the address, structured as KycState:
     *     "country": string, // The country code
     *     "name": string, // The name of the individual or entity (optional)
     *     "hash": string, // A hash representing the individual or entity (optional)
     *     "revoked": int, // The block height at which the KYC status was revoked (optional)
     *   },
     *   "issuance": [ // Optional, an array of asset IDs issued by the address
     *     string,
     *     ...
     *   ]
     * }
     *
     * This function returns a comprehensive overview of the address, including transaction records, balance changes, asset holdings, KYC information, and any assets issued by the address.
     */
    Json::Value getAddressData(const std::string& address) {
        Database* db=AppMain::GetInstance()->getDatabase();

        map<unsigned int,int64_t> balances; //todo should be uint64_t but needs to be int64_t until database fixed
        map<unsigned int,string> assetIds;
        bool processDigiByte=!db->getBeenPrunedNonAssetUTXOHistory();

        Json::Value result=Json::objectValue;
        result["address"]=address;
        result["index"]=0;  //not tracked
        result["group"]=0;  //not tracked
        result["firstUsed"]=0;  //hack
        result["lastUsed"]=0;   //hack
        Json::Value txs=Json::arrayValue;
        vector<string> txidList=db->getAddressTxList(address);
        for (const auto& txid: txidList) {
            map<unsigned int,int64_t> changes;

            //load the transaction in question
            DigiByteTransaction tx(txid);

            //process inputs
            for (size_t i=0;i<tx.getInputCount();i++) {
                AssetUTXO input=tx.getInput(i);         //get input info
                if (input.address!=address) continue;      //skip if not related to this address

                //handle digibyte
                if (processDigiByte) {
                    balances[1]-=input.digibyte;
                    changes[1]-=static_cast<int64_t>(input.digibyte);
                }
                for (const auto& asset: input.assets) {
                    unsigned int assetIndex=asset.getAssetIndex();
                    balances[assetIndex]-=asset.getCount();
                    changes[assetIndex]-=static_cast<int64_t>(asset.getCount());
                    assetIds[assetIndex]=asset.getAssetId();
                }
            }

            //process outputs
            for (size_t i=0;i<tx.getOutputCount();i++) {
                AssetUTXO output =tx.getOutput(i);         //get output info
                if (output.address!=address) continue;      //skip if not related to this address

                //handle digibyte
                if (processDigiByte) {
                    balances[1]+= output.digibyte;
                    changes[1]+= static_cast<int64_t>(output.digibyte);
                }
                for (const auto& asset: output.assets) {
                    unsigned int assetIndex=asset.getAssetIndex();
                    balances[assetIndex]+=asset.getCount();
                    changes[assetIndex]+=static_cast<int64_t>(asset.getCount());
                    assetIds[assetIndex]=asset.getAssetId();
                }
            }

            //convert results to desired format
            for (const auto& change : changes) {
                unsigned int assetIndex = change.first;
                int64_t changeAmount = change.second;

                Json::Value transaction;
                if (assetIndex!=1) transaction["assetId"] = assetIds[assetIndex];
                transaction["time"] = Json::Value::Int64(0); // Placeholder for timestamp, replace 0 with actual timestamp if available
                transaction["height"] = tx.getHeight();
                transaction["txid"] = txid;
                transaction["change"] = Json::Value::Int64(changeAmount);
                transaction["balance"] = Json::Value::UInt64(balances[assetIndex]);
                txs.append(transaction);
            }
        }
        result["txs"]=txs;

        //hack to create withdraw and withdraw that shows the balance we know but doesn't bother with real numbers
        Json::Value withdraw =Json::objectValue;
        withdraw["min"]=0;
        withdraw["max"]=0;
        withdraw["sum"]=0;
        withdraw["count"]=0;
        result["withdraw"]= withdraw;
        Json::Value deposit=Json::objectValue;
        deposit["min"]=0;
        deposit["max"]=balances[1];
        deposit["sum"]=balances[1];
        deposit["count"]=1;
        result["deposit"]=deposit;

        //create the assets entry
        Json::Value assets=Json::objectValue;
        for (const auto& pair : assetIds) {
            unsigned int assetIndex = pair.first;
            const string& assetId = pair.second;
            if (assetIndex==1) continue;    //skip DigiByte
            uint64_t balance = balances[assetIndex];
            if (!assets.isMember(assetId)) assets[assetId]=0;
            assets[assetId] = Json::Value::UInt64(assets[assetId].asUInt64() + balance);
        }
        result["assets"]=assets;

        //get kyc data
        KYC kycData=db->getAddressKYC(address);
        if (!kycData.empty()) {
            Json::Value kyc=Json::objectValue;
            kyc["country"]=kycData.getCountry();
            string hash=kycData.getHash();
            if (hash.empty()) {
                kyc["name"]=kycData.getName();
            } else {
                kyc["hash"]=hash;
            }
            if (!kycData.valid()) {
                kyc["revoked"]=kycData.getHeightRevoked();
            }
            result["kyc"]=kyc;
        }

        //get issuance
        Json::Value issuance=Json::arrayValue;
        set<string> uniqueAssetIds;
        vector<uint64_t> createdAssets=db->getAssetsCreatedByAddress(address);
        for (uint64_t assetIndex: createdAssets) {
            string assetId=db->getAsset(assetIndex).getAssetId();
            if (uniqueAssetIds.insert(assetId).second) {    //make sure unique
                issuance.append(assetId);
            }
        }
        if (!issuance.empty()) result["issuance"]=issuance;

        return result;
    }






    Json::Value getKey(unsigned int key) {
        string hash=AppMain::GetInstance()->getDigiByteCore()->getBlockHash(key);
        return getDigiByteBlockData(hash);
    }
    Json::Value getKey(const string& key) {
        if (key.empty()) return Json::objectValue;  //just return empty for empty key

        //check if key is a hash
        if (key.length()==64) {
            //probably a sha256
            unsigned int height;
            try {
                //check if it is a block hash
                Json::Value result=getDigiByteBlockData(key);
                return result;
            } catch (...) {
                //not a block hash so assume it is a txid
                return getTxData(key);
            }
        }

        //check if key is a DigiAsset
        if (key[0]=='U' || key[0]=='L') {
            return getAssetData(key);
        }

        //check if key is specialty command
        if (key=="height") {
            return getHeight();
        }

        //check if key is looking for utxo data
        const std::string suffix = "_utxos";
        if ((key.length() > 6) && (0 == key.compare(key.length() - suffix.length(), suffix.length(), suffix))) {
            return getAddressUtxoData(key.substr(0,key.length()-6));
        }

        //check if unsupported keys
        if (key.substr(0,6)=="index_") throw exception();
        if (key.substr(0,5)=="data_") throw exception();

        //assume address if left over
        return getAddressData(key);
    }
}