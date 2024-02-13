//
// Created by mctrivia on 19/07/23.
//

#include "DigiAsset.h"
#include "DigiByteCore.h"
#include "DigiByteTransaction.h"
#include "gtest/gtest.h"
#include <cmath>

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <vector>

using namespace std;

std::string uint8_to_hex_string(const vector<uint8_t>& v) {
    std::stringstream ss;

    ss << std::hex << std::setfill('0');
    size_t w = 0;

    for (size_t i = 0; i < v.size(); i++) {
        ss << std::hex << "0x" << std::setw(2) << static_cast<int>(v[i]) << ",";
        if (w++ == 8) {
            w = 0;
            ss << "\n";
        }
    }

    return ss.str();
}

TEST(DigiAssetRules, serialize) {
    //Test KYC Required Only
    DigiAssetRules test;
    test.setRequireKYC();
    vector<uint8_t> serialized;
    serialize(serialized, test);
    vector<uint8_t> expected{0x60, 0x00};
    EXPECT_EQ(serialized, expected);

    //Test KYC Required allowing only Canada
    test = DigiAssetRules();
    test.setRequireKYC({"CAD"});
    serialized = {};
    serialize(serialized, test);
    expected = {0x40, 0x20, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 3, 0x43, 0x41, 0x44};
    EXPECT_EQ(serialized, expected);

    //Test KYC Required allowing only Canada, US and rules are modifiable
    test = DigiAssetRules();
    test.setRewritable();
    test.setRequireKYC({"CAD", "USA"});
    serialized = {};
    serialize(serialized, test);
    expected = {0xc0, 0x20, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 3, 0x43, 0x41, 0x44, 0, 0, 0, 0, 0, 0, 0, 3,
                0x55, 0x53, 0x41};
    EXPECT_EQ(serialized, expected);

    //Test fails when trying to ban all countries
    try {
        test.setRequireKYC({}, false);
        EXPECT_FALSE(true); //should not run
    } catch (const exception& e) {
        EXPECT_TRUE(true);
    }

    //Try banning only USA
    test = DigiAssetRules();
    test.setRequireKYC({"USA"}, true);
    serialized = {};
    serialize(serialized, test);
    expected = {0x60, 0x20, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 3, 0x55, 0x53, 0x41};
    EXPECT_EQ(serialized, expected);

    //Try requiring signers
    test = DigiAssetRules();
    test.setRequireSigners(3, {
                                      {.address = "fake1", .weight = 3},
                                      {.address = "fake2", .weight = 2},
                                      {.address = "fake3", .weight = 2},
                                      {.address = "fake4", .weight = 1},
                              });
    serialized = {};
    serialize(serialized, test);
    expected = {0x40, 0x80, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 4,
                0, 0, 0, 0, 0, 0, 0, 5, 0x66, 0x61, 0x6b, 0x65, 0x31, 0, 0, 0, 0, 0, 0, 0, 3,
                0, 0, 0, 0, 0, 0, 0, 5, 0x66, 0x61, 0x6b, 0x65, 0x32, 0, 0, 0, 0, 0, 0, 0, 2,
                0, 0, 0, 0, 0, 0, 0, 5, 0x66, 0x61, 0x6b, 0x65, 0x33, 0, 0, 0, 0, 0, 0, 0, 2,
                0, 0, 0, 0, 0, 0, 0, 5, 0x66, 0x61, 0x6b, 0x65, 0x34, 0, 0, 0, 0, 0, 0, 0, 1};
    EXPECT_EQ(serialized, expected);

    //try requiring royalties(1 dgb)
    test = DigiAssetRules();
    test.setRoyalties({{.address = "fake1", .amount = 100000000}});
    serialized = {};
    serialize(serialized, test);
    expected = {0x40, 0x40, 0, 0, 0, 0, 0, 0, 0, 1,
                0, 0, 0, 0, 0, 0, 0, 5, 0x66, 0x61, 0x6b, 0x65, 0x31, 0, 0, 0, 0, 0x05, 0xf5, 0xe1, 0x00};
    EXPECT_EQ(serialized, expected);

    //try requiring royalties(1 CAD)
    test = DigiAssetRules();
    test.setRoyalties({{.address = "fake1", .amount = 100000000}}, DigiAsset::standardExchangeRates[0]);
    serialized = {};
    serialize(serialized, test);
    expected = {0x40, 0x42,
                0, 0, 0, 0, 0, 0, 0, 1,                               //number of addresses
                0, 0, 0, 0, 0, 0, 0, 5, 0x66, 0x61, 0x6b, 0x65, 0x31, //address to pay to
                0, 0, 0, 0, 0x05, 0xf5, 0xe1, 0x00,                   //amount to pay
                0, 0, 0, 0, 0, 0, 0, 43, 0x64, 0x67, 0x62, 0x31, 0x71, 0x75, 0x6E, 0x78, 0x68, 0x33, 0x37, 0x38, 0x65,
                0x6C, 0x74, 0x6A, 0x32, 0x6A, 0x72, 0x77, 0x7A, 0x61, 0x35, 0x73, 0x6A, 0x39, 0x67, 0x72, 0x76, 0x75,
                0x35, 0x78, 0x75, 0x64, 0x34, 0x33, 0x76, 0x71, 0x76, 0x75, 0x64, 0x77, 0x68, //rate address
                0,                                                                            //rate index
                0, 0, 0, 0, 0, 0, 0, 3, 0x43, 0x41, 0x44};                                    //serialize name
    EXPECT_EQ(serialized, expected);

    //try rewritable, royalties 5 DGB - same as c57fc42847ebf7b3842fde56ed3ef1897d330413d3325e6b2043b78b5ed7f3fa
    test = DigiAssetRules();
    test.setRewritable(true);
    test.setRoyalties({{.address = "fake1", .amount = 500000000}});
    serialized = {};
    serialize(serialized, test);
    expected = {0xc0, 0x40,
                0, 0, 0, 0, 0, 0, 0, 1,
                0, 0, 0, 0, 0, 0, 0, 5, 0x66, 0x61, 0x6b, 0x65, 0x31,
                0, 0, 0, 0, 0x1d, 0xcd, 0x65, 0x00};
    EXPECT_EQ(serialized, expected);

    //try royalty 24.75 and 0.25 like in 500f85a4bfcedc462702e5d86c55cbc058b8f758db712b97d0067e73e5243848
    test = DigiAssetRules();
    test.setRoyalties({
            {.address = "dgb1qkspymp9gk4rht2f43qrrpdkd6k05h82j0sykr0", .amount = 2475000000},
            {.address = "dgb1qwlnzswupjvlczfclqxgwcsgzlzf803yzzv97q8", .amount = 25000000},
    });
    serialized = {};
    serialize(serialized, test);
    expected = {0x40, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2b,
                0x64, 0x67, 0x62, 0x31, 0x71, 0x6b, 0x73, 0x70, 0x79,
                0x6d, 0x70, 0x39, 0x67, 0x6b, 0x34, 0x72, 0x68, 0x74,
                0x32, 0x66, 0x34, 0x33, 0x71, 0x72, 0x72, 0x70, 0x64,
                0x6b, 0x64, 0x36, 0x6b, 0x30, 0x35, 0x68, 0x38, 0x32,
                0x6a, 0x30, 0x73, 0x79, 0x6b, 0x72, 0x30, 0x00, 0x00,
                0x00, 0x00, 0x93, 0x85, 0x80, 0xc0, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x2b, 0x64, 0x67, 0x62, 0x31,
                0x71, 0x77, 0x6c, 0x6e, 0x7a, 0x73, 0x77, 0x75, 0x70,
                0x6a, 0x76, 0x6c, 0x63, 0x7a, 0x66, 0x63, 0x6c, 0x71,
                0x78, 0x67, 0x77, 0x63, 0x73, 0x67, 0x7a, 0x6c, 0x7a,
                0x66, 0x38, 0x30, 0x33, 0x79, 0x7a, 0x7a, 0x76, 0x39,
                0x37, 0x71, 0x38, 0x00, 0x00, 0x00, 0x00, 0x01, 0x7d,
                0x78, 0x40};
    EXPECT_EQ(serialized, expected);

    //todo add more tests to make sure all combinations of rules have been tried
}


TEST(DigiAssetRules, deserialize) {
    size_t i = 0;

    //Test KYC Required Only
    vector<uint8_t> serialized = {0x60, 0x00};
    DigiAssetRules test;
    i = 0;
    deserialize(serialized, i, test);
    DigiAssetRules expected;
    expected.setRequireKYC();
    EXPECT_TRUE(test == expected);

    //Test KYC Required allowing only Canada
    serialized = {0x40, 0x20, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 3, 0x43, 0x41, 0x44};
    test = DigiAssetRules();
    i = 0;
    deserialize(serialized, i, test);
    expected = DigiAssetRules();
    expected.setRequireKYC({"CAD"});
    EXPECT_TRUE(test == expected);

    //Test KYC Required allowing only Canada, US and rules are modifiable
    serialized = {0xc0, 0x20, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 3, 0x43, 0x41, 0x44, 0, 0, 0, 0, 0, 0, 0, 3,
                  0x55, 0x53, 0x41};
    test = DigiAssetRules();
    i = 0;
    deserialize(serialized, i, test);
    expected = DigiAssetRules();
    expected.setRewritable();
    expected.setRequireKYC({"CAD", "USA"});
    EXPECT_TRUE(test == expected);

    //Try banning only USA
    serialized = {0x60, 0x20, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 3, 0x55, 0x53, 0x41};
    test = DigiAssetRules();
    i = 0;
    deserialize(serialized, i, test);
    expected = DigiAssetRules();
    expected.setRequireKYC({"USA"}, true);
    EXPECT_TRUE(test == expected);

    //Try requiring signers
    serialized = {0x40, 0x80, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 4,
                  0, 0, 0, 0, 0, 0, 0, 5, 0x66, 0x61, 0x6b, 0x65, 0x31, 0, 0, 0, 0, 0, 0, 0, 3,
                  0, 0, 0, 0, 0, 0, 0, 5, 0x66, 0x61, 0x6b, 0x65, 0x32, 0, 0, 0, 0, 0, 0, 0, 2,
                  0, 0, 0, 0, 0, 0, 0, 5, 0x66, 0x61, 0x6b, 0x65, 0x33, 0, 0, 0, 0, 0, 0, 0, 2,
                  0, 0, 0, 0, 0, 0, 0, 5, 0x66, 0x61, 0x6b, 0x65, 0x34, 0, 0, 0, 0, 0, 0, 0, 1};
    test = DigiAssetRules();
    i = 0;
    deserialize(serialized, i, test);
    expected = DigiAssetRules();
    expected.setRequireSigners(3, {
                                          {.address = "fake1", .weight = 3},
                                          {.address = "fake2", .weight = 2},
                                          {.address = "fake3", .weight = 2},
                                          {.address = "fake4", .weight = 1},
                                  });
    EXPECT_TRUE(test == expected);

    //try requiring royalties(1 dgb)
    serialized = {0x40, 0x40, 0, 0, 0, 0, 0, 0, 0, 1,
                  0, 0, 0, 0, 0, 0, 0, 5, 0x66, 0x61, 0x6b, 0x65, 0x31, 0, 0, 0, 0, 0x05, 0xf5, 0xe1, 0x00};
    test = DigiAssetRules();
    i = 0;
    deserialize(serialized, i, test);
    expected = DigiAssetRules();
    expected.setRoyalties({{.address = "fake1", .amount = 100000000}});
    EXPECT_TRUE(test == expected);

    //try requiring royalties(1 CAD)
    serialized = {0x40, 0x42,
                  0, 0, 0, 0, 0, 0, 0, 1,                               //number of addresses
                  0, 0, 0, 0, 0, 0, 0, 5, 0x66, 0x61, 0x6b, 0x65, 0x31, //address to pay to
                  0, 0, 0, 0, 0x05, 0xf5, 0xe1, 0x00,                   //amount to pay
                  0, 0, 0, 0, 0, 0, 0, 43, 0x64, 0x67, 0x62, 0x31, 0x71, 0x75, 0x6E, 0x78, 0x68, 0x33, 0x37, 0x38, 0x65,
                  0x6C, 0x74, 0x6A, 0x32, 0x6A, 0x72, 0x77, 0x7A, 0x61, 0x35, 0x73, 0x6A, 0x39, 0x67, 0x72, 0x76, 0x75,
                  0x35, 0x78, 0x75, 0x64, 0x34, 0x33, 0x76, 0x71, 0x76, 0x75, 0x64, 0x77, 0x68, //rate address
                  0,                                                                            //rate index
                  0, 0, 0, 0, 0, 0, 0, 3, 0x43, 0x41, 0x44};                                    //serialize name;
    test = DigiAssetRules();
    i = 0;
    deserialize(serialized, i, test);
    expected = DigiAssetRules();
    expected.setRoyalties({{.address = "fake1", .amount = 100000000}}, DigiAsset::standardExchangeRates[0]);
    EXPECT_TRUE(test == expected);

    //try rewritable, royalties 5 DGB - same as c57fc42847ebf7b3842fde56ed3ef1897d330413d3325e6b2043b78b5ed7f3fa
    serialized = {0xc0, 0x40,
                  0, 0, 0, 0, 0, 0, 0, 1,
                  0, 0, 0, 0, 0, 0, 0, 5, 0x66, 0x61, 0x6b, 0x65, 0x31,
                  0, 0, 0, 0, 0x1d, 0xcd, 0x65, 0x00};
    test = DigiAssetRules();
    i = 0;
    deserialize(serialized, i, test);
    expected = DigiAssetRules();
    expected.setRewritable(true);
    expected.setRoyalties({{.address = "fake1", .amount = 500000000}});
    EXPECT_TRUE(test == expected);

    //try royalty 24.75 and 0.25 like in 500f85a4bfcedc462702e5d86c55cbc058b8f758db712b97d0067e73e5243848

    serialized = {0x40, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                  0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2b,
                  0x64, 0x67, 0x62, 0x31, 0x71, 0x6b, 0x73, 0x70, 0x79,
                  0x6d, 0x70, 0x39, 0x67, 0x6b, 0x34, 0x72, 0x68, 0x74,
                  0x32, 0x66, 0x34, 0x33, 0x71, 0x72, 0x72, 0x70, 0x64,
                  0x6b, 0x64, 0x36, 0x6b, 0x30, 0x35, 0x68, 0x38, 0x32,
                  0x6a, 0x30, 0x73, 0x79, 0x6b, 0x72, 0x30, 0x00, 0x00,
                  0x00, 0x00, 0x93, 0x85, 0x80, 0xc0, 0x00, 0x00, 0x00,
                  0x00, 0x00, 0x00, 0x00, 0x2b, 0x64, 0x67, 0x62, 0x31,
                  0x71, 0x77, 0x6c, 0x6e, 0x7a, 0x73, 0x77, 0x75, 0x70,
                  0x6a, 0x76, 0x6c, 0x63, 0x7a, 0x66, 0x63, 0x6c, 0x71,
                  0x78, 0x67, 0x77, 0x63, 0x73, 0x67, 0x7a, 0x6c, 0x7a,
                  0x66, 0x38, 0x30, 0x33, 0x79, 0x7a, 0x7a, 0x76, 0x39,
                  0x37, 0x71, 0x38, 0x00, 0x00, 0x00, 0x00, 0x01, 0x7d,
                  0x78, 0x40};
    test = DigiAssetRules();
    i = 0;
    deserialize(serialized, i, test);
    expected = DigiAssetRules();
    expected.setRoyalties({
            {.address = "dgb1qkspymp9gk4rht2f43qrrpdkd6k05h82j0sykr0", .amount = 2475000000},
            {.address = "dgb1qwlnzswupjvlczfclqxgwcsgzlzf803yzzv97q8", .amount = 25000000},
    });
    EXPECT_TRUE(test == expected);
    //todo add more tests to make sure all combinations of rules have been tried
}
