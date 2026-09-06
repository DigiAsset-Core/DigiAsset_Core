//
// Created by mctrivia on 13/06/23.
//

#include "gtest/gtest.h"
#include "IPFS.h"

#include <boost/asio.hpp>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <functional>

using namespace std;


TEST(IPFS, sha256ToCID) {
    EXPECT_EQ(IPFS::sha256ToCID("51D3CC662F89E8535D9CF74751DA0F91335A083CF12CB4C9BA81FFF25458274D"),
              "bafkreicr2pggml4j5bjv3hhxi5i5ud4rgnnaqphrfs2mtoub77zfiwbhju");
    BitIO testData = BitIO::makeHexString("51D3CC662F89E8535D9CF74751DA0F91335A083CF12CB4C9BA81FFF25458274D");
    EXPECT_EQ(IPFS::sha256ToCID(testData), "bafkreicr2pggml4j5bjv3hhxi5i5ud4rgnnaqphrfs2mtoub77zfiwbhju");
}

// ─────────────────────────────────────────────────────────────────────────────
// Static helper methods — no daemon required
// ─────────────────────────────────────────────────────────────────────────────

TEST(IPFS, isValidCID) {
    // Known valid CID (alphanumeric only)
    EXPECT_TRUE(IPFS::isValidCID("bafkreicr2pggml4j5bjv3hhxi5i5ud4rgnnaqphrfs2mtoub77zfiwbhju"));
    EXPECT_TRUE(IPFS::isValidCID("QmYwAPJzv5CZsnANOEV4KmUTRMGW9TQYAKnGJkFsaMoZg2"));

    // Empty string is invalid
    EXPECT_FALSE(IPFS::isValidCID(""));

    // Spaces and slashes are invalid
    EXPECT_FALSE(IPFS::isValidCID("has space"));
    EXPECT_FALSE(IPFS::isValidCID("has/slash"));
    EXPECT_FALSE(IPFS::isValidCID("has-dash"));
    EXPECT_FALSE(IPFS::isValidCID("has.dot"));
}

TEST(IPFS, isIPFSurl) {
    // Valid IPFS URL
    EXPECT_TRUE(IPFS::isIPFSurl("ipfs://bafkreicr2pggml4j5bjv3hhxi5i5ud4rgnnaqphrfs2mtoub77zfiwbhju"));

    // Non-IPFS schemes
    EXPECT_FALSE(IPFS::isIPFSurl("http://example.com/file.png"));
    EXPECT_FALSE(IPFS::isIPFSurl("https://example.com/file.png"));
    EXPECT_FALSE(IPFS::isIPFSurl(""));

    // Prefix only (no CID after)
    EXPECT_FALSE(IPFS::isIPFSurl("ipfs://"));

    // CID with invalid character after prefix
    EXPECT_FALSE(IPFS::isIPFSurl("ipfs://has space"));
}

TEST(IPFS, getCID) {
    const string cid = "bafkreicr2pggml4j5bjv3hhxi5i5ud4rgnnaqphrfs2mtoub77zfiwbhju";
    EXPECT_EQ(IPFS::getCID("ipfs://" + cid), cid);

    // Throws on non-IPFS URL
    EXPECT_THROW(IPFS::getCID("http://example.com"), std::out_of_range);
    EXPECT_THROW(IPFS::getCID(""), std::out_of_range);
    EXPECT_THROW(IPFS::getCID(cid), std::out_of_range); // bare CID without scheme
}

TEST(IPFS, isLostCID) {
    // Known lost CID (first entry from _knownLostCID)
    EXPECT_TRUE(IPFS::isLostCID("bafkreiabavnsbsrrlfgisxcgmd7ontytbyh2ilruux7gjfc2hzi4qa5vxy"));

    // Valid but not in the lost list
    EXPECT_FALSE(IPFS::isLostCID("bafkreicr2pggml4j5bjv3hhxi5i5ud4rgnnaqphrfs2mtoub77zfiwbhju"));
    EXPECT_FALSE(IPFS::isLostCID(""));
    EXPECT_FALSE(IPFS::isLostCID("QmNotInTheLostList"));
}
// ─────────────────────────────────────────────────────────────────────────────
// Timeouts — a node that stops answering must not stop the caller
// ─────────────────────────────────────────────────────────────────────────────
//
// Regression: every request built by _command left CURLOPT_TIMEOUT_MS unset unless the caller
// asked for one, and most callers do not.  getSize() is one of them, and it used to run on the
// chain analyzer's own thread(a storage pool sizes each file an issuance references), so a
// wedged ipfs daemon stopped the whole node on whichever block held the next issuance -
// silently, because progress is only logged once a block finishes.
//
// The socket these tests use completes the TCP handshake from the listen backlog and then never
// says another word, which is exactly what that looks like from the client side.

namespace {
    ///a listening socket that accepts connections and then answers nothing
    class SilentNode {
    public:
        SilentNode()
            : _acceptor(_io, boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address("127.0.0.1"), 0)) {}
        unsigned short port() { return _acceptor.local_endpoint().port(); }

    private:
        boost::asio::io_context _io;
        boost::asio::ip::tcp::acceptor _acceptor;
    };

    ///writes a config pointing IPFS at the silent node and returns its file name
    string writeSilentNodeConfig(const string& fileName, unsigned short port, unsigned int commandSeconds,
                                 unsigned int downloadSeconds) {
        ofstream config(fileName);
        config << "ipfspath=http://127.0.0.1:" << port << "/api/v0/\n";
        config << "ipfstimeoutcommand=" << commandSeconds << "\n";
        config << "ipfstimeoutdownload=" << downloadSeconds << "\n";
        return fileName;
    }

    long long secondsToRun(const function<void()>& work) {
        auto start = chrono::steady_clock::now();
        work();
        return chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - start).count();
    }
} // namespace

TEST(IPFS, localCommandGivesUpWhenTheNodeStopsAnswering) {
    SilentNode node;
    const string configFile = "_ipfs_command_timeout_test.cfg";
    writeSilentNodeConfig(configFile, node.port(), 2, 60);

    IPFS ipfs(configFile, false); //false: no job threads, these tests make the calls themselves

    //isPinned only asks the local node for its pin list, so it gets the short command timeout.
    //It reports "not pinned" rather than throwing - the caller then queues a download job
    bool pinned = true;
    long long elapsed = secondsToRun([&]() {
        pinned = ipfs.isPinned("bafkreicr2pggml4j5bjv3hhxi5i5ud4rgnnaqphrfs2mtoub77zfiwbhju");
    });

    EXPECT_FALSE(pinned);
    EXPECT_LT(elapsed, 30) << "a local lookup waited " << elapsed << "s on a silent node";

    remove(configFile.c_str());
}

TEST(IPFS, sizeLookupUsesTheDownloadTimeoutNotTheCommandOne) {
    //Working out CumulativeSize walks the whole dag and fetches any block the node is missing,
    //so a large file on a slow link legitimately takes minutes.  Cutting it off at the short
    //command timeout would fail perfectly good downloads, so it must follow
    //ipfstimeoutdownload - here 6 seconds against a 1 second command timeout.
    SilentNode node;
    const string configFile = "_ipfs_size_timeout_test.cfg";
    writeSilentNodeConfig(configFile, node.port(), 1, 6);

    IPFS ipfs(configFile, false);

    long long elapsed = secondsToRun([&]() {
        EXPECT_THROW(ipfs.getSize("bafkreicr2pggml4j5bjv3hhxi5i5ud4rgnnaqphrfs2mtoub77zfiwbhju"),
                     IPFS::exceptionTimeout);
    });

    EXPECT_GE(elapsed, 5) << "getSize() gave up after " << elapsed
                          << "s - it is following ipfstimeoutcommand, so a slow download would be cut short";
    EXPECT_LT(elapsed, 30) << "getSize() waited " << elapsed
                           << "s - before this change it never came back at all";

    remove(configFile.c_str());
}
