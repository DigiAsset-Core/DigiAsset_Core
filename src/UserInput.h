//
// Created by mctrivia on 10/02/23.
//

#ifndef DIGIBYTECORE_USERINPUT_H
#define DIGIBYTECORE_USERINPUT_H

#include <string>
#include <climits>

class UserInput {
public:
    static bool getBool(const std::string& question);  //recommend adding "(Y/N)" to question
    static unsigned int getUnsignedInt(const std::string& question);
    static unsigned int getUnsignedInt(const std::string& question, unsigned int min, unsigned int max = UINT_MAX);
    static std::string getString(const std::string& question);
};


#endif //DIGIBYTECORE_USERINPUT_H
