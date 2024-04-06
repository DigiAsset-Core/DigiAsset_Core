///This file is automatically generated by CMakeLists.txt do not edit

#ifndef DIGIASSET_CORE_RPC_METHODLIST_H
#define DIGIASSET_CORE_RPC_METHODLIST_H

#include "Response.h"
#include <map>
#include <functional>
#include "jsoncpp/json/value.h"

namespace RPC {
    namespace Methods {
        extern const Response addressstats(const Json::Value& params);
        extern const Response algostats(const Json::Value& params);
        extern const Response createoldstreamkey(const Json::Value& params);
        extern const Response debugwaittimes(const Json::Value& params);
        extern const Response getaddressholdings(const Json::Value& params);
        extern const Response getaddresskyc(const Json::Value& params);
        extern const Response getassetdata(const Json::Value& params);
        extern const Response getassetholders(const Json::Value& params);
        extern const Response getassetindexes(const Json::Value& params);
        extern const Response getdgbequivalent(const Json::Value& params);
        extern const Response getdomainaddress(const Json::Value& params);
        extern const Response getexchangerates(const Json::Value& params);
        extern const Response getipfscount(const Json::Value& params);
        extern const Response getoldstreamkey(const Json::Value& params);
        extern const Response getpsp(const Json::Value& params);
        extern const Response getrawtransaction(const Json::Value& params);
        extern const Response listaddresshistory(const Json::Value& params);
        extern const Response listassetissuances(const Json::Value& params);
        extern const Response listassets(const Json::Value& params);
        extern const Response listunspent(const Json::Value& params);
        extern const Response resyncmetadata(const Json::Value& params);
        extern const Response send(const Json::Value& params);
        extern const Response sendmany(const Json::Value& params);
        extern const Response sendtoaddress(const Json::Value& params);
        extern const Response shutdown(const Json::Value& params);
        extern const Response syncstate(const Json::Value& params);
        extern const Response version(const Json::Value& params);
    }

    extern std::map<std::string, std::function<Response(const Json::Value&)>> methods;

}

#endif  //DIGIASSET_CORE_RPC_METHODLIST_H