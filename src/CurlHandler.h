//
// Created by mctrivia on 01/02/24.
//

#ifndef DIGIASSET_CORE_CURLHANDLER_H
#define DIGIASSET_CORE_CURLHANDLER_H



#include <curl/curl.h>
#include <map>
#include <mutex>
#include <string>

namespace CurlHandler {

    ///abort(or stop aborting) every in flight and future transfer process wide.
    ///Used during shutdown: a pin/download can legitimately block for many minutes
    ///and joining those threads would hang otherwise.  Aborted transfers fail the
    ///same way a timeout does so all retry paths behave as if the request timed out.
    void abortAllTransfers(bool abort);

    std::string get(const std::string& url, unsigned int timeout = 0);
    std::string post(const std::string& url, const std::map<std::string, std::string>& data = {}, unsigned int timeout = 0);
    std::string postFile(const std::string& url, const std::string& fieldName, const std::string& fileName,
                         const std::string& content, unsigned int timeout = 0);
    void getDownload(const std::string& url, const std::string& fileName, unsigned int timeout = 0);
    void postDownload(const std::string& url, const std::string& fileName, const std::map<std::string, std::string>& data = {}, unsigned int timeout = 0);

    class exceptionTimeout : public std::exception {
    public:
        char* what() {
            return const_cast<char*>("request timed out");
        }
    };

}; // namespace CurlHandler

#endif //DIGIASSET_CORE_CURLHANDLER_H
