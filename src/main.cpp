#include "AppMain.h"
#include "ChainAnalyzer.h"
#include "Config.h"
#include "Database.h"
#include "DigiByteCore.h"
#include "EventBroadcaster.h"
#include "IPFS.h"
#include "Log.h"
#include "RPC/Server.h"
#include "Version.h"
#include "utils.h"
#include <atomic>
#include <csignal>
#include <iostream>
#include "InstanceLock.h"

namespace {
    std::atomic<bool> shutdownRequested{false};
    extern "C" void handleShutdownSignal(int) {
        shutdownRequested = true; //signal safe: everything else happens on the main thread
    }
} // namespace

int main(int argc, char* argv[]) {
    /*
     * Parse command line
     */
    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if ((arg == "--help") || (arg == "-h")) {
            cout << "DigiAsset Core " << getVersionString() << "\n"
                 << "Usage: digiasset_core [options]\n"
                 << "  --help      Show this message\n";
            return 0;
        } else {
            cerr << "Unknown option: " << arg << "\nTry --help\n";
            return 1;
        }
    }

    //make sure only one instance
    InstanceLock lock("digiasset_core");
    if (!lock.acquire()) {
        return 1;
    }
    std::cout << "Core application running. PID: " << getpid() << std::endl;

    ///Bootstrap images are gone - every node syncs from genesis now.  These are the images nodes
    ///used to pin, kept here only so a node that already has one lets go of it.  Every running node
    ///pinned the image whether or not it ever restored from one, so without this list they would
    ///each keep carrying several GB with no way to release it.  Safe to delete once enough time has
    ///passed that no node is still on a build old enough to have pinned them.
    const vector<string> oldBootstrapCIDs = {
            "QmVYaAEq5Whh1951RtRrBx1aFXiLuPoho4apRRa9tX6BDM",
            "QmaAHM9ZPGDWjW2Y5HhVzRVKAyrWofjzkN7pCW1juKgizU",
            "QmUUpXkcajwApumJ9KGz9nX7x1QmTQ4kTW4YzPc4HXqu4Z" //last official image, height 21505152
    };

    ///Files that every node keeps a copy of so they stay findable.  The storage pool only keeps asset
    ///metadata alive, so anything else on ipfs survives purely on whoever happens to still have it -
    ///and when that was one machine, the test fixtures below dropped to zero providers the moment it
    ///went offline.  Spreading them over every node costs each one a copy but means the test suite is
    ///never blocked on a single operator.
    ///Reviewers: only ever changed by a trusted party
    const vector<string> officialPinnedCIDs = {
            "QmNPyr5tkm48cUu5iMbReiM8GN8AW6PRpzUztPFadaxC8j", //tests/testFiles/assetTest.csv
            "QmUXQ2SMCvNAL4THgMm2g5vM4t6dBzj78ArnW9YBFmk81m"  //tests/testFiles/assetTest.db
    };

    ///Superseded entries from the list above, unpinned on start the same way oldBootstrapCIDs are.
    ///Without this a node that pinned one keeps carrying it forever with no way to let go
    const vector<string> retiredPinnedCIDs = {
            "QmVoawgnYej8TNwpBB7DtJ75KbrAB99k7f9VAWzqSLJBeX" //assetTest.db before it was vacuumed(243MB)
    };

    /*
     * Check if config exists and prompt user to make one if it doesn't
     */
    if (!utils::fileExists("config.cfg")) {
        Config config;
        cout << "Config file not found starting config wizard\n";

        //get DigiByte Core IP
        cout << "Is DigiByte Core running on this machine(Y/N)? ";
        bool localCore = utils::getAnswerBool();
        string rpcbind = "127.0.0.1";
        if (!localCore) {
            cout << "What is the IP address of DigiByte core? ";
            rpcbind = utils::getAnswerString(R"(^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$)");
        }
        config.setString("rpcbind", rpcbind);

        //Get DigiByte Core Port
        cout << "What port is DigiByte Core running on(Default 14022)? ";
        int rpcport = utils::getAnswerInt(0, 65535);
        config.setInteger("rpcport", rpcport);

        //Get DigiByte Core username
        cout << "What is the username for DigiByte Core? ";
        string rpcuser = utils::getAnswerString();
        config.setString("rpcuser", rpcuser);

        //Get DigiByte Core password
        cout << "What is the password for DigiByte Core? ";
        string rpcpassword = utils::getAnswerString();
        config.setString("rpcpassword", rpcpassword);

        //todo check if above is correct

        cout << "Is IPFS running on this machine(Y/N)? ";
        bool localIPFS = utils::getAnswerBool();
        string ipfsPath = "http://localhost:5001/api/v0/";
        if (!localIPFS) {
            cout << "What is the path to the IPFS node? ";
            ipfsPath = utils::getAnswerString();
        }
        config.setString("ipfspath", ipfsPath);

        //todo check if above is correct

        //Get payout address
        cout << "You will get paid for running this app.  What DigiByte address would you like to get paid to? ";
        string payout = utils::getAnswerString(R"(^(D|S)[1-9A-HJ-NP-Za-km-z]{25,34}|(dgb1)[qpzry9x8gf2tvdw0s3jn54khce6mua7l]{6,90}$)");
        config.setString("psp0payout", payout);
        config.setString("psp1payout", payout);

        //check if user wants to store minimal information or everything
        cout << "Unpruned DigiAsset Core requires 100GB of storage.  Pruned DigiAsset Core requires 2 GB of storage.  Unless running a service like an explorer or wallet back end Pruned Mode is recommended.\n";
        cout << "Would you like DigiAsset Core to run in pruning mode(Y/N)? ";
        bool pruneMode = utils::getAnswerBool();
        config.setInteger("pruneage", pruneMode ? 5760 : -1);

        //get list of allowed rpc calls
        cout << "Do you wish to allow all RPC commands(Y/N)? ";
        bool allowAllRPC = utils::getAnswerBool();
        if (allowAllRPC) {
            config.setBool("rpcallow*", true);
        } else {
            cout << "Please list all RPC commands you would like to allow.  Press Enter on blank line when done";
            while (true) {
                string command = utils::getAnswerString();
                if (command.empty()) break;
                config.setBool("rpcallow" + command, true);
            }
        }

        //save config
        config.write("config.cfg");
    }

    /*
     * Start Log
     */
    Log* log = Log::GetInstance("debug.log");
    Config config = Config("config.cfg");
    log->setMinLevelToScreen(static_cast<Log::LogLevel>(config.getInteger("logscreen", static_cast<int>(Log::INFO))));
    log->setMinLevelToFile(static_cast<Log::LogLevel>(config.getInteger("logfile", static_cast<int>(Log::WARNING))));

    /*
     * Refuse to run a config that does not mean what it says.
     * example.cfg writes the per pool options as psp#subscribe and so on, where # stands for the
     * pool number.  Copied over as is the # ends up part of the key name, so the line quietly does
     * nothing and the default applies - psp#subscribe=0 leaves the node subscribed.
     */
    vector<string> placeholderKeys = config.getPlaceholderKeys();
    for (const string& key: placeholderKeys) {
        string message = "config.cfg has \"" + key + "\" which is not a real config key.  The # in example.cfg stands for the pool number";
        if (key.substr(0, 4) == "psp#") {
            message += " - use psp0" + key.substr(4) + " or psp1" + key.substr(4);
        }
        message += ".  As written it does nothing";
        if (key == "psp#subscribe") message += ", so the pool stays subscribed";
        message += ".";
        log->addMessage(message, Log::CRITICAL);
    }
    if (!placeholderKeys.empty()) return 1;

    /*
     * Print starting message
     */
    log->addMessage("Starting DigiAsset Core " + getVersionString());

    /*
     * Get database filename from config (default "chain.db")
     */
    string dbFilename = config.getString("dbfilename", "chain.db");
    log->addMessage("Using database file: " + dbFilename);

    /*
     * Create AppMain
     */
    AppMain* main = AppMain::GetInstance();

    /*
     * Connect to core wall
     */

    DigiByteCore dgb;
    log->addMessage("Checking for DigiByte Core");
    dgb.setFileName("config.cfg");
    bool online = false;
    while (!online) {
        //connect to DigiByte Core
        try {
            dgb.makeConnection();
            log->addMessage("DigiByte Core Online");
            online = true;
        } catch (const DigiByteCore::exceptionCoreOffline& e) {
            log->addMessage("DigiByte Core Offline try again in 30 sec");
            online = false;
            this_thread::sleep_for(chrono::seconds(30)); //Don't hammer wallet
        } catch (const Config::exceptionConfigFileInvalid& e) {
            log->addMessage("DigiByte Core config values wrong in config file", Log::CRITICAL);
            return -1;
        }
    }
    main->setDigiByteCore(&dgb);

    /*
     * Get wallet version
     */
    DigiByteCore::WalletVersion walletVersion = dgb.coreVersion();
    if (walletVersion < DigiByteCore::WalletVersion::v9) {
        log->addMessage("DigiByte Core wallet " + DigiByteCore::walletVersionName(walletVersion) +
                                " is no longer supported.  DigiAsset Core requires a v9 wallet or "
                                "newer - upgrade DigiByte Core and restart.",
                        Log::CRITICAL);
        return -1;
    }

    /**
     * Connect to Database
     * Make sure it is initialized with correct database
     */
    Database* db;
    try {
        log->addMessage("Loading Database");
        db = new Database(dbFilename);
        auto compatibleWalletVersion = db->getCompatibleWalletVersion();
        if ((compatibleWalletVersion>0) && (compatibleWalletVersion != walletVersion)) {
            cout << "██ ███    ██  ██████  ██████  ███    ███ ██████   █████  ████████ ██ ██████  ██      ███████ \n"
                    "██ ████   ██ ██      ██    ██ ████  ████ ██   ██ ██   ██    ██    ██ ██   ██ ██      ██      \n"
                    "██ ██ ██  ██ ██      ██    ██ ██ ████ ██ ██████  ███████    ██    ██ ██████  ██      █████   \n"
                    "██ ██  ██ ██ ██      ██    ██ ██  ██  ██ ██      ██   ██    ██    ██ ██   ██ ██      ██      \n"
                    "██ ██   ████  ██████  ██████  ██      ██ ██      ██   ██    ██    ██ ██████  ███████ ███████ \n"
                    "                                                                                             \n"
                    " DigiByte Core Wallet " << DigiByteCore::walletVersionName(walletVersion) << " detected. \n"
                    " Database compatible with " << DigiByteCore::walletVersionName(compatibleWalletVersion) << "\n"
                    " Change core version or delete chain.db and restart\n";
            return -1;
        }
        main->setDatabase(db);
    } catch (const Database::exceptionFailedToOpen& e) {
        log->addMessage("Database could not be opened", Log::CRITICAL);
        return -1;
    }

    /**
     * Connect to IPFS
     */
    log->addMessage("Starting IPFS handler");
    IPFS ipfs("config.cfg");
    main->setIPFS(&ipfs);
    for (const auto& cid: officialPinnedCIDs) {
        ipfs.pin(cid);
    }
    for (const auto& cid: oldBootstrapCIDs) {
        ipfs.unpin(cid);
    }
    for (const auto& cid: retiredPinnedCIDs) {
        ipfs.unpin(cid);
    }

    /**
     * Connect to Permanent Storage Pools
     */
    PermanentStoragePoolList* psp;
    try {
        log->addMessage("Starting Permanent Storage Pool handler");
        psp = new PermanentStoragePoolList("config.cfg");
        main->setPermanentStoragePoolList(psp);
    } catch (const DigiByteException& e) {
        log->addMessage("Error PSP payout address not set and couldn't auto create one", Log::CRITICAL);
        return 0;
    }

    /**
     * Start RPC Cache
     */
    log->addMessage("Starting RPC Cache");
    RPC::Cache rpcCache;
    main->setRpcCache(&rpcCache);

    /**
     * Start Chain Analyzer
     */
    log->addMessage("Starting Chain Analyzer");
    ChainAnalyzer analyzer;
    analyzer.loadConfig();
    analyzer.start();
    main->setChainAnalyzer(&analyzer);

    //analyzer.stop();

    /**
     * Start event stream(TCP newline delimited JSON events.  config eventport, 0 disables)
     */
    EventBroadcaster::GetInstance()->start(config.getInteger("eventport", 14025),
                                           config.getString("eventbind", "127.0.0.1"));

    /**
     * Start RPC Server
     */
    RPC::Server* server = nullptr;
    try {
        // Create and start the Bitcoin RPC server
        log->addMessage("Starting RPC Server");
        server = new RPC::Server();
        main->setRpcServer(server);
        server->start();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    /*
     * Wait for SIGINT(ctrl-c)/SIGTERM then shut down cleanly
     */
    std::signal(SIGINT, handleShutdownSignal);
    std::signal(SIGTERM, handleShutdownSignal);
    while (!shutdownRequested) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    log->addMessage("Shutdown signal received.  Stopping");

    //order matters: stop everything that could touch the database before flushing/closing it
    if (server != nullptr) server->stop(); //no new RPC calls; joins all RPC threads
    analyzer.stop();                       //joins the chain analyzer thread
    ipfs.stop();                           //joins the IPFS job threads
    EventBroadcaster::GetInstance()->stop();
    delete server;
    delete psp; //closes the pools' own sqlite handles
    db->walCheckpoint(); //flush WAL into chain.db so the db file is complete on its own
    delete db; //closes the chain.db sqlite handles
    log->addMessage("Shutdown complete");
    return 0;
}