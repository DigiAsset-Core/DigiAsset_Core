//
// Tests for EventBroadcaster - no external services needed.
//
// Regression: stop() closed the acceptor and joined the accept thread, but a thread already inside
// a blocking accept() is not woken by closing the socket on Linux.  With nothing connected to the
// event stream the join never returned, so the daemon logged "Shutdown signal received" and then
// sat there until something opened a TCP connection to the stream port.
//

#include "EventBroadcaster.h"
#include "gtest/gtest.h"
#include <atomic>
#include <boost/asio.hpp>
#include <chrono>
#include <thread>

using namespace std;

namespace {
    constexpr unsigned int EVENT_TEST_PORT = 42130;
    constexpr unsigned int EVENT_TEST_PORT_WITH_CLIENT = 42131;

    // Runs stop() on its own thread so a hang fails the test instead of hanging the suite.
    bool stopsWithin(EventBroadcaster* broadcaster, chrono::seconds limit) {
        auto done = make_shared<atomic<bool>>(false);
        thread([broadcaster, done]() {
            broadcaster->stop();
            done->store(true);
        }).detach(); //deliberately detached: if stop() hangs there is nothing safe to join

        auto deadline = chrono::steady_clock::now() + limit;
        while (chrono::steady_clock::now() < deadline) {
            if (done->load()) return true;
            this_thread::sleep_for(chrono::milliseconds(20));
        }
        return false;
    }
} // namespace

TEST(EventBroadcaster, stopReturnsWithNothingConnected) {
    EventBroadcaster* broadcaster = EventBroadcaster::GetInstance();
    broadcaster->start(EVENT_TEST_PORT, "127.0.0.1");

    //the accept thread has to actually be inside accept() for this to test anything.  Without the
    //wait, stop() closes the acceptor before the thread gets there, accept() fails immediately and
    //the test passes against the broken code as happily as against the fixed code
    this_thread::sleep_for(chrono::milliseconds(300));

    EXPECT_TRUE(stopsWithin(broadcaster, chrono::seconds(5)))
            << "stop() did not return - the accept thread is still parked in accept()";
}

TEST(EventBroadcaster, stopReturnsWithAClientConnected) {
    EventBroadcaster* broadcaster = EventBroadcaster::GetInstance();
    broadcaster->start(EVENT_TEST_PORT_WITH_CLIENT, "127.0.0.1");

    boost::asio::io_context io;
    boost::asio::ip::tcp::socket client(io);
    boost::system::error_code error;
    client.connect(boost::asio::ip::tcp::endpoint(
                           boost::asio::ip::make_address("127.0.0.1"), EVENT_TEST_PORT_WITH_CLIENT),
                   error);
    ASSERT_FALSE(error) << "could not connect to the event stream";
    this_thread::sleep_for(chrono::milliseconds(100)); //let the accept land before stopping

    broadcaster->broadcast("{\"event\":\"test\"}"); //a live client must not stop it shutting down
    EXPECT_TRUE(stopsWithin(broadcaster, chrono::seconds(5)));
    client.close(error);
}

TEST(EventBroadcaster, stopOnANeverStartedBroadcasterDoesNothing) {
    // port 0 means disabled, so there is no acceptor at all to wake
    EventBroadcaster* broadcaster = EventBroadcaster::GetInstance();
    broadcaster->start(0, "127.0.0.1");
    this_thread::sleep_for(chrono::milliseconds(300));
    EXPECT_TRUE(stopsWithin(broadcaster, chrono::seconds(5)));
}
