//
// Created by mctrivia on 17/03/24.
//

#include "AppMain.h"
#include "OldStream.h"
#include "RPC/Response.h"
#include "RPC/Server.h"
#include "utils.h"
#include <fstream>
#include <iostream>
#include <jsoncpp/json/value.h>

namespace RPC {
    namespace Methods {
        namespace {
            UniqueTaskQueue _taskQueue;
            std::atomic<bool> _processingThreadStarted{false};
        }

        /**
        * This function will be depricated eventually and should not be used for new projects
        * Simulates old DigiAsset Stream
        *
        *  params[0] - key(string)
        *
        *  returns the number of items in the job que
        */
        extern const Response createoldstreamkey(const Json::Value& params) {
            if (params.size() != 1) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");

            //make sure que is being processed
            if (!_processingThreadStarted.exchange(true)) {
                std::thread([]() {
                    while (true) {
                        //get key
                        string key = _taskQueue.dequeue(); // This will block if the queue is empty

                        //process request
                        Json::Value result = OldStream::getKey(key);

                        // Add cacheTime with the current epoch time in seconds
                        if (result.isObject()) {
                            auto now = std::chrono::system_clock::now();
                            auto epoch = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
                            result["cacheTime"] = static_cast<Json::Int64>(epoch);
                        }

                        // Save the result to a file
                        std::string filename = "stream/" + key + ".json";
                        if (utils::fileExists(filename)) remove(filename.c_str());
                        std::ofstream file(filename);
                        if (file.is_open()) {
                            Json::StreamWriterBuilder builder;
                            const std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
                            writer->write(result, &file);
                            file.close();
                        } else {
                            // Handle the error, e.g., throw an exception or log an error
                        }
                    }
                }).detach();
            }

            //add to que
            string key;
            if (params[0].isInt()) {
                key= to_string(params[0].asInt());
            } else if (params[0].isString()){
                key = params[0].asString();
            } else {
                throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
            }
            _taskQueue.enqueue(key);

            //return response
            Response response;
            response.setResult(_taskQueue.length());
            response.setBlocksGoodFor(-1);//no caching
            return response;
        }

    }
}