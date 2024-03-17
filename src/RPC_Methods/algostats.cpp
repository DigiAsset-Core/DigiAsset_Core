//
// Created by mctrivia on 17/03/24.
//

#include "AppMain.h"
#include "BitcoinRpcServer.h"
#include <jsoncpp/json/value.h>

namespace RPCMethods {
    /**
     * Returns mining stats over a given time period
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
     *    - "algo": An array of objects, one for each algorithm, containing:
     *        - "min": Minimum difficulty (float)
     *        - "max": Maximum difficulty (float)
     *        - "avg": Average difficulty (float)
     *        - "count": Number of blocks (integer)
     *
     * Example Output:
     *  [
     *    {
     *      "time": 1620000000,
     *      "algo": [
     *        {"min": 0.1, "max": 0.2, "avg": 0.15, "count": 50},
     *        null,
     *        {"min": 0.3, "max": 0.4, "avg": 0.35, "count": 40}
     *      ]
     *    },
     *    ...
     *  ]
     *
     * Note: 'null' is used to fill in missing algorithms.
     */
    extern const Json::Value algostats(const Json::Value& params) {
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
            vector<AlgoStats> stats = db->getAlgoStats(start, end, timeFrame);

            // Initialize Json::Value
            Json::Value result(Json::arrayValue);

            // Initialize variables to keep track of the current time and algo array
            unsigned int currentTime = 0;
            int lastAlgo = -1;
            Json::Value currentTimeObject(Json::objectValue);
            Json::Value currentAlgoArray(Json::arrayValue);

            // Populate result from stats
            for (const auto& stat: stats) {
                if (stat.time == 0) continue;
                if (stat.time != currentTime) {
                    // Save the previous time object if it exists
                    if (!currentAlgoArray.empty()) {
                        currentTimeObject["algo"] = currentAlgoArray;
                        result.append(currentTimeObject);
                    }

                    // Reset for the new time
                    currentTime = stat.time;
                    lastAlgo = -1;
                    currentTimeObject = Json::Value(Json::objectValue);
                    currentTimeObject["time"] = currentTime;
                    currentAlgoArray = Json::Value(Json::arrayValue);
                }

                // Fill in missing algos with null values
                while (static_cast<unsigned int>(lastAlgo + 1) < stat.algo) {
                    ++lastAlgo;
                    currentAlgoArray.append(Json::Value(Json::nullValue));
                }

                // Create and append the algo object
                Json::Value algoObject(Json::objectValue);
                algoObject["min"] = stat.difficultyMin;
                algoObject["max"] = stat.difficultyMax;
                algoObject["avg"] = stat.difficultyAvg;
                algoObject["count"] = stat.blocks;
                currentAlgoArray.append(algoObject);

                lastAlgo = stat.algo;
            }

            // Append the last time object if it exists
            if (!currentAlgoArray.empty()) {
                currentTimeObject["algo"] = currentAlgoArray;
                result.append(currentTimeObject);
            }

            //return results
            return result;
        } catch (const Database::exceptionDataPruned& e) {
            return Json::arrayValue;
        }
    }
}