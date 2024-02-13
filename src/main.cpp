#include "AppMain.h"
#include "BitcoinRpcServer.h"
#include "ChainAnalyzer.h"
#include "Config.h"
#include "Database.h"
#include "DigiByteCore.h"
#include "IPFS.h"
#include "Log.h"
#include "utils.h"
#include <iostream>


int main() {
    /*
     * Start IPFS
     */
    //IPFS* ipfs = IPFS::GetInstance();
    //ipfs->start();
    //IPFS::get("bafkreicr2pggml4j5bjv3hhxi5i5ud4rgnnaqphrfs2mtoub77zfiwbhju", "temp.json");

    /*
     * Start Log
     */
    Log* log = Log::GetInstance("debug.log");
    Config config = Config("config.cfg");
    log->setMinLevelToScreen(static_cast<Log::LogLevel>(config.getInteger("logscreen", static_cast<int>(Log::INFO))));
    log->setMinLevelToFile(static_cast<Log::LogLevel>(config.getInteger("logfile", static_cast<int>(Log::WARNING))));

    /*
     * Predownload database files if config files allow and database missing
     */
    unsigned int pauseHeight = 0;
    if (config.getBool("bootstrapchainstate", true) && !utils::fileExists("chain.db")) {
        IPFS ipfs("config.cfg", false);
        ipfs.downloadFile("Qme6x3nU9TuLxjGhhBWNoKMcKWA44w2z1v5rSHZHd4j2jF", "chain.db", true);
        pauseHeight = 18738063; ///when updating images always set this to 1 greater than largest height in blocks table
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
    Database db("chain.db");
    main->setDatabase(&db);

    /**
     * Connect to IPFS
     */
    IPFS ipfs("config.cfg");
    main->setIPFS(&ipfs);

    /**
     * Connect to Permanent Storage Pools
     */
    PermanentStoragePoolList psp("config.cfg");
    main->setPermanentStoragePoolList(&psp);

    /**
     * Start Chain Analyzer
     */
    ChainAnalyzer analyzer;
    analyzer.loadConfig();
    analyzer.start();
    //analyzer.stop();

    /**
     * Start RPC Server
     */

    try {
        // Create and start the Bitcoin RPC server
        BitcoinRpcServer server(dgb, analyzer);
        server.start();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }


    while (true) {
        std::chrono::seconds dura(100);
        std::this_thread::sleep_for(dura);
    }
}