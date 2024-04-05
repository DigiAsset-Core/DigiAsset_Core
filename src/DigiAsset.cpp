//
// Created by mctrivia on 07/06/23.
//

#include "DigiAsset.h"
#include "AppMain.h"
#include "Base58.h"
#include "BitIO.h"
#include "Blob.h"
#include "IPFS.h"
#include "KYC.h"
#include "PermanentStoragePool/PermanentStoragePoolList.h"
#include "crypto/ripemd.h"
#include "crypto/SHA256.h"

using namespace std;




/*
 ██████╗ ██████╗ ███╗   ██╗███████╗████████╗ █████╗ ███╗   ██╗████████╗███████╗
██╔════╝██╔═══██╗████╗  ██║██╔════╝╚══██╔══╝██╔══██╗████╗  ██║╚══██╔══╝██╔════╝
██║     ██║   ██║██╔██╗ ██║███████╗   ██║   ███████║██╔██╗ ██║   ██║   ███████╗
██║     ██║   ██║██║╚██╗██║╚════██║   ██║   ██╔══██║██║╚██╗██║   ██║   ╚════██║
╚██████╗╚██████╔╝██║ ╚████║███████║   ██║   ██║  ██║██║ ╚████║   ██║   ███████║
 ╚═════╝ ╚═════╝ ╚═╝  ╚═══╝╚══════╝   ╚═╝   ╚═╝  ╚═╝╚═╝  ╚═══╝   ╚═╝   ╚══════╝
 */

/**
 * List of addresses that can be used for voting without hard encoding the addresses you wish to use in to the asset issuance.
 * You don't need to use these but it makes your asset issuance much smaller if you do and garbage collection is automatically
 * run on these addresses to keep the chain UTXO list small.
 */
const string DigiAsset::standardVoteAddresses[] = {"D8LWk1fGksGDxZai17A5wQUVsRiV69Nk7J",
                                                   "DBJNvWeirccgeAdZn9gV5otheutdthzWxx",
                                                   "D9zaWjGHuVNB32G7Pf5BMmtvDifdoS3Wsq",
                                                   "DEKQEMFHTc1M8Gs4xvY6paZ5RKtE1cbqNp",
                                                   "D8jnQigMYwhrB6Zjs73deF5RKprUdX5uvd",
                                                   "DELKWiuSj86pMfDb7aDaAUhLYG8D7H6JVj",
                                                   "DHUg85Pbc6mDK3y7kaWmsqjRaWfRVGys2U",
                                                   "D9rxXUhaDxku4ZhdkLtyZzmmGG5ViUAYds",
                                                   "DNehqnpzLWnv7vTTxkbHsneajBEzGjvLo2",
                                                   "DAKiRnvVCfD4imp5A41tCeoZkezzkPXB4C",
                                                   "D69jwFMuawBkG1hii1muQESFrbZFenZrmL",
                                                   "D7PJqwFSLmCURDNnd5cc9Ham856GwDQ9zy",
                                                   "DFHa9HQ9BDHuDKmBPvPzBE5dsLm85prUd4",
                                                   "DAJMr7m4ZyaCRa9Y1o8pMaAPViBhSZTENs",
                                                   "DFXqwRzai3Khd3n1uRaYgZTq1BhAUhyu3m",
                                                   "DSTKiCYQqpvrXME3rEFeYEsH3dZHCPU8ez",
                                                   "DG1rJMg6zCMoiptWeEozxpuVWKGmZkiHTf",
                                                   "DRgWqHV6d7HSxYhA5bCMvtLhuS3kbRYKo3",
                                                   "DD9kssWTzT8s5fv4Xg3MthRNCT7RtawQSw",
                                                   "DLLYN7hv535nXzpvZv25ySG8GdsfYNk1Bx",
                                                   "DJQEaiT39GyJgCJK7noarscutoeWHXMLaM",
                                                   "DEkaR4NfvWx3bq1MBw2nTcTP2JEPxKyaBX",
                                                   "DN9vVGNYzbjqTGRRGXkjiGVTpuKRz1eYe3",
                                                   "D6kCF8PDhwdPzSg3xeUmrDzVK9eK3nuEJj",
                                                   "DR3F3WE78aJmHvyGA35NjkLLf5F9X8eKaz",
                                                   "DSMdwgWYbEpPNQJ2Hs9Y89JqNmQdiwhWaq",
                                                   "DGJCxLgqW2sbhomNZvsDGsjam8pnY2b7uA",
                                                   "DAaQuGSbvRQA2B7zzbrQ3SRRYC9qaQVZch",
                                                   "DJbcjvGf7wzQaAQQ9GpHP1menk6jHyCsW4",
                                                   "DC5vxafEZQeqpqDawTyDx7nBW81V6LfrE5",
                                                   "DKJhzwe5PzFQuUdrzJR9gUfkvk4jzvUrZU",
                                                   "DRX7r83LHBf1cWKnBoAd8q1iN6UP833kPx",
                                                   "DShw4ZaRmW9fyWnP3umEyZ9KyJHWR9v6BD",
                                                   "DK9zdFCv9yz3C7jVbnTbVCZgGAd8S1Xqxk",
                                                   "DG2Gv2aZRALtMkKtaJEQr75bFqsL3JbKmB",
                                                   "D9zt7Xb1RgBepPrrYSRPv6N6YCUcS5CRx6",
                                                   "DAgRoBgYaDx7g6JPRZyufvFQAQc4zdjaP1",
                                                   "DPLi14JkyEjkbWQMQGavBARN8xo4avmuMh",
                                                   "DU4mqG99gi77BcZoS8FaJEiHk5HRYXNEcb",
                                                   "D6QdquB54saxViwAfL9xKiwoXaFo7UU5ec",
                                                   "DStTMUY2U1XSLsdq9uuWgfQefPDkQJkGQC",
                                                   "DPUV7Htc7jBwhc9z5rqoDFmKW3y1fE8xwA",
                                                   "DEGatQLqYCD9BumaXAqTFRCYZ4vznhQXcY",
                                                   "DCL7fkgzSSQSLvDMXqiRdnR9qQx5MjK89t",
                                                   "DNxc93Q2rrCm92sVyrtKhCn5MMC5YtuXxK",
                                                   "DDtWWXHe9a4EPn2EawiTDzYjq8SEKKRS6J",
                                                   "DNr54LSpN6iAQda1QYqykqeU7j7TyLeCcA",
                                                   "DE6eJePsjMDrTdKoi8HAGbX6Sdwh4RGTP9",
                                                   "D5kY1eMcDfLZWznQFSjCQMUW8DiSoxhmuy",
                                                   "D6dSnsPqcLaVvcH1MSFRMUy5KyVbnDufiX"};
const ExchangeRate DigiAsset::standardExchangeRates[] = {
        {.address = "dgb1qunxh378eltj2jrwza5sj9grvu5xud43vqvudwh",
         .index = 0,
         .name = "CAD"},
        {.address = "dgb1qunxh378eltj2jrwza5sj9grvu5xud43vqvudwh",
         .index = 1,
         .name = "USD"},
        {.address = "dgb1qunxh378eltj2jrwza5sj9grvu5xud43vqvudwh",
         .index = 2,
         .name = "EUR"},
        {.address = "dgb1qunxh378eltj2jrwza5sj9grvu5xud43vqvudwh",
         .index = 3,
         .name = "GBP"},
        {.address = "dgb1qunxh378eltj2jrwza5sj9grvu5xud43vqvudwh",
         .index = 4,
         .name = "AUD"},
        {.address = "dgb1qunxh378eltj2jrwza5sj9grvu5xud43vqvudwh",
         .index = 5,
         .name = "JPY"},
        {.address = "dgb1qunxh378eltj2jrwza5sj9grvu5xud43vqvudwh",
         .index = 6,
         .name = "CNY"},
        {.address = "dgb1qunxh378eltj2jrwza5sj9grvu5xud43vqvudwh",
         .index = 7,
         .name = "TRY"},
        {.address = "dgb1qunxh378eltj2jrwza5sj9grvu5xud43vqvudwh",
         .index = 8,
         .name = "BRL"},
        {.address = "dgb1qunxh378eltj2jrwza5sj9grvu5xud43vqvudwh",
         .index = 9,
         .name = "CHF"},
        {.address = "dgb1qlk3hldeynl3prqw259u8gv0jh7w5nwppxlvt3v",
         .index = 0,
         .name = "BTC"},
        {.address = "dgb1qlk3hldeynl3prqw259u8gv0jh7w5nwppxlvt3v",
         .index = 1,
         .name = "ETH"},
        {.address = "dgb1qlk3hldeynl3prqw259u8gv0jh7w5nwppxlvt3v",
         .index = 2,
         .name = "LTC"},
        {.address = "dgb1qlk3hldeynl3prqw259u8gv0jh7w5nwppxlvt3v",
         .index = 3,
         .name = "DCR"},
        {.address = "dgb1qlk3hldeynl3prqw259u8gv0jh7w5nwppxlvt3v",
         .index = 4,
         .name = "ZIL"},
        {.address = "dgb1qlk3hldeynl3prqw259u8gv0jh7w5nwppxlvt3v",
         .index = 5,
         .name = "RVN"},
        {.address = "dgb1qlk3hldeynl3prqw259u8gv0jh7w5nwppxlvt3v",
         .index = 6,
         .name = "XVG"},
        {.address = "dgb1qlk3hldeynl3prqw259u8gv0jh7w5nwppxlvt3v",
         .index = 7,
         .name = "RDD"},
        {.address = "dgb1qlk3hldeynl3prqw259u8gv0jh7w5nwppxlvt3v",
         .index = 8,
         .name = "NXS"},
        {.address = "dgb1qlk3hldeynl3prqw259u8gv0jh7w5nwppxlvt3v",
         .index = 9,
         .name = "POT"}};


/*
██╗  ██╗███████╗██╗     ██████╗ ███████╗██████╗ ███████╗
██║  ██║██╔════╝██║     ██╔══██╗██╔════╝██╔══██╗██╔════╝
███████║█████╗  ██║     ██████╔╝█████╗  ██████╔╝███████╗
██╔══██║██╔══╝  ██║     ██╔═══╝ ██╔══╝  ██╔══██╗╚════██║
██║  ██║███████╗███████╗██║     ███████╗██║  ██║███████║
╚═╝  ╚═╝╚══════╝╚══════╝╚═╝     ╚══════╝╚═╝  ╚═╝╚══════╝
 */

/**
 * Takes binary data stored in a string and hashes it twice.  First with sha256 then with ripemd160
 */
void DigiAsset::insertSRHash(vector<uint8_t> dataToHash, vector<uint8_t>& result, size_t startIndex) {
    // Perform SHA256 hash
    SHA256 sha256;
    sha256.update(dataToHash.data(), dataToHash.size());
    auto sha256Digest = sha256.digest();

    // Perform RIPEMD160 hash on the SHA256 result
    uint8_t ripemd160Digest[20]; // RIPEMD160 produces a 160-bit hash (20 bytes)
    ripemd160(sha256Digest.data(), sha256Digest.size(), ripemd160Digest);

    // Add the RIPEMD160 hash to the result vector starting at the specified index
    for (size_t i = 0; i < 20; i++) {
        result[i + startIndex] = ripemd160Digest[i];
    }
}

/**
 * Takes binary data stored in a string and hashes it twice.  First with sha256 then with ripemd160
 */
void DigiAsset::insertSRHash(const std::string& dataToHash, vector<uint8_t>& result, size_t startIndex) {
    // Convert string to byte array for hashing
    std::vector<uint8_t> dataBytes(dataToHash.begin(), dataToHash.end());

    // Perform SHA256 hash
    SHA256 sha256;
    sha256.update(dataBytes.data(), dataBytes.size());
    auto sha256Digest = sha256.digest();

    // Perform RIPEMD160 hash on the SHA256 result
    uint8_t ripemd160Digest[20];
    ripemd160(sha256Digest.data(), sha256Digest.size(), ripemd160Digest);

    // Add the RIPEMD160 hash to the result vector starting at the specified index
    for (size_t i = 0; i < 20; i++) {
        result[i + startIndex] = ripemd160Digest[i];
    }
}

/**
 * Calculates a valid script pub key for all existing locked assets as of block 17000000 but not necessarily valid for all future blocks
 * it does not matter that future blocks may calculate wrong answer as long as everyone uses this formula.
 */
vector<uint8_t> DigiAsset::calcSimpleScriptPubKey(const vin_t& vinData) {
    if (vinData.scriptSig.hex.empty()) {
        //dgb1 address
        //return 0014 + ripmd160(sha256(vinData.txwitness.end()))
        vector<uint8_t> result{0x00, 0x14, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        Blob lastPart{vinData.txinwitness.back()};
        insertSRHash(lastPart.vector(), result, 2);
        return result;
    }

    //find last value in scriptSig
    BitIO scriptSig = BitIO::makeHexString(vinData.scriptSig.hex);
    size_t parts = 0;
    vector<uint8_t> lastPart;
    while (scriptSig.getNumberOfBitLeft() > 0) {
        if (scriptSig.getBitcoinDataHeader() == -2) {
            parts++;
            lastPart = scriptSig.getBitcoinData();
        }
    }

    if (vinData.txinwitness.empty() && (parts < 3)) {
        //D address
        //return 76a914 + ripmd160(sha256( last value in vinData.scriptSig.hex )) + 88ac
        vector<uint8_t> result{0x76, 0xa9, 0x14, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x88,
                               0xac};
        insertSRHash(lastPart, result, 3);
        return result;
    }

    //S.. address
    //return a914 + ripmd160(sha256( last value in vinData.scriptSig.hex )) + 87
    vector<uint8_t> result{0xa9, 0x14, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x87};
    insertSRHash(lastPart, result, 2);
    return result;
}

/**
 * Calculate the assetId of an asset
 * @param firstVin - first Vin of the transaction it was issued on
 * @param issuanceFlags - the issuance flags
 * @return
 */
string DigiAsset::calculateAssetId(const vin_t& firstVin, uint8_t issuanceFlags) const {
    vector<uint8_t> assetIdBinary(28);
    //get header
    const uint16_t headerOptions[] = {0x2e37, 0x2e6b, 0x2e4e, 0, 0x20ce, 0x2102, 0x20e4, 0};

    uint16_t header = headerOptions[(issuanceFlags & 0x1c) >> 2]; //gets the assetId header based on lock status and aggregation
    assetIdBinary[0] = header >> 8;
    assetIdBinary[1] = header & 0xff;

    //add data hash
    if (_locked) {
        //locked assets use the ascii txid and output number as there hash data
        insertSRHash(firstVin.txid + ":" + to_string(firstVin.n), assetIdBinary, 2);
    } else {
        //unlocked use simplified script pub key
        insertSRHash(calcSimpleScriptPubKey(firstVin), assetIdBinary, 2);
    }

    //add footer
    assetIdBinary[22] = 0;
    assetIdBinary[23] = issuanceFlags >> 5;

    //create check footer(first 4 bytes of double sha256b)
    SHA256 sha256a;
    sha256a.update(assetIdBinary.data(), 24);
    auto firstHash = sha256a.digest();
    SHA256 sha256b;
    sha256b.update(firstHash.data(), 32);
    auto secondHash = sha256b.digest();
    for (size_t i = 0; i < 4; i++) {
        assetIdBinary[24 + i] = secondHash[i];
    }

    //convert to base 58
    return Base58::encode(assetIdBinary);
}


/*
██████╗ ██████╗ ███╗   ██╗███████╗████████╗██████╗ ██╗   ██╗ ██████╗████████╗ ██████╗ ██████╗
██╔════╝██╔═══██╗████╗  ██║██╔════╝╚══██╔══╝██╔══██╗██║   ██║██╔════╝╚══██╔══╝██╔═══██╗██╔══██╗
██║     ██║   ██║██╔██╗ ██║███████╗   ██║   ██████╔╝██║   ██║██║        ██║   ██║   ██║██████╔╝
██║     ██║   ██║██║╚██╗██║╚════██║   ██║   ██╔══██╗██║   ██║██║        ██║   ██║   ██║██╔══██╗
╚██████╗╚██████╔╝██║ ╚████║███████║   ██║   ██║  ██║╚██████╔╝╚██████╗   ██║   ╚██████╔╝██║  ██║
 ╚═════╝ ╚═════╝ ╚═╝  ╚═══╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝  ╚═════╝   ╚═╝    ╚═════╝ ╚═╝  ╚═╝
 */


DigiAsset::DigiAsset(const getrawtransaction_t& txData, unsigned int height, unsigned char version,
                     unsigned char opcode, BitIO& opReturnData) {
    if (!processIssuance(txData, height, version, opcode, opReturnData)) throw DigiAsset::exceptionInvalidIssuance();
}


DigiAsset::DigiAsset(uint64_t assetIndex, const string& assetId, const string& cid, const KYC& issuer,
                     const DigiAssetRules& rules,
                     unsigned int heightCreated, unsigned int heightUpdated, uint64_t amount) {
    //store variables
    _assetIndex = assetIndex;
    _assetId = assetId;
    _cid = cid;
    _issuer = issuer;
    _rules = rules;
    _heightCreated = heightCreated;
    _heightUpdated = heightUpdated;
    _count = amount;
    _existingAsset = true;
    _enableWrite = false;

    //compute flags
    _locked = (assetId.c_str()[0] == 'L');
    switch (assetId.c_str()[1]) {
        case 'a':
            _aggregation = AGGREGABLE;
            break;
        case 'd':
            _aggregation = DISPERSED;
            break;
        case 'h':
            _aggregation = HYBRID;
            break;
        default:
            if ((assetId!="DigiByte")&&(assetIndex==1)) throw out_of_range("invalid assetId");
            _aggregation = AGGREGABLE;
            _divisibility=8;
            return;
    }
    _divisibility = Base58::decode(assetId)[23];
}


/*
██████╗ ██████╗  ██████╗  ██████╗███████╗███████╗███████╗
██╔══██╗██╔══██╗██╔═══██╗██╔════╝██╔════╝██╔════╝██╔════╝
██████╔╝██████╔╝██║   ██║██║     █████╗  ███████╗███████╗
██╔═══╝ ██╔══██╗██║   ██║██║     ██╔══╝  ╚════██║╚════██║
██║     ██║  ██║╚██████╔╝╚██████╗███████╗███████║███████║
╚═╝     ╚═╝  ╚═╝ ╚═════╝  ╚═════╝╚══════╝╚══════╝╚══════╝
 */


/**
 * Checks a tx for DigiAsset data and if present decodes the version number opcode and the dataStream and sets the pointer to bit 32.
 *
 * DigiAsset Transaction Header structure:
 * byte 0 and 1: 0x4441
 * byte 2: version number(0 not allowed)
 * byte 3: opcode
 *     In Version 1 and 2 Opcodes 1&2 store a 160 bit hash that is completely useless.  In version 3 this hash was
 *     dropped from the specs to save space.
 *     Opcode 3 & 4 where never used in Version 1 & 2 so there uses where dropped from the specs and changed to mean that
 *     rules where to be included with the asset.  3 meaning the rules could be changed in the future and 4 that they can not
 *
 *     OpCode 0x01 - Issuance. No rules
 *     OpCode 0x02 - Issuance. No rules and data stored in multi sig(sunset because not needed with more efficient V3)
 *     OpCode 0x03 - Issuance. Rules that can be changed
 *     OpCode 0x04 - Issuance. Rules that can not be changed
 *     OpCode 0x05 - Issuance. No MetaData or rules(Not Recommended)
 *     OpCode 0x15 - Transfer
 *     OpCode 0x25 - Burn.  Any assets sent to output 31 are burnt
 *
 * @param txData  - Input: Transaction Data
 * @param version - Output: If valid will be set to the version number of thr transaction
 * @param opcode - Output: Will be set to 0 if not a valid DigiAsset transaction
 * @param dataStream - Output: If valid transaction outputs the dataStream and sets the pointer to bit 32
 */
void DigiAsset::decodeAssetTxHeader(const getrawtransaction_t& txData, unsigned char& version, unsigned char& opcode,
                                    BitIO& dataStream) {
    //set not a DigiAsset(will change if it is at the end)
    version = 0;
    opcode = 0;

    //find the encoded data
    int iO = -1;
    for (const vout_t& output: txData.vout) {
        if (output.scriptPubKey.type != "nulldata") continue;
        iO = output.n;
    }
    if (iO == -1) {
        return;
    }

    //check encoded data on output 1 has correct header
    dataStream = BitIO::makeHexString(txData.vout[iO].scriptPubKey.hex);
    if (dataStream.getLength() < DIGIASSET_MIN_POSSIBLE_LENGTH) {
        return; //fc8ac69d67c298152a8b93b1b7a054e28427f02e69025249e09be123de2986f3 has an OP_RETURN with no extra data after.  This prevents error
    }
    if (!dataStream.checkIsBitcoinOpReturn()) {
        return; //not an OP_RETURN
    }
    if (dataStream.getBitcoinDataHeader() != BITIO_BITCOIN_TYPE_DATA) {
        return; //not data
    }
    dataStream = dataStream.copyBitcoinData(); //strip the header out

    if (dataStream.getBits(16) != 0x4441) {
        return; //not asset tx
    }

    //get version number
    version = dataStream.getBits(8);

    //get opcode
    opcode = dataStream.getBits(8);
}

/**
 * Handles decoding an issuance transaction into the DigiAsset Object it created
 *
 * DigiAsset Transaction Body structure:
 * Body structure is not fixed bit length so we can't specify byte per byte what is happening so here is some psudo code
 * to describe it.  We will start at bit 32 after the header
 *
 * if (version <3 and opcode <3) skip next 20 bytes.  They don't contain anything useful
 * if (opcode 1,3, or 4) the next 32 bytes is the sha256 of the meta data
 * the next 1 to 8 bytes contains the number of assets to create(see BitIO::getFixedPrecision for exactly how to decode)
 * the next several bytes contains the rules if there are any(see DigiAssetRules::DigiAssetRules for exactly how to decode)
 * the next several bytes contains transfer instructions(see DigiByteTransaction::decodeAssetTransfer for exactly how to decode)
 * the last 1 byte contains the issuance flags(bit 0 being the LSB)
 *      bit 7,6,5: divisibility
 *      bit 4: locked
 *      bit 3,2: aggregation(see DigiAsset.h public constants for meaning of each)
 *
 * @param txData - chain data
 * @param height - block height
 * @param version - DigiAsset Version Number
 * @param opcode - Opcode
 * @param dataStream - Binary Data with pointer set to bit 32
 * @return - bool of weather it was an issuance or not
 */
bool DigiAsset::processIssuance(const getrawtransaction_t& txData, unsigned int height, unsigned char version,
                                unsigned char opcode, BitIO& dataStream) {
    Database* db = AppMain::GetInstance()->getDatabase();
    _existingAsset = true;
    try {
        //for now assume this is first issuance of asset we will correct later if not
        _heightCreated = height;
        _heightUpdated = height;

        //if version<3 & opcode 1 or 2 skip the sha1 data since it is useless
        if ((version < 3) && (opcode < 3)) dataStream.movePositionBy(160);

        //get metadata hash if it exists
        BitIO metadataHash;
        if ((opcode == 1) || (opcode == 3) || (opcode == 4)) metadataHash = dataStream.copyBits(256);
        if (opcode == 2) {
            //meta data hash encoded in
            BitIO multiSigData = BitIO::makeHexString(txData.vout[0].scriptPubKey.hex);
            multiSigData.getBitcoinDataHeader();           //number of signers needed
            multiSigData.getBitcoinData();                 //hash of first signers key
            metadataHash = multiSigData.copyBitcoinData(); //sha256 of meta data
        }

        //calculate cid
        if (metadataHash.getLength() == 256) {
            metadataHash.movePositionToBeginning();
            _cid = IPFS::sha256ToCID(metadataHash);
        }

        //get amount of assets to create
        _count = dataStream.getFixedPrecision();

        //get issuance flags
        size_t rulesStart = dataStream.getPosition();
        if (dataStream.getLength() - 8 < rulesStart) return false; //data missing
        dataStream.movePositionTo(dataStream.getLength() - 8);     //set to 8th last bit
        unsigned char issuanceFlags = dataStream.getBits(8);
        _divisibility = issuanceFlags >> 5;
        _locked = ((issuanceFlags & 0x10) > 0);
        _aggregation = (issuanceFlags & 0x0c) >> 2;
        if (_aggregation == 3) return false;   //invalid header type
        dataStream.movePositionTo(rulesStart); //put pointer back

        //fix amount if version 1
        if (version == 1) {
            _count /= BitIO::pow10(_divisibility);
        }

        //check counts are valid
        if ((_locked) && (_count == 0)) return false;                    //not assets possible to create
        if ((_aggregation != AGGREGABLE) && (_count == 0)) return false; //not assets possible to create
        if (_count > (uint64_t) 18014398509481983) return false;         //invalid amount

        //special cases for locked assets
        if (_locked) _rules.lock(); //can't rewrite rules if locked

        //get the rules
        _rules = DigiAssetRules(txData, dataStream, _cid, opcode);

        //calculate assetId
        _assetId = calculateAssetId(txData.vin[0], issuanceFlags);

        //check if this is update or issuance
        if ((!_locked) && (isAggregable())) { //these types never update
            _heightCreated = db->getAssetHeightCreated(_assetId, height, _assetIndex);
        }

        //if unlocked check for conflicting rules
        handleRulesConflict();

        //lookup KYC state
        string issuer = db->getSendingAddress(txData.vin[0].txid, txData.vin[0].n);
        _issuer = db->getAddressKYC(issuer);
        return true;

    } catch (const out_of_range& e) {
        return false;
    }
}

/**
 * If an asset is unlocked then rules included with the issuance may or may not be ignored in favour of older rules that exist.
 * Criteria to decide what rules should be used.  Do in order and stop when 1 is valid
 * 1) If locked then always use rules encoded with asset
 * 2) If no older rules exist than use the rules encoded with the asset
 * 3) If no new rules included use old rules(there must be old rules to get past step 2)
 * 4) If old rule was rewritable than use the new rule
 * 5) Use the old rules
 */
void DigiAsset::handleRulesConflict() {
    //check if possible to have old rules
    if (_locked) return;                    //not possible to have conflicts
    if (_aggregation != AGGREGABLE) return; //not possible to have conflicts

    //lookup old rules
    Database* db = AppMain::GetInstance()->getDatabase();
    DigiAssetRules oldRules = db->getRules(_assetId);
    if (oldRules.empty()) return; //no old rules exist

    //check if rules should be rewound
    if (!oldRules.isRewritable() || _rules.empty()) {
        //if rules where not rewritable or no rules where set this time use the old rules(to remove rules when creating an asset you need to use rules op code but than not include any rules) if non rules op code is used than this means don't change to save space
        _rules = oldRules;
    }
}

/**
 * Allows checking if 2 assets are the same type
 * if (assetA==assetB) ...
 */
bool DigiAsset::operator==(const DigiAsset& rhs) const {
    return (_assetIndex == rhs._assetIndex);
}

/**
 * Allows checking if 2 assets are not the same type
 * if (assetA!=assetB) ...
 */
bool DigiAsset::operator!=(const DigiAsset& rhs) const {
    return (_assetIndex != rhs._assetIndex);
}

/**
 * Get Original Asset Count
 */
uint64_t DigiAsset::getOriginalCount() const {
    Database* db = AppMain::GetInstance()->getDatabase();
    uint64_t count = db->getOriginalAssetCount(getAssetIndex());
    return count;
}

/**
 * Allow reducing the number of assets in the object
 */
void DigiAsset::removeCount(uint64_t count) {
    if (count > _count) throw out_of_range("Can't remove more than have");
    _count -= count;
}

/**
 * Allows setting the number of assets in the object
 */
void DigiAsset::setCount(uint64_t count) {
    _count = count;
}

/**
 * Allows adding to the number of assets in the object
 */
void DigiAsset::addCount(uint64_t count) {
    _count += count;
}

/**
 * Returns the number of assets this asset object is holding.
 * Remember to take in to account the number of decimals.  If getCount() returns 100 and getDecimals(2) there are 1.00
 * assets in the object.
 * @return
 */
uint64_t DigiAsset::getCount() const {
    return _count;
}

/**
 * Returns the number of assets in the object as a string.  If asset has decimals will always show all decimals even if
 * right most digits are 0s.
 * @return examples
 * "1" , "4.01", "4.00"
 */
std::string DigiAsset::getStrCount() const {
    string result = to_string(_count); //convert to string
    int neededDecimals = _divisibility + 1 - result.length();
    if (neededDecimals > 0) result.insert(0, neededDecimals, '0'); //pad start so at least 1 zero before decimal
    result.insert(result.length() - _divisibility, 1, '.');        //add decimal point
    return result;
}

/**
 * Gets the number of decimals the asset uses.
 * @return number between 0 and 8 inclusive.
 */
uint8_t DigiAsset::getDecimals() const {
    return _divisibility;
}

/**
 * Calculates the CID based on a sha256
 * returns empty if there is no meta data
 */
std::string DigiAsset::getCID() const {
    return _cid;
}

/**
 * Gets the assets assetIndex number.
 * @return
 * 0 - not yet set
 * 1 and up - assetIndex.  This value is only valid on this particular node.
 */
uint64_t DigiAsset::getAssetIndex(bool allowUnknownAssetIndex) const {
    if (allowUnknownAssetIndex) return _assetIndex;
    if (_assetIndex==0) throw exceptionUnknownAssetIndex();
    return _assetIndex;
}

/**
 * This function should only be used by the DigiByte Transaction class.
 * It assigns a sub class index number when an asset is being added to the database.
 */
void DigiAsset::setAssetIndex(uint64_t assetIndex) {
    if (_assetIndex != 0) throw exceptionWriteProtected();
    _assetIndex = assetIndex;
}

/**
 * Returns if hybrid asset
 * Hybrid is like a mixture of aggregable and dispersed.  You can create multiple chunks of assets each chunk being a sub
 * asset.  As you send assets they can be split up and tracked but they don't recombine to save space like aggregable assets do.
 */
bool DigiAsset::isHybrid() const {
    return (_aggregation == HYBRID);
}

/**
 * Returns if aggregable
 * Aggregable assets when receiving from multiple sources will combine together to save transaction space.
 * There are no sub types with aggregable assets so assetIndex is not necessary.  This is the most common kind of asset.
 */
bool DigiAsset::isAggregable() const {
    return (_aggregation == AGGREGABLE);
}

/**
 * Returns if the asset is dispersed
 * Dispersed assets can be individually tracked through the chain.
 * Under most circumstances you would want them to be unlocked and issued 1 at a time so each is unique but all share the
 * same assetId.  assetIndex is used internally to tell them apart
 */
bool DigiAsset::isDispersed() const {
    return (_aggregation == DISPERSED);
}

/**
 * Returns if the asset is locked or not
 * Assets that are locked can not be modified in any way
 */
bool DigiAsset::isLocked() const {
    return _locked;
}

/**
 * Gets the assets public ID
 */
std::string DigiAsset::getAssetId() const {
    return _assetId;
}

/**
 * Returns information about the person that created the asset
 */
KYC DigiAsset::getIssuer() const {
    return _issuer;
}

/**
 * Returns a copy of the assets rules object
 */
DigiAssetRules DigiAsset::getRules() const {
    return _rules;
}

/**
 * Returns the height asset was created
 */
unsigned int DigiAsset::getHeightCreated() const {
    return _heightCreated;
}

/**
 * Returns the height asset was last updated.
 * For locked assets this will always be the same as height created
 */
unsigned int DigiAsset::getHeightUpdated() const {
    return _heightUpdated;
}

/**
 * Get the expiry height(blocks) or time(ms from epoch) the asset expires.
 * Alternatively use getRules() to get the rules and than on that use
 * rules.isExpiryHeight() - returns true if block height false if time
 * rules.expires() - returns true if it expires at all, false if never
 * rules.getExpiry() - same as this function
 * @return - unsigned int
 *  0 to MIN_EPOCH_VALUE-1: block height
 *  MIN_EPOCH_VALUE to EXPIRE_NEVER-1: ms from epoch
 *  EXPIRE_NEVER: never
 */
uint64_t DigiAsset::getExpiry() const {
    return _rules.getExpiry();
}

/**
 * returns true if any pool has marked this asset as bad.
 * Optionally can put in a pool index to check just 1 pool
 * @return
 */
bool DigiAsset::isBad(int poolIndex) const {
    //get the range off pools we will check
    PermanentStoragePoolList* pools = AppMain::GetInstance()->getPermanentStoragePoolList();
    size_t start = 0;
    size_t stop = pools->getPoolCount() - 1;
    if (poolIndex >= 0) {
        start = poolIndex;
        stop = poolIndex;
    }

    //check pools and return true if any say bad
    for (size_t i = start; i <= stop; i++) {
        if (pools->getPool(i)->isAssetBad(_assetId)) return true;
    }
    return false;
}

/**
 * Declare you own the asset.
 * Please note unless you have the associated private key to the address that issued the asset declaring you own an asset
 * is pointless, you will just get errors when you try to send to the network
 */
void DigiAsset::setOwned() {
    if (isLocked()) return;
    _enableWrite = true;
}

/**
 * Setter function to allow changing an assets rules.  Will throw an exceptionWriteProtected if you have not enabled editing
 * or if the asset is unchangeable.
 * Run setOwned() method first before trying to used
 * @param rules
 */
void DigiAsset::setRules(const DigiAssetRules& rules) {
    if (!_enableWrite) throw exceptionWriteProtected();          //need to mark you own the asset
    if (!_rules.isRewritable()) throw exceptionWriteProtected(); //not possible to change
    _rules = rules;
}

/**
 * Checks over the list of inputs and outputs and throws an exception if it is not a valid transaction
 * Expect: exceptionRuleFailed
 * @param inputs
 * @param outputs
 * @param height - block height of transaction
 * @param time - time in ms of transaction
 */
void DigiAsset::checkRulesPass(const vector<AssetUTXO>& inputs, const vector<AssetUTXO>& outputs, unsigned int height,
                               uint64_t time) const {
    //if no rules than no need to check if they were followed
    if (_rules.empty()) return;
    Database* db = AppMain::GetInstance()->getDatabase();

    //make list of changes
    map<string, int64_t> changes;

    //subtract any assets we spent
    for (const AssetUTXO& input: inputs) {
        for (const DigiAsset& asset: input.assets) {
            if (asset.getAssetIndex() == _assetIndex) {
                changes[input.address] -= asset.getCount();
            }
        }
    }

    //add any assets we received
    for (const AssetUTXO& output: outputs) {
        for (const DigiAsset& asset: output.assets) {
            if (asset.getAssetIndex() == _assetIndex) {
                changes[output.address] += asset.getCount();
            }
        }
    }

    //rules don't apply if consolidation transaction or pure burn so check if assets actually moved anywhere
    bool consolidation = true;
    for (const auto& change: changes) { //[address,change]
        if (change.second > 0) consolidation = false;
    }
    if (consolidation) return;

    //check signer rules
    if (_rules.getRequiredSignerWeight() > 0) {
        vector<Signer> signers = _rules.getSigners();
        uint64_t totalWeight = 0;
        for (const Signer& signer: signers) {
            bool wasSigned = false;
            for (const AssetUTXO& utxo: inputs) {
                if (signer.address == utxo.address) {
                    wasSigned = true;
                    break;
                }
            }
            if (wasSigned) totalWeight += signer.weight;
        }
        if (totalWeight < _rules.getRequiredSignerWeight()) throw exceptionRuleFailed("Signers");
    }

    //check royalties rule
    if (_rules.getIfRequiresRoyalty()) {
        //get the exchange rate
        uint64_t exchangeRate = floor(db->getAcceptedExchangeRate(_rules.getRoyaltyCurrency(), height));

        //get the number of new recipients(assume 1 is change if more than 1 output)
        size_t count = -1;
        for (const AssetUTXO& utxo: outputs) {
            for (const DigiAsset& asset: utxo.assets) {
                if (asset.getAssetIndex() == _assetIndex) {
                    count++;
                    break;
                }
            }
        }
        if (count < 1) count = 1;

        //check that royalty was paid
        vector<Royalty> royalties = _rules.getRoyalties();
        for (const Royalty& royalty: royalties) {
            uint64_t minAccepted = (count * royalty.amount * exchangeRate / 100000000) -
                                   1; //in case of rounding error minimum accepted is 1 sat lower
            bool paid = false;
            for (const AssetUTXO& utxo: outputs) {
                if ((utxo.address == royalty.address) && (utxo.digibyte >= minAccepted)) {
                    paid = true;
                    break;
                }
            }
            if (!paid) throw exceptionRuleFailed("Royalty");
        }
    }

    //check kyc rules
    if (_rules.getIfGeoFenced()) {
        for (const auto& change: changes) {
            if (change.second <= 0) {
                continue;
            }                                                  //didn't receive so don't check
            KYC addressData = db->getAddressKYC(change.first); //gets the addresses KYC data
            if (
                    (!addressData.valid(height)) ||
                    (!_rules.getIfCountryAllowedToReceive(addressData.getCountry()))) {
                throw exceptionRuleFailed("KYC");
            } //throw if not valid
        }
    }

    //check expiry rules
    if (_rules.getIfExpired(height, time)) throw exceptionRuleFailed("Expiry");

    //check vote rules
    if (_rules.getIfVoteRestricted()) {
        for (const auto& change: changes) {
            if (change.second <= 0) continue;
            if (!_rules.getIfValidVoteAddress(change.first)) throw exceptionRuleFailed("Vote");
        }
    }

    //check deflate rules
    if (_rules.getRequiredBurn() > 0) {
        //count how many assets have been burned
        int64_t burns = 0;
        for (const auto& change: changes) {
            burns -= change.second; //negative so a negative change results in positive numbers
        }
        //negative numbers is not possible at this point
        if ((uint64_t) burns < _rules.getRequiredBurn()) throw exceptionRuleFailed("Deflation");
    }
}

/**
 * Converts DigiAsset Object into JSON for outputting by API
 *
 * @param simplified - if true, only includes assetIndex, assetId, cid, count, and decimals (bool)
 *
 * @return Value - Returns a Json::Value object that represents the DigiAsset in JSON format.
 *                 The JSON object contains the following keys and their expected data types:
 *
 *  Base Fields (always included):
 *                     - assetIndex (unsigned int): A unique identifier for the asset sub type.
 *                       Since an asset can have multiple sub types this number allows specifying a single asset and its
 *                       sub type in 1 number.  It is not guaranteed to match between nodes and should only ever be used
 *                       to cross reference data within one node.
 *                     - assetId (string): The public identifier for a DigiAsset.
 *                     - cid (string): The Content ID for the asset metadata
 *                     - count (unsigned int): The number of assets.  This value will be 0 if context isn't referring to
 *                       a quantity
 *                     - decimals (unsigned int): The number of decimals for the asset.  If decimal is 2 and count is 100
 *                       that means there are 1.00 assets
 *                     - height (unsigned int):  Height created
 *
 *  Additional Fields (included if simplified is false):
 *                     - ipfs (string or Json::Value): IPFS metadata or error message
 *                       Possible error messages:
 *                         "Metadata is corrupt" - IPFS data was downloadable but did not contain properly formatted JSON
 *                         "Metadata could not be found" - IPFS data was not downloadable.  This could mean data has been
 *                           lost or it could just mean its temporarily not retrievable.
 *                     - rules (Json::Value): Rules associated with the DigiAsset, may contain:
 *                         - changeable (bool): Indicates if the asset rules are changeable
 *                         - deflation (unsigned int, optional): Number of assets that must be burned to make a valid transaction
 *                         - expiry (unsigned int, optional): Block height or timestamp when asset can no longer be transferred
 *                           values less than MIN_EPOCH_VALUE are block height
 *                           values greater are epoch time in ms
 *                         - royalty (Json::Value): Information about royalty payments, may contain:
 *                             - units (Json::Value): Exchange rate units, may contain:
 *                                 - address (string): Conversion rate tracking address
 *                                 - index (unsigned int): Conversion rate trackers can track up to 10 rates so this is which of the 10(0 - 9)
 *                                 - name (string, optional): If it is one of default exchange rates there will be a currency code
 *                             - addresses (Json::Value): Royalty recipient addresses(key) and their amounts(value)
 *                               value is in sats.  so if not units than 10000000=1DGB.  if units point to USD than
 *                               100000000=1 USD
 *                         - geofence (Json::Value): Assets with this rule can only be sent to KYC verified addresses
 *                           and will contain 1 of the following
 *                             - denied (array of strings): Everyone can hold the asset accept those countries listed here
 *                             - allowed (array of strings): Only countries listed here can hold
 *                             There will never be both allowed and denied
 *                         - voting (Json::Value): Voting options, may contain:
 *                             - restricted (bool): If true asset can only be sent to one of the voting addresses
 *                             - options (Json::Value): Voting options addresses(key) and labels(value)
 *                         - approval (Json::Value): Rarely used rule but allows for an asset that requires approval by creator for all trades
 *                             - required (unsigned int): Number of votes required for a transaction to be valid
 *                             - approvers (Json::Value): List of addresses(key) and their weights(value) that can cast
 *                               there approval to the transaction.  Total included in any transaction must be greater
 *                               or equal to required
 *                     - issuer (Json::Value): Information about the issuer, may contain:
 *                         - address (string): Issuer's address
 *                         - country (string, optional): Issuer's country
 *                         - name (string, optional): Issuer's name
 *                         - hash (string, optional): Issuer's hash
 *                         name and hash will never both be present.  hash is returned if creator is anonymous
 */
Value DigiAsset::toJSON(bool simplified, bool ignoreIpfs) const {
    Json::Value result(Json::objectValue);

    // Simplified
    result["assetIndex"] = static_cast<Json::UInt64>(getAssetIndex());
    result["assetId"] = getAssetId();
    result["cid"] = getCID();
    result["count"] = static_cast<Json::UInt64>(getCount());
    result["decimals"] = getDecimals();
    result["height"] = _heightCreated;
    result["initial"] = static_cast<Json::UInt64>(getOriginalCount());

    if (simplified) return result;

    // Include meta data
    if (!_cid.empty() && !ignoreIpfs) {
        try {
            IPFS* ipfs = AppMain::GetInstance()->getIPFS();
            string metadata = ipfs->callOnDownloadSync(_cid, "", DIGIASSET_JSON_IPFS_MAX_WAIT);
            Json::Value metadataValue;
            Json::Reader reader;
            bool parsingSuccessful = reader.parse(metadata, metadataValue);
            if (!parsingSuccessful) {
                result["ipfs"] = "Metadata is corrupt";
            } else {
                result["ipfs"] = metadataValue;
            }
        } catch (const IPFS::exception& e) {
            result["ipfs"] = "Metadata could not be found";
        }
    }

    // Rules
    DigiAssetRules rules = getRules();
    if (!rules.empty()) {
        result["rules"] = rules.toJSON();
    }

    // Issuer
    Json::Value kycObj(Json::objectValue);
    kycObj["address"] = _issuer.getAddress();
    if (_issuer.valid()) {
        kycObj["country"] = _issuer.getCountry();
        string name = _issuer.getName();
        if (!name.empty()) {
            kycObj["name"] = name;
        } else {
            string hash = _issuer.getHash();
            kycObj["hash"] = hash;
        }
    }
    result["issuer"] = kycObj;

    return result;
}

/**
 * when loading already existing assets use this command to lookup assetIndex.
 * Will do nothing if already known
 */
void DigiAsset::lookupAssetIndex(const string& txid, unsigned int vout) {
    if (_assetIndex>0) return;
    Database* db=AppMain::GetInstance()->getDatabase();
    _assetIndex=db->getAssetIndex(_assetId,txid,vout);
}
bool DigiAsset::isAssetIndexSet() const {
    return _assetIndex!=0;
}
