/**
 * @file    exception.h
 * @author  Krzysztof Okupski
 * @date    29.10.2014
 * @version 1.0
 *
 * Declaration of error class for the JSON-RPC wrapper.
 */

#ifndef DIGIASSET_CORE_DIGIBYTECORE_EXCEPTION_H
#define DIGIASSET_CORE_DIGIBYTECORE_EXCEPTION_H


#include <string>
#include <sstream>
#include <iostream>

#include <jsoncpp/json/json.h>
#include <jsoncpp/json/reader.h>
#include <jsoncpp/json/value.h>
#include <jsonrpccpp/client.h>

using Json::Value;
using Json::Reader;
using jsonrpc::Errors;


class DigiByteException : public std::exception {
private:
    int code;
    std::string msg;

public:
    explicit DigiByteException(int errcode, const std::string& message) noexcept {

        /* Connection error */
        if (errcode == Errors::ERROR_CLIENT_CONNECTOR) {
            this->code = errcode;
            this->msg = removePrefix(message, " -> ");
            /* Authentication error */
        } else if (errcode == Errors::ERROR_RPC_INTERNAL_ERROR && message.size() == 18) {
            this->code = errcode;
            this->msg = "Failed to authenticate successfully";
            /* Wrapped DigiByte Core error — message is a bitcoin-style JSON blob */
        } else if (containsErrorBlob(message)) {
            this->code = parseCode(message);
            this->msg = parseMessage(message);
            /* Directly thrown error — keep the code and message as given */
        } else {
            this->code = errcode;
            this->msg = message;
        }
    }

    ~DigiByteException() throw() {};

    int getCode() const {
        return code;
    }

    std::string getMessage() const {
        return msg;
    }


    std::string removePrefix(const std::string& in, const std::string& pattern) {
        std::string ret = in;

        unsigned int pos = ret.find(pattern);

        if (pos <= ret.size()) {
            ret.erase(0, pos + pattern.size());
        }

        return ret;
    }

    /* True if the message carries a bitcoin-style {"error":{...}} JSON blob
       (the format jsonrpccpp produces when DigiByte Core rejects a call) */
    bool containsErrorBlob(const std::string& in) {
        Value root;
        Reader reader;
        std::string strJson = removePrefix(in, "INTERNAL_ERROR: : ");
        return reader.parse(strJson.c_str(), root) && root.isObject() && root.isMember("error");
    }

    /* Auxiliary JSON parsing */
    int parseCode(const std::string& in) {
        Value root;
        Reader reader;

        /* Remove JSON prefix */
        std::string strJson = removePrefix(in, "INTERNAL_ERROR: : ");
        int ret = -1;

        /* Parse error message */
        bool parsingSuccessful = reader.parse(strJson.c_str(), root);
        if (parsingSuccessful) {
            ret = root["error"]["code"].asInt();
        }

        return ret;
    }

    std::string parseMessage(const std::string& in) {
        Value root;
        Reader reader;

        /* Remove JSON prefix */
        std::string strJson = removePrefix(in, "INTERNAL_ERROR: : ");
        std::string ret = "Error during parsing of >>" + strJson + "<<";

        /* Parse error message */
        bool parsingSuccessful = reader.parse(strJson.c_str(), root);
        if (parsingSuccessful) {
            ret = removePrefix(root["error"]["message"].asString(), "Error: ");
            if (!ret.empty()) ret[0] = toupper(ret[0]);
        }

        return ret;
    }
};


#endif //DIGIASSET_CORE_DIGIBYTECORE_EXCEPTION_H
