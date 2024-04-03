//
// Created by mctrivia on 17/03/24.
//

#include "AppMain.h"
#include "BitcoinRpcServer.h"
#include "DigiByteDomain.h"
#include <jsoncpp/json/value.h>

namespace RPCMethods {
    /**
     * Returns the DigiByte address currently associated with a domain
     *  params[0] - domain(string)
     */
    extern const Json::Value getdomainaddress(const Json::Value& params) {
        if (params.size() != 1) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
        if (!params[0].isString()) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
        try {
            return DigiByteDomain::getAddress(params[0].asString());
        } catch (const DigiByteDomain::exceptionUnknownDomain& e) {
            throw DigiByteException(RPC_MISC_ERROR, "Unknown Domain");
        } catch (const DigiByteDomain::exceptionRevokedDomain& e) {
            throw DigiByteException(RPC_MISC_ERROR, "Domain Revoked");
        }
    }
}