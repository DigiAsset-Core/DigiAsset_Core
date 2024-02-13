//
// Created by mctrivia on 14/01/24.
//

#include "utils.h"
#include <random>
#include <sstream>
#include <sys/stat.h>

namespace utils {

    /**
     * Function that splits a string by some kind of delimiter and returns it as a vector of strings
     * @param s - input string
     * @param delimiter - delimiter
     * @return
     */
    std::vector<std::string> split(const std::string& s, char delimiter) {
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream tokenStream(s);
        while (std::getline(tokenStream, token, delimiter)) {
            tokens.push_back(token);
        }
        return tokens;
    }

    /**
     * Generates a random string of a specific length
     * @param length
     * @param type
     * @return
     */
    std::string generateRandom(unsigned char length, CodeType type) {
        const std::string numerics = "0123456789";
        const std::string uppercases = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        const std::string alphanumerics = numerics + uppercases;

        std::string characters;
        switch (type) {
            case CodeType::NUMERIC:
                characters = numerics;
                break;
            case CodeType::UPPERCASE:
                characters = uppercases;
                break;
            case CodeType::ALPHANUMERIC:
                characters = alphanumerics;
                break;
        }

        std::random_device rd;
        std::mt19937 generator(rd());
        std::uniform_int_distribution<> distribution(0, characters.size() - 1);

        std::string randomString;
        for (unsigned char i = 0; i < length; ++i) {
            randomString += characters[distribution(generator)];
        }

        return randomString;
    }


    bool fileExists(const std::string& fileName) {
        //see if this is first run
        struct stat buffer {};
        return (stat(fileName.c_str(), &buffer) == 0);
    }
} // namespace utils