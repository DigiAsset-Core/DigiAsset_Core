#include "AppMain.h"
#include "ChainAnalyzer.h"
#include "Config.h"
#include "Database.h"
#include "DigiByteCore.h"
#include "IPFS.h"
#include "Log.h"
#include "RPC/Server.h"
#include "Version.h"
#include "utils.h"
#include <iostream>


int main() {
    ///When updating bootstrap image change both values.   Reviewers make sure this value is only ever changed by trusted party
    const string officialBootstrapCID="QmVYaAEq5Whh1951RtRrBx1aFXiLuPoho4apRRa9tX6BDM";
    const unsigned int officialBootStrapHeight=18927358;

    /*
     * Start Log
     */
    Log* log = Log::GetInstance("debug.log");
    Config config = Config("config.cfg");
    log->setMinLevelToScreen(static_cast<Log::LogLevel>(config.getInteger("logscreen", static_cast<int>(Log::INFO))));
    log->setMinLevelToFile(static_cast<Log::LogLevel>(config.getInteger("logfile", static_cast<int>(Log::WARNING))));

    /*
     * Print starting message
     */
    log->addMessage("Starting DigiAsset Core " + getVersionString());

    /*
     * Predownload database files if config files allow and database missing
     */
    unsigned int pauseHeight = 0;
    if (                                                                        //download bootstrap if all of the above are true
            config.getBool("bootstrapchainstate", true) &&      //if bootstrap is allowed by config(default true)
            !config.getBool("storenonassetutxo", false) &&      //if we are not storing the non asset utxo
            !utils::fileExists("chain.db")) {                           //if the chain database does not yet exist
        log->addMessage("Bootstraping Database.  This may take a while depending on how faster your internet is.");
        IPFS ipfs("config.cfg", false);
        ipfs.downloadFile(officialBootstrapCID, "chain.db",true);
        pauseHeight = officialBootStrapHeight;
    }

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
        }
    }
    main->setDigiByteCore(&dgb);

    //make sure if we predownloaded data from ipfs that the wallet is synced past the point image was syned to
    if (pauseHeight > 0) {
        while (dgb.getBlockCount() < pauseHeight) {
            log->addMessage("DigiByte Core Syncing try again in 2 minutes");
            this_thread::sleep_for(chrono::minutes(2)); //Don't hammer wallet
        }
    }

    /**
     * Connect to Database
     * Make sure it is initialized with correct database
     */
    log->addMessage("Loading Database");
    Database db("chain.db");
    main->setDatabase(&db);

    /**
     * Connect to IPFS
     */
    log->addMessage("Starting IPFS handler");
    IPFS ipfs("config.cfg");
    main->setIPFS(&ipfs);
    ipfs.pin(officialBootstrapCID);

    /**
     * Connect to Permanent Storage Pools
     */
    log->addMessage("Starting Permanent Storage Pool handler");
    PermanentStoragePoolList psp("config.cfg");
    main->setPermanentStoragePoolList(&psp);

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
     * Start RPC Server
     */

    try {
        // Create and start the Bitcoin RPC server
        log->addMessage("Starting RPC Server");
        RPC::Server server;
        main->setRpcServer(&server);
        server.start();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }


    while (true) {
        std::chrono::seconds dura(100);
        std::this_thread::sleep_for(dura);
    }
}