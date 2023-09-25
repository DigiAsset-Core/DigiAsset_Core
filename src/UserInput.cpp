//
// Created by mctrivia on 10/02/23.
//

#include "UserInput.h"

#include <iostream>
#include <string>
#include <algorithm>

using namespace std;

bool UserInput::getBool(const std::string& question) {
    while (true) {
        cout << "\n" << question;
        string input;
        cin >> input;
        transform(input.begin(), input.end(), input.begin(), ::toupper); //make uppercase
        if ((input == "YES") || (input == "Y")) return true;
        if ((input == "NO") || (input == "N")) return false;
    }
}

unsigned int UserInput::getUnsignedInt(const std::string& question) {
    while (true) {
        cout << "\n" << question;
        string input;
        cin >> input;
        if (!input.empty() &&
            std::find_if(input.begin(), input.end(), [](unsigned char c) { return !std::isdigit(c); }) == input.end()) {
            //it is an unsigned integer
            return stoi(input);
        }
    }
}

unsigned int UserInput::getUnsignedInt(const string& question, unsigned int min, unsigned int max) {
    unsigned int value;
    do {
        value = getUnsignedInt(question);
    } while ((value < min) || (value > max));
    return value;
}

std::string UserInput::getString(const string& question) {
    cout << "\n" << question;
    string input;
    cin >> input;
    return input;
}
