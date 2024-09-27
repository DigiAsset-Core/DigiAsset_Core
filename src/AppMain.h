//
// Created by mctrivia on 25/01/24.
//

#ifndef DIGIASSET_CORE_APPMAIN_H
#define DIGIASSET_CORE_APPMAIN_H



#include "ChainAnalyzer.h"
#include "Database.h"
#include "PermanentStoragePool/PermanentStoragePoolList.h"
#include "RPC/Cache.h"
#include "RPC/Server.h"
#include "SmartContract/SmartContractList.h"
#include <mutex>
class AppMain {
    /**
 * Singleton Start
 */
private:
    static AppMain* _pinstance;
    static std::mutex _mutex;

protected:
    AppMain() = default;
    ~AppMain() = default;

public:
    AppMain(AppMain& other) = delete;
    void operator=(const AppMain&) = delete;
    static AppMain* GetInstance();

    /**
 * Singleton End
 */


private:
    Database* _db = nullptr;
    IPFS* _ipfs = nullptr;
    DigiByteCore* _dgb = nullptr;
    PermanentStoragePoolList* _psp = nullptr;
    ChainAnalyzer* _analyzer = nullptr;
    RPC::Cache* _rpcCache = nullptr;
    RPC::Server* _rpcServer = nullptr;
    SmartContractList* _smartContracts = nullptr;

public:
    void setDatabase(Database* db);
    Database* getDatabase();

    void setIPFS(IPFS* ipfs);
    IPFS* getIPFS();

    void setDigiByteCore(DigiByteCore* dgb);
    DigiByteCore* getDigiByteCore();
    bool isDigiByteCoreSet();

    void setPermanentStoragePoolList(PermanentStoragePoolList* psp);
    PermanentStoragePoolList* getPermanentStoragePoolList();

    void setChainAnalyzer(ChainAnalyzer* analyzer);
    ChainAnalyzer* getChainAnalyzer();

    void setRpcCache(RPC::Cache* cache);
    RPC::Cache* getRpcCache();

    void setRpcServer(RPC::Server* server);
    RPC::Server* getRpcServer();

    void setSmartContracts(SmartContractList* smartContracts);
    SmartContractList* getSmartContracts();

    void reset();
};



#endif //DIGIASSET_CORE_APPMAIN_H
