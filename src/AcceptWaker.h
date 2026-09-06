//
// Created by DigiAsset Core on 25/08/26.
//

#ifndef DIGIASSET_CORE_ACCEPTWAKER_H
#define DIGIASSET_CORE_ACCEPTWAKER_H

#include <boost/asio.hpp>

/**
 * Wakes a thread that is sitting in a blocking acceptor::accept().
 *
 * Closing the acceptor does not do it, however much it looks like it should.  A thread already
 * inside accept() is parked in the kernel on the listening socket, and closing the descriptor from
 * another thread leaves it parked - it only comes back when a connection actually arrives.  So a
 * daemon that nothing is connected to hangs on shutdown until something connects: the event stream
 * held the whole process open that way, with the log already saying "Shutdown complete" was next.
 *
 * So open the connection ourselves.  The accept returns with a throwaway client, the loop sees its
 * stop flag and leaves.  Call this BEFORE closing the acceptor, while it is still listening.
 *
 * @param acceptor - listening acceptor whose accept() thread needs waking
 */
inline void wakeBlockedAccept(boost::asio::ip::tcp::acceptor& acceptor) {
    try {
        if (!acceptor.is_open()) return;
        boost::system::error_code error;
        boost::asio::ip::tcp::endpoint endpoint = acceptor.local_endpoint(error);
        if (error) return;

        //a wildcard bind(0.0.0.0 or ::) is not a connectable address everywhere, so aim the wake up
        //at loopback instead - a listener bound to the wildcard answers there too
        if (endpoint.address().is_unspecified()) {
            endpoint.address(endpoint.address().is_v6()
                                     ? boost::asio::ip::address(boost::asio::ip::address_v6::loopback())
                                     : boost::asio::ip::address(boost::asio::ip::address_v4::loopback()));
        }

        boost::asio::io_context io;
        boost::asio::ip::tcp::socket waker(io);
        waker.connect(endpoint, error);
        waker.close(error);
    } catch (...) {
        //nothing listening any more, or it never started.  The join that follows settles it either way
    }
}

#endif //DIGIASSET_CORE_ACCEPTWAKER_H
