//
// Created by DigiAsset Core on 19/07/26.
//

#include "EventBroadcaster.h"
#include "AcceptWaker.h"
#include "Log.h"

using boost::asio::ip::tcp;
using namespace std;

EventBroadcaster* EventBroadcaster::_pinstance = nullptr;
std::mutex& EventBroadcaster::getLock() {
    static std::mutex* m = new std::mutex; //intentionally leaked - see header
    return *m;
}

EventBroadcaster* EventBroadcaster::GetInstance() {
    std::lock_guard<std::mutex> lock(getLock());
    if (_pinstance == nullptr) _pinstance = new EventBroadcaster();
    return _pinstance;
}

void EventBroadcaster::start(unsigned int port, const std::string& bindAddress) {
    if (_running || (port == 0)) return;
    try {
        //no auth on this stream so default to loopback; config "eventbind" widens it deliberately
        tcp::endpoint endpoint(boost::asio::ip::make_address(bindAddress), port);
        _acceptor = std::unique_ptr<tcp::acceptor>(new tcp::acceptor(_io, endpoint));
    } catch (const std::exception& e) {
        Log* log = Log::GetInstance();
        log->addMessage("Event stream could not bind " + bindAddress + ":" + to_string(port) + ": " + e.what(), Log::WARNING);
        return;
    }
    _running = true;
    _acceptThread = std::thread([this]() {
        while (_running) {
            try {
                auto socket = std::make_shared<tcp::socket>(_io);
                _acceptor->accept(*socket);
                socket->non_blocking(true); //writes must never block the daemon
                std::lock_guard<std::mutex> lock(_clientsMutex);
                _clients.push_back(socket);
            } catch (const std::exception& e) {
                if (_running) std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
    });
    Log::GetInstance()->addMessage("Event stream listening on " + bindAddress + ":" + to_string(port));
}

void EventBroadcaster::stop() {
    if (!_running) return;
    _running = false;
    //the accept thread is parked in accept() and closing the acceptor will not bring it back, so
    //knock on the door first - without this the daemon never exits unless something connects
    if (_acceptor != nullptr) wakeBlockedAccept(*_acceptor);
    try {
        _acceptor->close();
    } catch (...) {}
    if (_acceptThread.joinable()) _acceptThread.join();
    std::lock_guard<std::mutex> lock(_clientsMutex);
    _clients.clear();
}

EventBroadcaster::~EventBroadcaster() {
    stop();
}

void EventBroadcaster::broadcast(const std::string& jsonLine) {
    if (!_running) return;
    std::string data = jsonLine + "\n";
    std::lock_guard<std::mutex> lock(_clientsMutex);
    for (auto it = _clients.begin(); it != _clients.end();) {
        boost::system::error_code error;
        boost::asio::write(**it, boost::asio::buffer(data), error);
        if (error) {
            //client gone or too slow(non blocking socket would_block) - drop it
            it = _clients.erase(it);
        } else {
            ++it;
        }
    }
}
