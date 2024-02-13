//
// Created by mctrivia on 11/09/23.
//

#include "BitcoinRpcServer.h"
#include "AppMain.h"
#include "Config.h"
#include "DigiByteCore.h"
#include "DigiByteDomain.h"
#include "DigiByteTransaction.h"
#include "Log.h"
#include "PermanentStoragePool/PermanentStoragePoolList.h"
#include <iostream>
#include <jsonrpccpp/client.h>
#include <jsonrpccpp/client/connectors/httpclient.h>
#include <openssl/bio.h>
#include <openssl/evp.h>


using namespace std;

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
BitcoinRpcServer::BitcoinRpcServer(DigiByteCore& api, ChainAnalyzer& analyzer, const string& fileName) {
    _api = &api;
    _analyzer = &analyzer;

    Config config = Config(fileName);
    _username = config.getString("rpcuser");
    _password = config.getString("rpcpassword");
    _port = config.getInteger("rpcassetport", 14023);
    tcp::endpoint endpoint(tcp::v4(), _port);
    _acceptor.open(endpoint.protocol());
    _acceptor.set_option(tcp::acceptor::reuse_address(true));
    _acceptor.bind(endpoint);
    _acceptor.listen();
    _allowedRPC = config.getBoolMap("rpcallow");
    defineMethods();
}

void BitcoinRpcServer::start() {
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

[[noreturn]] void BitcoinRpcServer::accept() {
    while (true) {
        try {
            //get the socket
            tcp::socket socket(_io);
            SocketRAII socketGuard(socket); //make sure socket always gets closed
            _acceptor.accept(socket);

            // Handle the request and send the response
            Value response;
            Value request;
            try {
                request = parseRequest(socket);
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
    }
}


Value BitcoinRpcServer::parseRequest(tcp::socket& socket) {
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

string BitcoinRpcServer::getHeader(const string& headers, const string& wantedHeader) {
    //find the auth header
    size_t start = headers.find(wantedHeader + ": ");
    if (start == string::npos) throw DigiByteException(HTTP_BAD_REQUEST, wantedHeader + " header not found");
    size_t end = headers.find("\r\n", start);
    size_t headerLength = wantedHeader.length() + 2; //+2 for ": "
    return headers.substr(start + headerLength, end - start - headerLength);
}

// Basic authentication function
bool BitcoinRpcServer::basicAuth(const string& headers) {
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


Value BitcoinRpcServer::handleRpcRequest(const Value& request) {
    Json::Value response;

    //lets get the id(user defined value they can use as a reference)
    if (request.isMember("id")) {
        response["id"] = request["id"];
    } else {
        response["id"] = Value(Json::nullValue);
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

    //see if custom method handler
    bool methodFound;
    for (const Method& method: _methods) {
        //check if correct method
        if (method.name != methodName) continue;
        methodFound = true;

        //get result of this method
        response["result"] = method.func(params); //this calls lambda

        //stop checking methods
        break;
    }
    if (!methodFound) {
        response["result"] = _api->sendcommand(methodName, params);
    }

    //add the null error value to show no errors and return
    response["error"] = Json::nullValue; // No error
    return response;
}


Value BitcoinRpcServer::createErrorResponse(int code, const string& message, const Value& request) {
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

void BitcoinRpcServer::sendResponse(tcp::socket& socket, const Value& response) {
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
 ██████╗ ██████╗ ███╗   ███╗███╗   ███╗ █████╗ ███╗   ██╗██████╗ ███████╗
██╔════╝██╔═══██╗████╗ ████║████╗ ████║██╔══██╗████╗  ██║██╔══██╗██╔════╝
██║     ██║   ██║██╔████╔██║██╔████╔██║███████║██╔██╗ ██║██║  ██║███████╗
██║     ██║   ██║██║╚██╔╝██║██║╚██╔╝██║██╔══██║██║╚██╗██║██║  ██║╚════██║
╚██████╗╚██████╔╝██║ ╚═╝ ██║██║ ╚═╝ ██║██║  ██║██║ ╚████║██████╔╝███████║
 ╚═════╝ ╚═════╝ ╚═╝     ╚═╝╚═╝     ╚═╝╚═╝  ╚═╝╚═╝  ╚═══╝╚═════╝ ╚══════╝
 List of modified and new commands.  Commands not listed here will be sent to core unmodified
 */

void BitcoinRpcServer::defineMethods() {

    _methods = {
            Method{
                    /**
                     * params[0] - txid(string)
                     * params[1] - verbose(optional bool=false)
                     * params[2] - ignored
                     *
                     * Returns same as before but now extra fields form DigiAsset::toJSON are not present
                     */
                    .name = "getrawtransaction",
                    .func = [this](const Json::Value& params) -> Value {
                        if (params.size() < 1 || params.size() > 3) {
                            throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
                        }
                        if (!params[0].isString()) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");

                        //get what core wallet has to say
                        Value rawTransactionData = _api->sendcommand("getrawtransaction", params);
                        if ((params.size() == 1) || (params[1].isBool() && params[1].asBool() == false)) {
                            return rawTransactionData;
                        }

                        //load transaction
                        DigiByteTransaction tx{params[0].asString()};

                        //convert to a value and return
                        return tx.toJSON(rawTransactionData);
                    }},
            Method{
                    /**
                     * params - see https://developer.bitcoin.org/reference/rpc/send.html
                     * only difference is we now accept domains
                     */
                    .name = "send",
                    .func = [this](const Json::Value& params) -> Value {
                        if (params.size() < 1 || params.size() > 5) {
                            throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
                        }
                        if (!params[0].isArray()) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");

                        //check if any domains in outputs
                        Value newParams = params;
                        for (Value& output: newParams[0]) {
                            auto it = output.begin();
                            string key = it.name();
                            if (DigiByteDomain::isDomain(key)) {
                                //change the domain into an address
                                string newKey = DigiByteDomain::getAddress(key);
                                output[newKey] = *it;
                                output.removeMember(key);
                            }
                        }

                        //send modified params to wallet
                        return _api->sendcommand("send", newParams);
                    }},
            Method{
                    /**
                     * params - see https://developer.bitcoin.org/reference/rpc/sendmany.html
                     * only difference is we now accept domains
                     */
                    .name = "sendmany",
                    .func = [this](const Json::Value& params) -> Value {
                        if (params.size() < 2 || params.size() > 9) {
                            throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
                        }
                        if (!params[1].isObject()) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");

                        //check if any domains in outputs
                        std::vector<std::string> keysToRemove;
                        Value newParams = params;
                        for (auto it = newParams[1].begin(); it != newParams[1].end(); ++it) {
                            std::string key = it.name();
                            Json::Value value = *it;

                            if (DigiByteDomain::isDomain(key)) {
                                //change the domain into an address
                                string newKey = DigiByteDomain::getAddress(key);
                                if (newParams[1].isMember(newKey)) {
                                    newParams[1][newKey] = newParams[1][newKey].asDouble() + value.asDouble();
                                } else {
                                    newParams[1][newKey] = value;
                                }

                                // Mark the old key for removal
                                keysToRemove.push_back(key);
                            }
                        }

                        // Remove the old keys
                        for (const auto& key: keysToRemove) {
                            newParams[1].removeMember(key);
                        }

                        //send modified params to wallet
                        return _api->sendcommand("sendmany", newParams);
                    }},
            Method{
                    /**
                     * params - see https://developer.bitcoin.org/reference/rpc/sendtoaddress.html
                     * only difference is we now accept domains
                     */
                    .name = "sendtoaddress",
                    .func = [this](const Json::Value& params) -> Value {
                        if (params.size() < 2 || params.size() > 9) {
                            throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
                        }
                        if (!params[0].isString()) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");

                        //check if any domains in outputs
                        std::vector<std::string> keysToRemove;
                        Value newParams = params;
                        if (DigiByteDomain::isDomain(newParams[0].asString())) {
                            //change the domain into an address
                            newParams[0] = DigiByteDomain::getAddress(newParams[0].asString());
                        }

                        //send modified params to wallet
                        return _api->sendcommand("sendtoaddress", newParams);
                    }},



            //new methods
            Method{
                    .name = "shutdown",
                    .func = [this](const Json::Value& params) -> Value {
                        _analyzer->stop();
                        AppMain::GetInstance()->getIPFS()->stop();
                        Log* log = Log::GetInstance();
                        log->addMessage("Safe to shut down", Log::CRITICAL);
                        return true;
                    }},
            Method{
                    /**
                     * Returns data about a specific asset
                     *
                     * Usage 1:
                     *  params[0] - assetIndex(integer)
                     *
                     * Usage 2:
                     *  params[0] - assetId(string)
                     *  params[1] - txid(string optional)
                     *  params[2] - vout(integer optional)
                     *  txid and vout are for any transaction involving the asset.  These are only needed for assets that
                     *  have more than 1 index.  All assets starting with L or Ua have only 1 index
                     *
                     * @return Json::Value - Returns a Json::Value object that represents the DigiAsset in JSON format.
                     *                       Refer to DigiAsset::toJSON for the format of the returned JSON object.
                     */
                    .name = "getassetdata",
                    .func = [this](const Json::Value& params) -> Value {
                        if (params.size() < 1 || params.size() > 3) {
                            throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
                        }

                        Database* db = AppMain::GetInstance()->getDatabase();
                        DigiAsset asset;

                        if (params.size() == 3) {
                            //definitely usage 2(all values included)
                            if (!params[0].isString() || !params[1].isString() || !params[2].isInt()) {
                                throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
                            }
                            asset = db->getAsset(db->getAssetIndex(
                                    params[0].asString(),
                                    params[1].asString(),
                                    params[2].asInt()));
                        } else if (params.size() == 2) {
                            throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
                        } else if (params[0].isString()) {
                            asset = db->getAsset(db->getAssetIndex(params[0].asString()));
                        } else if (params[0].isInt()) {
                            asset = db->getAsset(params[0].asInt());
                        } else {
                            throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
                        }

                        return asset.toJSON();
                    }},
            Method{
                    /**
                     * Returns the DigiByte address currently associated with a domain
                     *  params[0] - domain(string)
                     */
                    .name = "getdomainaddress",
                    .func = [this](const Json::Value& params) -> Value {
                        if (params.size() != 1) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
                        if (!params[0].isString()) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
                        return DigiByteDomain::getAddress(params[0].asString());
                    }},
            Method{
                    /**
                     * pins all ipfs meta data
                     * returns true - does not mean they are all downloaded yet.  Will likely take a while to finish
                     */
                    .name = "resyncmetadata",
                    .func = [this](const Json::Value& params) -> Value {
                        AppMain* main = AppMain::GetInstance();
                        PermanentStoragePoolList* pools = main->getPermanentStoragePoolList();
                        for (const auto& pool: *pools) {
                            if (!pool->subscribed()) continue;
                            pool->repinAllFiles();
                        }
                        return true;
                    }},
            Method{
                    /**
                     * Returns the current DigiByte block height and state of DigiAsset Sync
                     * {
                     *      count (unsigned int) - height DigiByte Core is currently synced to
                     *      sync (int) - negative numbers mean how many blocks behind DigiAsset processing is anything over 120 is unsafe to use
                     *                   0 = fully synced
                     *                   1 = stopped
                     *                   2 = initializing
                     *                   3 = rewinding
                     * }
                     */
                    .name = "syncstate",
                    .func = [this](const Json::Value& params) -> Value {
                        Value result = Value(Json::objectValue);
                        result["count"] = _api->getBlockCount();
                        result["sync"] = _analyzer->getSync();
                        return result;
                    }},
            Method{
                    /**
                     * Returns stats about addresses over time.
                     * Warning: The last result is likely not a full time period.
                     *
                     * Input Parameters:
                     *  params[0] - start time (integer, default = beginning(0))
                     *  params[1] - end time (integer, default = end(max value))
                     *  params[2] - time frame (integer, default = day(86400))
                     *
                     * Output Format:
                     *  JSON array of objects, each containing:
                     *    - "time": The end time of the time frame (integer)
                     *    - "new": Number of new addresses created (integer)
                     *    - "used": Number of addresses used (integer)
                     *    - "withAssets": Number of addresses with assets (integer)
                     *    - "over0": Number of addresses with balance over 0 (integer)
                     *    - "over1": Number of addresses with balance over 1 (integer)
                     *    - "over1k": Number of addresses with balance over 1k (integer)
                     *    - "over1m": Number of addresses with balance over 1m (integer)
                     *    - "quantumInsecure": Number of quantum insecure addresses (integer)
                     *    - "total": Total number of addresses (integer)
                     *
                     * Example Output:
                     *  [
                     *    {
                     *      "time": 1620000000,
                     *      "new": 100,
                     *      "used": 50,
                     *      "withAssets": 30,
                     *      "over0": 120,
                     *      "over1": 110,
                     *      "over1k": 5,
                     *      "over1m": 1,
                     *      "quantumInsecure": 10,
                     *      "total": 200
                     *    },
                     *    ...
                     *  ]
                     */
                    .name = "addressstats",
                    .func = [this](const Json::Value& params) -> Value {
                        //get paramaters
                        unsigned int timeFrame = 86400;
                        unsigned int start = 0;
                        unsigned int end = std::numeric_limits<unsigned int>::max();
                        if (params.size() > 3) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
                        if (params.size() >= 1) {
                            if (!params[0].isInt()) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
                            start = params[0].asInt();
                        }
                        if (params.size() >= 2) {
                            if (!params[1].isInt()) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
                            end = params[1].asInt();
                        }
                        if (params.size() == 3) {
                            if (!params[2].isInt()) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
                            timeFrame = params[2].asInt();
                        }

                        //lookup stats
                        try {
                            Database* db = AppMain::GetInstance()->getDatabase();
                            vector<AddressStats> stats = db->getAddressStats(start, end, timeFrame);

                            //convert to json
                            Value result = Value(Json::arrayValue);
                            for (const auto& stat: stats) {
                                Json::Value statObject(Json::objectValue);
                                statObject["time"] = stat.time;
                                statObject["new"] = stat.created;
                                statObject["used"] = stat.used;
                                statObject["withAssets"] = stat.withAssets;
                                statObject["over0"] = stat.over0;
                                statObject["over1"] = stat.over1;
                                statObject["over1k"] = stat.over1k;
                                statObject["over1m"] = stat.over1m;
                                statObject["quantumInsecure"] = stat.quantumInsecure;
                                statObject["total"] = stat.total;

                                // Add this statObject to the result array under a key (e.g., the index or time)
                                result.append(statObject);
                            }
                            return result;

                        } catch (const Database::exceptionDataPruned& e) {
                            return Json::arrayValue;
                        }
                    }},
            Method{
                    /**
                     * Returns mining stats over a given time period
                     * Warning: The last result is likely not a full time period.
                     *
                     * Input Parameters:
                     *  params[0] - start time (integer, default = beginning(0))
                     *  params[1] - end time (integer, default = end(max value))
                     *  params[2] - time frame (integer, default = day(86400))
                     *
                     * Output Format:
                     *  JSON array of objects, each containing:
                     *    - "time": The end time of the time frame (integer)
                     *    - "algo": An array of objects, one for each algorithm, containing:
                     *        - "min": Minimum difficulty (float)
                     *        - "max": Maximum difficulty (float)
                     *        - "avg": Average difficulty (float)
                     *        - "count": Number of blocks (integer)
                     *
                     * Example Output:
                     *  [
                     *    {
                     *      "time": 1620000000,
                     *      "algo": [
                     *        {"min": 0.1, "max": 0.2, "avg": 0.15, "count": 50},
                     *        null,
                     *        {"min": 0.3, "max": 0.4, "avg": 0.35, "count": 40}
                     *      ]
                     *    },
                     *    ...
                     *  ]
                     *
                     * Note: 'null' is used to fill in missing algorithms.
                     */
                    .name = "algostats",
                    .func = [this](const Json::Value& params) -> Value {
                        //get paramaters
                        unsigned int timeFrame = 86400;
                        unsigned int start = 0;
                        unsigned int end = std::numeric_limits<unsigned int>::max();
                        if (params.size() > 3) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
                        if (params.size() >= 1) {
                            if (!params[0].isInt()) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
                            start = params[0].asInt();
                        }
                        if (params.size() >= 2) {
                            if (!params[1].isInt()) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
                            end = params[1].asInt();
                        }
                        if (params.size() == 3) {
                            if (!params[2].isInt()) throw DigiByteException(RPC_INVALID_PARAMS, "Invalid params");
                            timeFrame = params[2].asInt();
                        }

                        //lookup stats
                        try {
                            Database* db = AppMain::GetInstance()->getDatabase();
                            vector<AlgoStats> stats = db->getAlgoStats(start, end, timeFrame);

                            // Initialize Json::Value
                            Json::Value result(Json::arrayValue);

                            // Initialize variables to keep track of the current time and algo array
                            unsigned int currentTime = 0;
                            int lastAlgo = -1;
                            Json::Value currentTimeObject(Json::objectValue);
                            Json::Value currentAlgoArray(Json::arrayValue);

                            // Populate result from stats
                            for (const auto& stat: stats) {
                                if (stat.time == 0) continue;
                                if (stat.time != currentTime) {
                                    // Save the previous time object if it exists
                                    if (!currentAlgoArray.empty()) {
                                        currentTimeObject["algo"] = currentAlgoArray;
                                        result.append(currentTimeObject);
                                    }

                                    // Reset for the new time
                                    currentTime = stat.time;
                                    lastAlgo = -1;
                                    currentTimeObject = Json::Value(Json::objectValue);
                                    currentTimeObject["time"] = currentTime;
                                    currentAlgoArray = Json::Value(Json::arrayValue);
                                }

                                // Fill in missing algos with null values
                                while (static_cast<unsigned int>(lastAlgo + 1) < stat.algo) {
                                    ++lastAlgo;
                                    currentAlgoArray.append(Json::Value(Json::nullValue));
                                }

                                // Create and append the algo object
                                Json::Value algoObject(Json::objectValue);
                                algoObject["min"] = stat.difficultyMin;
                                algoObject["max"] = stat.difficultyMax;
                                algoObject["avg"] = stat.difficultyAvg;
                                algoObject["count"] = stat.blocks;
                                currentAlgoArray.append(algoObject);

                                lastAlgo = stat.algo;
                            }

                            // Append the last time object if it exists
                            if (!currentAlgoArray.empty()) {
                                currentTimeObject["algo"] = currentAlgoArray;
                                result.append(currentTimeObject);
                            }

                            //return results
                            return result;
                        } catch (const Database::exceptionDataPruned& e) {
                            return Json::arrayValue;
                        }
                    }}};
}


/*
 ██████╗ ███████╗████████╗████████╗███████╗██████╗ ███████╗
██╔════╝ ██╔════╝╚══██╔══╝╚══██╔══╝██╔════╝██╔══██╗██╔════╝
██║  ███╗█████╗     ██║      ██║   █████╗  ██████╔╝███████╗
██║   ██║██╔══╝     ██║      ██║   ██╔══╝  ██╔══██╗╚════██║
╚██████╔╝███████╗   ██║      ██║   ███████╗██║  ██║███████║
 ╚═════╝ ╚══════╝   ╚═╝      ╚═╝   ╚══════╝╚═╝  ╚═╝╚══════╝
 */
unsigned int BitcoinRpcServer::getPort() {
    return _port;
}

bool BitcoinRpcServer::isRPCAllowed(const string& method) {
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
