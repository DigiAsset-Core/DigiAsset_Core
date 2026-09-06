//
// Created by mctrivia on 17/06/23.
//

#include "DigiByteTransaction.h"
#include "Log.h"
#include "AppMain.h"
#include "BitIO.h"
#include "DigiAsset.h"
#include "DigiByteDomain.h"
#include "IPFS.h"
#include "PermanentStoragePool/PermanentStoragePoolList.h"
#include "DigiAssetConstants.h"
#include <chrono>
#include <cmath>
#include <iostream>

using namespace std;


/**
 * Create a new DigiAsset Object
 */
DigiByteTransaction::DigiByteTransaction() {
    _time = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
}


/**
 * Creates an object that can hold a transactions data including any DigiAssets
 * height is optional but if known will speed things up if provided
 */
DigiByteTransaction::DigiByteTransaction(const string& txid, unsigned int height, bool dontBotherIfNotSpecial) {
    AppMain* main = AppMain::GetInstance();
    DigiByteCore* dgb = main->getDigiByteCore();
    Database* db = main->getDatabase();

    getrawtransaction_t txData = dgb->getRawTransaction(txid);
    if (height == 0) height = dgb->getBlock(txData.blockhash).height;

    //store transaction key data
    _height = height;
    _blockHash = txData.blockhash;
    _time = txData.time;
    _txid = txData.txid;

    //check if we should cheat loading to save time(chain analyzer when not storing non asset utxo data
    if (dontBotherIfNotSpecial) {
        bool mayNeedInputProcessing = false;

        //check if there is an op_return
        for (const vout_t& vout: txData.vout) {
            if (vout.scriptPubKey.type != "nulldata") continue;
            mayNeedInputProcessing = true;
            break;
        }

        //large transactions can only be DigiAsset transactions or normal so check if DigiAsset Transaction if large.
        if ((mayNeedInputProcessing) && (txData.vin.size() > 5)) {
            unsigned char opcode;
            BitIO dataStream;
            DigiAsset::decodeAssetTxHeader(txData, _assetTransactionVersion, opcode, dataStream);
            if (opcode == 0) mayNeedInputProcessing = false; //not a DigiAsset transaction
        }

        //if it doesn't need input processing then
        if (!mayNeedInputProcessing) {
            // we only need to copy the txid and vout for each input so they can be cleared
            // no need to check if there is any assets on inputs or add the outputs
            for (const vin_t& vin: txData.vin) {
                //if a coinbase transaction don't look up the input
                if (vin.txid.empty()) break;

                //find any assets on input utxos
                AssetUTXO input{
                        .txid = vin.txid,
                        .vout = static_cast<uint16_t>(vin.n)};
                _inputs.push_back(input);
            }
            _txType = STANDARD;
            return;
        }
    }

    //copy input data
    _assetFound = false;
    for (const vin_t& vin: txData.vin) {
        //if a coinbase transaction don't look up the input
        if (vin.txid.empty()) {
            break;
        }

        //find any assets on input utxos
        AssetUTXO input = db->getAssetUTXO(vin.txid, vin.n, height);
        if (!input.assets.empty()) _assetFound = true;
        _inputs.push_back(input);
    }

    //copy output data
    for (const vout_t& vout: txData.vout) {
        _outputs.emplace_back(AssetUTXO{
                .txid = txData.txid,
                .vout = static_cast<uint16_t>(vout.n),
                .address = (vout.scriptPubKey.addresses.empty()) ? "" : vout.scriptPubKey.addresses[0],
                .digibyte = vout.valueS});
    }

    //find index of op_return
    int dataIndex = -1;
    for (size_t i = 0; i < txData.vout.size(); i++) {
        if (txData.vout[i].scriptPubKey.type == "nulldata") {
            dataIndex = i;
            break;
        }
    }
    if (dataIndex == -1) return;

    //Check different tx types
    if (decodeKYC(txData, dataIndex)) return;
    if (decodeExchangeRate(txData, dataIndex)) return;
    if (decodeEncryptedKeyTx(txData, dataIndex)) return;
    if (decodeAssetTX(txData, dataIndex)) return;
    storeUnknown(txData, dataIndex);

    ///if any new special case types are added make sure they have no more than 5 inputs allowed or
    ///modify the mayNeedInputProcessing algorithm above to check for them.
}

/**
 * Looks to see if there is exchange rate data encoded in the transaction and we are tracking it
 * New exchange rate addresses are automatically added to tracking if they are defined in a DigiAsset
 * @param txData
 * @return if there was exchange rate data
 */
bool DigiByteTransaction::decodeExchangeRate(const getrawtransaction_t& txData, int dataIndex) {
    //quick check op_return in correct spot
    if (dataIndex != 0) return false;

    //exchange rate transactions always have 1 input and 2 outputs
    if (txData.vin.size() != 1) return false;
    if (txData.vout.size() != 2) return false;

    //check first output is an op_return
    BitIO dataStream = BitIO::makeHexString(txData.vout[0].scriptPubKey.hex);
    if (!dataStream.checkIsBitcoinOpReturn()) return false;                         //not an OP_RETURN
    if (dataStream.getBitcoinDataHeader() != BITIO_BITCOIN_TYPE_DATA) return false; //not data
    dataStream = dataStream.copyBitcoinData();                                      //strip the header out

    //check data is a multiple of 8 bytes
    if (dataStream.getLength() % 64 != 0) return false;

    //check only 1 address in output 2, and it is an exchange address
    if (txData.vout[1].scriptPubKey.addresses.size() != 1) return false;
    Database* db = AppMain::GetInstance()->getDatabase();
    string address = txData.vout[1].scriptPubKey.addresses[0];
    if (!db->isWatchAddress(address)) return false;

    //check input utxo is from same address as output 2
    if (address != db->getSendingAddress(txData.vin[0].txid, txData.vin[0].n)) return false;

    //if we get to this line the transaction is an exchange rate transaction

    //record exchange rates
    while (dataStream.getPosition() != dataStream.getLength()) {
        _exchangeRate.emplace_back(dataStream.getDouble());
    }
    _txType = EXCHANGE_PUBLISH;

    return true;
}

/**
 * Looks to see if there is KYC data encoded in the transaction and decodes it if there is
 * @param txData
 * @return true if there is KYC data in the transaction
 */
bool DigiByteTransaction::decodeKYC(const getrawtransaction_t& txData, int dataIndex) {
    //quick check op_return in correct spot
    if (dataIndex != 1) return false;

    //process
    unsigned int txType = _kycData.processTX(txData, _height, [this](string txid, unsigned int vout) -> string {
        Database* db = AppMain::GetInstance()->getDatabase();
        return db->getSendingAddress(txid, vout);
    });
    if (txType == KYC::NA) return false;
    if (txType == KYC::VERIFY) {
        _txType = KYC_ISSUANCE;
    } else { //KYC::REVOKE
        _txType = KYC_REVOKE;
    }
    return true;
}

/**
 * Checks to see if a transaction is an Encrypted Key Tx
 * @param txData
 * @param dataIndex
 * @return
 */
bool DigiByteTransaction::decodeEncryptedKeyTx(const getrawtransaction_t& txData, int dataIndex) {
    //quick check op_return in correct spot
    if (dataIndex != 1) return false;

    //check if valid
    if (txData.vin.size() != 1) return false;
    if ((txData.vout.size() < 2) || (txData.vout.size() > 3)) return false;
    if (txData.vout[0].valueS != 650) return false;

    //store
    _txType = ENCRYPTED_KEY;
    _opReturnHex = txData.vout[dataIndex].scriptPubKey.hex;
    return true;
}

/**
 * Stores an unknown op_return data if doesn't match known structure
 * @param txData
 * @param dataIndex
 */
void DigiByteTransaction::storeUnknown(const getrawtransaction_t& txData, int dataIndex) {
    if (dataIndex == -1) return;

    _txType = STANDARD;
    _opReturnHex = txData.vout[dataIndex].scriptPubKey.hex;
}

/**
 * Looks to see if there is DigiAsset Transaction in the transaction and decodes it if there is
 * @param txData
 */
bool DigiByteTransaction::decodeAssetTX(const getrawtransaction_t& txData, int dataIndex) {
    //get the DigiAsset header and read the version and opcode
    unsigned char opcode;
    BitIO dataStream;
    DigiAsset::decodeAssetTxHeader(txData, _assetTransactionVersion, opcode, dataStream);

    if (opcode == 0) {
        return false; //invalid op code
    }
    if (opcode < 16) {

        //try to build the DigiAsset Object that is encoded in chain
        try {

            //check if any assets where burnt since assets are not transferred during an issuance
            if (_assetFound) _unintentionalBurn = true;

            //create the asset
            _newAsset = DigiAsset{txData, _height, _assetTransactionVersion, opcode, dataStream};
            decodeAssetTransfer(dataStream, vector<AssetUTXO>{{.digibyte = 0, .assets = vector<DigiAsset>{_newAsset}}}, DIGIASSET_ISSUANCE);
            _txType = DIGIASSET_ISSUANCE;
            return true;
        } catch (const DigiAsset::exception& e) {
            return false;
        }
    } else if (opcode < 48) {

        //check if valid transfer or burn
        bool burn = (opcode >= 0x20);
        if ((opcode % 16) != 5) {
            return false; //invalid transfer opcode
        }

        if (!_assetFound) {
            return false;
        }

        //do transfer
        try {
            decodeAssetTransfer(dataStream, _inputs, burn ? DIGIASSET_BURN : DIGIASSET_TRANSFER);
        } catch (const DigiAsset::exceptionRuleFailed& e) {
            //Every asset output is cleared, the change output included.  That is deliberate: if
            //change survived a violation a sender could declare everything as change and sidestep
            //the royalty entirely.  There is also no alternative - the transaction is already on
            //chain and its inputs are spent, so assets that may not move to the outputs have
            //nowhere left to exist.  See docs/asset-rules-and-burns.md.
            //
            //It used to happen with nothing written anywhere, which is why it went unnoticed.
            Log* log = Log::GetInstance();
            log->addMessage("Transaction " + _txid + " in block " + std::to_string(_height) +
                                    " broke an asset rule(" + e.what() +
                                    ").  Every asset output it carries is being destroyed, including any change.",
                            Log::WARNING);
            _unintentionalBurn = true;
            for (AssetUTXO& output: _outputs) {
                output.assets.clear();
            }
            return false;
        } catch (const DigiAsset::exception& e) {
            return false;
        } catch (const out_of_range& e) {
            return false;
        }
        _txType = burn ? DIGIASSET_BURN : DIGIASSET_TRANSFER;
        return true;
    }
    return false;
}

/**
 * Helper function to handle processing where assets are now after this transaction
 * @param dataStream - with pointer at transfer section
 * @param inputAssets - list of input assets to this transaction
 * @param type - type of transaction(see constants)
 *    1 - Issuance
 *    2 - Transfer
 *    3 - Burn
 */
void DigiByteTransaction::decodeAssetTransfer(BitIO& dataStream, const vector<AssetUTXO>& inputAssets, uint8_t type) {
    bool allowSkip = true;
    size_t index = 0;

    //get list of input assets
    vector<vector<DigiAsset>> inputs;
    for (const AssetUTXO& vin: inputAssets) {
        if (vin.assets.empty()) continue;
        inputs.emplace_back(vin.assets); //copy the assets
    }

    //read transfer instructions
    size_t footerBitCount = (type == DIGIASSET_ISSUANCE) ? 8 : 0;
    while (dataStream.getNumberOfBitLeft() > footerBitCount) {
        //read the instruction
        bool skip = dataStream.getBits(1);
        bool range = dataStream.getBits(1);
        bool percent = dataStream.getBits(1);
        uint16_t output = range ? dataStream.getBits(13) : dataStream.getBits(5);
        uint64_t amount = percent ?
                                  inputs[index][0].getCount() * (dataStream.getBits(8)+1) / 256 : //if a percentage mode amount is 1 byte value.  0xff=100%, 0x00=0.39%
                                  dataStream.getFixedPrecision();
        uint64_t totalAmount = range ? (output + 1) * amount : amount;

        //there was an error in legacy code that a 0 amount causes the input to get wasted and go to change
        if ((_assetTransactionVersion < 3) && (type != DIGIASSET_ISSUANCE) && (_inputs[0].assets.empty())) {
            break;
        }

        //remove assets from input
        try {

            //check that input has assets
            if ((index >= inputs.size()) || (inputs[index].empty())) {
                throw DigiAsset::exceptionInvalidTransfer();
            } //Request from input with no assets

            //get
            uint64_t leftToRemoveFromInputs = totalAmount;
            DigiAsset removedAsset = inputs[index][0];
            while (leftToRemoveFromInputs > 0) {
                //check that input has assets
                if ((index >= inputs.size()) || (inputs[index].empty())) {
                    throw DigiAsset::exceptionInvalidTransfer();
                } //Request from input with no assets

                //get number available
                uint64_t currentAmount = inputs[index][0].getCount();

                //check asset id matched
                if (inputs[index][0] != removedAsset) {
                    throw DigiAsset::exceptionInvalidTransfer();
                } //Different asset then expected;

                //see if we used them all up
                allowSkip = true; //make sure true for all instructions except where ends on exactly 0 leftToRemoveFromInputs
                if ((currentAmount < leftToRemoveFromInputs) && (inputs[index][0].isHybrid())) {
                    throw DigiAsset::exceptionInvalidTransfer();
                } //"Hybrid assets can't rap over inputs;
                if (currentAmount <= leftToRemoveFromInputs) {
                    //used all assets in the input up
                    leftToRemoveFromInputs -= currentAmount;
                    inputs[index].erase(inputs[index].begin());
                    if (inputs[index].empty()) {
                        //used all inputs up so move to next
                        index++;
                        allowSkip = false; //exactly 0 leftToRemoveFromInputs so disable skip
                    }
                } else {
                    //there are assets leftToRemoveFromInputs in the input
                    inputs[index][0].removeCount(leftToRemoveFromInputs);
                    leftToRemoveFromInputs = 0;
                }
            }

            //apply removed assets to outputs
            if (totalAmount > 0) {
                if ((type == DIGIASSET_BURN) && (!range) && (output == 31)) {
                    //burn asset so do nothing
                } else {
                    size_t startI = range ? 0 : output;
                    removedAsset.setCount(amount);
                    if (output >= _outputs.size()) {
                        throw DigiAsset::exceptionInvalidTransfer();
                    } //Tried to send to an output that doesn't exist
                    for (size_t vout = startI; vout <= output; vout++) {
                        addAssetToOutput(vout, removedAsset);
                    }
                }
            }

            //skip remainder of vin inputs if called for
            if (skip) {
                if (allowSkip) index++; //ignore skip if last instruction emptied the input
                allowSkip = true;
            }
        } catch (const std::exception& e) {
            //remove any assets that where already applied
            for (AssetUTXO& vout: _outputs) {
                vout.assets.clear();
            }

            //reset inputs
            inputs.clear();
            for (const AssetUTXO& vin: inputAssets) {
                if (vin.assets.empty()) continue;
                inputs.emplace_back(vin.assets); //copy the assets
            }

            break; //breaks the while (dataStream.getNumberOfBitLeft() > footerBitCount) loop
        }
    }

    //see if any change
    size_t lastOutput = _outputs.size() - 1;
    for (const vector<DigiAsset>& input: inputs) {
        for (const DigiAsset& asset: input) {
            //check there is something there(sometimes count 0)
            if (asset.getCount() == 0) {
                continue;
            }

            //something left over so see if already in list
            bool needAdding = true;
            if (asset.isAggregable()) { //only search on aggregable
                for (DigiAsset& assetTest: _outputs[lastOutput].assets) {
                    if (assetTest.getAssetIndex() == asset.getAssetIndex()) {
                        //already there so add the amount
                        assetTest.addCount(asset.getCount());
                        needAdding = false;
                        break;
                    }
                }
            }

            //not found so add
            if (needAdding) addAssetToOutput(lastOutput, asset);
        }
    }

    //check rules where followed if there were any
    if (type != DIGIASSET_ISSUANCE) {
        checkRulesPass();
    }

    //make sure no outputs with assets are op_return data
    for (AssetUTXO& output: _outputs) {
        if ((output.address == "") && (!output.assets.empty())) {
            output.assets.clear();
            _unintentionalBurn = true;
        }
    }
}

/**
 * Checks rules pass.
 * Throws exception DigiAsset::exceptionRuleFailed if they don't
 */
void DigiByteTransaction::checkRulesPass() const {
    for (const AssetUTXO& utxo: _inputs) {
        for (const DigiAsset& asset: utxo.assets) asset.checkRulesPass(_inputs, _outputs, _height, _time);
    }
}


unsigned int DigiByteTransaction::getInputCount() const {
    return _inputs.size();
}

AssetUTXO DigiByteTransaction::getInput(size_t n) const {
    return _inputs[n];
}

unsigned int DigiByteTransaction::getOutputCount() const {
    return _outputs.size();
}

AssetUTXO DigiByteTransaction::getOutput(size_t n) const {
    return _outputs[n];
}


bool DigiByteTransaction::isStandardTransaction() const {
    return (_txType == STANDARD);
}

bool DigiByteTransaction::isNonAssetTransaction() const {
    if (isUnintentionalBurn()) return false;
    return ((_txType < DIGIASSET_ISSUANCE) || (_txType > DIGIASSET_BURN));
}

bool DigiByteTransaction::isIssuance() const {
    return (_txType == DIGIASSET_ISSUANCE);
}

bool DigiByteTransaction::isTransfer(bool includeIntentionalBurn) const {
    if (_txType == DIGIASSET_TRANSFER) return true;
    if (includeIntentionalBurn && (_txType == DIGIASSET_BURN)) return true;
    return false;
}

bool DigiByteTransaction::isBurn(bool includeUnintentionalBurn) const {
    if (_txType == DIGIASSET_BURN) return true;
    if (!includeUnintentionalBurn) return false;
    return isUnintentionalBurn();
}

bool DigiByteTransaction::isUnintentionalBurn() const {
    if (_unintentionalBurn) return true;
    if (_txType != STANDARD) return false;
    return _assetFound;
}

bool DigiByteTransaction::isKYCTransaction() const {
    return ((_txType == KYC_REVOKE) || (_txType == KYC_ISSUANCE));
}

bool DigiByteTransaction::isKYCRevoke() const {
    return (_txType == KYC_REVOKE);
}

bool DigiByteTransaction::isKYCIssuance() const {
    return (_txType == KYC_ISSUANCE);
}

KYC DigiByteTransaction::getKYC() const {
    return _kycData;
}

bool DigiByteTransaction::isExchangeTransaction() const {
    return (_txType == EXCHANGE_PUBLISH);
}

size_t DigiByteTransaction::getExchangeRateCount() const {
    return _exchangeRate.size();
}

double DigiByteTransaction::getExchangeRate(uint8_t i) const {
    if (i >= _exchangeRate.size()) throw out_of_range("Non existent exchange rate");
    return _exchangeRate[i];
}

ExchangeRate DigiByteTransaction::getExchangeRateName(uint8_t i) const {
    if (i >= _exchangeRate.size()) throw out_of_range("Non existent exchange rate");
    string name;
    for (size_t offset = 0; offset < DigiAssetConstants::standardExchangeRatesCount; offset += 10) {
        if (_outputs[1].address == DigiAssetConstants::standardExchangeRates[offset].address) {
            name = DigiAssetConstants::standardExchangeRates[offset + i].name;
        }
    }
    return {
            .address = _outputs[1].address,
            .index = i,
            .name = name};
}

/**
 * Handles adding an asset to a specific output
 * @param output - index of output to add the asset to
 * @param asset - asset to be added
 */
void DigiByteTransaction::addAssetToOutput(size_t output, const DigiAsset& asset) {
    //see if asset already in output
    if (asset.isAggregable() && (!_outputs[output].assets.empty())) {
        for (DigiAsset& existingOutput: _outputs[output].assets) {
            if (existingOutput.getAssetIndex(true) == asset.getAssetIndex(true)) {
                existingOutput.addCount(asset.getCount());
                return;
            }
        }
    }

    //add asset to end of assets on the output
    _outputs[output].assets.emplace_back(asset);
}

/**
 * Adds the transaction to the database
 */
void DigiByteTransaction::addToDatabase() {
    AppMain* main = AppMain::GetInstance();
    Database* db = main->getDatabase();

    //set to process all changes at once
    db->startTransaction();

    //add special tx types
    switch (_txType) {
        case KYC_ISSUANCE: {
            db->addKYC(_kycData.getAddress(), _kycData.getCountry(), _kycData.getName(), _kycData.getHash(), _height);

            break;
        }
        case KYC_REVOKE: {
            db->revokeKYC(_kycData.getAddress(), _height);
            break;
        }
        case EXCHANGE_PUBLISH: {
            for (size_t index = 0; index < _exchangeRate.size(); index++) {
                if (isnan(_exchangeRate[index])) continue;
                db->addExchangeRate(_outputs[1].address, index, _height, _exchangeRate[index]);
            }
            break;
        }
        case DIGIASSET_ISSUANCE: {
            //Set that caches based on new issuances should be deleted
            main->getRpcCache()->newAssetIssued();

            //add to the database and get the asset index number
            bool indexAlreadySet = _newAsset.isAssetIndexSet();
            uint64_t assetIndex = db->addAsset(_newAsset);

            //see if part of a PSP and pin files for those we subscribe to
            PermanentStoragePoolList* pools = main->getPermanentStoragePoolList();
            pools->processNewMetaData(*this, assetIndex, _newAsset.getCID());

            //Handle DigiByte Domain Assets
            DigiByteDomain::processAssetIssuance(_newAsset);

            //set that assetIndex on the outputs if not set
            if (indexAlreadySet) break;
            for (AssetUTXO& vout: _outputs) {
                if (vout.assets.empty()) continue;
                for (DigiAsset& asset: vout.assets) {
                    asset.setAssetIndex(assetIndex);
                }
            }
            if (isIssuance()) _newAsset.setAssetIndex(assetIndex);
            break;
        }
        case ENCRYPTED_KEY: {
            //add op_return data to database
            db->addEncryptedKey(_outputs[0].address, Blob(_opReturnHex));
            break;
        }
        case STANDARD: {
            if (!_opReturnHex.empty()) {
                db->addUnknown(_txid, Blob(_opReturnHex));
            }
            break;
        }
    }

    //mark spent old UTXOs
    for (const AssetUTXO& vin: _inputs) {
        if (vin.txid == "") continue; //coinbase
        db->spendUTXO(vin.txid, vin.vout, _height, _txid);
    }

    //add utxos
    bool isIssuance = (_txType == DIGIASSET_ISSUANCE);
    for (const AssetUTXO& vout: _outputs) {
        db->createUTXO(vout, _height, isIssuance);
    }

    //handle votes
    for (const AssetUTXO& vout: _outputs) {
        if (vout.assets.empty()) continue;
        for (const DigiAsset& asset: vout.assets) {
            if (!asset.getRules().getIfValidVoteAddress(vout.address)) continue;
            db->addVote(vout.address, asset.getAssetIndex(), asset.getCount(), _height);
        }
    }

    //finalise changes
    db->endTransaction();
}

/**
 * If there is an asset issuance will lookup its assetIndex
 * other then chain explorer that sets this through the addToDatabase function this should always be run before using getAssetIndex on any asset in
 */
void DigiByteTransaction::lookupAssetIndexes() {
    //see if we even need to do the setting
    if (!isIssuance()) return;
    if (_newAsset.isAssetIndexSet()) return;

    //find first vout with asset
    unsigned int index;
    for (const AssetUTXO& vout: _outputs) {
        if (vout.assets.empty()) continue;
        index = vout.vout;
        break;
    }

    //lookup assetIndex from database
    _newAsset.lookupAssetIndex(_txid, index);
    uint64_t assetIndex = _newAsset.getAssetIndex();

    //set all vouts
    for (AssetUTXO& vout: _outputs) {
        if (vout.assets.empty()) continue;
        for (DigiAsset& asset: vout.assets) {
            asset.setAssetIndex(assetIndex);
        }
    }
}

/**
 * Converts DigiByteTransaction Object into JSON for outputting by API.
 *
 * @param original - A Json::Value object that may contain existing data to which this method will add.
 *
 * @return Value - Returns a Json::Value object that represents the DigiByteTransaction in JSON format.
 *                 The JSON object contains the following keys and their expected data types:
 *
 * - txid (string): The transaction ID.
 * - blockhash (string): The hash of the block containing the transaction.
 * - height (int): The height of the block containing the transaction.
 * - time (int): The timestamp of the transaction.
 * - vin (Json::Array): Array of inputs, each with the following fields:
 *   - txid (string): The transaction ID of the input.
 *   - vout (int): The output index of the input.
 *   - address (string): The address associated with the input.
 *   - valueS (unsigned int): The DigiByte value of the input in satoshis.
 *   - assets (array): An array of assets involved in the input, represented in simplified JSON format.
 *                     See DigiAsset::toJSON documentation for the format
 * - vout (Json::Array): Array of outputs, each with the following fields:
 *   - n (int): The output index.
 *   - address (string): The address associated with the output.
 *   - valueS (unsigned int): The DigiByte value of the output in satoshis.
 *   - assets (array): An array of assets involved in the output, represented in simplified JSON format.
 *                     See DigiAsset::toJSON documentation for the format
 * - issued (Json::Object, if applicable): See DigiAsset::toJSON documentation for the format.
 * - exchange (Json::Array, if applicable): Array of exchange rates, each with the following fields:
 *   - address (string): The address associated with the exchange rate.
 *   - index (unsigned int): The index of the exchange rate.
 *   - name (string if applicable): The name of the exchange rate.
 *   - rate (double): The actual exchange rate.
 * - kyc (Json::Object, if applicable): KYC information with the following fields:
 *   - address (string): The address associated with the KYC.
 *   - country (string): The country associated with the KYC(ISO 3166-1 alpha-3).
 *   - name (string, optional): The name associated with the KYC(name or hash not both).
 *   - hash (string, optional): The hash associated with the KYC(name or hash not both).
 *   - revoked (bool): Indicates whether the KYC is beign revoked
 */
Value DigiByteTransaction::toJSON(const Value& original) const {
    Json::Value result = original;
    bool addingToOriginal = !original.empty();

    // Get basic data
    result["txid"] = _txid;
    result["blockhash"] = _blockHash;
    result["height"] = _height;
    result["time"] = static_cast<Json::UInt64>(_time);

    // Add vin data
    Json::Value inputsArray(Json::arrayValue);
    for (size_t i = 0; i < _inputs.size(); i++) {
        const AssetUTXO& input = _inputs[i];
        Json::Value inputObject = addingToOriginal ? result["vin"][static_cast<Json::ArrayIndex>(i)]
                                                   : Json::objectValue; //load original or blank
        inputObject["txid"] = input.txid;
        inputObject["vout"] = input.vout;
        inputObject["address"] = input.address;
        inputObject["valueS"] = static_cast<Json::UInt64>(input.digibyte);

        Json::Value assetArray(Json::arrayValue);
        for (const DigiAsset& asset: input.assets) {
            assetArray.append(asset.toJSON(true));
        }
        inputObject["assets"] = assetArray;
        inputsArray.append(inputObject);
    }
    result["vin"] = inputsArray;

    // Add vout data
    Json::Value outputArray(Json::arrayValue);
    for (size_t i = 0; i < _outputs.size(); i++) {
        const AssetUTXO& output = _outputs[i];
        Json::Value outputObject = addingToOriginal ? result["vout"][static_cast<Json::ArrayIndex>(i)]
                                                    : Json::objectValue; //load original or blank
        outputObject["n"] = output.vout;
        outputObject["address"] = output.address;
        outputObject["valueS"] = static_cast<Json::UInt64>(output.digibyte);

        Json::Value assetArray(Json::arrayValue);
        for (const DigiAsset& asset: output.assets) {
            assetArray.append(asset.toJSON(true));
        }
        outputObject["assets"] = assetArray;
        outputArray.append(outputObject);
    }
    result["vout"] = outputArray;

    // Add issuance
    if (isIssuance()) {
        DigiAsset issuedAsset = getIssuedAsset();
        result["issued"] = issuedAsset.toJSON();
    }

    // Add exchange data
    if (isExchangeTransaction()) {
        size_t count = getExchangeRateCount();
        Json::Value exchangeArray(Json::arrayValue);
        for (size_t i = 0; i < count; ++i) {
            Json::Value exchangeObj(Json::objectValue);
            ExchangeRate rateName = getExchangeRateName(i);
            exchangeObj["address"] = rateName.address;
            exchangeObj["index"] = rateName.index;
            exchangeObj["name"] = rateName.name;
            double rate = getExchangeRate(i);
            exchangeObj["rate"] = rate;
            exchangeArray.append(exchangeObj);
        }
        result["exchange"] = exchangeArray;
    }

    // Add KYC data
    if (isKYCTransaction()) {
        KYC kyc = getKYC();
        Json::Value kycObj(Json::objectValue);
        kycObj["address"] = kyc.getAddress();
        kycObj["country"] = kyc.getCountry();
        string name = kyc.getName();
        if (!name.empty()) {
            kycObj["name"] = name;
        }
        string hash = kyc.getHash();
        if (!hash.empty()) {
            kycObj["hash"] = hash;
        }
        kycObj["revoked"] = (!kyc.valid());
        result["kyc"] = kycObj;
    }

    return result;
}

/**
 * Returns the issued asset if there is one or throws an out_of_range exception
 */
DigiAsset DigiByteTransaction::getIssuedAsset() const {
    if (!isIssuance()) throw out_of_range("Not an issuance");
    return _newAsset;
}

unsigned int DigiByteTransaction::getHeight() const {
    return _height;
}


/**
 * Returns true if this transaction is still being built and can be modified.
 * Transactions loaded from chain are never writable.
 */
bool DigiByteTransaction::isWritable() const {
    return _txid.empty();
}

/**
 * Throws an exception if the transaction is not writable
 */
void DigiByteTransaction::checkWritable() const {
    if (!isWritable()) throw exception("Transaction is already on chain and can not be modified");
}

/**
 * Adds a UTXO that will be spent by this transaction.
 * Include the asset data(see Database::getAssetUTXO) so transfer instructions can be computed.
 * Fee only inputs do not need to be added here; they can be added by the wallet's fundrawtransaction.
 */
void DigiByteTransaction::addInput(const AssetUTXO& utxo) {
    checkWritable();
    if (_txType == DIGIASSET_ISSUANCE && !utxo.assets.empty()) {
        throw exception("Asset bearing inputs can not be used in an issuance(they would be burned)");
    }
    _inputs.push_back(utxo);
    if (!utxo.assets.empty()) {
        _assetFound = true;
        if (_txType == STANDARD) _txType = DIGIASSET_TRANSFER;
    }
}

/**
 * Marks this transaction as an issuance of the provided new asset(see DigiAsset new asset constructor).
 * The new assets can then be assigned to outputs with addDigiAssetOutput.
 */
void DigiByteTransaction::setIssuance(const DigiAsset& asset) {
    checkWritable();
    if (_assetFound) throw exception("Asset bearing inputs can not be used in an issuance(they would be burned)");
    _newAsset = asset;
    _txType = DIGIASSET_ISSUANCE;
}

/**
 * Marks amounts of assets on the inputs to be destroyed by this transaction.  The burn
 * travels as a transfer instruction targeting output 31, exactly what decodeAssetTransfer
 * treats as a burn.  Anything on the inputs that is neither burned nor assigned to an
 * asset output still needs an explicit change output.
 */
void DigiByteTransaction::addAssetBurn(const std::vector<DigiAsset>& assets) {
    checkWritable();
    if (!_assetFound) throw exception("Add the asset bearing inputs before declaring burns");
    if (_txType == DIGIASSET_ISSUANCE) throw exception("Issuances can not burn assets");
    for (const DigiAsset& asset: assets) {
        if (asset.getCount() == 0) throw exception("Can not burn 0 of an asset");
        _burns.push_back(asset);
    }
    _txType = DIGIASSET_BURN;
}

/**
 * Adds the outputs required by the issued asset's rules(signer/royalty/custom vote
 * address outputs - see DigiAssetRules::getRequiredOutputs).  Must be called AFTER all
 * asset outputs(the rules bitstream references these outputs by their final vout index)
 * and BEFORE any other extra outputs like storage pool fees.  No-op when the asset has
 * no rules or no outputs are needed.
 */
void DigiByteTransaction::addRuleOutputs() {
    checkWritable();
    if (_txType != DIGIASSET_ISSUANCE) throw exception("Rule outputs only apply to issuances");
    if (_ruleOutputsAdded) throw exception("Rule outputs already added");

    std::vector<DigiAssetRules::RuleOutput> required = _newAsset.getRules().getRequiredOutputs();
    _ruleOutputsStart = _outputs.size();
    _ruleOutputsAdded = true;
    for (const DigiAssetRules::RuleOutput& output: required) {
        addDigiByteOutput(output.address, output.sats);
    }
}

/**
 * Adds a DigiByte only output to the transaction being built.
 * Warning: fee and balance checks are not done here.  They are handled when the transaction
 * is funded by the wallet(fundrawtransaction will fail if the wallet lacks funds).
 */
void DigiByteTransaction::addDigiByteOutput(const string& address, uint64_t amount) {
    checkWritable();
    if (address.empty()) throw exception("Output address can not be blank");
    _outputs.emplace_back(AssetUTXO{
            .txid = "",
            .vout = static_cast<uint16_t>(_outputs.size()),
            .address = address,
            .digibyte = amount});
}

/**
 * Adds an asset bearing output to the transaction being built.
 * The output is given the standard asset dust value of DigiByte.
 * Asset outputs must be within the first 32 outputs(protocol limit on transfer instructions).
 */
void DigiByteTransaction::addDigiAssetOutput(const string& address, const vector<DigiAsset>& assets) {
    checkWritable();
    if (address.empty()) throw exception("Output address can not be blank");
    if (assets.empty()) throw exception("Asset output must contain at least 1 asset");
    if (_outputs.size() > 31) throw exception("Asset outputs must be within the first 32 outputs");
    for (const DigiAsset& asset: assets) {
        if (asset.getCount() == 0) throw exception("Asset output can not contain 0 of an asset");
    }
    _outputs.emplace_back(AssetUTXO{
            .txid = "",
            .vout = static_cast<uint16_t>(_outputs.size()),
            .address = address,
            .digibyte = DigiAssetConstants::DIGIBYTE_DUST,
            .assets = assets});
}

/**
 * Computes the transfer instruction section of an asset OP_RETURN from the difference between
 * input assets and desired output assets.  Mirrors the consumption logic of decodeAssetTransfer:
 * instructions consume asset chunks from asset bearing inputs strictly in order, front to back.
 * One instruction is emitted per (input chunk, output) pair so instructions never span inputs
 * (which keeps the encoding legal for all aggregation types including hybrid).
 *
 * Throws if input assets and output assets don't balance exactly.  Callers must explicitly
 * assign every input asset to an output(add a change output for any left over).
 */
void DigiByteTransaction::buildTransferInstructions(BitIO& data) const {
    //flatten input asset chunks in the same order decodeAssetTransfer consumes them
    vector<vector<DigiAsset>> inputs;
    if (_txType == DIGIASSET_ISSUANCE) {
        inputs.push_back(vector<DigiAsset>{_newAsset});
    } else {
        for (const AssetUTXO& vin: _inputs) {
            if (vin.assets.empty()) continue;
            inputs.push_back(vin.assets);
        }
    }
    if (inputs.empty()) throw exception("No assets to build transfer instructions for");

    //copy of desired output assets that we can count down as we assign
    vector<vector<DigiAsset>> desired;
    desired.reserve(_outputs.size());
    for (const AssetUTXO& vout: _outputs) desired.push_back(vout.assets);

    //copy of the burn amounts we can count down the same way
    vector<DigiAsset> burns = _burns;

    //walk input chunks in consumption order and route them to outputs
    for (vector<DigiAsset>& chunkList: inputs) {
        for (DigiAsset& chunk: chunkList) {
            uint64_t remaining = chunk.getCount();
            for (size_t o = 0; (o < desired.size()) && (remaining > 0); o++) {
                for (DigiAsset& want: desired[o]) {
                    if (want.getCount() == 0) continue;
                    if (want.getAssetIndex(true) != chunk.getAssetIndex(true)) continue;
                    uint64_t take = min(remaining, want.getCount());

                    //encode instruction: skip=0, range=0, percent=0, 5 bit output, fixed precision amount
                    if (o > 31) throw exception("Asset outputs must be within the first 32 outputs");
                    if ((o == 31) && (_txType == DIGIASSET_BURN)) throw exception("Output 31 is reserved for the burn marker in burn transactions");
                    data.appendBits(0, 3);
                    data.appendBits(o, 5);
                    BitIO amount = BitIO::makeFixedPrecision(take);
                    data.appendBits(amount);

                    want.setCount(want.getCount() - take);
                    remaining -= take;
                    if (remaining == 0) break;
                }
            }

            //anything declared for burning is routed to the burn marker(output 31)
            for (DigiAsset& burn: burns) {
                if (remaining == 0) break;
                if (burn.getCount() == 0) continue;
                if (burn.getAssetIndex(true) != chunk.getAssetIndex(true)) continue;
                uint64_t take = min(remaining, burn.getCount());
                data.appendBits(0, 3);
                data.appendBits(31, 5); //burn instruction
                BitIO amount = BitIO::makeFixedPrecision(take);
                data.appendBits(amount);
                burn.setCount(burn.getCount() - take);
                remaining -= take;
            }
            if (remaining > 0) throw exception("Input assets not fully assigned to outputs.  Add an asset change output");
        }
    }

    //make sure no output wants assets that weren't available on inputs
    for (const vector<DigiAsset>& vout: desired) {
        for (const DigiAsset& want: vout) {
            if (want.getCount() > 0) throw exception("Output requests more assets than inputs provide");
        }
    }
    for (const DigiAsset& burn: burns) {
        if (burn.getCount() > 0) throw exception("Burn requests more assets than inputs provide");
    }
}

/**
 * Encodes the OP_RETURN payload for the asset transaction being built.
 * Returns the payload as a hex string suitable for the "data" output of createrawtransaction
 * (the wallet adds the OP_RETURN opcode and push bytes itself).
 *
 * Supported types:
 *   DIGIASSET_ISSUANCE - opcode 0x01(metadata, no rules) or 0x05(no metadata).  Rule encoding not yet implemented.
 *   DIGIASSET_TRANSFER - opcode 0x15
 */
string DigiByteTransaction::encodeAssetOpReturn() const {
    BitIO data;
    data.appendBits(0x4441, 16); //"DA" magic
    data.appendBits(3, 8);       //version 3

    if (_txType == DIGIASSET_ISSUANCE) {
        DigiAssetRules rules = _newAsset.getRules();
        bool hasMetaData = !_newAsset.getCID().empty();

        //opcode: 1 = metadata no rules, 5 = no metadata no rules,
        //        3 = metadata + rewritable rules, 4 = metadata + locked rules
        if (rules.empty()) {
            data.appendBits(hasMetaData ? 0x01 : 0x05, 8);
        } else {
            if (!hasMetaData) throw exception("Assets with rules require metadata(rule op codes 3 and 4 always carry a metadata hash)");
            if (!_ruleOutputsAdded) throw exception("Call addRuleOutputs() before encoding an issuance with rules");
            data.appendBits(rules.isRewritable() ? 0x03 : 0x04, 8);
        }

        //32 byte sha256 of the metadata(derived from the raw mode cid)
        if (hasMetaData) {
            BitIO hash = BitIO::makeHexString(IPFS::cidToSha256(_newAsset.getCID()));
            data.appendBits(hash);
        }

        //number of assets to create
        BitIO count = BitIO::makeFixedPrecision(_newAsset.getCount());
        data.appendBits(count);

        //rules section(only op codes 3 and 4)
        if (!rules.empty()) rules.encode(data, _ruleOutputsStart);

        //transfer instructions assigning the new assets to outputs
        buildTransferInstructions(data);

        //issuance flags byte
        data.appendBits(_newAsset.getIssuanceFlags(), 8);
    } else if ((_txType == DIGIASSET_TRANSFER) || (_txType == DIGIASSET_BURN)) {
        data.appendBits((_txType == DIGIASSET_BURN) ? 0x25 : 0x15, 8);
        buildTransferInstructions(data);
    } else {
        throw exception("Transaction is not an asset transaction");
    }

    //convert to hex(everything encoded above is byte aligned)
    data.movePositionToBeginning();
    return data.getHexString(data.getLength() / 4);
}
