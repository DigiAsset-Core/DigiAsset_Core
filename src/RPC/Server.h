//
// Created by mctrivia on 11/09/23.
//

#ifndef DIGIASSET_CORE_RPC_SERVER_H
#define DIGIASSET_CORE_RPC_SERVER_H


#define HTTP_BAD_REQUEST    400
#define HTTP_UNAUTHORIZED   401
#define RPC_METHOD_NOT_FOUND (-32601)
#define RPC_INVALID_PARAMS (-32602)
#define RPC_PARSE_ERROR (-32700)
#define RPC_FORBIDDEN_BY_SAFE_MODE  (-2)
#define RPC_MISC_ERROR (-1)

// Macro definition in a common header or the RPC server file
#define REGISTER_RPC_METHOD(methodName) registerMethod(#methodName, &std::methodName)



#include <string>
#include <boost/asio.hpp>
#include <jsonrpccpp/server.h>
#include <jsonrpccpp/server/connectors/httpserver.h>
#include "DigiByteCore.h"
#include "ChainAnalyzer.h"
#include "UniqueTaskQueue.h"

using namespace std;
using namespace jsonrpc;
using boost::asio::ip::tcp;

namespace RPC {

    class Server {

        boost::asio::io_service _io{};
        tcp::acceptor _acceptor{_io};
        std::string _username;
        std::string _password;
        unsigned int _port;
        std::map<std::string, bool> _allowedRPC;
        int8_t _allowRPCDefault = -1; //unknown

        //functions to handle requests
        Value parseRequest(tcp::socket& socket);
        [[noreturn]] void accept();
        Value handleRpcRequest(const Value& request);
        Value createErrorResponse(int code, const std::string& message, const Value& request);
        void sendResponse(tcp::socket& socket, const Value& response);
        bool basicAuth(const std::string& header);
        static std::string getHeader(const std::string& headers, const std::string& wantedHeader);

    public:
        explicit Server(const std::string& fileName = "config.cfg");

        void start();
        unsigned int getPort();
        bool isRPCAllowed(const string& method);
    };

}
#endif //DIGIASSET_CORE_RPC_SERVER_H
