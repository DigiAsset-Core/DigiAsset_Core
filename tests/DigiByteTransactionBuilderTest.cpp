//
// Tests for the new transaction building side of DigiByteTransaction
// (addInput/addDigiByteOutput/addDigiAssetOutput/setIssuance/encodeAssetOpReturn)
// plus the AssetWallet helpers that don't need a wallet connection.
//
// These are pure unit tests - no wallet, database or IPFS node needed.
// Round trip tests replay the encoded OP_RETURN through the same decoder
// (decodeAssetTransfer) the chain analyzer uses on real chain data.
//

#include "AssetWallet.h"
#include "BitIO.h"
#include "DigiAsset.h"
#include "DigiByteTransaction.h"
#include "IPFS.h"
#include "gtest/gtest.h"

using namespace std;

namespace {
    ///creates a test asset that is not on chain but has an assetIndex like a database asset would
    DigiAsset makeTestAsset(uint64_t assetIndex, uint64_t count, unsigned char divisibility = 0,
                            unsigned char aggregation = DigiAsset::AGGREGABLE) {
        DigiAsset asset("", count, divisibility, true, aggregation);
        asset.setAssetIndex(assetIndex);
        return asset;
    }

    ///creates an asset bearing UTXO for use as a transaction input
    AssetUTXO makeInput(const string& txid, uint16_t vout, const vector<DigiAsset>& assets) {
        AssetUTXO utxo;
        utxo.txid = txid;
        utxo.vout = vout;
        utxo.address = "dgb1qsenderaddress";
        utxo.digibyte = 600;
        utxo.assets = assets;
        return utxo;
    }
} // namespace


TEST(DigiByteTransactionBuilder, transferRoundTrip) {
    //100 units of one asset on a single input.  Send 30, change 70
    DigiAsset asset = makeTestAsset(5, 100);
    AssetUTXO input = makeInput("aa11", 0, {asset});

    DigiByteTransaction tx;
    tx.addInput(input);
    DigiAsset sendPart = asset;
    sendPart.setCount(30);
    DigiAsset changePart = asset;
    changePart.setCount(70);
    tx.addDigiAssetOutput("dgb1qrecipient", {sendPart});
    tx.addDigiAssetOutput("dgb1qchange", {changePart});

    string hex = tx.encodeAssetOpReturn();

    //header: "DA" magic, version 3, transfer opcode 0x15
    ASSERT_GE(hex.length(), (size_t) 8);
    EXPECT_EQ(hex.substr(0, 8), "44410315");

    //replay the instructions through the decode path the chain analyzer uses
    DigiByteTransaction rx;
    rx._height = 20000000;
    rx._assetTransactionVersion = 3;
    rx._inputs.push_back(input);
    AssetUTXO out0;
    out0.address = "dgb1qrecipient";
    out0.digibyte = 600;
    AssetUTXO out1;
    out1.address = "dgb1qchange";
    out1.digibyte = 600;
    rx._outputs = {out0, out1};

    BitIO stream = BitIO::makeHexString(hex);
    stream.movePositionTo(32); //skip header
    rx.decodeAssetTransfer(stream, rx._inputs, 2 /*DIGIASSET_TRANSFER*/);

    ASSERT_EQ(rx._outputs[0].assets.size(), (size_t) 1);
    EXPECT_EQ(rx._outputs[0].assets[0].getAssetIndex(), (uint64_t) 5);
    EXPECT_EQ(rx._outputs[0].assets[0].getCount(), (uint64_t) 30);
    ASSERT_EQ(rx._outputs[1].assets.size(), (size_t) 1);
    EXPECT_EQ(rx._outputs[1].assets[0].getAssetIndex(), (uint64_t) 5);
    EXPECT_EQ(rx._outputs[1].assets[0].getCount(), (uint64_t) 70);
}

TEST(DigiByteTransactionBuilder, transferMultiAssetRoundTrip) {
    //input 1 holds 50 of asset A and 10 of asset B.  input 2 holds 25 more of asset A.
    //send 60 of A to the recipient, everything else(15 A + 10 B) to change
    DigiAsset assetA = makeTestAsset(7, 50);
    DigiAsset assetB = makeTestAsset(9, 10);
    DigiAsset assetA2 = makeTestAsset(7, 25);
    AssetUTXO input1 = makeInput("aa11", 0, {assetA, assetB});
    AssetUTXO input2 = makeInput("bb22", 1, {assetA2});

    DigiByteTransaction tx;
    tx.addInput(input1);
    tx.addInput(input2);
    DigiAsset sendPart = makeTestAsset(7, 60);
    DigiAsset changeA = makeTestAsset(7, 15);
    DigiAsset changeB = makeTestAsset(9, 10);
    tx.addDigiAssetOutput("dgb1qrecipient", {sendPart});
    tx.addDigiAssetOutput("dgb1qchange", {changeA, changeB});

    string hex = tx.encodeAssetOpReturn();
    EXPECT_EQ(hex.substr(0, 8), "44410315");

    //replay through the decoder
    DigiByteTransaction rx;
    rx._height = 20000000;
    rx._assetTransactionVersion = 3;
    rx._inputs = {input1, input2};
    AssetUTXO out0;
    out0.address = "dgb1qrecipient";
    out0.digibyte = 600;
    AssetUTXO out1;
    out1.address = "dgb1qchange";
    out1.digibyte = 600;
    rx._outputs = {out0, out1};

    BitIO stream = BitIO::makeHexString(hex);
    stream.movePositionTo(32);
    rx.decodeAssetTransfer(stream, rx._inputs, 2 /*DIGIASSET_TRANSFER*/);

    //recipient gets 60 of asset A(aggregable parts merge on decode)
    ASSERT_EQ(rx._outputs[0].assets.size(), (size_t) 1);
    EXPECT_EQ(rx._outputs[0].assets[0].getAssetIndex(), (uint64_t) 7);
    EXPECT_EQ(rx._outputs[0].assets[0].getCount(), (uint64_t) 60);

    //change output gets 10 of B and 15 of A(order follows consumption order: B first)
    uint64_t totalA = 0;
    uint64_t totalB = 0;
    for (const DigiAsset& asset: rx._outputs[1].assets) {
        if (asset.getAssetIndex() == 7) totalA += asset.getCount();
        if (asset.getAssetIndex() == 9) totalB += asset.getCount();
    }
    EXPECT_EQ(totalA, (uint64_t) 15);
    EXPECT_EQ(totalB, (uint64_t) 10);
}

TEST(DigiByteTransactionBuilder, issuanceEncoding) {
    //check every byte of an issuance payload against the documented format
    string hash = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
    string cid = IPFS::sha256ToCID(hash);
    EXPECT_EQ(IPFS::cidToSha256(cid), hash); //cid conversion round trip

    DigiAsset asset(cid, 1000, 2, true, DigiAsset::AGGREGABLE);
    DigiByteTransaction tx;
    tx.setIssuance(asset);
    tx.addDigiAssetOutput("dgb1qrecipient", {asset});

    string hex = tx.encodeAssetOpReturn();

    //header: "DA" magic, version 3, opcode 1(issuance with metadata, no rules)
    EXPECT_EQ(hex.substr(0, 8), "44410301");

    //32 byte metadata hash
    EXPECT_EQ(hex.substr(8, 64), hash);

    //amount, transfer instruction, issuance flags
    BitIO stream = BitIO::makeHexString(hex);
    stream.movePositionTo(32 + 256);
    EXPECT_EQ(stream.getFixedPrecision(), (uint64_t) 1000); //number of assets created
    EXPECT_EQ(stream.getBits(3), (uint64_t) 0);             //skip/range/percent all 0
    EXPECT_EQ(stream.getBits(5), (uint64_t) 0);             //everything to output 0
    EXPECT_EQ(stream.getFixedPrecision(), (uint64_t) 1000); //full amount
    EXPECT_EQ(stream.getBits(8), (uint64_t) 0x50);          //divisibility 2, locked, aggregable
    EXPECT_EQ(stream.getNumberOfBitLeft(), (size_t) 0);     //nothing extra
}

TEST(DigiByteTransactionBuilder, issuanceNoMetadata) {
    DigiAsset asset("", 21000000, 0, false, DigiAsset::AGGREGABLE);
    DigiByteTransaction tx;
    tx.setIssuance(asset);
    tx.addDigiAssetOutput("dgb1qrecipient", {asset});

    string hex = tx.encodeAssetOpReturn();

    //opcode 5 = no metadata.  flags: divisibility 0, unlocked, aggregable = 0x00
    EXPECT_EQ(hex.substr(0, 8), "44410305");
    BitIO stream = BitIO::makeHexString(hex);
    stream.movePositionTo(32);
    EXPECT_EQ(stream.getFixedPrecision(), (uint64_t) 21000000);
    EXPECT_EQ(stream.getBits(3), (uint64_t) 0);
    EXPECT_EQ(stream.getBits(5), (uint64_t) 0);
    EXPECT_EQ(stream.getFixedPrecision(), (uint64_t) 21000000);
    EXPECT_EQ(stream.getBits(8), (uint64_t) 0x00);
    EXPECT_EQ(stream.getNumberOfBitLeft(), (size_t) 0);
}

TEST(DigiByteTransactionBuilder, unbalancedTransferThrows) {
    //input has 100 but outputs only account for 30 - must throw, not silently burn
    DigiAsset asset = makeTestAsset(5, 100);
    DigiByteTransaction tx;
    tx.addInput(makeInput("aa11", 0, {asset}));
    DigiAsset sendPart = asset;
    sendPart.setCount(30);
    tx.addDigiAssetOutput("dgb1qrecipient", {sendPart});
    EXPECT_THROW(tx.encodeAssetOpReturn(), DigiByteTransaction::exception);
}

TEST(DigiByteTransactionBuilder, overspendTransferThrows) {
    //outputs want more than the inputs hold
    DigiAsset asset = makeTestAsset(5, 100);
    DigiByteTransaction tx;
    tx.addInput(makeInput("aa11", 0, {asset}));
    DigiAsset sendPart = asset;
    sendPart.setCount(150);
    tx.addDigiAssetOutput("dgb1qrecipient", {sendPart});
    EXPECT_THROW(tx.encodeAssetOpReturn(), DigiByteTransaction::exception);
}

TEST(DigiByteTransactionBuilder, outputValidation) {
    DigiByteTransaction tx;
    EXPECT_THROW(tx.addDigiByteOutput("", 600), DigiByteTransaction::exception);
    EXPECT_THROW(tx.addDigiAssetOutput("dgb1qaddr", {}), DigiByteTransaction::exception);
    DigiAsset zero = makeTestAsset(5, 100);
    zero.setCount(0);
    EXPECT_THROW(tx.addDigiAssetOutput("dgb1qaddr", {zero}), DigiByteTransaction::exception);
}

TEST(DigiByteTransactionBuilder, issuanceRejectsAssetInputs) {
    //assets on issuance inputs would be burned so both orders of setup must throw
    DigiAsset existing = makeTestAsset(5, 100);
    DigiAsset newAsset("", 1000, 0, true, DigiAsset::AGGREGABLE);

    DigiByteTransaction tx1;
    tx1.addInput(makeInput("aa11", 0, {existing}));
    EXPECT_THROW(tx1.setIssuance(newAsset), DigiByteTransaction::exception);

    DigiByteTransaction tx2;
    tx2.setIssuance(newAsset);
    EXPECT_THROW(tx2.addInput(makeInput("aa11", 0, {existing})), DigiByteTransaction::exception);
}

TEST(AssetWallet, parseAssetAmount) {
    //integers are in display units
    EXPECT_EQ(AssetWallet::parseAssetAmount(Json::Value(3), 0), (uint64_t) 3);
    EXPECT_EQ(AssetWallet::parseAssetAmount(Json::Value(3), 2), (uint64_t) 300);

    //decimal strings
    EXPECT_EQ(AssetWallet::parseAssetAmount(Json::Value("1.5"), 2), (uint64_t) 150);
    EXPECT_EQ(AssetWallet::parseAssetAmount(Json::Value("0.01"), 2), (uint64_t) 1);
    EXPECT_EQ(AssetWallet::parseAssetAmount(Json::Value("42"), 0), (uint64_t) 42);
    EXPECT_EQ(AssetWallet::parseAssetAmount(Json::Value("1.50"), 2), (uint64_t) 150);

    //doubles
    EXPECT_EQ(AssetWallet::parseAssetAmount(Json::Value(1.5), 2), (uint64_t) 150);

    //invalid amounts
    EXPECT_THROW(AssetWallet::parseAssetAmount(Json::Value("1.234"), 2), std::out_of_range); //too many decimals
    EXPECT_THROW(AssetWallet::parseAssetAmount(Json::Value("0"), 2), std::out_of_range);     //zero
    EXPECT_THROW(AssetWallet::parseAssetAmount(Json::Value(0), 2), std::out_of_range);       //zero
    EXPECT_THROW(AssetWallet::parseAssetAmount(Json::Value(-5), 2), std::out_of_range);      //negative
    EXPECT_THROW(AssetWallet::parseAssetAmount(Json::Value("abc"), 2), std::out_of_range);   //not a number
}

TEST(AssetWallet, satsToDecimal) {
    EXPECT_EQ(AssetWallet::satsToDecimal(600), "0.00000600");
    EXPECT_EQ(AssetWallet::satsToDecimal(100000000), "1.00000000");
    EXPECT_EQ(AssetWallet::satsToDecimal(123456789012), "1234.56789012");
    EXPECT_EQ(AssetWallet::satsToDecimal(0), "0.00000000");
}

TEST(IPFSHelper, cidToSha256Errors) {
    EXPECT_THROW(IPFS::cidToSha256("QmNPyr5tkm48cUu5iMbReiM8GN8AW6PRpzUztPFadaxC8j"), IPFS::exceptionInvalidCID); //v0 cid unsupported
    EXPECT_THROW(IPFS::cidToSha256(""), IPFS::exceptionInvalidCID);
    EXPECT_THROW(IPFS::cidToSha256("b1234"), IPFS::exceptionInvalidCID); //too short
}

TEST(DigiByteTransactionBuilder, burnRoundTrip) {
    //100 units on one input: burn 40, change 60.  Burn instructions target output 31
    DigiAsset asset = makeTestAsset(5, 100);
    AssetUTXO input = makeInput("aa11", 0, {asset});

    DigiByteTransaction tx;
    tx.addInput(input);
    DigiAsset changePart = asset;
    changePart.setCount(60);
    tx.addDigiAssetOutput("dgb1qchange", {changePart});
    DigiAsset burnPart = asset;
    burnPart.setCount(40);
    tx.addAssetBurn({burnPart});

    string hex = tx.encodeAssetOpReturn();
    EXPECT_EQ(hex.substr(0, 8), "44410325"); //DA v3 burn op code

    //replay through the decoder
    DigiByteTransaction rx;
    rx._height = 20000000;
    rx._assetTransactionVersion = 3;
    rx._inputs.push_back(input);
    AssetUTXO out0;
    out0.address = "dgb1qchange";
    out0.digibyte = 600;
    rx._outputs = {out0};

    BitIO stream = BitIO::makeHexString(hex);
    stream.movePositionTo(32);
    rx.decodeAssetTransfer(stream, rx._inputs, 3 /*DIGIASSET_BURN*/);

    //change kept 60, the other 40 went nowhere(burned)
    ASSERT_EQ(rx._outputs[0].assets.size(), (size_t) 1);
    EXPECT_EQ(rx._outputs[0].assets[0].getCount(), (uint64_t) 60);
}

TEST(DigiByteTransactionBuilder, burnAllRoundTrip) {
    //burning the full input needs no asset outputs at all
    DigiAsset asset = makeTestAsset(5, 25);
    AssetUTXO input = makeInput("aa11", 0, {asset});

    DigiByteTransaction tx;
    tx.addInput(input);
    DigiAsset burnPart = asset;
    burnPart.setCount(25);
    tx.addAssetBurn({burnPart});

    string hex = tx.encodeAssetOpReturn();
    EXPECT_EQ(hex.substr(0, 8), "44410325");

    DigiByteTransaction rx;
    rx._height = 20000000;
    rx._assetTransactionVersion = 3;
    rx._inputs.push_back(input);
    AssetUTXO out0;
    out0.address = "dgb1qanything";
    out0.digibyte = 600;
    rx._outputs = {out0};

    BitIO stream = BitIO::makeHexString(hex);
    stream.movePositionTo(32);
    rx.decodeAssetTransfer(stream, rx._inputs, 3 /*DIGIASSET_BURN*/);
    EXPECT_TRUE(rx._outputs[0].assets.empty()); //everything burned
}

TEST(DigiByteTransactionBuilder, burnUnbalancedThrows) {
    //burn + change that don't cover the inputs must be rejected
    DigiAsset asset = makeTestAsset(5, 100);
    AssetUTXO input = makeInput("aa11", 0, {asset});

    DigiByteTransaction tx;
    tx.addInput(input);
    DigiAsset burnPart = asset;
    burnPart.setCount(40); //60 units unaccounted for
    tx.addAssetBurn({burnPart});
    EXPECT_THROW(tx.encodeAssetOpReturn(), DigiByteTransaction::exception);
}
