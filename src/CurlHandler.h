//
// Created by mctrivia on 01/02/24.
//

#ifndef DIGIASSET_CORE_CURLHANDLER_H
#define DIGIASSET_CORE_CURLHANDLER_H



#include <curl/curl.h>
#include <map>
#include <mutex>
#include <string>
#include <exception>
#include <vector>

namespace CurlHandler {

    std::string get(const std::string& url, unsigned int timeout = 0);
    std::string post(const std::string& url, const std::map<std::string, std::string>& data = {}, unsigned int timeout = 0);
    void getDownload(const std::string& url, const std::string& fileName, unsigned int timeout = 0);
    void postDownload(const std::string& url, const std::string& fileName, const std::map<std::string, std::string>& data = {}, unsigned int timeout = 0);

    std::string dnsTxtLookup(const std::string& domain);

    class exceptionTimeout : public std::exception {
    public:
        char* what() {
            return const_cast<char*>("request timed out");
        }
    };

}; // namespace CurlHandler

#endif //DIGIASSET_CORE_CURLHANDLER_H
