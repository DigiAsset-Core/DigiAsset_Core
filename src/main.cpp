#include "DigiByteCore.h"
#include "Database.h"
#include "ChainAnalyzer.h"
#include "IPFS.h"
#include "BitcoinRpcServer.h"
#include <iostream>


int main() {
    /*
     * Start IPFS
     */
    //IPFS* ipfs = IPFS::GetInstance();
    //ipfs->start();
    //IPFS::get("bafkreicr2pggml4j5bjv3hhxi5i5ud4rgnnaqphrfs2mtoub77zfiwbhju", "temp.json");

    /*
     * Connect to core wall
     */

    DigiByteCore api = DigiByteCore();
    api.setFileName("config.cfg");
    api.makeConnection();
    std::cout << "\n(temp in main)Current Block Height: " << api.getBlockCount();


    /**
     * Connect to Database
     * Make sure it is initialized with correct database
     */
    Database* db = Database::GetInstance("chain.db");


    /**
     * Start Chain Analyzer
     */

    ChainAnalyzer analyzer(api);
    analyzer.loadConfig();
    analyzer.start();
    //analyzer.stop();
    std::cout << "\nChain Analyzer Running";

    /**
     * Start RPC Server
     */

    try {
        // Create and start the Bitcoin RPC server
        BitcoinRpcServer server(api);
        server.start();

        std::cout << "Bitcoin RPC server started on port " << server.getPort() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }


    while (true) {
        std::chrono::seconds dura(100);
        std::this_thread::sleep_for(dura);
    }
}