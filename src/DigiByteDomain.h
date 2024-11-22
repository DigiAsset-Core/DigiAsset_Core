//
// Created by mctrivia on 31/07/23.
//

#ifndef DIGIASSET_CORE_DIGIBYTEDOMAIN_H
#define DIGIASSET_CORE_DIGIBYTEDOMAIN_H

#define DIGIBYTEDOMAIN_CALLBACK_NEWMETADATA_ID "DigiByteDomain::_callbackNewMetadata"

#include "DigiAsset.h"
#include "DigiByteTransaction.h"

class DigiByteDomain {
    static void initializeClassValues();

public:
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
    protected:
        std::string _lastErrorMessage;
        mutable std::string _fullErrorMessage;

    public:
        explicit exception(const std::string& message = "Unknown") : _lastErrorMessage(message) {}

        virtual const char* what() const noexcept override {
            _fullErrorMessage = "DigiByte Domain Exception: " + _lastErrorMessage;
            return _fullErrorMessage.c_str();
        }
    };

    class exceptionRevokedDomain : public exception {
    public:
        explicit exceptionRevokedDomain()
            : exception("Domain has been revoked") {}
    };

    class exceptionBurnedDomain : public exception {
    public:
        explicit exceptionBurnedDomain()
            : exception("Domain has been burned") {}
    };

    class exceptionUnknownDomain : public exception {
    public:
        explicit exceptionUnknownDomain()
            : exception("Domain has not been issued") {}
    };
};



#endif //DIGIASSET_CORE_DIGIBYTEDOMAIN_H
