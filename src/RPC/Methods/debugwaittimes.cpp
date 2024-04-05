//
// Created by mctrivia on 21/03/24.
//
#include "AppMain.h"
#include "RPC/Response.h"
#include "RPC/Server.h"
#include <jsoncpp/json/value.h>

namespace RPC {
    namespace Methods {
        /**
        * Returns a string of wait times to help improve performance
        */
        extern const Response debugwaittimes(const Json::Value& params) {
            std::string result;
            result+=AppMain::GetInstance()->getDatabase()->printProfilingInfo();
            result+=AppMain::GetInstance()->getDigiByteCore()->printStatementInfo();

            std::istringstream stream(result);
            std::string line;
            Json::Value jsonArray(Json::arrayValue);
            while (std::getline(stream, line)) {
                // If you want to exclude empty lines, uncomment the following line
                // if (line.empty()) continue;
                jsonArray.append(line);
            }

            //return response
            Response response;
            response.setResult(jsonArray);
            response.setBlocksGoodFor(-1);
            return response;
        }

    }
}