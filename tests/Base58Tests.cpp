//
// Created by mctrivia on 18/06/23.
//

#include <cmath>
#include "gtest/gtest.h"
#include "Base58.h"

#include <vector>
#include <random>
#include <string>
#include <algorithm>
#include <iomanip>
#include <iostream>

using namespace std;


/*
 * ██████╗░██████╗░██╗██╗░░░██╗░█████╗░████████╗███████╗
 * ██╔══██╗██╔══██╗██║██║░░░██║██╔══██╗╚══██╔══╝██╔════╝
 * ██████╔╝██████╔╝██║╚██╗░██╔╝███████║░░░██║░░░█████╗░░
 * ██╔═══╝░██╔══██╗██║░╚████╔╝░██╔══██║░░░██║░░░██╔══╝░░
 * ██║░░░░░██║░░██║██║░░╚██╔╝░░██║░░██║░░░██║░░░███████╗
 * ╚═╝░░░░░╚═╝░░╚═╝╚═╝░░░╚═╝░░░╚═╝░░╚═╝░░░╚═╝░░░╚══════╝
 */

TEST(base58, Hand) {
    vector<uint8_t> test{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
                         26, 27};

    EXPECT_EQ(Base58::encode(test), "18uEYg7a7p8DdMuZugMSsYkUUiRiLz2F5XKcv");
}

TEST(base58, RandomTest) {

    std::random_device device;
    std::mt19937 generator(device());
    std::uniform_int_distribution<uint8_t> d1(1, 100);
    std::uniform_int_distribution<uint8_t> d2(0, 255);

    auto create_data = [&]() -> std::vector<uint8_t> {
        std::vector<uint8_t> data(d1(generator));
        for (uint8_t& v: data) {
            v = static_cast<uint8_t>(d2(generator));
        }
        return data;
    };

    std::vector<uint8_t> test_data, decoded_data;
    std::string encoded_data;

    for (size_t i = 0; i < 10000; i++) {
        test_data = create_data();
        encoded_data = Base58::encode(test_data);
        decoded_data = Base58::decode(encoded_data);

        EXPECT_TRUE(test_data.size() == decoded_data.size() && test_data == decoded_data);
    }
}