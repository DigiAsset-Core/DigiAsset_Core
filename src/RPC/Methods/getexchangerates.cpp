//
// Created by mctrivia on 17/03/24.
//

#include "AppMain.h"
#include "RPC/Response.h"
#include "RPC/Server.h"
#include <jsoncpp/json/value.h>

namespace RPC {
    namespace Methods {
        /**
        * Returns a list of all current exchange rates
        * params[0] - optional block height(if provided will give what the exchange rates where at that time)
        * Warning will throw error if height provided is above current sync height or if not provided sync is more then 120 blocks behind
        *  JSON array of objects, each containing:
        *    - "height": the block height this exchange rate was recorded at (integer)
        *    - "address": the address this exchange rate was recorded at (string)
        *    - "index": What index the recorded value was at (integer 0 to 9)
        *    - "value": the exchange rate value(double)
        *
        *    DGB has an implied value of 100000000 and is never included in the returned value
        *    To convert from one exchange rate to another use the formula
        *    inputAmount*inputRate/outputRate
        *    units will be same as units used for inputAmount
        *
        *    so for example if you wanted to convert from USD to DGB you would get the inputRate where
        *    address="dgb1qunxh378eltj2jrwza5sj9grvu5xud43vqvudwh" and index=1(see DigiAsset::standardExchangeRates in DigiAsset.cpp for standard exchange rate addresses and indexes)
        *    and use 100000000 as the outputRate
        */
        extern const Response getexchangerates(const Json::Value& params) {
            unsigned int height;
            ChainAnalyzer* analyzer=AppMain::GetInstance()->getChainAnalyzer();
            if (params.size() == 1) {
                //use height provided
                if (!params[0].isUInt()) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
                height=params[0].asUInt();
                if (height>analyzer->getSyncHeight()) throw DigiByteException(RPC_MISC_ERROR, "Height out of range");

            } else if (params.size() == 0) {
                //find current height
                if (analyzer->getSync()<-120) throw DigiByteException(RPC_MISC_ERROR,"To far behind to get current exchange rate");
                height=analyzer->getSyncHeight();

            } else {
                throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
            }

            //get desired exchange rates
            Database* db = AppMain::GetInstance()->getDatabase();
            vector<Database::exchangeRateHistoryValue> rates = db->getExchangeRatesAtHeight(height);

            //convert to json
            Response response;
            Value result=Json::arrayValue;
            for (const Database::exchangeRateHistoryValue& rate: rates) {
                Value entry=Json::objectValue;
                entry["height"]=rate.height;
                entry["address"]=rate.address;
                entry["index"]=rate.index;
                entry["value"]=rate.value;
                result.append(entry);
                response.addInvalidateOnAddressChange(rate.address); //result good until new exchange rate published
            }

            //return response
            response.setResult(result);
            response.setBlocksGoodFor(5760); //day
            return response;
        }

    }
}