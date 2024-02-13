//
// Created by mctrivia on 06/06/23.
//

#ifndef DIGIASSET_CORE_KYC_H
#define DIGIASSET_CORE_KYC_H


#include "DigiByteCore_Types.h"
#include <functional>

class KYC {
    static std::string _lastErrorMessage;

    std::string _address;
    std::string _name;
    std::string _country;
    std::string _hash;
    int _heightCreated = -1;
    int _heightRevoked = -1;

    bool processKYCVerify(const getrawtransaction_t& txData, unsigned int height,
                          std::function<std::string(std::string, unsigned int)>& addressGetterFunction);
    bool processKYCRevoke(const getrawtransaction_t& txData, unsigned int height,
                          std::function<std::string(std::string, unsigned int)>& addressGetterFunction);
    static bool isKYCVerifier(const std::string& address, unsigned int height);

public:
    KYC() = default;
    KYC(const std::string& address);
    KYC(const std::string& address, const std::string& country, const std::string& name, const std::string& hash,
        unsigned int heightCreated, int heightRevoked = -1);
    KYC(const getrawtransaction_t& txData, unsigned int height,
        std::function<std::string(std::string, unsigned int)> addressGetterFunction);

    unsigned int processTX(const getrawtransaction_t& txData, unsigned int height,
                           std::function<std::string(std::string, unsigned int)> addressGetterFunction);
    std::string getAddress() const;
    std::string getName() const;
    std::string getHash() const;
    std::string getCountry() const;
    unsigned int getHeightCreated() const;
    int getHeightRevoked() const;      //-1 not yet revoked or haven't yet processed
    bool valid(int height = -1) const; //-1 highest scanned(returns false if empty also)
    bool empty() const;

    const static unsigned int NA = 0;
    const static unsigned int VERIFY = 1;
    const static unsigned int REVOKE = 2;


    /*
    ███████╗██████╗ ██████╗  ██████╗ ██████╗ ███████╗
    ██╔════╝██╔══██╗██╔══██╗██╔═══██╗██╔══██╗██╔════╝
    █████╗  ██████╔╝██████╔╝██║   ██║██████╔╝███████╗
    ██╔══╝  ██╔══██╗██╔══██╗██║   ██║██╔══██╗╚════██║
    ███████╗██║  ██║██║  ██║╚██████╔╝██║  ██║███████║
    ╚══════╝╚═╝  ╚═╝╚═╝  ╚═╝ ╚═════╝ ╚═╝  ╚═╝╚══════╝
     */

    class exceptionUnknownValue : public std::exception {
    public:
        char* what() {
            _lastErrorMessage = "Tried to get a value that is unknown";
            return const_cast<char*>(_lastErrorMessage.c_str());
        }
    };
};


#endif //DIGIASSET_CORE_KYC_H
