//
// Created by mctrivia on 14/01/24.
//

#ifndef DIGIASSET_CORE_UTILS_H
#define DIGIASSET_CORE_UTILS_H

#include <jsoncpp/json/value.h>
#include <limits>
#include <string>
#include <vector>

namespace utils {
    enum class CodeType {
        NUMERIC,
        UPPERCASE,
        ALPHANUMERIC
    };


    void printProgressBar(float fraction, int progressWidth=60);
    std::vector<std::string> split(const std::string& s, char delimiter);
    std::string generateRandom(unsigned char length, CodeType type);
    bool fileExists(const std::string& fileName);
    bool isInteger(const std::string& s);
    void printJson(const Json::Value& params);  //added to make debugging easier
    bool copyFile(const std::string& sourcePath, const std::string& destinationPath);
    size_t estimateJsonMemoryUsage(const Json::Value& value);

    bool getAnswerBool();
    int getAnswerInt(int min=std::numeric_limits<int>::min(), int max=std::numeric_limits<int>::max());
    std::string getAnswerString(const std::string& regexPattern = "");
} // namespace utils

#endif //DIGIASSET_CORE_UTILS_H
