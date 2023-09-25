//
// Created by mctrivia on 31/07/23.
//

#ifndef DIGIASSET_CORE_DIGIBYTEDOMAIN_H
#define DIGIASSET_CORE_DIGIBYTEDOMAIN_H

#define DIGIBYTEDOMAIN_CALLBACK_NEWMETADATA_ID  "DigiByteDomain::_callbackNewMetadata"

#include "DigiAsset.h"
#include "DigiByteTransaction.h"

class DigiByteDomain {
    static void initializeClassValues();

public:
    static std::string _lastErrorMessage;

    static void processAssetIssuance(const DigiAsset& asset);

    //API interface calls
    static std::string getAssetId(const std::string& domain);
    static std::string getAddress(const std::string& domain);
    static bool isDomain(const std::string& domain);

    ///public because needs to be but should only be used by DigiByteDomain.cpp
    static void
    _callbackNewMetadata(const std::string& cid, const std::string& extra, const std::string& content, bool failed);

    /*
    ███████╗██████╗ ██████╗  ██████╗ ██████╗ ███████╗
    ██╔════╝██╔══██╗██╔══██╗██╔═══██╗██╔══██╗██╔════╝
    █████╗  ██████╔╝██████╔╝██║   ██║██████╔╝███████╗
    ██╔══╝  ██╔══██╗██╔══██╗██║   ██║██╔══██╗╚════██║
    ███████╗██║  ██║██║  ██║╚██████╔╝██║  ██║███████║
    ╚══════╝╚═╝  ╚═╝╚═╝  ╚═╝ ╚═════╝ ╚═╝  ╚═╝╚══════╝
     */
    class exception : public std::exception {
    public:
        char* what() {
            _lastErrorMessage = "Something went wrong with DigiByte Domain";
            return const_cast<char*>(_lastErrorMessage.c_str());
        }
    };

    class exceptionRevokedDomain : public exception {
    public:
        char* what() {
            _lastErrorMessage = "Domain has been revoked";
            return const_cast<char*>(_lastErrorMessage.c_str());
        }
    };

    class exceptionBurnedDomain : public exception {
    public:
        char* what() {
            _lastErrorMessage = "Domain has been burned";
            return const_cast<char*>(_lastErrorMessage.c_str());
        }
    };

    class exceptionUnknownDomain : public exception {
    public:
        char* what() {
            _lastErrorMessage = "Domain has not been issued";
            return const_cast<char*>(_lastErrorMessage.c_str());
        }
    };


};



#endif //DIGIASSET_CORE_DIGIBYTEDOMAIN_H
