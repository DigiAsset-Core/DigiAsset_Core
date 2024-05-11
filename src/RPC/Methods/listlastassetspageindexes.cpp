//
// Created by mctrivia on 07/04/24.
//

#include "AppMain.h"
#include "RPC/MethodList.h"
#include "RPC/Response.h"
#include "RPC/Server.h"
#include <jsoncpp/json/value.h>

namespace RPC {
    namespace Methods {
        /**
        * Returns a list of assetIDs ordered by issuance height
        *  params[0] - numberOfRecords(optional unsigned int)
        *  params[1] - filter(optional object)
        *      {
        *        psp:  bool - if true only returns assets that are part of a psp, false only those that are not part of a psp
        *              int  - if int returns only ones that are part of that specific psp
        *      }
        * ** please note filtering may result in the number of items not equaling the number asked for.
        *
        *
        * returns an array of objects of type
        * {
        *     start:  highest assetIndex(unsigned int),
        *     skips:  array of unsigned int representing any indexes that got skipped
        * }
        */
        extern const Response listlastassetspageindexes(const Json::Value& params) {
            if (
                (params.size() < 1) ||
                (params.size() > 2) ||
                (!params[0].isUInt())
            ) {
                throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
            }

            //get number of records
            unsigned int numberOfRecords=params[0].asUInt();

            //get filter if any
            Json::Value filter=Json::objectValue;
            if (params.size()>1) {
                if (params[1].isObject()) {
                    filter=params[1];
                } else {
                    throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
                }
            }

            //get asset list
            Json::Value requestParams=Json::arrayValue;
            requestParams.append(Json::nullValue);
            requestParams.append(Json::nullValue);
            requestParams.append(true);
            requestParams.append(filter);
            auto assets=listlastassets(requestParams).toJSON(1)["result"];

            // Initialize variables for processing
            Json::Value results = Json::arrayValue; // This will hold the final output
            unsigned int currentCount = 0; // Count of assets added to the current object
            unsigned int currentIndexesChecked = 0; // Indexes checked for the current object
            unsigned int lastAssetIndex = 0; // Track the last asset index we encountered
            Json::Value currentObject; // Current object being populated
            Json::Value skips = Json::arrayValue; // Skips for the current object

            // Function to reset and prepare a new object
            auto prepareNewObject = [&]() {
                currentObject = Json::objectValue;
                skips = Json::arrayValue;
                currentCount = 0;
                currentIndexesChecked = 0;
            };

            // Prepare the first object
            prepareNewObject();

            for (const auto& asset : assets) {
                unsigned int assetIndex = asset["assetIndex"].asUInt();

                // Initialize the start for the first asset encountered
                if (currentCount == 0) {
                    currentObject["start"] = assetIndex;
                    lastAssetIndex = assetIndex;
                } else {
                    // Check if there's a gap
                    if (lastAssetIndex - assetIndex > 1) {
                        // Record skipped indexes
                        for (unsigned int i = lastAssetIndex - 1; i > assetIndex; --i) {
                            skips.append(i);
                            currentIndexesChecked++;
                            // Break if we've checked 4x the number of records without filling the current group
                            if (currentIndexesChecked >= 4 * numberOfRecords) {
                                currentObject["skips"] = skips;
                                results.append(currentObject);
                                prepareNewObject();
                                currentObject["start"] = assetIndex;
                            }
                        }
                    }
                }

                lastAssetIndex = assetIndex;
                currentCount++;
                currentIndexesChecked++;

                // If we've added enough records to the current object, or checked enough indexes, start a new one
                if (currentCount == numberOfRecords || currentIndexesChecked >= 4 * numberOfRecords) {
                    currentObject["skips"] = skips;
                    results.append(currentObject);
                    if (currentCount == numberOfRecords) {
                        prepareNewObject();
                    }
                }
            }

            // Don't forget to add the last object if it has any assets
            if (currentCount > 0) {
                currentObject["skips"] = skips;
                results.append(currentObject);
            }



            //return response
            Response response;
            response.setResult(results);
            response.setBlocksGoodFor(5760);    //day
            response.setInvalidateOnNewAsset();
            return response;
        }

    }
}