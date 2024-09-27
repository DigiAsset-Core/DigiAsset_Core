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
    uint64_t mod256by64(const std::array<uint8_t, 32>& numerator, uint64_t divisor);

    Json::Value fromJSON(const std::string& str);

    bool getAnswerBool();
    int getAnswerInt(int min=std::numeric_limits<int>::min(), int max=std::numeric_limits<int>::max());
    std::string getAnswerString(const std::string& regexPattern = "");


    bool isValidAddress(const std::string& address);

    template<typename T>
    std::vector<T> concatenate(const std::vector<T>& v1, const std::vector<T>& v2) {
        std::vector<T> result = v1;                        // Copy the first vector
        result.insert(result.end(), v2.begin(), v2.end()); // Append the second vector
        return result;
    }
} // namespace utils

#endif //DIGIASSET_CORE_UTILS_H
