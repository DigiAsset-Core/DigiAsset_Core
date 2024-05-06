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
    const string officialBootstrapCID = "QmVYaAEq5Whh1951RtRrBx1aFXiLuPoho4apRRa9tX6BDM";
    const unsigned int officialBootStrapHeight = 18927358;

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
        bool bootstrap = false;
        if (pruneMode) {
            cout << "Would you like to bootstrap the database from IPFS(Y) or sync from the begining(N)? ";
            bootstrap = utils::getAnswerBool();
        }
        config.setInteger("pruneage", pruneMode ? 5760 : -1);
        config.setBool("bootstrapchainstate", bootstrap);

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
     * Print starting message
     */
    log->addMessage("Starting DigiAsset Core " + getVersionString());

    /*
     * Predownload database files if config files allow and database missing
     */
    unsigned int pauseHeight = 0;
    if (                                                   //download bootstrap if all of the above are true
            config.getBool("bootstrapchainstate", true) && //if bootstrap is allowed by config(default true)
            !config.getBool("storenonassetutxo", false) && //if we are not storing the non asset utxo
            !utils::fileExists("chain.db")) {              //if the chain database does not yet exist
        log->addMessage("Bootstraping Database.  This may take a while depending on how faster your internet is.");
        IPFS ipfs("config.cfg", false);
        ipfs.downloadFile(officialBootstrapCID, "chain.db", true);
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
        } catch (const Config::exceptionConfigFileInvalid& e) {
            log->addMessage("DigiByte Core config values wrong in config file", Log::CRITICAL);
            throw;
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