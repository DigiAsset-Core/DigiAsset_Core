//
// Created by mctrivia on 20/06/23.
//

#ifndef DIGIASSET_CORE_BASE58_H
#define DIGIASSET_CORE_BASE58_H

#include <cstdint>
#include <string>
#include <vector>

class Base58 {
    //static const uint8_t map[];


    static const uint8_t base58Map[];
    static const uint8_t alphaMap[];


public:
    static std::string encode(const std::vector<uint8_t>& data);
    static std::vector<uint8_t> decode(const std::string& data);
};


#endif //DIGIASSET_CORE_BASE58_H
