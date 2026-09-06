//
// Socket-level integration tests for RPC::Server (request routing + auth).
//
// These tests start real Server instances on test ports and talk to them over
// TCP with raw HTTP, so they exercise the full pipeline:
//   accept → parseRequest (headers/auth/body) → handleRpcRequest →
//   executeCall (allowlist, cache, method dispatch) → sendResponse.
//
// Only the "version" method is used for the happy path — it needs no wallet,
// database, or IPFS, so these tests run without any external services.
//

#include "AppMain.h"
#include "RPC/Cache.h"
#include "RPC/Server.h"
#include "Version.h"
#include "gtest/gtest.h"
#include <boost/asio.hpp>
#include <cstdio>
#include <fstream>
#include <jsoncpp/json/value.h>
#include <atomic>
#include <chrono>
#include <memory>
#include <sstream>
#include <string>
#include <thread>

using namespace std;

namespace {
    constexpr unsigned int OPEN_PORT = 42124;       // rpcallow*=1
    constexpr unsigned int RESTRICTED_PORT = 42125; // only version allowed

    const string OPEN_CONFIG = "serverTestOpen.cfg";
    const string RESTRICTED_CONFIG = "serverTestRestricted.cfg";

    // base64("testuser:testpass") / base64("testuser:wrongpass")
    const string GOOD_AUTH = "dGVzdHVzZXI6dGVzdHBhc3M=";
    const string BAD_AUTH = "dGVzdHVzZXI6d3JvbmdwYXNz";
} // namespace

class RPCServerTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        // write config files (tests run from bin/)
        ofstream open(OPEN_CONFIG);
        open << "rpcuser=testuser\n"
             << "rpcpassword=testpass\n"
             << "rpcassetport=" << OPEN_PORT << "\n"
             << "rpcparallel=2\n"
             << "rpcallow*=1\n";
        open.close();

        ofstream restricted(RESTRICTED_CONFIG);
        restricted << "rpcuser=testuser\n"
                   << "rpcpassword=testpass\n"
                   << "rpcassetport=" << RESTRICTED_PORT << "\n"
                   << "rpcparallel=2\n"
                   << "rpcallow*=0\n"
                   << "rpcallowversion=1\n";
        restricted.close();

        // executeCall needs the RPC cache on AppMain; version needs nothing else
        cache = new RPC::Cache();
        AppMain::GetInstance()->setRpcCache(cache);

        openServer = new RPC::Server(OPEN_CONFIG);
        restrictedServer = new RPC::Server(RESTRICTED_CONFIG);
        openServer->start();
        restrictedServer->start();
    }

    static void TearDownTestSuite() {
        // delete = stop(): joins every server thread.  Leaving them running
        // used to crash the process at exit("mutex lock failed") when a still
        // live thread touched an already destroyed static mutex.
        delete openServer;
        openServer = nullptr;
        delete restrictedServer;
        restrictedServer = nullptr;
        AppMain::GetInstance()->reset();
        delete cache;
        cache = nullptr;
        remove(OPEN_CONFIG.c_str());
        remove(RESTRICTED_CONFIG.c_str());
    }

    // Send a raw string to the server and return everything it replies with
    // (the server closes the socket after responding, so read until EOF).
    static string httpRequest(unsigned int port, const string& raw) {
        boost::asio::io_context io;
        tcp::socket sock(io);
        sock.connect(tcp::endpoint(boost::asio::ip::address_v4::loopback(), port));
        boost::asio::write(sock, boost::asio::buffer(raw));

        string response;
        boost::system::error_code ec;
        char buf[1024];
        while (!ec) {
            size_t n = sock.read_some(boost::asio::buffer(buf), ec);
            response.append(buf, n);
        }
        return response;
    }

    static string buildRequest(const string& body, const string& authB64) {
        string request = "POST / HTTP/1.1\r\n";
        request += "Host: 127.0.0.1\r\n";
        if (!authB64.empty()) request += "Authorization: Basic " + authB64 + "\r\n";
        request += "Content-Length: " + to_string(body.length()) + "\r\n";
        request += "\r\n";
        request += body;
        return request;
    }

    // POST a JSON-RPC body and return the parsed JSON response body
    static Json::Value rpcCall(unsigned int port, const string& body, const string& authB64 = GOOD_AUTH) {
        string response = httpRequest(port, buildRequest(body, authB64));

        size_t bodyStart = response.find("\r\n\r\n");
        EXPECT_NE(bodyStart, string::npos) << "no body in response: " << response;
        if (bodyStart == string::npos) return Json::nullValue;

        Json::CharReaderBuilder readerBuilder;
        Json::Value doc;
        string errs;
        istringstream stream(response.substr(bodyStart + 4));
        EXPECT_TRUE(Json::parseFromStream(readerBuilder, stream, &doc, &errs)) << errs;
        return doc;
    }

    static RPC::Server* openServer;
    static RPC::Server* restrictedServer;
    static RPC::Cache* cache;
};

RPC::Server* RPCServerTest::openServer = nullptr;
RPC::Server* RPCServerTest::restrictedServer = nullptr;
RPC::Cache* RPCServerTest::cache = nullptr;

/*
 * Direct (non-socket) behavior
 */

// Regression: stop() closed the acceptor and joined the accept thread, but closing a socket does
// not wake a thread already inside accept() on Linux.  A server nothing is talking to therefore
// never finished stopping, which held up shutdown of the whole daemon.
TEST_F(RPCServerTest, stopReturnsWithNothingConnected) {
    const string config = "serverTestShutdown.cfg";
    ofstream out(config);
    out << "rpcuser=testuser\n"
        << "rpcpassword=testpass\n"
        << "rpcassetport=42126\n"
        << "rpcparallel=2\n"
        << "rpcallow*=1\n";
    out.close();

    RPC::Server* server = new RPC::Server(config);
    server->start();
    //the accept thread has to actually be inside accept() for this to test anything - stop it too
    //early and the acceptor closes first, accept() fails at once and the broken code passes too
    this_thread::sleep_for(chrono::milliseconds(300));

    auto done = make_shared<atomic<bool>>(false);
    thread([server, done]() {
        server->stop();
        done->store(true);
    }).detach(); //deliberately detached: if stop() hangs there is nothing safe to join

    auto deadline = chrono::steady_clock::now() + chrono::seconds(5);
    while ((chrono::steady_clock::now() < deadline) && !done->load()) {
        this_thread::sleep_for(chrono::milliseconds(20));
    }

    EXPECT_TRUE(done->load()) << "stop() did not return - the accept thread is still parked in accept()";
    if (done->load()) delete server; //only safe to destroy once its threads are joined
    remove(config.c_str());
}

TEST_F(RPCServerTest, getPortReturnsConfiguredPort) {
    EXPECT_EQ(openServer->getPort(), OPEN_PORT);
    EXPECT_EQ(restrictedServer->getPort(), RESTRICTED_PORT);
}

TEST_F(RPCServerTest, isRPCAllowedWildcardDefault) {
    EXPECT_TRUE(openServer->isRPCAllowed("version"));
    EXPECT_TRUE(openServer->isRPCAllowed("neverheardofit"));
}

TEST_F(RPCServerTest, isRPCAllowedExplicitList) {
    EXPECT_TRUE(restrictedServer->isRPCAllowed("version"));
    EXPECT_FALSE(restrictedServer->isRPCAllowed("getblockcount"));
    EXPECT_FALSE(restrictedServer->isRPCAllowed("neverheardofit"));
}

/*
 * Socket-level: authentication and malformed requests
 */

TEST_F(RPCServerTest, missingAuthorizationHeaderRejected) {
    Json::Value response = rpcCall(OPEN_PORT, R"({"method":"version"})", "");
    EXPECT_EQ(response["error"]["code"].asInt(), HTTP_BAD_REQUEST);
    EXPECT_TRUE(response["result"].isNull());
}

TEST_F(RPCServerTest, wrongCredentialsRejected) {
    Json::Value response = rpcCall(OPEN_PORT, R"({"method":"version"})", BAD_AUTH);
    EXPECT_EQ(response["error"]["code"].asInt(), HTTP_UNAUTHORIZED);
    EXPECT_TRUE(response["result"].isNull());
}

TEST_F(RPCServerTest, requestWithoutBlankLineRejected) {
    // no \r\n\r\n → server can't find a body
    string response = httpRequest(OPEN_PORT, "POST / HTTP/1.1\r\nHost: 127.0.0.1\r\n");
    EXPECT_NE(response.find("\"code\":" + to_string(HTTP_BAD_REQUEST)), string::npos) << response;
}

TEST_F(RPCServerTest, invalidJsonRejected) {
    Json::Value response = rpcCall(OPEN_PORT, "this is not json");
    EXPECT_EQ(response["error"]["code"].asInt(), RPC_PARSE_ERROR);
}

/*
 * Socket-level: JSON-RPC validation
 */

TEST_F(RPCServerTest, missingMethodRejected) {
    Json::Value response = rpcCall(OPEN_PORT, R"({"id":7})");
    EXPECT_EQ(response["error"]["code"].asInt(), RPC_METHOD_NOT_FOUND);
    EXPECT_EQ(response["id"].asInt(), 7); // id echoed back even on error
}

TEST_F(RPCServerTest, nonStringMethodRejected) {
    Json::Value response = rpcCall(OPEN_PORT, R"({"method":5})");
    EXPECT_EQ(response["error"]["code"].asInt(), RPC_METHOD_NOT_FOUND);
}

TEST_F(RPCServerTest, nonArrayParamsRejected) {
    Json::Value response = rpcCall(OPEN_PORT, R"({"method":"version","params":{"a":1}})");
    EXPECT_EQ(response["error"]["code"].asInt(), RPC_INVALID_PARAMS);
}

TEST_F(RPCServerTest, forbiddenMethodRejected) {
    Json::Value response = rpcCall(RESTRICTED_PORT, R"({"method":"getblockcount","params":[]})");
    EXPECT_EQ(response["error"]["code"].asInt(), RPC_FORBIDDEN_BY_SAFE_MODE);
}

/*
 * Socket-level: happy path
 */

TEST_F(RPCServerTest, versionHappyPath) {
    string response = httpRequest(OPEN_PORT, buildRequest(R"({"method":"version","params":[],"id":42})", GOOD_AUTH));
    EXPECT_EQ(response.rfind("HTTP/1.1 200 OK\r\n", 0), 0u) << response;

    Json::Value body = rpcCall(OPEN_PORT, R"({"method":"version","params":[],"id":42})");
    EXPECT_TRUE(body["error"].isNull()) << body.toStyledString();
    EXPECT_EQ(body["result"].asString(), getVersionString());
    EXPECT_EQ(body["id"].asInt(), 42);
}

TEST_F(RPCServerTest, versionAllowedOnRestrictedServer) {
    Json::Value body = rpcCall(RESTRICTED_PORT, R"({"method":"version","params":[]})");
    EXPECT_TRUE(body["error"].isNull()) << body.toStyledString();
    EXPECT_EQ(body["result"].asString(), getVersionString());
}

TEST_F(RPCServerTest, repeatedCallServedFromCache) {
    // second identical call goes through Cache::isCached — must return the same result
    Json::Value first = rpcCall(OPEN_PORT, R"({"method":"version","params":[],"id":1})");
    Json::Value second = rpcCall(OPEN_PORT, R"({"method":"version","params":[],"id":2})");
    EXPECT_EQ(first["result"], second["result"]);
    EXPECT_EQ(second["id"].asInt(), 2);
}
