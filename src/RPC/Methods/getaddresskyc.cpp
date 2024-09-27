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
        * Returns address kyc information
        * params[0] - address(string)
        *
        * return {
        *      address: (string) containing requested address
        *
        *      #bellow only pressent if kyc verified
        *      country: (string) contain ISO 3166-1 alpha-3 country code
        *      name: (string optional) will contain either name or hash field but not both.  If name is omitted address has been verified that country is correct but is left anonymous
        *      hash: (string optional) will contain either name or hash field but not both.  Hash is sha256 of persons full name and a pin they provided at the time of verification.
        * }
        */
        extern const Response getaddresskyc(const Json::Value& params) {
            if (params.size() != 1) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
            if (!params[0].isString()) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");

            string address=params[0].asString();

            //get desired exchange rates
            Database* db = AppMain::GetInstance()->getDatabase();
            KYC data = db->getAddressKYC(address);

            Json::Value result=data.toJSON();


            //return response
            Response response;
            response.setResult(result);
            response.setBlocksGoodFor(5760); //day
            response.addInvalidateOnAddressChange(address);
            return response;
        }

    }
}