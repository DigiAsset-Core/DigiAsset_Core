//
// Created by mctrivia on 26/07/23.
//

#include "TestHelpers.h"
#include "BitIO.h"

#include <vector>
#include <string>
#include <cmath>

using namespace std;

string TestHelpers::getCSVValue(const string& line, size_t& li) {
    string result;
    while ((line[li] != '\n') && (line[li] != ',')) {
        result.push_back(line[li]);
        li++;
    }
    li++;
    return result;
}

vector<uint8_t> TestHelpers::hexToVector(const string& value) {
    const std::string charSet = BITIO_CHARSET_HEX;
    vector<uint8_t> result;
    for (size_t i = 0; i < value.size(); i += 2) {
        uint8_t hNibble = charSet.find(value[i]);
        uint8_t lNibble = charSet.find(value[i + 1]);
        result.push_back((hNibble << 4) | lNibble);
    }
    return result;
}

bool TestHelpers::approximatelyEqual(double a, double b) {
    return fabs(a - b) <= ((fabs(a) < fabs(b) ? fabs(b) : fabs(a)) * std::numeric_limits<double>::epsilon());
}
