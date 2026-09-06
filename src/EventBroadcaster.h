//
// Created by DigiAsset Core on 19/07/26.
//

#ifndef DIGIASSET_CORE_EVENTBROADCASTER_H
#define DIGIASSET_CORE_EVENTBROADCASTER_H

#include <atomic>
#include <boost/asio.hpp>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

/**
 * Pushes daemon events to any connected TCP client as newline delimited JSON.
 * Connect(default port 14025, config key "eventport", 0 disables), read lines:
 *    {"event":"newBlock","height":23842000,"blocksBehind":0}
 *    {"event":"assetIssued","assetIds":["La..."],"txid":"...","height":...}
 *    {"event":"assetTransfer","assetIds":[...],"txid":"...","height":...}
 *    {"event":"assetBurn","assetIds":[...],"txid":"...","height":...}
 *    {"event":"balanceChanged","addresses":[...],"txid":"...","height":...}
 * newBlock is only sent near the tip; the asset events fire for every asset bearing
 * transaction processed(rare enough to be safe even during initial sync).
 * There is NO authentication so by default the stream only listens on
 * 127.0.0.1.  Set config key "eventbind" (e.g. eventbind=0.0.0.0) to expose it
 * to the LAN if you understand the risk.
 * Writes never block the caller: clients that can't keep up are disconnected.
 * Consumers needing reliability should treat the stream as a wake up signal and
 * re-query the RPC interface for state.
 */
class EventBroadcaster {
private:
    static EventBroadcaster* _pinstance;
    static std::mutex& getLock(); //never destroyed - safe to use during process exit

    boost::asio::io_context _io;
    std::unique_ptr<boost::asio::ip::tcp::acceptor> _acceptor;
    std::vector<std::shared_ptr<boost::asio::ip::tcp::socket>> _clients;
    std::mutex _clientsMutex;
    std::thread _acceptThread;
    std::atomic<bool> _running{false}; //read by the accept thread, written by stop()

    EventBroadcaster() = default;

public:
    static EventBroadcaster* GetInstance();

    void start(unsigned int port, const std::string& bindAddress = "127.0.0.1"); //port 0 = disabled
    void stop();
    ~EventBroadcaster();

    ///sends one event line to every connected client(never blocks, drops slow clients)
    void broadcast(const std::string& jsonLine);

    EventBroadcaster(const EventBroadcaster&) = delete;
    void operator=(const EventBroadcaster&) = delete;
};

#endif //DIGIASSET_CORE_EVENTBROADCASTER_H
