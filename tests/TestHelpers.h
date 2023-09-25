//
// Created by mctrivia on 02/08/23.
//

#ifndef DIGIASSET_CORE_TESTHELPERS_H
#define DIGIASSET_CORE_TESTHELPERS_H


#include <cstdint>
#include <string>
#include <vector>

class TestHelpers {
public:
    static std::string getCSVValue(const std::string& line, size_t& li);
    static std::vector<uint8_t> hexToVector(const std::string& value);
    static bool approximatelyEqual(double a, double b);
};


#endif //DIGIASSET_CORE_TESTHELPERS_H
