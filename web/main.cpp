//
// Created by mctrivia on 12/04/24.
//

#include "Config.h"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/config.hpp>
#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>

using namespace std;
using tcp = boost::asio::ip::tcp;                   // from <boost/asio/ip/tcp.hpp>
namespace http = boost::beast::http;                // from <boost/beast/http.hpp>
namespace fs = std::filesystem;

//taken from src/utils.cpp didn't use direct because didn't want to include other requirements
bool fileExists(const std::string& fileName) {
    //see if this is first run
    struct stat buffer {};
    return (stat(fileName.c_str(), &buffer) == 0);
}

// This function produces an HTTP response for the given request.
// The type of the request body and the response body must match,
// so the body type is templated to allow flexibility.
template<class Body, class Allocator, class Send>
void handle_request(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send)
{
    // Returns a bad request response
    auto bad_request = [&req](boost::beast::string_view why)
    {
        http::response<http::string_body> res{http::status::bad_request, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = string(why);
        res.prepare_payload();
        return res;
    };

    // Returns a not found response
    auto not_found = [&req](boost::beast::string_view target)
    {
        http::response<http::string_body> res{http::status::not_found, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = "The resource '" + string(target) + "' was not found.";
        res.prepare_payload();
        return res;
    };

    // Returns a server error response
    /*
    auto server_error = [&req](boost::beast::string_view what)
    {
        http::response<http::string_body> res{http::status::internal_server_error, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = "An error occurred: '" + string(what) + "'";
        res.prepare_payload();
        return res;
    };*/

    // Handle GET request
    if(req.method() != http::verb::get)
        return send(bad_request("Unknown HTTP-method"));

    if(req.target().empty() ||
        req.target()[0] != '/' ||
        req.target().find("..") != boost::beast::string_view::npos)
        return send(bad_request("Illegal request-target"));

    // Build the path to the requested file
    string path;
    if (req.target().substr(0,5)=="/src/") {

        //get from source folder
        path = ".." + string(req.target());

    } else if (req.target().substr(0,5)=="/rpc/") {

        //rpc method check if it has been overwritten
        path="../src/RPC/Methods/"+string(req.target()).substr(5);
        if (!fileExists(path)) {
            path = "../web" + string(req.target());
        }
    } else  {

        //normal path
        path = "../web" + string(req.target());

    }
    if(req.target().back() == '/')
        path += "index.html";
    cout << "Request: " << path << "\n";

    // Attempt to open the file
    ifstream is(path, ifstream::binary);
    if(!is)
        return send(not_found(req.target()));

    // Read the file
    string content((istreambuf_iterator<char>(is)), istreambuf_iterator<char>());

    // Respond to GET request
    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/html");
    res.body() = content;
    res.prepare_payload();
    res.keep_alive(req.keep_alive());
    send(std::move(res));
}

int main()
{
    while (true) {
        try {
            Config config("config.cfg");
            auto const address = boost::asio::ip::make_address("0.0.0.0");
            unsigned short port = config.getInteger("webport", 8080);

            boost::asio::io_context ioc{1};
            tcp::acceptor acceptor{ioc, {address, port}};
            while (true) {
                tcp::socket socket{ioc};
                acceptor.accept(socket);

                // This lambda is used to send responses
                auto const send = [&socket](auto&& response) {
                    // We need to declare these as per Boost.Beast recommendations
                    auto sp = make_shared<decay_t<decltype(response)>>(forward<decltype(response)>(response));

                    // Write the response
                    http::async_write(socket, *sp,
                                      [sp, &socket](boost::system::error_code ec, size_t) {
                                          socket.shutdown(tcp::socket::shutdown_send, ec);
                                      });
                };

                boost::beast::flat_buffer buffer;
                http::request<http::string_body> req;
                http::read(socket, buffer, req);
                handle_request(std::move(req), send);
            }
        } catch (const std::exception& e) {
            cerr << "Error: " << e.what() << endl;
        }
    }
    return EXIT_SUCCESS;
}
