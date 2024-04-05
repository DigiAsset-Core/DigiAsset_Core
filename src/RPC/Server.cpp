//
// Created by mctrivia on 11/09/23.
//

#include "RPC/Server.h"
#include "AppMain.h"
#include "RPC/MethodList.h"
#include "Config.h"
#include "DigiByteCore.h"
#include "DigiByteDomain.h"
#include "DigiByteTransaction.h"
#include "Log.h"
#include "OldStream.h"
#include "PermanentStoragePool/PermanentStoragePoolList.h"
#include "Version.h"
#include "utils.h"
#include <iostream>
#include <jsonrpccpp/client.h>
#include <jsonrpccpp/client/connectors/httpclient.h>
#include <openssl/bio.h>
#include <openssl/evp.h>


using namespace std;

namespace RPC {
    /**
     * Small class that allows easy forcing close socket when it goes out of scope
     */
    class SocketRAII {
    public:
        explicit SocketRAII(tcp::socket& s) : socket(s) {}
        ~SocketRAII() { socket.close(); }

    private:
        tcp::socket& socket;
    };

    /*
    ███████╗███████╗████████╗██╗   ██╗██████╗
    ██╔════╝██╔════╝╚══██╔══╝██║   ██║██╔══██╗
    ███████╗█████╗     ██║   ██║   ██║██████╔╝
    ╚════██║██╔══╝     ██║   ██║   ██║██╔═══╝
    ███████║███████╗   ██║   ╚██████╔╝██║
    ╚══════╝╚══════╝   ╚═╝    ╚═════╝ ╚═╝
     */
    Server::Server(const string& fileName) {
        Config config = Config(fileName);
        _username = config.getString("rpcuser");
        _password = config.getString("rpcpassword");
        _port = config.getInteger("rpcassetport", 14024);
        tcp::endpoint endpoint(tcp::v4(), _port);
        _acceptor.open(endpoint.protocol());
        _acceptor.set_option(tcp::acceptor::reuse_address(true));
        _acceptor.bind(endpoint);
        _acceptor.listen();
        _allowedRPC = config.getBoolMap("rpcallow");
    }

    void Server::start() {
        accept();

        // Run the IO service to handle asynchronous operations
        _io.run();
    }

    /*
     ██████╗ ███████╗███╗   ██╗███████╗██████╗ ██╗ ██████╗
    ██╔════╝ ██╔════╝████╗  ██║██╔════╝██╔══██╗██║██╔════╝
    ██║  ███╗█████╗  ██╔██╗ ██║█████╗  ██████╔╝██║██║
    ██║   ██║██╔══╝  ██║╚██╗██║██╔══╝  ██╔══██╗██║██║
    ╚██████╔╝███████╗██║ ╚████║███████╗██║  ██║██║╚██████╗
     ╚═════╝ ╚══════╝╚═╝  ╚═══╝╚══════╝╚═╝  ╚═╝╚═╝ ╚═════╝
     */

    [[noreturn]] void Server::accept() {
        Log* log=Log::GetInstance();
        while (true) {
            std::chrono::steady_clock::time_point startTime;
            string method="NA";
            try {
                //get the socket
                tcp::socket socket(_io);
                SocketRAII socketGuard(socket); //make sure socket always gets closed
                _acceptor.accept(socket);

                //start timer and add debug log
                log->addMessage("request start",Log::DEBUG);
                startTime=std::chrono::steady_clock::now();

                // Handle the request and send the response
                Value response;
                Value request;
                try {
                    request = parseRequest(socket);
                    method=request["method"].asString();
                    response = handleRpcRequest(request);
                } catch (const DigiByteException& e) {
                    response = createErrorResponse(e.getCode(), e.getMessage(), request);
                } catch (const out_of_range& e) {
                    string text = e.what();
                    response = createErrorResponse(-32601, "Unauthorized", request);
                }

                sendResponse(socket, response);
            } catch (...) {
                Log* log = Log::GetInstance();
                log->addMessage("Unexpected exception caught", Log::DEBUG);
            }

            //calculate time took
            auto duration = std::chrono::steady_clock::now() - startTime;
            log->addMessage("request("+method+") finished in "+to_string(std::chrono::duration_cast<std::chrono::microseconds>(duration).count())+" µs",Log::DEBUG);
        }
    }


    Value Server::parseRequest(tcp::socket& socket) {
        // Read the HTTP request headers
        char data[1024];
        size_t length = socket.read_some(boost::asio::buffer(data, sizeof(data)));

        // Parse the HTTP request headers to determine content length
        string requestStr(data, length);
        size_t bodyStart = requestStr.find("\r\n\r\n");
        if (bodyStart == string::npos) {
            throw DigiByteException(HTTP_BAD_REQUEST, "Invalid HTTP request: No request body found.");
        }
        string headers = requestStr.substr(0, bodyStart);

        //verify the authentication
        if (!basicAuth(headers)) {
            throw DigiByteException(HTTP_UNAUTHORIZED, "Invalid HTTP request: No request body found.");
        }

        // Find the Content-Length header
        int contentLength = stoi(getHeader(headers, "Content-Length"));

        // Read the JSON content based on the content length
        string jsonContent;
        if (requestStr.size() < bodyStart + 4 + contentLength) {
            throw DigiByteException(HTTP_BAD_REQUEST, "Invalid Content-Length");
        }
        jsonContent = requestStr.substr(bodyStart + 4, contentLength);

        // Parse the JSON content
        Json::CharReaderBuilder readerBuilder;
        Json::Value doc;
        istringstream jsonContentStream(jsonContent);
        string errs;

        if (!Json::parseFromStream(readerBuilder, jsonContentStream, &doc, &errs)) {
            throw DigiByteException(RPC_PARSE_ERROR, "JSON parsing error: " + errs);
        }

        return doc;
    }

    string Server::getHeader(const string& headers, const string& wantedHeader) {
        //find the auth header
        size_t start = headers.find(wantedHeader + ": ");
        if (start == string::npos) throw DigiByteException(HTTP_BAD_REQUEST, wantedHeader + " header not found");
        size_t end = headers.find("\r\n", start);
        size_t headerLength = wantedHeader.length() + 2; //+2 for ": "
        return headers.substr(start + headerLength, end - start - headerLength);
    }

    // Basic authentication function
    bool Server::basicAuth(const string& headers) {
        string authHeader = getHeader(headers, "Authorization");

        string decoded;
        // Extract and decode the base64-encoded credentials
        string base64Credentials = authHeader.substr(6); // Remove "Basic "
        BIO* bio = BIO_new(BIO_f_base64());
        BIO* bioMem = BIO_new_mem_buf(base64Credentials.c_str(), -1);
        bio = BIO_push(bio, bioMem);
        BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);

        char buffer[1024];
        int len;
        while ((len = BIO_read(bio, buffer, sizeof(buffer))) > 0) {
            decoded.append(buffer, len);
        }
        BIO_free_all(bio);

        return (decoded == _username + ":" + _password);
    }


    Value Server::handleRpcRequest(const Value& request) {
        AppMain* app=AppMain::GetInstance();
        Json::Value id;

        //lets get the id(user defined value they can use as a reference)
        if (request.isMember("id")) {
            id = request["id"];
        } else {
            id = Value(Json::nullValue);
        }

        //lets get the command the user wants
        if (!request.isMember("method") || !request["method"].isString()) {
            throw DigiByteException(RPC_METHOD_NOT_FOUND, "Method not found");
        }
        string methodName = request["method"].asString();

        //lets check params is present
        if (request.isMember("params") && !request["params"].isArray()) {
            throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
        }
        const Json::Value& params = request.isMember("params") ? request["params"] : Value(Json::nullValue);

        //see if method on approved list
        if (!isRPCAllowed(methodName)) throw DigiByteException(RPC_FORBIDDEN_BY_SAFE_MODE, methodName + " is forbidden");

        //see if cached
        Response response;
        if (app->getRpcCache()->isCached(methodName,params,response)) return response.toJSON(id);

        //see if custom method handler
        if (methods.find(methodName) != methods.end()) {
            // Method exists in the map, so call it and set the result
            response = methods[methodName](params);
        } else {
            // Method does not exist in the map, fallback to sending to DigiByte core
            response.setResult(app->getDigiByteCore()->sendcommand(methodName, params));
        }

        //cache response
        app->getRpcCache()->addResponse(methodName,params,response);

        //return as json
        return response.toJSON(id);
    }


    Value Server::createErrorResponse(int code, const string& message, const Value& request) {
        // Create a JSON-RPC error response
        Json::Value response;
        response["result"] = Json::nullValue;
        Json::Value error;
        error["code"] = code;
        error["message"] = message;
        response["error"] = error;
        if (request.isMember("id")) response["id"] = request["id"];
        return response;
    }

    void Server::sendResponse(tcp::socket& socket, const Value& response) {
        Json::StreamWriterBuilder writer;
        writer.settings_["indentation"] = "";

        // Serialize JSON object to string.
        string jsonResponse = writeString(writer, response);

        // Create the HTTP response string.
        string httpResponse = "HTTP/1.1 200 OK\r\n";
        httpResponse += "Content-Length: " + to_string(jsonResponse.length()) + "\r\n";
        httpResponse += "Content-Type: application/json\r\n\r\n";
        httpResponse += jsonResponse;

        // Write the HTTP response to the socket.
        boost::asio::write(socket, boost::asio::buffer(httpResponse.c_str(), httpResponse.length()));
    }


    /*
     ██████╗ ███████╗████████╗████████╗███████╗██████╗ ███████╗
    ██╔════╝ ██╔════╝╚══██╔══╝╚══██╔══╝██╔════╝██╔══██╗██╔════╝
    ██║  ███╗█████╗     ██║      ██║   █████╗  ██████╔╝███████╗
    ██║   ██║██╔══╝     ██║      ██║   ██╔══╝  ██╔══██╗╚════██║
    ╚██████╔╝███████╗   ██║      ██║   ███████╗██║  ██║███████║
     ╚═════╝ ╚══════╝   ╚═╝      ╚═╝   ╚══════╝╚═╝  ╚═╝╚══════╝
     */
    unsigned int Server::getPort() {
        return _port;
    }

    bool Server::isRPCAllowed(const string& method) {
        auto it = _allowedRPC.find(method);
        if (it == _allowedRPC.end()) {
            if (_allowRPCDefault == -1) {
                _allowRPCDefault = 0; //default.  must set to prevent possible infinite loop
                _allowRPCDefault = isRPCAllowed("*") ? 1 : 0;
            }
            return _allowRPCDefault;
        }
        return it->second;
    }

}