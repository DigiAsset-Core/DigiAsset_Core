//
// Created by mctrivia on 08/04/24.
//

#include "AppMain.h"
#include "Log.h"
#include "RPC/MethodList.h"
#include "RPC/Response.h"
#include "RPC/Server.h"
#include "crypto/SHA256.h"
#include "utils.h"
#include <fstream>
#include <jsoncpp/json/value.h>

namespace RPC {
    namespace Methods {
        namespace {
            UniqueTaskQueue _taskQueue;
            std::atomic<bool> _processingThreadStarted{false};
        } // namespace

        /**
         * Starts a RPC call without waiting for the response.   Useful for calls that may take a long time.
         * You can get the response using asyncget.
         * Job output will be cached indefinitely so if output type is an object a cacheTime parameter will
         * be added so you know how old it is
         *
         * Input Parameters:
         *  params[0] - method name
         *  params[1] - param1
         *  params[2] - param2
         *  ...
         *
         * returns the number of items in the job que
         */
        extern const Response asyncstart(const Json::Value& params) {
            if ((params.size() < 1) || (!params[0].isString())) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");

            //make sure que is being processed
            if (!_processingThreadStarted.exchange(true)) {
                std::thread([]() {
                    Log* log=Log::GetInstance();
                    while (true) {
                        //get key
                        string key = _taskQueue.dequeue(); // This will block if the queue is empty
                        Json::Value params;
                        Json::Reader reader;
                        bool parsingSuccessful = reader.parse(key, params);
                        if (!parsingSuccessful) continue; //shouldn't happen but just in case

                        //split key up
                        string method = params[0].asString();
                        params.removeIndex(0, nullptr);

                        //compute file name
                        SHA256 sha256;
                        sha256.update(key);
                        auto sha256Digest = sha256.digest();
                        std::string filename = "cache/" + method + "_" + SHA256::toString(sha256Digest) + ".json";

                        //debug log
                        log->addMessage("Starting async job: "+filename,Log::DEBUG);

                        //process request
                        Json::Value result;
                        try {
                            result = AppMain::GetInstance()->getRpcServer()->executeCall(method, params);
                        } catch (...) {
                            result = Json::objectValue;
                            result["error"] = "There was an error during executing your request";
                        }

                        // Add cacheTime with the current epoch time in seconds
                        if (result.isObject()) {
                            auto now = std::chrono::system_clock::now();
                            auto epoch = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
                            result["cacheTime"] = static_cast<Json::Int64>(epoch);
                        }

                        // Save the result to a file
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

                        //debug message
                        log->addMessage("Finished async job: "+filename,Log::DEBUG);
                    }
                }).detach();
            }

            //add to que
            Json::FastWriter writer;
            string key = writer.write(params);
            _taskQueue.enqueue(key);

            //return response
            Response response;
            response.setResult(_taskQueue.length());
            response.setBlocksGoodFor(-1); //no caching
            return response;
        }

    } // namespace Methods
} // namespace RPC