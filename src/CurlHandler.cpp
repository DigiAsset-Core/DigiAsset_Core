//
// Created by mctrivia on 01/02/24.
//

#include "CurlHandler.h"
#include "static_block.hpp"
#include <atomic>
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

        std::atomic<bool> _abortAll{false};

        //curl calls this roughly once a second during a transfer; nonzero return aborts
        int progressCallback(void*, curl_off_t, curl_off_t, curl_off_t, curl_off_t) {
            return _abortAll ? 1 : 0;
        }

        //attach the abort check to a request(curl disables progress callbacks by default)
        void applyAbortCheck(CURL* curl) {
            curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progressCallback);
            curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        }

        ///seconds allowed to establish a connection.  A host that never completes the handshake
        ///used to block the calling thread forever because CURLOPT_TIMEOUT_MS is optional and
        ///several callers leave it at 0 - and some of those calls are made by the chain analyzer
        ///itself, so one unreachable host stopped the node dead in the middle of a block
        const long CONNECT_TIMEOUT_SECONDS = 10;

        //options every request needs regardless of what it is doing
        void applyCommonOptions(CURL* curl, unsigned int timeout) {
            //these run on worker threads and libcurl may use signals for its own timeouts
            curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
            curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, CONNECT_TIMEOUT_SECONDS);
            if (timeout > 0) curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout);
            applyAbortCheck(curl);
        }
    } // namespace

    void abortAllTransfers(bool abort) {
        _abortAll = abort;
    }

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
        applyCommonOptions(curl, timeout);
        CURLcode res = curl_easy_perform(curl);
        if ((res == CURLE_OPERATION_TIMEDOUT) || (res == CURLE_ABORTED_BY_CALLBACK)) {
            curl_easy_cleanup(curl);
            throw exceptionTimeout();
        }
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
        applyCommonOptions(curl, timeout);
        CURLcode res = curl_easy_perform(curl);
        if ((res == CURLE_OPERATION_TIMEDOUT) || (res == CURLE_ABORTED_BY_CALLBACK)) {
            curl_easy_cleanup(curl);
            throw exceptionTimeout();
        }
        if (res != CURLE_OK) {
            curl_easy_cleanup(curl);
            throw runtime_error(curl_easy_strerror(res));
        }
        curl_easy_cleanup(curl);
        return readBuffer;
    }

    /**
     * Makes a multipart/form-data post request with content uploaded as a file attachment.
     * Needed for the IPFS "add" api endpoint.
     */
    string postFile(const string& url, const string& fieldName, const string& fileName,
                    const string& content, unsigned int timeout) {
        //check CURL is installed and get a thread safe instance of it
        CURL* curl = curl_easy_init();
        if (!curl) {
            throw runtime_error("Failed to initialize CURL");
        }

        //build multipart form with the content as an in memory file
        curl_mime* mime = curl_mime_init(curl);
        curl_mimepart* part = curl_mime_addpart(mime);
        curl_mime_name(part, fieldName.c_str());
        curl_mime_filename(part, fileName.c_str());
        curl_mime_data(part, content.data(), content.size());

        //make post request
        string readBuffer;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
        applyCommonOptions(curl, timeout);
        CURLcode res = curl_easy_perform(curl);
        curl_mime_free(mime);
        if ((res == CURLE_OPERATION_TIMEDOUT) || (res == CURLE_ABORTED_BY_CALLBACK)) {
            curl_easy_cleanup(curl);
            throw exceptionTimeout();
        }
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
        applyCommonOptions(curl, timeout);
        CURLcode res = curl_easy_perform(curl);
        if ((res == CURLE_OPERATION_TIMEDOUT) || (res == CURLE_ABORTED_BY_CALLBACK)) {
            curl_easy_cleanup(curl);
            throw exceptionTimeout();
        }
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
        applyCommonOptions(curl, timeout);
        CURLcode res = curl_easy_perform(curl);
        if ((res == CURLE_OPERATION_TIMEDOUT) || (res == CURLE_ABORTED_BY_CALLBACK)) {
            curl_easy_cleanup(curl);
            throw exceptionTimeout();
        }
        if (res != CURLE_OK) {
            curl_easy_cleanup(curl);
            throw runtime_error(curl_easy_strerror(res));
        }
        curl_easy_cleanup(curl);
        fclose(fp);
    }


} // namespace CurlHandler