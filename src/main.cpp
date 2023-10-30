#include "DigiByteCore.h"
#include "Database.h"
#include "ChainAnalyzer.h"
#include "IPFS.h"
#include "BitcoinRpcServer.h"
#include "Log.h"
#include "Config.h"
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
     * Connect to core wall
     */

    DigiByteCore api = DigiByteCore();
    log->addMessage("Checking for DigiByte Core");
    api.setFileName("config.cfg");
    bool online=false;
    while (!online) {
        try {
            api.makeConnection();
            log->addMessage("DigiByte Core Online");
            online=true;
        } catch (const DigiByteCore::exceptionCoreOffline& e) {
            log->addMessage("DigiByte Core Offline try again in 30 sec");
            online=false;
            this_thread::sleep_for(chrono::seconds(30));  //Don't hammer wallet
        }
    }

    /**
     * Connect to Database
     * Make sure it is initialized with correct database
     */
    Database::GetInstance("chain.db",&api);


    /**
     * Start Chain Analyzer
     */

    ChainAnalyzer analyzer(api);
    analyzer.loadConfig();
    analyzer.start();
    //analyzer.stop();

    /**
     * Start RPC Server
     */

    try {
        // Create and start the Bitcoin RPC server
        BitcoinRpcServer server(api,analyzer);
        server.start();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }


    while (true) {
        std::chrono::seconds dura(100);
        std::this_thread::sleep_for(dura);
    }
}