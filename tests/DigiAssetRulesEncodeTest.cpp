//
// Round trip tests for DigiAssetRules::encode/getRequiredOutputs.
// Every test encodes a rules object to a bitstream, then decodes it with the SAME
// constructor the chain analyzer uses on real chain data and compares the two.
// Pure unit tests - no wallet, database or IPFS node needed(standard exchange rates
// and standard vote addresses only - custom royalty units need the database).
//

#include "DigiAsset.h"
#include "DigiAssetConstants.h"
#include "DigiAssetRules.h"
#include "DigiByteTransaction.h"
#include "gtest/gtest.h"

using namespace std;

namespace {
    ///builds the fake transaction the decoder needs: output 0 is the asset output, rule
    ///outputs(from getRequiredOutputs) follow starting at vout 1
    getrawtransaction_t makeTxData(const DigiAssetRules& rules) {
        getrawtransaction_t txData;
        vout_t assetOut;
        assetOut.n = 0;
        assetOut.valueS = 10000;
        assetOut.scriptPubKey.addresses = {"dgb1qassetrecipient"};
        txData.vout.push_back(assetOut);
        unsigned int n = 1;
        for (const DigiAssetRules::RuleOutput& out: rules.getRequiredOutputs()) {
            vout_t ruleOut;
            ruleOut.n = n++;
            ruleOut.valueS = out.sats;
            ruleOut.scriptPubKey.addresses = {out.address};
            txData.vout.push_back(ruleOut);
        }
        return txData;
    }

    ///encodes rules then decodes them with the chain decoder and returns the result
    DigiAssetRules roundTrip(const DigiAssetRules& rules) {
        getrawtransaction_t txData = makeTxData(rules);
        BitIO stream;
        rules.encode(stream, 1); //rule outputs start at vout 1
        stream.movePositionToBeginning();
        return DigiAssetRules(txData, stream, "testcid", rules.isRewritable() ? 3 : 4);
    }
} // namespace


TEST(DigiAssetRulesEncode, approvalRoundTrip) {
    DigiAssetRules rules;
    rules.setRequireSigners(2, {Signer{"dgb1qsigner1", 1},
                                Signer{"dgb1qsigner2", 1},
                                Signer{"dgb1qsigner3", 3}});
    DigiAssetRules decoded = roundTrip(rules);
    EXPECT_TRUE(decoded == rules);
    EXPECT_EQ(decoded.getRequiredSignerWeight(), (uint64_t) 2);
    ASSERT_EQ(decoded.getSigners().size(), (size_t) 3);
    EXPECT_EQ(decoded.getSigners()[2].weight, (uint64_t) 3);
}

TEST(DigiAssetRulesEncode, royaltiesRoundTrip) {
    DigiAssetRules rules;
    rules.setRoyalties({Royalty{"dgb1qroyalty1", 100000000},
                        Royalty{"dgb1qroyalty2", 25000000}});
    DigiAssetRules decoded = roundTrip(rules);
    EXPECT_TRUE(decoded == rules);
    ASSERT_EQ(decoded.getRoyalties().size(), (size_t) 2);
    EXPECT_EQ(decoded.getRoyalties()[0].amount, (uint64_t) 100000000);
}

TEST(DigiAssetRulesEncode, royaltiesWithUnitsRoundTrip) {
    DigiAssetRules rules;
    rules.setRoyalties({Royalty{"dgb1qroyalty1", 50000000}},
                       DigiAssetConstants::standardExchangeRates[1]); //USD
    DigiAssetRules decoded = roundTrip(rules);
    EXPECT_TRUE(decoded == rules);
    EXPECT_EQ(decoded.getRoyaltyCurrency().name, "USD");
}

TEST(DigiAssetRulesEncode, geofenceAllowedRoundTrip) {
    DigiAssetRules rules;
    rules.setRequireKYC({"CAN", "USA", "DEU"}, false);
    DigiAssetRules decoded = roundTrip(rules);
    EXPECT_TRUE(decoded == rules);
    EXPECT_TRUE(decoded.getIfGeoFenced());
    EXPECT_TRUE(decoded.getIfCountryAllowedToReceive("USA"));
    EXPECT_FALSE(decoded.getIfCountryAllowedToReceive("FRA"));
}

TEST(DigiAssetRulesEncode, geofenceDeniedRoundTrip) {
    DigiAssetRules rules;
    rules.setRequireKYC({"PRK"}, true);
    DigiAssetRules decoded = roundTrip(rules);
    EXPECT_TRUE(decoded == rules);
    EXPECT_FALSE(decoded.getIfCountryAllowedToReceive("PRK"));
    EXPECT_TRUE(decoded.getIfCountryAllowedToReceive("CAN"));
}

TEST(DigiAssetRulesEncode, kycAnyCountryRoundTrip) {
    DigiAssetRules rules;
    rules.setRequireKYC(); //ban list, no countries = KYC required from anywhere
    DigiAssetRules decoded = roundTrip(rules);
    EXPECT_TRUE(decoded == rules);
    EXPECT_TRUE(decoded.getIfGeoFenced());
    EXPECT_TRUE(decoded.getIfCountryAllowedToReceive("CAN"));
}

TEST(DigiAssetRulesEncode, voteStandardAddressesRoundTrip) {
    DigiAssetRules rules;
    vector<VoteOption> options;
    for (size_t i = 0; i < 3; i++) {
        options.push_back(VoteOption{DigiAssetConstants::standardVoteAddresses[i], "option"});
    }
    rules.setVote(options, 25000000, false); //cutoff at block 25M, restricted
    DigiAssetRules decoded = roundTrip(rules);
    EXPECT_TRUE(decoded == rules);
    EXPECT_TRUE(decoded.getIfVote());
    EXPECT_TRUE(decoded.getIfVoteRestricted());
    EXPECT_EQ(decoded.getExpiry(), (uint64_t) 25000000);
    EXPECT_TRUE(decoded.isExpiryHeight());
    EXPECT_TRUE(decoded.getIfValidVoteAddress(DigiAssetConstants::standardVoteAddresses[0]));
}

TEST(DigiAssetRulesEncode, voteCustomAddressesRoundTrip) {
    DigiAssetRules rules;
    rules.setVote({VoteOption{"dgb1qvoteyes", "yes"},
                   VoteOption{"dgb1qvoteno", "no"}},
                  DigiAssetRules::EXPIRE_NEVER, true);
    DigiAssetRules decoded = roundTrip(rules);
    EXPECT_TRUE(decoded == rules);
    EXPECT_TRUE(decoded.getIfValidVoteAddress("dgb1qvoteno"));
    EXPECT_FALSE(decoded.expires());
}

TEST(DigiAssetRulesEncode, expiryOnlyRoundTrip) {
    DigiAssetRules rules;
    rules.setExpiry(1800000000000); //ms epoch time(2027)
    DigiAssetRules decoded = roundTrip(rules);
    EXPECT_TRUE(decoded == rules);
    EXPECT_TRUE(decoded.expires());
    EXPECT_FALSE(decoded.isExpiryHeight());
    EXPECT_FALSE(decoded.getIfVote());
}

TEST(DigiAssetRulesEncode, deflationRoundTrip) {
    DigiAssetRules rules;
    rules.setDeflationary(500);
    DigiAssetRules decoded = roundTrip(rules);
    EXPECT_TRUE(decoded == rules);
    EXPECT_EQ(decoded.getRequiredBurn(), (uint64_t) 500);
}

TEST(DigiAssetRulesEncode, kitchenSinkRoundTrip) {
    //every rule type that can coexist, all at once
    DigiAssetRules rules;
    rules.setRequireSigners(5, {Signer{"dgb1qsigner1", 5}});
    rules.setRoyalties({Royalty{"dgb1qroyalty", 200000000}},
                       DigiAssetConstants::standardExchangeRates[0]); //CAD
    rules.setRequireKYC({"CAN", "USA"}, false);
    rules.setVote({VoteOption{DigiAssetConstants::standardVoteAddresses[0], "a"},
                   VoteOption{DigiAssetConstants::standardVoteAddresses[1], "b"}},
                  26000000, true);
    rules.setDeflationary(10);
    DigiAssetRules decoded = roundTrip(rules);
    EXPECT_TRUE(decoded == rules);
}

TEST(DigiAssetRulesEncode, rewritableRoundTrip) {
    //changeable rules use op code 3
    DigiAssetRules rules;
    rules.setRewritable(true);
    rules.setRoyalties({Royalty{"dgb1qroyalty", 100000000}});
    DigiAssetRules decoded = roundTrip(rules);
    EXPECT_TRUE(decoded == rules);
    EXPECT_TRUE(decoded.isRewritable());
}

TEST(DigiAssetRulesEncode, requiredOutputsOrderAndValues) {
    DigiAssetRules rules;
    rules.setRequireSigners(1, {Signer{"dgb1qsigner", 7}});
    rules.setRoyalties({Royalty{"dgb1qroyalty", 123456789}});
    rules.setVote({VoteOption{"dgb1qvote", "x"}}, DigiAssetRules::EXPIRE_NEVER, true);

    vector<DigiAssetRules::RuleOutput> outputs = rules.getRequiredOutputs();
    ASSERT_EQ(outputs.size(), (size_t) 3);
    EXPECT_EQ(outputs[0].address, "dgb1qsigner"); //signers first(address only, dust value)
    EXPECT_EQ(outputs[0].sats, DigiAssetConstants::DIGIBYTE_DUST);
    EXPECT_EQ(outputs[1].address, "dgb1qroyalty"); //royalty value IS the price
    EXPECT_EQ(outputs[1].sats, (uint64_t) 123456789);
    EXPECT_EQ(outputs[2].address, "dgb1qvote"); //custom vote addresses last
    EXPECT_EQ(outputs[2].sats, DigiAssetConstants::DIGIBYTE_DUST);
}

TEST(DigiAssetRulesEncode, standardVoteAddressesNeedNoOutputs) {
    DigiAssetRules rules;
    rules.setVote({VoteOption{DigiAssetConstants::standardVoteAddresses[0], "a"}});
    EXPECT_TRUE(rules.getRequiredOutputs().empty());
}

TEST(DigiAssetRulesEncode, subDustRoyaltyThrows) {
    DigiAssetRules rules;
    rules.setRoyalties({Royalty{"dgb1qroyalty", 600}}); //below the v8.22 dust threshold
    EXPECT_THROW(rules.getRequiredOutputs(), DigiAssetRules::exception);
}

TEST(DigiAssetRulesEncode, customExchangeRateThrows) {
    DigiAssetRules rules;
    rules.setRoyalties({Royalty{"dgb1qroyalty", 100000000}},
                       ExchangeRate{"dgb1qcustomrateaddress", 4, "?"});
    BitIO stream;
    EXPECT_THROW(rules.encode(stream, 1), DigiAssetRules::exception);
}

TEST(DigiAssetRulesEncode, voteOnRewritableThrows) {
    DigiAssetRules rules;
    rules.setRewritable(true);
    rules.setVote({VoteOption{DigiAssetConstants::standardVoteAddresses[0], "a"}});
    BitIO stream;
    EXPECT_THROW(rules.encode(stream, 1), DigiAssetRules::exception);
}

TEST(DigiAssetRulesEncode, issuanceOpReturnUsesRuleOpcodes) {
    //locked asset with rules encodes op code 4, changeable rules encode op code 3
    DigiAssetRules rules;
    rules.setDeflationary(1);
    //cid must be a valid raw sha256 cid for cidToSha256 - reuse a real one
    string cid = "bafkreibq35mdomkc56ppqgvblb4aqdvormcdo74jhcigzbgulwwsoz7hmy";

    DigiAsset locked(cid, 100, 0, true, DigiAsset::AGGREGABLE, rules);
    DigiByteTransaction tx;
    tx.setIssuance(locked);
    tx.addDigiAssetOutput("dgb1qassetrecipient", {locked});
    tx.addRuleOutputs();
    string hex = tx.encodeAssetOpReturn();
    EXPECT_EQ(hex.substr(0, 8), "44410304"); //DA v3 op code 4(locked rules)

    DigiAssetRules changeable;
    changeable.setRewritable(true);
    changeable.setDeflationary(1);
    DigiAsset unlocked(cid, 100, 0, false, DigiAsset::AGGREGABLE, changeable);
    DigiByteTransaction tx2;
    tx2.setIssuance(unlocked);
    tx2.addDigiAssetOutput("dgb1qassetrecipient", {unlocked});
    tx2.addRuleOutputs();
    string hex2 = tx2.encodeAssetOpReturn();
    EXPECT_EQ(hex2.substr(0, 8), "44410303"); //DA v3 op code 3(rewritable rules)
}

TEST(DigiAssetRulesEncode, rulesWithoutMetadataThrows) {
    DigiAssetRules rules;
    rules.setDeflationary(1);
    DigiAsset asset("", 100, 0, true, DigiAsset::AGGREGABLE, rules); //no cid
    DigiByteTransaction tx;
    tx.setIssuance(asset);
    tx.addDigiAssetOutput("dgb1qassetrecipient", {asset});
    tx.addRuleOutputs();
    EXPECT_THROW(tx.encodeAssetOpReturn(), DigiByteTransaction::exception);
}
