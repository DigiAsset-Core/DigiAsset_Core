//
// Created by mctrivia on 27/06/24.
//

#include <cmath>
#include "gtest/gtest.h"
#include "Bech32.h"

#include <vector>
#include <random>
#include <string>
#include <algorithm>
#include <iostream>

using namespace std;


TEST(bech32, Hand) {
    const vector<uint8_t> expected={0,2,26,27,28,31,21,25,13,30,15,28,29,6,1,9,17,19,7,24,3,9,0,28,5,13,11,1,20,16,16,13,5};

    //try decoding
    try {
        auto decoded = Bech32::Decode("dgb1qz6mul4ed70uaxpf3n8crfqu9dtp5ssd9ckehjh");
        EXPECT_EQ(decoded.hrp,"dgb");
        EXPECT_EQ(decoded.encoding,Bech32::Encoding::BECH32);
        for (size_t i=0; i<expected.size(); i++) {
            EXPECT_EQ(decoded.data[i],expected[i]);
        }
    } catch (...) {
        EXPECT_TRUE(false);
    }

    //try encoding
    try {
        auto encoded=Bech32::Encode(Bech32::Encoding::BECH32,"dgb",expected);
        EXPECT_EQ(encoded,"dgb1qz6mul4ed70uaxpf3n8crfqu9dtp5ssd9ckehjh");
    } catch (...) {
        EXPECT_TRUE(false);
    }

    //add intentional error
    try {
        auto decoded = Bech32::Decode("dgb1qz6mul4ed70uaxpf3n8crfqu9dtp5ssd9ckehjj");
        EXPECT_EQ(decoded.encoding,Bech32::Encoding::INVALID);
    } catch (...) {
        EXPECT_TRUE(false);
    }

}

TEST(bech32, RandomTest) {
    std::random_device device;
    std::mt19937 generator(device());
    std::uniform_int_distribution<uint8_t> d1(0, 31);

    auto create_data = [&]() -> std::vector<uint8_t> {
        std::vector<uint8_t> data(33);
        for (uint8_t& v: data) {
            v = static_cast<uint8_t>(d1(generator));
        }
        return data;
    };

    std::vector<uint8_t> expected;
    std::string encoded_data;

    for (size_t i = 0; i < 10000; i++) {
        //create random address
        expected = create_data();
        encoded_data = Bech32::Encode(Bech32::Encoding::BECH32,"dgb",expected);

        //decode address
        auto decoded = Bech32::Decode(encoded_data);

        //make sure the same
        EXPECT_EQ(decoded.hrp,"dgb");
        EXPECT_EQ(decoded.encoding,Bech32::Encoding::BECH32);
        for (size_t i=0; i<expected.size(); i++) {
            EXPECT_EQ(decoded.data[i],expected[i]);
        }
    }
}