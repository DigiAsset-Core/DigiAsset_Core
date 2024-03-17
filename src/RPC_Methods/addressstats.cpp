//
// Created by mctrivia on 17/03/24.
//

#include "AppMain.h"
#include "BitcoinRpcServer.h"
#include <jsoncpp/json/value.h>

namespace RPCMethods {
    /**
     * Returns stats about addresses over time.
     * Warning: The last result is likely not a full time period.
     *
     * Input Parameters:
     *  params[0] - start time (integer, default = beginning(0))
     *  params[1] - end time (integer, default = end(max value))
     *  params[2] - time frame (integer, default = day(86400))
     *
     * Output Format:
     *  JSON array of objects, each containing:
     *    - "time": The end time of the time frame (integer)
     *    - "new": Number of new addresses created (integer)
     *    - "used": Number of addresses used (integer)
     *    - "withAssets": Number of addresses with assets (integer)
     *    - "over0": Number of addresses with balance over 0 (integer)
     *    - "over1": Number of addresses with balance over 1 (integer)
     *    - "over1k": Number of addresses with balance over 1k (integer)
     *    - "over1m": Number of addresses with balance over 1m (integer)
     *    - "quantumInsecure": Number of quantum insecure addresses (integer)
     *    - "total": Total number of addresses (integer)
     *
     * Example Output:
     *  [
     *    {
     *      "time": 1620000000,
     *      "new": 100,
     *      "used": 50,
     *      "withAssets": 30,
     *      "over0": 120,
     *      "over1": 110,
     *      "over1k": 5,
     *      "over1m": 1,
     *      "quantumInsecure": 10,
     *      "total": 200
     *    },
     *    ...
     *  ]
     */
    extern const Json::Value addressstats(const Json::Value& params) {
        //get paramaters
        unsigned int timeFrame = 86400;
        unsigned int start = 0;
        unsigned int end = std::numeric_limits<unsigned int>::max();
        if (params.size() > 3) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
        if (params.size() >= 1) {
            if (!params[0].isInt()) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
            start = params[0].asInt();
        }
        if (params.size() >= 2) {
            if (!params[1].isInt()) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
            end = params[1].asInt();
        }
        if (params.size() == 3) {
            if (!params[2].isInt()) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
            timeFrame = params[2].asInt();
        }

        //lookup stats
        try {
            Database* db = AppMain::GetInstance()->getDatabase();
            vector<AddressStats> stats = db->getAddressStats(start, end, timeFrame);

            //convert to json
            Value result = Value(Json::arrayValue);
            for (const auto& stat: stats) {
                Json::Value statObject(Json::objectValue);
                statObject["time"] = stat.time;
                statObject["new"] = stat.created;
                statObject["used"] = stat.used;
                statObject["withAssets"] = stat.withAssets;
                statObject["over0"] = stat.over0;
                statObject["over1"] = stat.over1;
                statObject["over1k"] = stat.over1k;
                statObject["over1m"] = stat.over1m;
                statObject["quantumInsecure"] = stat.quantumInsecure;
                statObject["total"] = stat.total;

                // Add this statObject to the result array under a key (e.g., the index or time)
                result.append(statObject);
            }
            return result;

        } catch (const Database::exceptionDataPruned& e) {
            return Json::arrayValue;
        }
    }
}