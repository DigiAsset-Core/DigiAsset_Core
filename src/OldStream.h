//
// Created by mctrivia on 26/02/24.
//

#ifndef DIGIASSET_CORE_OLDSTREAM_H
#define DIGIASSET_CORE_OLDSTREAM_H


#include <jsoncpp/json/value.h>
namespace OldStream {
    Json::Value getKey(unsigned int key);
    Json::Value getKey(const std::string& key);
}

#endif //DIGIASSET_CORE_OLDSTREAM_H
