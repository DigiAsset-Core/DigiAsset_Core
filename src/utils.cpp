//
// Created by mctrivia on 14/01/24.
//

#include "utils.h"
#include <jsoncpp/json/value.h>
#include <random>
#include <sstream>
#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <algorithm>

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

    /**
     * Returns if a string contains an integer
     * @param s
     * @return
     */
    bool isInteger(const std::string& s) {
        std::istringstream iss(s);
        int n;
        iss >> n;
        return iss.eof() && !iss.fail(); // Check if reading was successful and the entire string was consumed
    }

    /**
     * Function to help debug Json values
     * @param params
     */
    void printJson(const Json::Value& params) {
        std::cout << params.toStyledString() << std::endl;
    }


    bool copyFile(const std::string& sourcePath, const std::string& destinationPath) {
        std::ifstream source(sourcePath, std::ios::binary);
        std::ofstream destination(destinationPath, std::ios::binary);

        // Check if the source file and destination file are opened successfully
        if (!source.is_open() || !destination.is_open()) {
            std::cerr << "Error opening files!" << std::endl;
            return false;
        }

        // Copy the file
        destination << source.rdbuf();

        // Check if the copying was successful
        if (!source || !destination) {
            std::cerr << "Error copying the file!" << std::endl;
            return false;
        }

        return true;
    }

    /**
     * Erases last line and prints a progress bar
     * @param fraction
     * @param progressWidth
     */
    void printProgressBar(float fraction, int progressWidth) {
        int left=std::max(static_cast<int>(fraction*progressWidth),1);
        int right=progressWidth-left;
        std::cout << "\r[" << std::setfill('#') << std::setw(left) << '#';
        std::cout << std::setfill(' ') << std::setw(right) << "]";
        std::cout << std::fixed << std::setprecision(1) << std::setw(5) << (fraction*100) << "%";
        std::cout.flush();
    }
} // namespace utils