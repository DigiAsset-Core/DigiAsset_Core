//
// Created by mctrivia on 01/02/24.
//

#include "CurlHandler.h"
#include "static_block.hpp"
#include <map>
#include <stdexcept>

using namespace std;


// Static block to register our callback function with IPFS Controller
static_block {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

namespace CurlHandler {
    namespace {
        /**
         * Private callback function for curl to build a string with returned data
         */
        size_t writeCallback(void* contents, size_t size, size_t nmemb, string* s) {
            size_t newLength = size * nmemb;
            try {
                s->append((char*) contents, newLength);
                return newLength;
            } catch (bad_alloc& e) {
                // handle memory problem
                return 0;
            }
        }

        /**
         * Private callback function for curl to build a file with returned data
         */
        size_t writeData(void* ptr, size_t size, size_t nmemb, FILE* stream) {
            size_t written = fwrite(ptr, size, nmemb, stream);
            return written;
        }
    } // namespace

    string get(const string& url, unsigned int timeout) {
        //check CURL is installed and get a thread safe instance of it
        CURL* curl = curl_easy_init();
        if (!curl) {
            throw runtime_error("Failed to initialize CURL");
        }

        //make get request
        string readBuffer;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        if (timeout > 0) curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout);
        CURLcode res = curl_easy_perform(curl);
        if (res == CURLE_OPERATION_TIMEDOUT) throw exceptionTimeout();
        if (res != CURLE_OK) {
            curl_easy_cleanup(curl);
            throw runtime_error(curl_easy_strerror(res));
        }
        curl_easy_cleanup(curl);
        return readBuffer;
    }

    string post(const string& url, const map<string, string>& data, unsigned int timeout) {
        //preprocess post data
        string postData;
        for (const auto& entry: data) {
            if (!postData.empty()) {
                postData += "&";
            }
            postData += entry.first + "=" + entry.second;
        }

        //check CURL is installed and get a thread safe instance of it
        CURL* curl = curl_easy_init();
        if (!curl) {
            throw runtime_error("Failed to initialize CURL");
        }

        //make post request
        string readBuffer;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
        if (timeout > 0) curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout);
        CURLcode res = curl_easy_perform(curl);
        if (res == CURLE_OPERATION_TIMEDOUT) throw exceptionTimeout();
        if (res != CURLE_OK) {
            curl_easy_cleanup(curl);
            throw runtime_error(curl_easy_strerror(res));
        }
        curl_easy_cleanup(curl);
        return readBuffer;
    }

    void getDownload(const string& url, const string& fileName, unsigned int timeout) {
        //check CURL is installed and get a thread safe instance of it
        CURL* curl = curl_easy_init();
        if (!curl) {
            throw runtime_error("Failed to initialize CURL");
        }

        //make get request
        FILE* fp;
        fp = fopen(fileName.c_str(), "wb");
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeData);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        if (timeout > 0) curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout);
        CURLcode res = curl_easy_perform(curl);
        if (res == CURLE_OPERATION_TIMEDOUT) throw exceptionTimeout();
        if (res != CURLE_OK) {
            curl_easy_cleanup(curl);
            throw runtime_error(curl_easy_strerror(res));
        }
        curl_easy_cleanup(curl);
        fclose(fp);
    }

    void postDownload(const string& url, const string& fileName, const map<string, string>& data, unsigned int timeout) {
        //preprocess post data
        string postData;
        for (const auto& entry: data) {
            if (!postData.empty()) {
                postData += "&";
            }
            postData += entry.first + "=" + entry.second;
        }

        //check CURL is installed and get a thread safe instance of it
        CURL* curl = curl_easy_init();
        if (!curl) {
            throw runtime_error("Failed to initialize CURL");
        }

        //make post request
        FILE* fp;
        fp = fopen(fileName.c_str(), "wb");
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeData);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
        if (timeout > 0) curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout);
        CURLcode res = curl_easy_perform(curl);
        if (res == CURLE_OPERATION_TIMEDOUT) throw exceptionTimeout();
        if (res != CURLE_OK) {
            curl_easy_cleanup(curl);
            throw runtime_error(curl_easy_strerror(res));
        }
        curl_easy_cleanup(curl);
        fclose(fp);
    }


} // namespace CurlHandler