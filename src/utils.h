//
// Created by mctrivia on 14/01/24.
//

#ifndef DIGIASSET_CORE_UTILS_H
#define DIGIASSET_CORE_UTILS_H

#include <jsoncpp/json/value.h>
#include <string>
#include <vector>
namespace utils {
    enum class CodeType {
        NUMERIC,
        UPPERCASE,
        ALPHANUMERIC
    };


    std::vector<std::string> split(const std::string& s, char delimiter);
    std::string generateRandom(unsigned char length, CodeType type);
    bool fileExists(const std::string& fileName);
    bool isInteger(const std::string& s);
    void printJson(const Json::Value& params);  //added to make debugging easier

} // namespace utils

#endif //DIGIASSET_CORE_UTILS_H
