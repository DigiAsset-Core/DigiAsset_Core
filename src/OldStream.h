//
// Created by mctrivia on 26/02/24.
//

#ifndef DIGIASSET_CORE_OLDSTREAM_H
#define DIGIASSET_CORE_OLDSTREAM_H


#include "RPC/Response.h"
#include <jsoncpp/json/value.h>
namespace OldStream {
    RPC::Response getKey(const std::string& key);
}

#endif //DIGIASSET_CORE_OLDSTREAM_H
