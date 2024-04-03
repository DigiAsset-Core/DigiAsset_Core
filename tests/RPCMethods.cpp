//
// Created by mctrivia on 31/03/24.
//
#include "RPCMethods.h"
#include "AppMain.h"
#include "utils.h"

AppMain* RPCMethodsTest::appMain = nullptr;
DigiByteCore* RPCMethodsTest::dgb = nullptr;
Database* RPCMethodsTest::db = nullptr;
IPFS* RPCMethodsTest::ipfs = nullptr;
PermanentStoragePoolList* RPCMethodsTest::psp = nullptr;
ChainAnalyzer* RPCMethodsTest::analyzer = nullptr;

void RPCMethodsTest::SetUpTestSuite() {

    if (!utils::fileExists("../tests/testFiles/rpcTest.db")) {
        std::cerr << "DigiAssetTransactionTest must finish at least once before RPC command tests can finish\n";
        exit(EXIT_FAILURE);
    }

    appMain = AppMain::GetInstance();
    dgb = new DigiByteCore();
    dgb->setFileName("config.cfg");
    dgb->makeConnection();
    appMain->setDigiByteCore(dgb);

    db = new Database("../tests/testFiles/rpcTest.db");
    appMain->setDatabase(db);

    ipfs = new IPFS("config.cfg", false);
    appMain->setIPFS(ipfs);
    ipfs->start();

    psp = new PermanentStoragePoolList("config.cfg");
    appMain->setPermanentStoragePoolList(psp);

    analyzer = new ChainAnalyzer();
    analyzer->loadFake(17579454,-1);
    ///do not start() analyzer we are using in fake mode
    appMain->setChainAnalyzer(analyzer);

}

void RPCMethodsTest::TearDownTestSuite() {
    delete dgb;
    delete ipfs;
    delete psp;
    delete db;
    delete analyzer;
}
