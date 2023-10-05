//
// Created by mctrivia on 30/01/23.
//


#include <sys/stat.h>
#include "Database.h"
#include "Blob.h"
#include "DigiAsset.h"
#include "DigiByteDomain.h"
#include <stdio.h>
#include <sqlite3.h>
#include <cstring>
#include <cmath>
#include <mutex>

using namespace std;

/*
██████╗ ██╗   ██╗██╗██╗     ██████╗     ████████╗ █████╗ ██████╗ ██╗     ███████╗███████╗
██╔══██╗██║   ██║██║██║     ██╔══██╗    ╚══██╔══╝██╔══██╗██╔══██╗██║     ██╔════╝██╔════╝
██████╔╝██║   ██║██║██║     ██║  ██║       ██║   ███████║██████╔╝██║     █████╗  ███████╗
██╔══██╗██║   ██║██║██║     ██║  ██║       ██║   ██╔══██║██╔══██╗██║     ██╔══╝  ╚════██║
██████╔╝╚██████╔╝██║███████╗██████╔╝       ██║   ██║  ██║██████╔╝███████╗███████╗███████║
╚═════╝  ╚═════╝ ╚═╝╚══════╝╚═════╝        ╚═╝   ╚═╝  ╚═╝╚═════╝ ╚══════╝╚══════╝╚══════╝
 */

/**
 * Creates the tables needed by the database
 * Possible Errors:
 *  exceptionFailedInsert,
 *  exceptionFailedToCreateTable
 */
void Database::buildTables() {

    char* zErrMsg = 0;
    int rc;




    // create exchange table
    const char* sql =
            //chain data tables
            "CREATE TABLE \"assets\" (\"assetIndex\" INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, \"assetId\" TEXT NOT NULL, \"cid\" TEXT, \"issueAddress\" TEXT NOT NULL, \"rules\" BLOB, \"heightCreated\" INTEGER NOT NULL, \"heightUpdated\" INTEGER NOT NULL, \"expires\" INTEGER, \"bad\" BOOL);"
            "INSERT INTO \"assets\" VALUES (1,'DigiByte','QmfSVLAntanDUKrEHUnTXRh53GLUBHFfxk5x6LH4zz9PM4','STANDARD','{}',1,1,NULL,false);"

            "CREATE TABLE \"blocks\" (\"height\" INTEGER NOT NULL, \"hash\" BLOB NOT NULL, PRIMARY KEY(\"height\"));"
            "INSERT INTO \"blocks\" VALUES (1,X'4da631f2ac1bed857bd968c67c913978274d8aabed64ab2bcebc1665d7f4d3a0');"

            "CREATE TABLE \"exchange\" (\"address\" TEXT NOT NULL, \"index\" INTEGER NOT NULL, \"height\" INTEGER NOT NULL, \"value\" REAL NOT NULL, PRIMARY KEY(\"address\",\"index\",\"height\"));"

            "CREATE TABLE \"exchangeWatch\" (\"address\" TEXT NOT NULL, PRIMARY KEY(\"address\"));"
            "INSERT INTO \"exchangeWatch\" VALUES (\"dgb1qunxh378eltj2jrwza5sj9grvu5xud43vqvudwh\");"
            "INSERT INTO \"exchangeWatch\" VALUES (\"dgb1qlk3hldeynl3prqw259u8gv0jh7w5nwppxlvt3v\");"

            "CREATE TABLE \"flags\" (\"key\" TEXT NOT NULL, \"value\" INTEGER NOT NULL, PRIMARY KEY(\"key\"));"
            "INSERT INTO \"flags\" VALUES (\"wasPrunedExchangeHistory\",-1);"
            "INSERT INTO \"flags\" VALUES (\"wasPrunedUTXOHistory\",-1);"
            "INSERT INTO \"flags\" VALUES (\"wasPrunedVoteHistory\",-1);"
            "INSERT INTO \"flags\" VALUES (\"wasPrunedNonAssetUTXOHistory\",0);"
            "INSERT INTO \"flags\" VALUES (\"dbVersion\"," DIGIASSETCORE_DATABASE_VERSION_STR ");"

            "CREATE TABLE \"kyc\" (\"address\" TEXT NOT NULL, \"country\" TEXT NOT NULL, \"name\" TEXT NOT NULL, \"hash\" BLOB NOT NULL, \"height\" INTEGER NOT NULL, \"revoked\" INTEGER, PRIMARY KEY(\"address\"));"
            "CREATE INDEX kyc_height_index ON kyc(height);"

            "CREATE TABLE \"utxos\" (\"address\" TEXT NOT NULL, \"txid\" BLOB NOT NULL, \"vout\" INTEGER NOT NULL, \"aout\" INTEGER, \"assetIndex\" Integer NOT NULL, \"amount\" INTEGER NOT NULL, \"heightCreated\" INTEGER NOT NULL, \"heightDestroyed\" INTEGER, PRIMARY KEY(\"address\",\"txid\",\"vout\",\"aout\"));"

            "CREATE TABLE \"votes\" (\"assetIndex\" Integer NOT NULL, \"address\" TEXT NOT NULL, \"height\" INTEGER NOT NULL, \"count\" INTEGER NOT NULL, PRIMARY KEY(\"assetIndex\",\"address\",\"height\"));"

            //IPFS job tables
            "CREATE TABLE \"ipfs\" (\"jobIndex\" INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, \"sync\" TEXT NOT NULL, \"lock\" BOOL NOT NULL, \"cid\" TEXT NOT NULL, \"extra\" TEXT, \"callback\" TEXT NOT NULL, \"pause\" INTEGER, \"maxTime\" INTEGER);"
            "INSERT INTO \"ipfs\" VALUES (1,'pin',false,'QmfSVLAntanDUKrEHUnTXRh53GLUBHFfxk5x6LH4zz9PM4','','',NULL,NULL);"
            "INSERT INTO \"ipfs\" VALUES (2,'pin',false,'QmSAcz2H7veyeuuSyACLkSj9ts9EWm1c9v7uTqbHynsVbj','','',NULL,NULL);"

            //DigiByte Domain tables
            "CREATE TABLE \"domains\" (\"domain\" TEXT NOT NULL, \"assetId\" TEXT NOT NULL, \"revoked\" BOOL NOT NULL);"
            "CREATE TABLE \"domainsMasters\" (\"assetId\" TEXT NOT NULL, \"active\" BOOL NOT NULL);"
            "INSERT INTO \"domainsMasters\" VALUES (\"Ua7Bd7UVtrzavSHhpHxHZ2nzS2hGaHXRMT9sqy\",true);";
    rc = sqlite3_exec(_db, sql, Database::defaultCallback, 0, &zErrMsg);

    if (rc != SQLITE_OK) {
        sqlite3_free(zErrMsg);
        throw exceptionFailedToCreateTable();
    }

    ///IF ADDING ANY MORE TABLES MAKE SURE YOU UPDATE reset();
}



/*
██╗███╗   ██╗██╗████████╗██╗ █████╗ ██╗     ██╗███████╗███████╗
██║████╗  ██║██║╚══██╔══╝██║██╔══██╗██║     ██║╚══███╔╝██╔════╝
██║██╔██╗ ██║██║   ██║   ██║███████║██║     ██║  ███╔╝ █████╗
██║██║╚██╗██║██║   ██║   ██║██╔══██║██║     ██║ ███╔╝  ██╔══╝
██║██║ ╚████║██║   ██║   ██║██║  ██║███████╗██║███████╗███████╗
╚═╝╚═╝  ╚═══╝╚═╝   ╚═╝   ╚═╝╚═╝  ╚═╝╚══════╝╚═╝╚══════╝╚══════╝
 */

/**
 * Pre creates the sql statements so they can be executed faster
 */
void Database::initializeClassValues() {
    //statement to find a flag value
    const char* sql10 = "SELECT value FROM flags WHERE key LIKE ?;";
    int rc = sqlite3_prepare_v2(_db, sql10, strlen(sql10), &_stmtCheckFlag, nullptr);
    if (rc != SQLITE_OK) throw exceptionCreatingStatement();

    //statement to set a flag value
    const char* sql11 = "UPDATE flags SET value=? WHERE key LIKE ?;";
    rc = sqlite3_prepare_v2(_db, sql11, strlen(sql11), &_stmtSetFlag, nullptr);
    if (rc != SQLITE_OK) throw exceptionCreatingStatement();


    //statement to get block height
    const char* sql20 = "SELECT height FROM blocks ORDER BY height DESC LIMIT 1;";
    rc = sqlite3_prepare_v2(_db, sql20, strlen(sql20), &_stmtGetBlockHeight, nullptr);
    if (rc != SQLITE_OK) throw exceptionCreatingStatement();

    //statement to set block height
    const char* sql21 = "INSERT INTO blocks VALUES (?,?);";
    rc = sqlite3_prepare_v2(_db, sql21, strlen(sql21), &_stmtSetBlockHash, nullptr);
    if (rc != SQLITE_OK) throw exceptionCreatingStatement();

    //statement to get block hash
    const char* sql22 = "SELECT hash FROM blocks WHERE height=? LIMIT 1;";
    rc = sqlite3_prepare_v2(_db, sql22, strlen(sql22), &_stmtGetBlockHash, nullptr);
    if (rc != SQLITE_OK) throw exceptionCreatingStatement();

    //second half of prune statement.  removes blocks from list that it is no longer possible to roll back to
    const char* sql23 = "DELETE FROM blocks where height<?;";
    rc = sqlite3_prepare_v2(_db, sql23, strlen(sql23), &_stmtRemoveNonReachable, nullptr);
    if (rc != SQLITE_OK) throw exceptionCreatingStatement();




    //statement to create UTXO
    const char* sql40 = "INSERT INTO utxos VALUES (?,?,?,?,?,?,?,NULL);";
    rc = sqlite3_prepare_v2(_db, sql40, strlen(sql40), &_stmtCreateUTXO, nullptr);
    if (rc != SQLITE_OK) throw exceptionCreatingStatement();

    //statement to spend UTXO
    const char* sql42 = "UPDATE utxos SET heightDestroyed=? WHERE txid=? AND vout=?;";
    rc = sqlite3_prepare_v2(_db, sql42, strlen(sql42), &_stmtSpendUTXO, nullptr);
    if (rc != SQLITE_OK) throw exceptionCreatingStatement();

    //statement to get spending address from UTXO
    const char* sql43 = "SELECT address FROM utxos WHERE txid=? AND vout=?";
    rc = sqlite3_prepare_v2(_db, sql43, strlen(sql43), &_stmtGetSpendingAddress, nullptr);
    if (rc != SQLITE_OK) throw exceptionCreatingStatement();

    //statement to prune utxos
    const char* sql44 = "DELETE FROM utxos WHERE heightDestroyed<?;";
    rc = sqlite3_prepare_v2(_db, sql44, strlen(sql44), &_stmtPruneUTXOs, nullptr);
    if (rc != SQLITE_OK) throw exceptionCreatingStatement();

    //statement to get funds on a UTXO
    const char* sql45 = "SELECT address,aout,assetIndex,amount FROM utxos WHERE txid=? AND vout=? ORDER BY aout ASC;";
    rc = sqlite3_prepare_v2(_db, sql45, strlen(sql45), &_stmtGetAssetUTXO, nullptr);
    if (rc != SQLITE_OK) throw exceptionCreatingStatement();

    //statement to get asset holders
    const char* sql46 = "SELECT address,SUM(amount) FROM utxos WHERE assetIndex=5 GROUP BY address;";
    rc = sqlite3_prepare_v2(_db, sql46, strlen(sql46), &_stmtGetAssetHolders, nullptr);
    if (rc != SQLITE_OK) throw exceptionCreatingStatement();

    //statement to get all assetIndexs on a specific utxo
    const char* sql47 = "SELECT assetIndex FROM utxos WHERE txid=? AND vout=? AND aout IS NOT NULL;";
    rc = sqlite3_prepare_v2(_db, sql47, strlen(sql47), &_stmtGetAssetIndexOnUTXO, nullptr);
    if (rc != SQLITE_OK) throw exceptionCreatingStatement();

    //statement to get permanent storage paid on a specific transaction
    const char* sql48 = "SELECT amount,heightCreated FROM utxos where txid=? and address=? and aout is null;";
    rc = sqlite3_prepare_v2(_db, sql48, strlen(sql48), &_stmtGetPermanentPaid, nullptr);
    if (rc != SQLITE_OK) throw exceptionCreatingStatement();






    //statement to check if exchange watch address
    const char* sql50 = "SELECT address FROM exchangeWatch WHERE address=?;";
    rc = sqlite3_prepare_v2(_db, sql50, strlen(sql50), &_stmtIsWatchAddress, nullptr);
    if (rc != SQLITE_OK) throw exceptionCreatingStatement();

    //statement to add exchange watch address
    const char* sql51 = "INSERT INTO exchangeWatch VALUES (?);";
    rc = sqlite3_prepare_v2(_db, sql51, strlen(sql51), &_stmtAddWatchAddress, nullptr);
    if (rc != SQLITE_OK) throw exceptionCreatingStatement();


    //statement to insert new exchange rate
    const char* sql60 = "INSERT INTO exchange VALUES (?,?,?,?);";
    rc = sqlite3_prepare_v2(_db, sql60, strlen(sql60), &_stmtAddExchangeRate, nullptr);
    if (rc != SQLITE_OK) throw exceptionCreatingStatement();

    //statement to get current exchange rates(all rates)
    const char* sql61 = "WITH cte AS (\n"
                        "  SELECT *, ROW_NUMBER() OVER (PARTITION BY [address], [index] ORDER BY height DESC) AS row_number\n"
                        "  FROM exchange\n"
                        "  WHERE height < ?\n"
                        ")\n"
                        "SELECT [height], [address], [index], [value]\n"
                        "FROM cte\n"
                        "WHERE row_number = 1;";
    rc = sqlite3_prepare_v2(_db, sql61, strlen(sql61), &_stmtExchangeRatesAtHeight, nullptr);
    if (rc != SQLITE_OK) throw exceptionCreatingStatement();

    //statement to delete exchange rates bellow a specific height
    const char* sql62 = "DELETE FROM exchange WHERE height<?;";
    rc = sqlite3_prepare_v2(_db, sql62, strlen(sql62), &_stmtPruneExchangeRate, nullptr);
    if (rc != SQLITE_OK) throw exceptionCreatingStatement();

    //statement to get accepted exchange rate at a specific height
    const char* sql63 = "SELECT value, height FROM exchange WHERE height<? AND address=? AND [index]=? ORDER BY height DESC;";
    rc = sqlite3_prepare_v2(_db, sql63, strlen(sql63), &_stmtGetValidExchangeRate, nullptr);
    if (rc != SQLITE_OK) throw exceptionCreatingStatement();

    //statement to get current exchange rate(1 rate)
    const char* sql64 = "SELECT value FROM exchange WHERE address=? AND [index]=? ORDER BY height DESC LIMIT 1;";
    rc = sqlite3_prepare_v2(_db, sql64, strlen(sql64), &_stmtGetCurrentExchangeRate, nullptr);
    if (rc != SQLITE_OK) throw exceptionCreatingStatement();


    //statement to insert new kyc record
    const char* sql70 = "INSERT OR IGNORE INTO kyc VALUES (?,?,?,?,?,NULL);";
    rc = sqlite3_prepare_v2(_db, sql70, strlen(sql70), &_stmtAddKYC, nullptr);
    if (rc != SQLITE_OK) throw exceptionCreatingStatement();

    //statement to revoke kyc record
    const char* sql71 = "UPDATE kyc SET revoked=? WHERE address=?;";
    rc = sqlite3_prepare_v2(_db, sql71, strlen(sql71), &_stmtRevokeKYC, nullptr);
    if (rc != SQLITE_OK) throw exceptionCreatingStatement();

    //statement to revoke kyc record
    const char* sql72 = "SELECT country,name,hash,height,revoked FROM kyc WHERE address=?;";
    rc = sqlite3_prepare_v2(_db, sql72, strlen(sql72), &_stmtGetKYC, nullptr);
    if (rc != SQLITE_OK) throw exceptionCreatingStatement();


    //statement to get total votes at specific height
    const char* sql80 = "SELECT assetIndex,address,SUM([count]),MAX([height]) FROM votes WHERE height<10 GROUP BY assetIndex,address;";
    rc = sqlite3_prepare_v2(_db, sql80, strlen(sql80), &_stmtGetVoteCount, nullptr);
    if (rc != SQLITE_OK) throw exceptionCreatingStatement();

    //statement to prune votes
    const char* sql81 = "DELETE FROM votes WHERE height<?;";
    rc = sqlite3_prepare_v2(_db, sql81, strlen(sql81), &_stmtPruneVote, nullptr);
    if (rc != SQLITE_OK) throw exceptionCreatingStatement();

    //statement to record votes
    const char* sql82 = "INSERT INTO votes (assetIndex, address, height, count) VALUES (?, ?, ?, ?) ON CONFLICT (assetIndex, address, height) DO UPDATE SET count=count+?;";
    rc = sqlite3_prepare_v2(_db, sql82, strlen(sql82), &_stmtAddVote, nullptr);
    if (rc != SQLITE_OK) throw exceptionCreatingStatement();

    //statement to add asset
    const char* sql90 = "INSERT INTO assets (assetId, issueAddress, cid, rules, heightCreated, heightUpdated, expires, bad) VALUES (?, ?, ?, ?, ?, ?, ?, false);";
    rc = sqlite3_prepare_v2(_db, sql90, strlen(sql90), &_stmtAddAsset, nullptr);
    if (rc != SQLITE_OK) {

        string tempErrorMessage = sqlite3_errmsg(_db);
        throw exceptionCreatingStatement();
    }

    //statement to update asset(aggregable only)
    const char* sql91 = "UPDATE assets SET heightUpdated=?, cid=?, rules=?, expires=?, bad=? WHERE assetIndex=?";
    rc = sqlite3_prepare_v2(_db, sql91, strlen(sql91), &_stmtUpdateAsset, nullptr);
    if (rc != SQLITE_OK) throw exceptionCreatingStatement();

    //statement to get assetIndex(don't use for non aggregable)
    const char* sql92 = "SELECT assetIndex FROM assets WHERE assetId=?";
    rc = sqlite3_prepare_v2(_db, sql92, strlen(sql92), &_stmtGetAssetIndex, nullptr);
    if (rc != SQLITE_OK) throw exceptionCreatingStatement();

    //statement to get height created
    const char* sql93 = "SELECT assetIndex,heightCreated FROM assets WHERE assetId=?";
    rc = sqlite3_prepare_v2(_db, sql93, strlen(sql93), &_stmtGetHeightAssetCreated, nullptr);
    if (rc != SQLITE_OK) throw exceptionCreatingStatement();

    //statement to get asset rules
    const char* sql94 = "SELECT rules FROM assets WHERE assetId=?";
    rc = sqlite3_prepare_v2(_db, sql94, strlen(sql94), &_stmtGetAssetRules, nullptr);
    if (rc != SQLITE_OK) throw exceptionCreatingStatement();

    //statement to get asset
    const char* sql95 = "SELECT assetId,cid,issueAddress,rules,heightCreated,heightUpdated,bad FROM assets WHERE assetIndex=?";
    rc = sqlite3_prepare_v2(_db, sql95, strlen(sql95), &_stmtGetAsset, nullptr);
    if (rc != SQLITE_OK) throw exceptionCreatingStatement();


    //statement for IPFS
    const char* sql100 = "SELECT jobIndex,sync,cid,extra,callback,maxTime FROM ipfs WHERE pause is NULL AND lock is false ORDER BY jobIndex ASC LIMIT 1;";
    rc = sqlite3_prepare_v2(_db, sql100, strlen(sql100), &_stmtGetNextIPFSJob, nullptr);
    if (rc != SQLITE_OK) throw exceptionCreatingStatement();

    const char* sql101a = "DELETE FROM ipfs WHERE jobIndex=?;";
    rc = sqlite3_prepare_v2(_db, sql101a, strlen(sql101a), &_stmtClearNextIPFSJob_a, nullptr);
    if (rc != SQLITE_OK) throw exceptionCreatingStatement();

    const char* sql101b = "UPDATE ipfs set lock=false WHERE sync=?;";
    rc = sqlite3_prepare_v2(_db, sql101b, strlen(sql101b), &_stmtClearNextIPFSJob_b, nullptr);
    if (rc != SQLITE_OK) throw exceptionCreatingStatement();

    const char* sql102 = "INSERT INTO ipfs VALUES (NULL,?,false,?,?,?,?,?);";
    rc = sqlite3_prepare_v2(_db, sql102, strlen(sql102), &_stmtInsertIPFSJob, nullptr);
    if (rc != SQLITE_OK) throw exceptionCreatingStatement();

    const char* sql103 = "UPDATE ipfs set pause=?, lock=false WHERE sync=?;";
    rc = sqlite3_prepare_v2(_db, sql103, strlen(sql103), &_stmtSetIPFSPauseSync, nullptr);
    if (rc != SQLITE_OK) throw exceptionCreatingStatement();

    const char* sql104 = "UPDATE ipfs set pause=NULL WHERE pause<?;";
    rc = sqlite3_prepare_v2(_db, sql104, strlen(sql104), &_stmtClearIPFSPause, nullptr);
    if (rc != SQLITE_OK) throw exceptionCreatingStatement();

    const char* sql105 = "UPDATE ipfs set lock=true WHERE sync=?;";
    rc = sqlite3_prepare_v2(_db, sql105, strlen(sql105), &_stmtSetIPFSLockSync, nullptr);
    if (rc != SQLITE_OK) throw exceptionCreatingStatement();

    const char* sql106 = "UPDATE ipfs set pause=?, lock=false WHERE jobIndex=?;";
    rc = sqlite3_prepare_v2(_db, sql106, strlen(sql106), &_stmtSetIPFSPauseJob, nullptr);
    if (rc != SQLITE_OK) throw exceptionCreatingStatement();

    const char* sql107 = "UPDATE ipfs set lock=true WHERE jobIndex=?;";
    rc = sqlite3_prepare_v2(_db, sql107, strlen(sql107), &_stmtSetIPFSLockJob, nullptr);
    if (rc != SQLITE_OK) throw exceptionCreatingStatement();


    //DigiByte Domain statements
    const char* sql110 = "SELECT assetId,revoked FROM domains WHERE domain=?";
    rc = sqlite3_prepare_v2(_db, sql110, strlen(sql110), &_stmtGetDomainAssetId, nullptr);
    if (rc != SQLITE_OK) throw exceptionCreatingStatement();

    const char* sql112 = "INSERT INTO domains VALUES (?,?,false);";
    rc = sqlite3_prepare_v2(_db, sql112, strlen(sql112), &_stmtAddDomain, nullptr);
    if (rc != SQLITE_OK) throw exceptionCreatingStatement();

    const char* sql113 = "UPDATE domains SET revoked=true WHERE domain=?;";
    rc = sqlite3_prepare_v2(_db, sql113, strlen(sql113), &_stmtRevokeDomain, nullptr);
    if (rc != SQLITE_OK) throw exceptionCreatingStatement();

    const char* sql114a = "UPDATE domainsMasters SET active=false WHERE assetId=?;";
    rc = sqlite3_prepare_v2(_db, sql114a, strlen(sql114a), &_stmtSetDomainMasterAssetId_a, nullptr);
    if (rc != SQLITE_OK) throw exceptionCreatingStatement();

    const char* sql114b = "INSERT INTO domainsMasters VALUES (?,true);";
    rc = sqlite3_prepare_v2(_db, sql114b, strlen(sql114b), &_stmtSetDomainMasterAssetId_b, nullptr);
    if (rc != SQLITE_OK) throw exceptionCreatingStatement();



    //initialize exchange address watch list
    sqlite3_stmt* stmt1;
    const char* sqlInt10 = "SELECT address FROM exchangeWatch WHERE 1;";
    rc = sqlite3_prepare_v2(_db, sqlInt10, -1, &stmt1, NULL);
    if (rc != SQLITE_OK) throw exceptionCreatingStatement();
    for (;;) {
        rc = sqlite3_step(stmt1);
        if (rc == SQLITE_DONE) {
            break;
        }
        if (rc != SQLITE_ROW) {
            throw exceptionFailedSelect();
        }
        _exchangeWatchAddresses.emplace_back(reinterpret_cast<const char*>(sqlite3_column_text(stmt1, 0)));
        if (_exchangeWatchAddresses.size() ==
            DIGIBYTECORE_DATABASE_CHAIN_WATCH_MAX) {    //check if watch has grown too big to buffer
            _exchangeWatchAddresses.clear();
            break;
        }
    }
    sqlite3_finalize(stmt1);

    //initialize ipfs(remove all non-permanent jobs and any pauses)
    char* zErrMsg = 0;
    const char* sqlInt11 = "DELETE FROM ipfs WHERE callback LIKE \"_\";UPDATE ipfs SET pause=NULL,lock=0;";
    rc = sqlite3_exec(_db, sqlInt11, Database::defaultCallback, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        sqlite3_free(zErrMsg);
        throw exceptionFailedUpdate();
    }

    //remove non-permanent IPFS callbacks and pauses from ram
    _ipfsCurrentlyPaused.clear();
    auto it = _ipfsCallbacks.begin();
    while (it != _ipfsCallbacks.end()) {
        if (it->first[0] == '_') {
            it = _ipfsCallbacks.erase(it);
        } else {
            ++it;
        }
    }

    //get master domain asset
    sqlite3_stmt* stmt3;
    const char* sqlInt12 = "SELECT assetId,active FROM domainsMasters";
    rc = sqlite3_prepare_v2(_db, sqlInt12, -1, &stmt3, NULL);
    if (rc != SQLITE_OK) throw exceptionCreatingStatement();
    string last;
    while (sqlite3_step(stmt3) == SQLITE_ROW) {
        string assetId = reinterpret_cast<const char*>(sqlite3_column_text(stmt3, 0));
        bool active = sqlite3_column_int(stmt3, 1);
        if (active) {
            last = assetId;
        } else {
            _masterDomainAssetId.push_back(assetId);
        }
    };
    _masterDomainAssetId.push_back(last);
    sqlite3_finalize(stmt3);
}


/*
███████╗██╗███╗   ██╗ ██████╗ ██╗     ███████╗████████╗ ██████╗ ███╗   ██╗
██╔════╝██║████╗  ██║██╔════╝ ██║     ██╔════╝╚══██╔══╝██╔═══██╗████╗  ██║
███████╗██║██╔██╗ ██║██║  ███╗██║     █████╗     ██║   ██║   ██║██╔██╗ ██║
╚════██║██║██║╚██╗██║██║   ██║██║     ██╔══╝     ██║   ██║   ██║██║╚██╗██║
███████║██║██║ ╚████║╚██████╔╝███████╗███████╗   ██║   ╚██████╔╝██║ ╚████║
╚══════╝╚═╝╚═╝  ╚═══╝ ╚═════╝ ╚══════╝╚══════╝   ╚═╝    ╚═════╝ ╚═╝  ╚═══╝
*/
Database* Database::_pinstance = nullptr;
mutex Database::_mutex;

Database* Database::GetInstance() {
    std::lock_guard<std::mutex> lock(_mutex);
    if (_pinstance == nullptr) {
        _pinstance = new Database();
    }
    return _pinstance;
}

Database* Database::GetInstance(const string& fileName) {
    std::lock_guard<std::mutex> lock(_mutex);
    if (_pinstance == nullptr) {
        _pinstance = new Database();
    }
    _pinstance->load(fileName);
    return _pinstance;
}

/*
 ██████╗ ██████╗ ███╗   ██╗███████╗████████╗██████╗ ██╗   ██╗ ██████╗████████╗ ██████╗ ██████╗
██╔════╝██╔═══██╗████╗  ██║██╔════╝╚══██╔══╝██╔══██╗██║   ██║██╔════╝╚══██╔══╝██╔═══██╗██╔══██╗
██║     ██║   ██║██╔██╗ ██║███████╗   ██║   ██████╔╝██║   ██║██║        ██║   ██║   ██║██████╔╝
██║     ██║   ██║██║╚██╗██║╚════██║   ██║   ██╔══██╗██║   ██║██║        ██║   ██║   ██║██╔══██╗
╚██████╗╚██████╔╝██║ ╚████║███████║   ██║   ██║  ██║╚██████╔╝╚██████╗   ██║   ╚██████╔╝██║  ██║
 ╚═════╝ ╚═════╝ ╚═╝  ╚═══╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝  ╚═════╝   ╚═╝    ╚═════╝ ╚═╝  ╚═╝
 */





/**
 * Opens the database
 * if file does not exist will create it
 * Possible Errors:
 *  exceptionFailedToOpen,
 *  exceptionFailedInsert,
 *  exceptionFailedToCreateTable
 * @param fileName
 */
void Database::load(const string& newFileName) {
    //make sure database is closed
    if (_db != nullptr) sqlite3_close(_db);
    _db = nullptr;

    //see if this is first run
    struct stat buffer{};
    bool firstRun = (stat(newFileName.c_str(), &buffer) != 0);

    //open database
    int rc;
    rc = sqlite3_open(newFileName.c_str(), &_db);
    if (rc) throw exceptionFailedToOpen();

    //create needed tables
    if (firstRun) buildTables();

    //upgrade table if needed
    //todo

    //create needed statements
    initializeClassValues();
}


Database::~Database() {
    sqlite3_close(_db);
}

/*
██████╗ ███████╗██████╗ ███████╗ ██████╗ ██████╗ ███╗   ███╗ █████╗ ███╗   ██╗ ██████╗███████╗
██╔══██╗██╔════╝██╔══██╗██╔════╝██╔═══██╗██╔══██╗████╗ ████║██╔══██╗████╗  ██║██╔════╝██╔════╝
██████╔╝█████╗  ██████╔╝█████╗  ██║   ██║██████╔╝██╔████╔██║███████║██╔██╗ ██║██║     █████╗
██╔═══╝ ██╔══╝  ██╔══██╗██╔══╝  ██║   ██║██╔══██╗██║╚██╔╝██║██╔══██║██║╚██╗██║██║     ██╔══╝
██║     ███████╗██║  ██║██║     ╚██████╔╝██║  ██║██║ ╚═╝ ██║██║  ██║██║ ╚████║╚██████╗███████╗
╚═╝     ╚══════╝╚═╝  ╚═╝╚═╝      ╚═════╝ ╚═╝  ╚═╝╚═╝     ╚═╝╚═╝  ╚═╝╚═╝  ╚═══╝ ╚═════╝╚══════╝
 */


/**
 * If you have multiple write transactions going to database before any reads running this
 * first, do your transactions than run endTransaction() will significantly speed up the
 * time it takes to make all the transactions
 */
void Database::startTransaction() {
    char* zErrMsg = 0;
    sqlite3_exec(_db, "BEGIN TRANSACTION", NULL, NULL, &zErrMsg);
}

/**
 * Finishes the batch transaction
 */
void Database::endTransaction() {
    char* zErrMsg = 0;
    sqlite3_exec(_db, "END TRANSACTION", NULL, NULL, &zErrMsg);
}

/**
 * disables write verification.  gives significant speed increase but on power failure data
 * may not all be written to the drive so must recheck at startup
 */
void Database::disableWriteVerification() {
    char* zErrMsg = 0;
    sqlite3_exec(_db, "PRAGMA synchronous = OFF", NULL, NULL, &zErrMsg);
    sqlite3_exec(_db, "PRAGMA journal_mode = MEMORY", NULL, NULL, &zErrMsg);
}

/*
██████╗ ███████╗███████╗███████╗████████╗
██╔══██╗██╔════╝██╔════╝██╔════╝╚══██╔══╝
██████╔╝█████╗  ███████╗█████╗     ██║
██╔══██╗██╔══╝  ╚════██║██╔══╝     ██║
██║  ██║███████╗███████║███████╗   ██║
╚═╝  ╚═╝╚══════╝╚══════╝╚══════╝   ╚═╝
 */
/**
 * if there is a roll back that exceeds the pruned history we will need to start all over
 * this function resets all history related tables
 * Possible Errors:
 *  exceptionFailedReset
 */
void Database::reset() {
    char* zErrMsg = 0;
    int rc;

    const char* sql = "DELETE FROM exchange;"
                      "DELETE FROM kyc;"
                      "DELETE FROM utxos;"
                      "DELETE FROM votes;"
                      "DELETE FROM assets;"
                      "DELETE FROM blocks WHERE height>1;"
                      "DELETE FROM assetMetaHistory WHERE issuanceId>1;";
    rc = sqlite3_exec(_db, sql, Database::defaultCallback, 0, &zErrMsg);

    if (rc != SQLITE_OK) {
        sqlite3_free(zErrMsg);
        throw exceptionFailedReset();
    }
}


/*
 █████╗ ███████╗███████╗███████╗████████╗    ████████╗ █████╗ ██████╗ ██╗     ███████╗
██╔══██╗██╔════╝██╔════╝██╔════╝╚══██╔══╝    ╚══██╔══╝██╔══██╗██╔══██╗██║     ██╔════╝
███████║███████╗███████╗█████╗     ██║          ██║   ███████║██████╔╝██║     █████╗
██╔══██║╚════██║╚════██║██╔══╝     ██║          ██║   ██╔══██║██╔══██╗██║     ██╔══╝
██║  ██║███████║███████║███████╗   ██║          ██║   ██║  ██║██████╔╝███████╗███████╗
╚═╝  ╚═╝╚══════╝╚══════╝╚══════╝   ╚═╝          ╚═╝   ╚═╝  ╚═╝╚═════╝ ╚══════╝╚══════╝
 */

/**
 * Adds or updates an asset
 * returns assetIndex
 */
uint64_t Database::addAsset(const DigiAsset& asset) {
    int rc;
    uint64_t assetIndex = 0;
    string assetId = asset.getAssetId();
    if (asset.isAggregable()) { //hybrid and distinct never update so skip this leg

        //only 1 asset can exist with the same assetId so if not locked check if already exists
        if (!asset.isLocked()) {

            //get the assetIndex if the asset already exists
            sqlite3_reset(_stmtGetAssetIndex);
            sqlite3_bind_text(_stmtGetAssetIndex, 1, assetId.c_str(), assetId.length(), SQLITE_STATIC);
            rc = sqlite3_step(_stmtGetAssetIndex);
            if (rc == SQLITE_ROW) {    //if not found its new so leave assetIndex 0
                assetIndex = sqlite3_column_int(_stmtGetAssetIndex, 0);
            }

        }

        //update existing asset
        if (assetIndex != 0) {
            sqlite3_reset(_stmtUpdateAsset);
            sqlite3_bind_int(_stmtUpdateAsset, 1, asset.getHeightUpdated());
            string cid = asset.getCID();
            sqlite3_bind_text(_stmtUpdateAsset, 2, cid.c_str(), cid.length(), SQLITE_STATIC);
            vector<uint8_t> serializedRules;
            serialize(serializedRules, asset.getRules());
            Blob rules{serializedRules};
            sqlite3_bind_blob(_stmtUpdateAsset, 3, rules.data(), rules.length(), SQLITE_STATIC);
            sqlite3_bind_int64(_stmtUpdateAsset, 4, asset.getExpiry());
            sqlite3_bind_int(_stmtUpdateAsset, 5, asset.isBad());
            sqlite3_bind_int64(_stmtUpdateAsset, 6, assetIndex);
            rc = sqlite3_step(_stmtUpdateAsset);
            if (rc != SQLITE_DONE) {
                string tempErrorMessage = sqlite3_errmsg(_db);
                throw exceptionFailedUpdate();
            }
            return assetIndex;
        }
    }

    //insert new asset
    sqlite3_reset(_stmtAddAsset);
    sqlite3_bind_text(_stmtAddAsset, 1, assetId.c_str(), assetId.length(), SQLITE_STATIC);
    string issuingAddress = asset.getIssuer().getAddress();
    sqlite3_bind_text(_stmtAddAsset, 2, issuingAddress.c_str(), issuingAddress.length(), SQLITE_STATIC);
    string cid = asset.getCID();
    sqlite3_bind_text(_stmtAddAsset, 3, cid.c_str(), cid.length(), SQLITE_STATIC);
    vector<uint8_t> serializedRules;
    serialize(serializedRules, asset.getRules());
    Blob rules{serializedRules};
    sqlite3_bind_blob(_stmtAddAsset, 4, rules.data(), rules.length(), SQLITE_STATIC);
    sqlite3_bind_int(_stmtAddAsset, 5, asset.getHeightCreated());
    sqlite3_bind_int(_stmtAddAsset, 6, asset.getHeightUpdated());   //will be same as created
    sqlite3_bind_int64(_stmtAddAsset, 7, asset.getExpiry());
    sqlite3_bind_int(_stmtAddAsset, 8, asset.isBad());
    rc = sqlite3_step(_stmtAddAsset);
    if (rc != SQLITE_DONE) {
        string tempErrorMessage = sqlite3_errmsg(_db);
        throw exceptionFailedInsert();
    }
    return sqlite3_last_insert_rowid(_db);
}

/**
 * gets rules for a specific assetId
 * This function does not work on assets that may have more than 1 assetIndex but there is no reason to call it for those assets
 * @param assetId
 * @return
 */
DigiAssetRules Database::getRules(const string& assetId) const {
    int rc;
    sqlite3_reset(_stmtGetAssetRules);
    sqlite3_bind_text(_stmtGetAssetRules, 1, assetId.c_str(), assetId.length(), SQLITE_STATIC);
    rc = sqlite3_step(_stmtGetAssetRules);
    if (rc != SQLITE_ROW) return DigiAssetRules();

    vector<uint8_t> serializedRules = Blob(sqlite3_column_blob(_stmtGetAssetRules, 0),
                                           sqlite3_column_bytes(_stmtGetAssetRules, 0)).vector();
    DigiAssetRules rules;
    size_t i = 0;
    deserialize(serializedRules, i, rules);
    return rules;
}

/**
 * Checks if an asset already exists and returns the height it was created if it does.  If it does not exist already returns backupHeight
 * This function does not work on assets that may have more than 1 assetIndex but there is no reason to call it for those assets
 */
unsigned int
Database::getAssetHeightCreated(const string& assetId, unsigned int backupHeight, uint64_t& assetIndex) {
    int rc;
    sqlite3_reset(_stmtGetHeightAssetCreated);
    sqlite3_bind_text(_stmtGetHeightAssetCreated, 1, assetId.c_str(), assetId.length(), SQLITE_STATIC);
    rc = sqlite3_step(_stmtGetHeightAssetCreated);
    if (rc == SQLITE_ROW) {    //if not found its new so leave assetIndex 0
        assetIndex = sqlite3_column_int(_stmtGetHeightAssetCreated, 0);
        return sqlite3_column_int(_stmtGetHeightAssetCreated, 1);
    }
    return backupHeight;
}

/**
 * Gets an asset by its assetIndex
 * @param assetIndex
 * @param amount - optional number of assets to include in object
 * @return
 */
DigiAsset Database::getAsset(uint64_t assetIndex, uint64_t amount) const {
    //get data in assets table
    int rc;
    sqlite3_reset(_stmtGetAsset);
    sqlite3_bind_int64(_stmtGetAsset, 1, assetIndex);
    rc = sqlite3_step(_stmtGetAsset);
    if (rc != SQLITE_ROW) {
        string tempErrorMessage = sqlite3_errmsg(_db);
        throw exceptionFailedSelect();
    }
    string assetId = reinterpret_cast<const char*>(sqlite3_column_text(_stmtGetAsset, 0));
    string cid = reinterpret_cast<const char*>(sqlite3_column_text(_stmtGetAsset, 1));
    string issuerAddress = reinterpret_cast<const char*>(sqlite3_column_text(_stmtGetAsset, 2));
    vector<uint8_t> serializedRules = Blob(sqlite3_column_blob(_stmtGetAsset, 3),
                                           sqlite3_column_bytes(_stmtGetAsset, 3)).vector();
    unsigned int heightCreated = sqlite3_column_int(_stmtGetAsset, 4);
    unsigned int heightUpdated = sqlite3_column_int(_stmtGetAsset, 5);
    bool bad = sqlite3_column_int(_stmtGetAsset, 6);

    //lookup kyc and rules
    KYC issuer = getAddressKYC(issuerAddress);
    DigiAssetRules rules;
    size_t i = 0;
    deserialize(serializedRules, i, rules);

    //create and return object
    return {assetIndex, assetId, cid, issuer, rules, heightCreated, heightUpdated, bad, amount};
}

uint64_t Database::getAssetIndex(const string& assetId, const string& txid, unsigned int vout) const {
    //see if the asset exists and if only 1 index
    sqlite3_reset(_stmtGetAssetIndex);
    sqlite3_bind_text(_stmtGetAssetIndex, 1, assetId.c_str(), assetId.length(), SQLITE_STATIC);
    int rc = sqlite3_step(_stmtGetAssetIndex);
    if (rc != SQLITE_ROW) {
        throw out_of_range("assetIndex does not exist");
    }
    uint64_t assetIndex = sqlite3_column_int(_stmtGetAssetIndex, 0);

    //check if more than 1
    rc = sqlite3_step(_stmtGetAssetIndex);
    if (rc != SQLITE_ROW) return assetIndex;    //there was only 1

    //more than 1 so see if the txid and vout provided match a utxo
    if (txid.empty()) throw out_of_range("specific utxo needed");
    vector<uint64_t> assetIndexPossibilities;
    sqlite3_reset(_stmtGetAssetIndexOnUTXO);
    sqlite3_bind_text(_stmtGetAssetIndexOnUTXO, 1, assetId.c_str(), assetId.length(), SQLITE_STATIC);
    sqlite3_bind_text(_stmtGetAssetIndexOnUTXO, 2, txid.c_str(), txid.length(), SQLITE_STATIC);
    sqlite3_bind_int(_stmtGetAssetIndexOnUTXO, 3, vout);
    while (sqlite3_step(_stmtGetAssetIndexOnUTXO) == SQLITE_ROW) {
        assetIndexPossibilities.push_back(sqlite3_column_int(_stmtGetAssetIndexOnUTXO, 0));
    }

    //filter possibilities for those that are the same assetId
    for (uint64_t assetIndexTest: assetIndexPossibilities) {
        DigiAsset asset = getAsset(assetIndexTest);
        if (asset.getAssetId() == assetId) return assetIndexTest;
    }
    throw out_of_range("assetId not found in utxo");
}

/*
███████╗██╗      █████╗  ██████╗     ████████╗ █████╗ ██████╗ ██╗     ███████╗
██╔════╝██║     ██╔══██╗██╔════╝     ╚══██╔══╝██╔══██╗██╔══██╗██║     ██╔════╝
█████╗  ██║     ███████║██║  ███╗       ██║   ███████║██████╔╝██║     █████╗
██╔══╝  ██║     ██╔══██║██║   ██║       ██║   ██╔══██║██╔══██╗██║     ██╔══╝
██║     ███████╗██║  ██║╚██████╔╝       ██║   ██║  ██║██████╔╝███████╗███████╗
╚═╝     ╚══════╝╚═╝  ╚═╝ ╚═════╝        ╚═╝   ╚═╝  ╚═╝╚═════╝ ╚══════╝╚══════╝
 */
/**
 * Gets the value of a flag from the database
 * Possible Errors:
 *  exceptionFailedSelect
 */
int Database::getFlagInt(const string& flag) {
    //try to get from ram
    try {
        return _flagState.at(flag); //will throw out of range error if not present
    } catch (const out_of_range& e) {

    }

    //get from database
    int rc;
    sqlite3_reset(_stmtCheckFlag);
    sqlite3_bind_text(_stmtCheckFlag, 1, flag.c_str(), flag.length(), SQLITE_STATIC);
    rc = sqlite3_step(_stmtCheckFlag);
    if (rc != SQLITE_ROW) {    //there should always be one
        string tempErrorMessage = sqlite3_errmsg(_db);
        throw exceptionFailedSelect();  //failed to check database
    }

    //store in ram and return
    _flagState[flag] = sqlite3_column_int(_stmtCheckFlag, 0);
    return _flagState[flag];
}

/**
 * Gets the value of wasPrunedExchangeHistory flag from the database
 * Possible Errors:
 *  exceptionFailedSelect
 */
int Database::getBeenPrunedExchangeHistory() {
    return getFlagInt("wasPrunedExchangeHistory");
}

/**
 * Gets the value of wasPrunedUTXOHistory flag from the database
 * Possible Errors:
 *  exceptionFailedSelect
 */
int Database::getBeenPrunedUTXOHistory() {
    return getFlagInt("wasPrunedUTXOHistory");
}

/**
 * Gets the value of wasPrunedVoteHistory flag from the database
 * Possible Errors:
 *  exceptionFailedSelect
 */
int Database::getBeenPrunedVoteHistory() {
    return getFlagInt("wasPrunedVoteHistory");
}


bool Database::getBeenPrunedNonAssetUTXOHistory() {
    return getFlagInt("wasPrunedNonAssetUTXOHistory");
}

/**
 * Sets the value of a flag in the database
 * Possible Errors:
 *  exceptionFailedUpdate
 */
void Database::setFlagInt(const std::string& flag, int state) {
    if (getFlagInt(flag) == state) return;//no need to do anything

    //store in database
    int rc;
    sqlite3_reset(_stmtSetFlag);
    sqlite3_bind_int(_stmtSetFlag, 1, state);
    sqlite3_bind_text(_stmtSetFlag, 2, flag.c_str(), flag.length(), nullptr);
    rc = sqlite3_step(_stmtSetFlag);
    if (rc != SQLITE_DONE) {    //there should always be one
        string tempErrorMessage = sqlite3_errmsg(_db);
        throw exceptionFailedUpdate();  //failed to check database
    }

    //store in ram
    _flagState[flag] = state;
}

/**
 * Sets the value of wasPrunedExchangeHistory flag in the database
 * Possible Errors:
 *  exceptionFailedUpdate
 */
void Database::setBeenPrunedExchangeHistory(int state) {
    setFlagInt("wasPrunedExchangeHistory", state);
}

/**
 * Sets the value of wasPrunedUTXOHistory flag in the database
 * Possible Errors:
 *  exceptionFailedUpdate
 */
void Database::setBeenPrunedUTXOHistory(int state) {
    setFlagInt("wasPrunedUTXOHistory", state);
}

/**
 * Sets the value of wasPrunedVoteHistory flag in the database
 * Possible Errors:
 *  exceptionFailedUpdate
 */
void Database::setBeenPrunedVoteHistory(int state) {
    setFlagInt("wasPrunedVoteHistory", state);
}

void Database::setBeenPrunedNonAssetUTXOHistory(bool state) {
    setFlagInt("wasPrunedNonAssetUTXOHistory", state);
}


/*
██████╗ ██╗      ██████╗  ██████╗██╗  ██╗    ████████╗ █████╗ ██████╗ ██╗     ███████╗
██╔══██╗██║     ██╔═══██╗██╔════╝██║ ██╔╝    ╚══██╔══╝██╔══██╗██╔══██╗██║     ██╔════╝
██████╔╝██║     ██║   ██║██║     █████╔╝        ██║   ███████║██████╔╝██║     █████╗
██╔══██╗██║     ██║   ██║██║     ██╔═██╗        ██║   ██╔══██║██╔══██╗██║     ██╔══╝
██████╔╝███████╗╚██████╔╝╚██████╗██║  ██╗       ██║   ██║  ██║██████╔╝███████╗███████╗
╚═════╝ ╚══════╝ ╚═════╝  ╚═════╝╚═╝  ╚═╝       ╚═╝   ╚═╝  ╚═╝╚═════╝ ╚══════╝╚══════╝
 */

/**
 * Sets the block hash
 * Possible Errors:
 *  exceptionFailedInsert
 * @param height
 * @param hash
 */
void Database::setBlockHash(uint height, const std::string& hash) {
    int rc;
    sqlite3_reset(_stmtSetBlockHash);
    sqlite3_bind_int(_stmtSetBlockHash, 1, height);
    Blob hashBlob = Blob(hash);
    sqlite3_bind_blob(_stmtSetBlockHash, 2, hashBlob.data(), SHA256_LENGTH,
                      SQLITE_STATIC);//could use hashBlob.length() but always SHA256_LENGTH
    rc = sqlite3_step(_stmtSetBlockHash);
    if (rc != SQLITE_DONE) {    //there should always be one
        string tempErrorMessage = sqlite3_errmsg(_db);
        throw exceptionFailedInsert();  //failed to check database
    }
}

/**
 * Gets the block hash
 * Possible Errors:
 *  exceptionDataPruned
 * @param height
 * @param hash
 */
std::string Database::getBlockHash(uint height) const {
    int rc;
    sqlite3_reset(_stmtGetBlockHash);
    sqlite3_bind_int(_stmtGetBlockHash, 1, height);
    rc = sqlite3_step(_stmtGetBlockHash);
    if (rc != SQLITE_ROW) {
        throw exceptionDataPruned();
    }
    Blob hash = Blob(sqlite3_column_blob(_stmtGetBlockHash, 0),
                     SHA256_LENGTH);         // could have used sqlite3_column_bytes(_stmtGetBlockHash,0);  but always 8
    return hash.toHex();
}

/**
 * Gets the newest block height in database
 * Possible Errors:
 *  exceptionFailedSelect
 */
uint Database::getBlockHeight() const {
    int rc;
    sqlite3_reset(_stmtGetBlockHeight);
    rc = sqlite3_step(_stmtGetBlockHeight);
    if (rc != SQLITE_ROW) {    //there should always be one
        throw exceptionFailedSelect();
    }
    return sqlite3_column_int(_stmtGetBlockHeight, 0);
}

/**
 * Clears all blocks above a specific height.
 * Used when rolling back data
 * Possible Errors:
 *  exceptionFailedDelete
 */
void Database::clearBlocksAboveHeight(uint height) {
    int rc;
    char* zErrMsg = 0;
    string lineEnd = to_string(height) + ";";
    const string sql = "DELETE FROM assets WHERE heightCreated>=" + lineEnd +
                       "DELETE FROM exchange WHERE height>=" + lineEnd +
                       "DELETE FROM kyc WHERE height>=" + lineEnd +
                       "UPDATE kyc SET revoked=NULL WHERE revoked>=" + lineEnd +
                       "DELETE FROM utxos WHERE heightCreated>=" + lineEnd +
                       "UPDATE utxos SET heightDestroyed=NULL WHERE heightDestroyed>=" + lineEnd +
                       "DELETE FROM votes WHERE height>=" + lineEnd +
                       "DELETE FROM blocks WHERE height>" + lineEnd;
    //todo may be trouble with domains,ipfs
    rc = sqlite3_exec(_db, sql.c_str(), Database::defaultCallback, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        sqlite3_free(zErrMsg);
        throw exceptionFailedDelete();
    }
}





/*
██╗   ██╗████████╗██╗  ██╗ ██████╗ ███████╗    ████████╗ █████╗ ██████╗ ██╗     ███████╗
██║   ██║╚══██╔══╝╚██╗██╔╝██╔═══██╗██╔════╝    ╚══██╔══╝██╔══██╗██╔══██╗██║     ██╔════╝
██║   ██║   ██║    ╚███╔╝ ██║   ██║███████╗       ██║   ███████║██████╔╝██║     █████╗
██║   ██║   ██║    ██╔██╗ ██║   ██║╚════██║       ██║   ██╔══██║██╔══██╗██║     ██╔══╝
╚██████╔╝   ██║   ██╔╝ ██╗╚██████╔╝███████║       ██║   ██║  ██║██████╔╝███████╗███████╗
 ╚═════╝    ╚═╝   ╚═╝  ╚═╝ ╚═════╝ ╚══════╝       ╚═╝   ╚═╝  ╚═╝╚═════╝ ╚══════╝╚══════╝
 */

/**
 * Creates a utxo entry in the database
 * Possible Errors:
 *  exceptionFailedInsert
 */
void Database::createUTXO(const AssetUTXO& value, unsigned int heightCreated) {
    if (value.address.empty()) return;  //op return don't store
    if (value.assets.empty() && getBeenPrunedNonAssetUTXOHistory()) {
        //_recentNonAssetUTXO.add(value.txid, value.vout); //keep temp cache that this is non asset utxo
        return; //non asset utxo and we aren't storing those
    }
    int rc;

    //add the main utxo
    sqlite3_reset(_stmtCreateUTXO);
    string address = value.address;
    sqlite3_bind_text(_stmtCreateUTXO, 1, address.c_str(), address.length(), nullptr);
    Blob blobTXID = Blob(value.txid);
    sqlite3_bind_blob(_stmtCreateUTXO, 2, blobTXID.data(), SHA256_LENGTH, SQLITE_STATIC);
    sqlite3_bind_int(_stmtCreateUTXO, 3, value.vout);
    sqlite3_bind_null(_stmtCreateUTXO, 4);
    sqlite3_bind_int(_stmtCreateUTXO, 5, 1);
    sqlite3_bind_int64(_stmtCreateUTXO, 6, value.digibyte);
    sqlite3_bind_int(_stmtCreateUTXO, 7, heightCreated);
    rc = sqlite3_step(_stmtCreateUTXO);
    if (rc != SQLITE_DONE) {
        string tempErrorMessage = sqlite3_errmsg(_db);
        throw exceptionFailedInsert();
    }

    //add any assets
    for (size_t aout = 0; aout < value.assets.size(); aout++) {
        sqlite3_reset(_stmtCreateUTXO);
        string address = value.address;
        sqlite3_bind_text(_stmtCreateUTXO, 1, address.c_str(), address.length(), nullptr);
        Blob blobTXID = Blob(value.txid);
        sqlite3_bind_blob(_stmtCreateUTXO, 2, blobTXID.data(), SHA256_LENGTH, SQLITE_STATIC);
        sqlite3_bind_int(_stmtCreateUTXO, 3, value.vout);
        sqlite3_bind_int(_stmtCreateUTXO, 4, aout);
        sqlite3_bind_int64(_stmtCreateUTXO, 5, value.assets[aout].getAssetIndex());
        sqlite3_bind_int64(_stmtCreateUTXO, 6, value.assets[aout].getCount());
        sqlite3_bind_int(_stmtCreateUTXO, 7, heightCreated);
        rc = sqlite3_step(_stmtCreateUTXO);
        if (rc != SQLITE_DONE) {
            string errorMessage = sqlite3_errmsg(_db);
            throw exceptionFailedInsert();
        }
    }
}

/**
 * Marks a utxo as spent
 * Possible Errors:
 *  exceptionFailedUpdate
 */
void Database::spendUTXO(const std::string& txid, unsigned int vout, unsigned int heightSpent) {
    int rc;
    sqlite3_reset(_stmtSpendUTXO);
    Blob blobTXID = Blob(txid);
    sqlite3_bind_int(_stmtSpendUTXO, 1, heightSpent);
    sqlite3_bind_blob(_stmtSpendUTXO, 2, blobTXID.data(), SHA256_LENGTH, SQLITE_STATIC);
    sqlite3_bind_int(_stmtSpendUTXO, 3, vout);
    rc = sqlite3_step(_stmtSpendUTXO);
    if (rc != SQLITE_DONE) {
        string tempErrorMessage = sqlite3_errmsg(_db);
        throw exceptionFailedUpdate();
    }
}

std::string Database::getSendingAddress(const string& txid, unsigned int vout) {
    int rc;
    sqlite3_reset(_stmtGetSpendingAddress);
    Blob blobTXID = Blob(txid);
    sqlite3_bind_blob(_stmtGetSpendingAddress, 1, blobTXID.data(), SHA256_LENGTH, SQLITE_STATIC);
    sqlite3_bind_int(_stmtGetSpendingAddress, 2, vout);
    rc = sqlite3_step(_stmtGetSpendingAddress);
    if (rc != SQLITE_ROW) {
        //try checking wallet
        if ((_dgb != nullptr) && (getBeenPrunedNonAssetUTXOHistory())) {
            getrawtransaction_t txData = _dgb->getRawTransaction(txid);
            if (txData.vout[vout].scriptPubKey.addresses.size() == 1) {
                return txData.vout[vout].scriptPubKey.addresses[0];
            }
        }

        //could not be found
        throw exceptionFailedSelect();
    }
    return reinterpret_cast<const char*>(sqlite3_column_text(_stmtGetSpendingAddress, 0));
}

/**
 * Removes all spent UTXOs below(not including) height
 * @param height
 */
void Database::pruneUTXO(unsigned int height) {
    //prune UTXOs
    int rc;
    sqlite3_reset(_stmtPruneUTXOs);
    sqlite3_bind_int(_stmtPruneUTXOs, 1, height);
    rc = sqlite3_step(_stmtPruneUTXOs);
    if (rc != SQLITE_DONE) {
        string tempErrorMessage = sqlite3_errmsg(_db);
        throw exceptionFailedDelete();
    }

    //mark blocks that can't be rolled back anymore
    sqlite3_reset(_stmtRemoveNonReachable);
    sqlite3_bind_int(_stmtRemoveNonReachable, 1, height);
    rc = sqlite3_step(_stmtRemoveNonReachable);
    if (rc != SQLITE_DONE) {
        string tempErrorMessage = sqlite3_errmsg(_db);
        throw exceptionFailedUpdate();
    }

    //set was pruned flag
    setBeenPrunedUTXOHistory(height);
}

/**
 * Returns a UTXO with any possible assets that may be on it
 * Warning if looking up old transactions(behind chain analyser) this may return saying no assets if pruning is on and
 * not storing non digiasset utxos.
 * @param txid
 * @param vout
 * @return
 */
AssetUTXO Database::getAssetUTXO(const string& txid, unsigned int vout) {
    if (true || !_recentNonAssetUTXO.exists(txid, vout)) {
        //try to get data from database
        sqlite3_reset(_stmtGetAssetUTXO);
        Blob blobTXID = Blob(txid);
        sqlite3_bind_blob(_stmtGetAssetUTXO, 1, blobTXID.data(), SHA256_LENGTH, SQLITE_STATIC);
        sqlite3_bind_int(_stmtGetAssetUTXO, 2, vout);
        AssetUTXO result;
        result.txid = txid;
        result.vout = vout;
        bool exists = false;
        while (sqlite3_step(_stmtGetAssetUTXO) == SQLITE_ROW) {
            exists = true;
            unsigned int assetIndex = sqlite3_column_int(_stmtGetAssetUTXO, 2);
            uint64_t amount = sqlite3_column_int64(_stmtGetAssetUTXO, 3);
            if (assetIndex == 1) {
                result.address = reinterpret_cast<const char*>(sqlite3_column_text(_stmtGetAssetUTXO, 0));
                result.digibyte = amount;
            } else {
                result.assets.emplace_back(getAsset(assetIndex, amount));
            }
        }
        if (exists) return result;
    }

    //check if we can/should get from the wallet
    if ((_dgb == nullptr) || (!getBeenPrunedNonAssetUTXOHistory())) throw exceptionDataPruned();

    //get tx data from wallet
    getrawtransaction_t txData = _dgb->getRawTransaction(txid);
    AssetUTXO result;
    result.txid = txid;
    result.vout = vout;
    result.digibyte = txData.vout[vout].valueS;
    result.address = txData.vout[vout].scriptPubKey.addresses[0];
    result.assets.clear();
    return result;
}


std::vector<AssetHolder> Database::getAssetHolders(uint64_t assetIndex) const {
    sqlite3_reset(_stmtGetAssetHolders);
    sqlite3_bind_int64(_stmtGetAssetHolders, 1, assetIndex);
    vector<AssetHolder> result;
    while (sqlite3_step(_stmtGetAssetHolders) == SQLITE_ROW) {
        string address = reinterpret_cast<const char*>(sqlite3_column_text(_stmtGetAssetHolders, 0));
        uint64_t count = sqlite3_column_int64(_stmtGetAssetHolders, 1);
        result.emplace_back(AssetHolder{
                .address=address,
                .count=count
        });
    }
    return result;
}

/**
 * Gets number of bytes paid for to permanent storage pool
 * @param txid
 * @return
 */
unsigned int Database::getPermanentSize(const string& txid) {
    vector<string> addresses{"dgb1qjnzadu643tsfzjqjydnh06s9lgzp3m4sg3j68x",
                             "dgb1qva97ew3zdwyadm5aqstqxe6xzzgxmxm7d6m3uw",
                             "dgb1qhucf64cleqdme9637vukgxau8aflpk00thlq98",
                             "dgb1qm4putt429lu9mlc6ypukky0fq3q9spm7pjwcy8",
                             "dgb1qj4glly6ka7py8pkdme9t0vh77s0gym0vq2esee",
                             "dgb1qnseslpvugsxcnvmz7m4emvmlgeryg80ujduspw",
                             "dgb1q8c6p9nht8055lr5fczcvc4v29hunluqv3n3gaf",
                             "dgb1qxhx0ahcmuxxmlwvnkjdq6dhmnem570g587m7hk",
                             "dgb1qfc9029kc8ptvqt2nuqe4sxtps2nd83kq7pugtm",
                             "dgb1q84h0g4lpy0prppc2507wf7ngne26thza0sntgr",
                             "dgb1qkqggn9y85tlyxdfhg9ls3ygph4nd58j0acnlz6",
                             "dgb1qnynkfl44ztsw3et6rq9yhxmefrcm8ufd3afm3e",
                             "dgb1qylaqaen0jqs2sk7jlc74yarw5lg4nzwtac9vyp",
                             "dgb1qatvzudt2jey06kx8zn3a6p0nw689s9dxkjp57g"};

    //try to look up in database
    unsigned int height = 0;
    uint64_t paid = 0;
    bool found = false;
    for (const string& address: addresses) {
        sqlite3_reset(_stmtGetPermanentPaid);
        Blob blobTXID = Blob(txid);
        sqlite3_bind_blob(_stmtGetPermanentPaid, 1, blobTXID.data(), SHA256_LENGTH, SQLITE_STATIC);
        sqlite3_bind_text(_stmtGetPermanentPaid, 2, address.c_str(), address.length(), nullptr);
        while (sqlite3_step(_stmtGetPermanentPaid) == SQLITE_ROW) {
            uint64_t paidPart = sqlite3_column_int64(_stmtGetPermanentPaid, 0);
            height = sqlite3_column_int(_stmtGetPermanentPaid, 1);
            if (paidPart > paid) paid = paidPart;//only use biggest, if more than 1
            found = true;
        }
    }

    //if not in database it may have been pruned so check the wallet if possible
    if (!found) {
        if ((_dgb == nullptr) || (!getBeenPrunedNonAssetUTXOHistory())) return 0;

        //get tx data from wallet
        getrawtransaction_t txData = _dgb->getRawTransaction(txid);
        for (auto& vout: txData.vout) {
            //determine if on address list
            if (vout.scriptPubKey.addresses.size() != 1) continue;
            found = false;
            for (const string& address: addresses) {
                if (vout.scriptPubKey.addresses[0] == address) {
                    found = true;
                    break;
                }
            }
            if ((found) && (vout.valueS > paid)) paid = vout.valueS;
        }
        if (paid == 0) return 0;

        //value found lets get the height
        height = _dgb->getBlock(txData.blockhash).height;
        if (height < 12642645) return 0; //Exchange rate was not published before this point
    }

    //get exchange rate
    double rate = getAcceptedExchangeRate(DigiAsset::standardExchangeRates[1], height);
    return 100000000000000 * paid / (rate * 1.2);
}

/*
███████╗██╗  ██╗ ██████╗██╗  ██╗ █████╗ ███╗   ██╗ ██████╗ ███████╗    ██╗    ██╗ █████╗ ████████╗ ██████╗██╗  ██╗
██╔════╝╚██╗██╔╝██╔════╝██║  ██║██╔══██╗████╗  ██║██╔════╝ ██╔════╝    ██║    ██║██╔══██╗╚══██╔══╝██╔════╝██║  ██║
█████╗   ╚███╔╝ ██║     ███████║███████║██╔██╗ ██║██║  ███╗█████╗      ██║ █╗ ██║███████║   ██║   ██║     ███████║
██╔══╝   ██╔██╗ ██║     ██╔══██║██╔══██║██║╚██╗██║██║   ██║██╔══╝      ██║███╗██║██╔══██║   ██║   ██║     ██╔══██║
███████╗██╔╝ ██╗╚██████╗██║  ██║██║  ██║██║ ╚████║╚██████╔╝███████╗    ╚███╔███╔╝██║  ██║   ██║   ╚██████╗██║  ██║
╚══════╝╚═╝  ╚═╝ ╚═════╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═══╝ ╚═════╝ ╚══════╝     ╚══╝╚══╝ ╚═╝  ╚═╝   ╚═╝    ╚═════╝╚═╝  ╚═╝
 */

bool Database::isWatchAddress(const string& address) const {
    //if to many watch addresses to buffer effectively use database
    if (_exchangeWatchAddresses.empty()) {
        int rc;
        sqlite3_reset(_stmtIsWatchAddress);
        sqlite3_bind_text(_stmtIsWatchAddress, 1, address.c_str(), address.length(), nullptr);
        rc = sqlite3_step(_stmtIsWatchAddress);
        return (rc == SQLITE_ROW);
    }

    //check if watch address
    for (const string& watchAddress: _exchangeWatchAddresses) {
        if (watchAddress == address) return true;
    }
    return false;
}

void Database::addWatchAddress(const string& address) {
    //make sure not already on watch list
    if (isWatchAddress(address)) return;

    //add to database
    int rc;
    sqlite3_reset(_stmtAddWatchAddress);
    sqlite3_bind_text(_stmtAddWatchAddress, 1, address.c_str(), address.length(), nullptr);
    rc = sqlite3_step(_stmtAddWatchAddress);
    if (rc != SQLITE_OK) {
        string tempErrorMessage = sqlite3_errmsg(_db);
        throw exceptionFailedInsert();
    }

    //add to watch buffer if using buffer
    if (_exchangeWatchAddresses.empty()) return;
    if (_exchangeWatchAddresses.size() == DIGIBYTECORE_DATABASE_CHAIN_WATCH_MAX) {
        _exchangeWatchAddresses.clear();    //got to big won't use buffer anymore
        return;
    }
    _exchangeWatchAddresses.push_back(address);
}

/*
███████╗██╗  ██╗ ██████╗██╗  ██╗ █████╗ ███╗   ██╗ ██████╗ ███████╗    ██████╗  █████╗ ████████╗███████╗
██╔════╝╚██╗██╔╝██╔════╝██║  ██║██╔══██╗████╗  ██║██╔════╝ ██╔════╝    ██╔══██╗██╔══██╗╚══██╔══╝██╔════╝
█████╗   ╚███╔╝ ██║     ███████║███████║██╔██╗ ██║██║  ███╗█████╗      ██████╔╝███████║   ██║   █████╗
██╔══╝   ██╔██╗ ██║     ██╔══██║██╔══██║██║╚██╗██║██║   ██║██╔══╝      ██╔══██╗██╔══██║   ██║   ██╔══╝
███████╗██╔╝ ██╗╚██████╗██║  ██║██║  ██║██║ ╚████║╚██████╔╝███████╗    ██║  ██║██║  ██║   ██║   ███████╗
╚══════╝╚═╝  ╚═╝ ╚═════╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═══╝ ╚═════╝ ╚══════╝    ╚═╝  ╚═╝╚═╝  ╚═╝   ╚═╝   ╚══════╝
 */

/**
 * This function should only ever be called by the chain analyzer
 */
void
Database::addExchangeRate(const string& address, unsigned int index, unsigned int height, double exchangeRate) {
    int rc;
    sqlite3_reset(_stmtAddExchangeRate);
    sqlite3_bind_text(_stmtAddExchangeRate, 1, address.c_str(), address.length(), SQLITE_STATIC);
    sqlite3_bind_int(_stmtAddExchangeRate, 2, index);
    sqlite3_bind_int(_stmtAddExchangeRate, 3, height);
    sqlite3_bind_double(_stmtAddExchangeRate, 4, exchangeRate);
    rc = sqlite3_step(_stmtAddExchangeRate);
    if (rc != SQLITE_DONE) {
        string tempErrorMessage = sqlite3_errmsg(_db);
        throw exceptionFailedUpdate();
    }
}

/**
 * Removes all exchange history below(not including) pruneHeight
 * also include the last tx before this height
 * @param pruneHeight
 */
void Database::pruneExchange(unsigned int pruneHeight) {
    struct entry {
        unsigned int height;
        string address;
        unsigned char index;
        double value;
    };

    //we need to keep at least 1 value before prune height for each value so lets find the newest before prune height for each exchange rate
    vector<entry> firstEntry;
    sqlite3_reset(_stmtExchangeRatesAtHeight);
    sqlite3_bind_int(_stmtExchangeRatesAtHeight, 1, pruneHeight - 1);
    while (sqlite3_step(_stmtExchangeRatesAtHeight) == SQLITE_ROW) {
        firstEntry.push_back(entry{
                .height=(unsigned int) sqlite3_column_int(_stmtExchangeRatesAtHeight, 0),
                .address=reinterpret_cast<const char*>(sqlite3_column_text(_stmtExchangeRatesAtHeight, 1)),
                .index=(unsigned char) sqlite3_column_int(_stmtExchangeRatesAtHeight, 2),
                .value=sqlite3_column_double(_stmtExchangeRatesAtHeight, 3)
        });
    }

    //delete from exchange where pruneHeight<pruneHeight;
    sqlite3_reset(_stmtPruneExchangeRate);
    sqlite3_bind_int(_stmtPruneExchangeRate, 1, pruneHeight);
    int rc = sqlite3_step(_stmtPruneExchangeRate);
    if (rc != SQLITE_DONE) {
        string tempErrorMessage = sqlite3_errmsg(_db);
        throw exceptionFailedDelete();
    }

    //mark blocks that can't be rolled back anymore
    sqlite3_reset(_stmtRemoveNonReachable);
    sqlite3_bind_int(_stmtRemoveNonReachable, 1, pruneHeight);
    rc = sqlite3_step(_stmtRemoveNonReachable);
    if (rc != SQLITE_DONE) {
        string tempErrorMessage = sqlite3_errmsg(_db);
        throw exceptionFailedUpdate();
    }

    //add first valuessqlite3_bind_int back in to table
    for (entry& kept: firstEntry) {
        addExchangeRate(kept.address, kept.index, kept.height, kept.value);
    }

    //set was pruned flag
    setBeenPrunedExchangeHistory(pruneHeight);
}

/**
 * Return min value that was live within the last 240 blocks(so we need to include value before that time since it was live at cut off)
 * @param rate
 * @param height
 * @return
 */
double Database::getAcceptedExchangeRate(const ExchangeRate& rate, unsigned int height) const {
    //see if there is an exchange rate or not
    if (!rate.enabled()) return 100000000;

    //historic errors
    if ((height >= 16562773) && (height <= 16562782) && (rate.index == 1) &&
        (rate.address == "dgb1qunxh378eltj2jrwza5sj9grvu5xud43vqvudwh")) {
        return 9032962000;
    }

    //look up exchange rate
    sqlite3_reset(_stmtGetValidExchangeRate);
    sqlite3_bind_int(_stmtGetValidExchangeRate, 1, height);
    sqlite3_bind_text(_stmtGetValidExchangeRate, 2, rate.address.c_str(), rate.address.length(), SQLITE_STATIC);
    sqlite3_bind_int(_stmtGetValidExchangeRate, 3, rate.index);
    double min = numeric_limits<double>::infinity();
    while (sqlite3_step(_stmtGetValidExchangeRate) == SQLITE_ROW) {
        //check if smallest
        double currentRate = sqlite3_column_double(_stmtGetValidExchangeRate, 0);
        if (currentRate < min) min = currentRate;

        //check if that was the last sample(this test is done after the sample taken because we want to always get at least 1 sample past the expiry unless sample falls exactly on the expiry)
        unsigned int sampleHeight = sqlite3_column_int(_stmtGetValidExchangeRate, 1);
        if (sampleHeight <= height - DigiAsset::EXCHANGE_RATE_LENIENCY) break;
    };
    if (min == numeric_limits<double>::infinity()) throw out_of_range("Unknown Exchange Rate");
    return min;
}

double Database::getCurrentExchangeRate(const ExchangeRate& rate) const {
    sqlite3_reset(_stmtGetCurrentExchangeRate);
    sqlite3_bind_text(_stmtGetCurrentExchangeRate, 1, rate.address.c_str(), rate.address.length(), SQLITE_STATIC);
    sqlite3_bind_int(_stmtGetCurrentExchangeRate, 2, rate.index);
    if (sqlite3_step(_stmtGetCurrentExchangeRate) != SQLITE_ROW) throw out_of_range("Unknown Exchange Rate");
    return sqlite3_column_double(_stmtGetCurrentExchangeRate, 0);
}

/*
██╗  ██╗██╗   ██╗ ██████╗
██║ ██╔╝╚██╗ ██╔╝██╔════╝
█████╔╝  ╚████╔╝ ██║
██╔═██╗   ╚██╔╝  ██║
██║  ██╗   ██║   ╚██████╗
╚═╝  ╚═╝   ╚═╝    ╚═════╝
 */

void Database::addKYC(const string& address, const string& country, const string& name, const string& hash,
                      unsigned int height) {
    int rc;
    sqlite3_reset(_stmtAddKYC);
    Blob blobTXID = Blob(hash);
    sqlite3_bind_text(_stmtAddKYC, 1, address.c_str(), address.length(), SQLITE_STATIC);
    sqlite3_bind_text(_stmtAddKYC, 2, country.c_str(), country.length(), SQLITE_STATIC);
    sqlite3_bind_text(_stmtAddKYC, 3, name.c_str(), name.length(), SQLITE_STATIC);
    sqlite3_bind_blob(_stmtAddKYC, 4, blobTXID.data(), SHA256_LENGTH, SQLITE_STATIC);
    sqlite3_bind_int(_stmtAddKYC, 5, height);
    rc = sqlite3_step(_stmtAddKYC);
    if (rc != SQLITE_DONE) {
        string tempErrorMessage = sqlite3_errmsg(_db);
        throw exceptionFailedUpdate();
    }
}

void Database::revokeKYC(const string& address, unsigned int height) {

    int rc;
    sqlite3_reset(_stmtRevokeKYC);
    sqlite3_bind_int(_stmtRevokeKYC, 1, height);
    sqlite3_bind_text(_stmtRevokeKYC, 2, address.c_str(), address.length(), SQLITE_STATIC);
    rc = sqlite3_step(_stmtRevokeKYC);
    if (rc != SQLITE_DONE) {
        string tempErrorMessage = sqlite3_errmsg(_db);
        throw exceptionFailedUpdate();
    }
}

KYC Database::getAddressKYC(const string& address) const {
    int rc;
    sqlite3_reset(_stmtGetKYC);
    sqlite3_bind_text(_stmtGetKYC, 1, address.c_str(), address.length(), SQLITE_STATIC);
    rc = sqlite3_step(_stmtGetKYC);
    if (rc != SQLITE_ROW) {
        return {
                address
        };
    }
    Blob hash(sqlite3_column_blob(_stmtGetKYC, 2), sqlite3_column_bytes(_stmtGetKYC, 2));
    return {
            address,
            reinterpret_cast<const char*>(sqlite3_column_text(_stmtGetKYC, 0)),
            reinterpret_cast<const char*>(sqlite3_column_text(_stmtGetKYC, 1)),
            hash.toHex(),
            static_cast<unsigned int>(sqlite3_column_int(_stmtGetKYC, 3)),
            sqlite3_column_int(_stmtGetKYC, 4)
    };
}

/*
██╗   ██╗ ██████╗ ████████╗███████╗
██║   ██║██╔═══██╗╚══██╔══╝██╔════╝
██║   ██║██║   ██║   ██║   █████╗
╚██╗ ██╔╝██║   ██║   ██║   ██╔══╝
 ╚████╔╝ ╚██████╔╝   ██║   ███████╗
  ╚═══╝   ╚═════╝    ╚═╝   ╚══════╝
 */

/**
 * Records votes to database
 */
void Database::addVote(const string& address, unsigned int assetIndex, uint64_t count, unsigned int height) {
    int rc;
    sqlite3_reset(_stmtAddVote);
    sqlite3_bind_int(_stmtAddVote, 1, assetIndex);
    sqlite3_bind_text(_stmtAddVote, 2, address.c_str(), address.length(), SQLITE_STATIC);
    sqlite3_bind_int(_stmtAddVote, 3, height);
    sqlite3_bind_int64(_stmtAddVote, 4, count);
    sqlite3_bind_int64(_stmtAddVote, 5, count);
    rc = sqlite3_step(_stmtAddVote);
    if (rc != SQLITE_DONE) {
        string tempErrorMessage = sqlite3_errmsg(_db);
        throw exceptionFailedInsert();
    }
}

/**
 * Prune vote history under(not including) height
 * @param height
 */
void Database::pruneVote(unsigned int height) {
    struct entries {
        unsigned int assetIndex;
        string address;
        unsigned int count;
        unsigned int height;
    };

    //we need to keep the sum of all votes we are pruning so lets add them up
    int rc;
    sqlite3_reset(_stmtGetVoteCount);
    sqlite3_bind_int(_stmtGetVoteCount, 1, height);
    vector<entries> keep;
    while (sqlite3_step(_stmtGetVoteCount) == SQLITE_ROW) {
        keep.emplace_back(entries{
                static_cast<unsigned int>(sqlite3_column_int(_stmtGetVoteCount, 0)),
                reinterpret_cast<const char*>(sqlite3_column_text(_stmtGetVoteCount, 1)),
                static_cast<unsigned int>(sqlite3_column_int(_stmtGetVoteCount, 2)),
                static_cast<unsigned int>(sqlite3_column_int(_stmtGetVoteCount, 3))
        });
    }

    //delete from votes where height<pruneHeight
    sqlite3_reset(_stmtPruneVote);
    sqlite3_bind_int(_stmtPruneVote, 1, height);
    rc = sqlite3_step(_stmtPruneVote);
    if (rc != SQLITE_DONE) {
        string tempErrorMessage = sqlite3_errmsg(_db);
        throw exceptionFailedDelete();
    }

    //mark blocks that can't be rolled back anymore
    sqlite3_reset(_stmtRemoveNonReachable);
    sqlite3_bind_int(_stmtRemoveNonReachable, 1, height);
    rc = sqlite3_step(_stmtRemoveNonReachable);
    if (rc != SQLITE_DONE) {
        string tempErrorMessage = sqlite3_errmsg(_db);
        throw exceptionFailedUpdate();
    }

    //insert kept values
    if (!keep.empty()) {
        for (entries& kept: keep) {
            addVote(kept.address, kept.assetIndex, kept.count, kept.height);
        }
    }

    //set was pruned flag
    setBeenPrunedVoteHistory(height);
}

/*
██╗██████╗ ███████╗███████╗
██║██╔══██╗██╔════╝██╔════╝
██║██████╔╝█████╗  ███████╗
██║██╔═══╝ ██╔══╝  ╚════██║
██║██║     ██║     ███████║
╚═╝╚═╝     ╚═╝     ╚══════╝
 */


void Database::registerIPFSCallback(const string& callbackSymbol, const IPFSCallbackFunction& callback) {
    //make sure valid name
    if ((callbackSymbol.empty()) || (callbackSymbol[0] == '_')) {
        throw out_of_range("Invalid callback symbol");
    }

    //register callback
    _ipfsCallbacks[callbackSymbol] = callback;
}


IPFSCallbackFunction& Database::getIPFSCallback(const string& callbackSymbol) {
    auto it = _ipfsCallbacks.find(callbackSymbol);
    if (it == _ipfsCallbacks.end()) throw std::out_of_range("Non existent callback");
    return it->second;
}

/**
 * Gets the next IPFS job that can be processed
 * @param jobIndex - number to refer back to this job for updating it
 * @param cid - cid of the file that should be downloaded or pinned
 * @param sync - string that represents what order jobs should be done in
 *                  "pin", "_", or "" can be done in any order any other value must be done in sequential order by jobIndex for matching sync values
 *                  "pin" means the job is to be pinned not downloaded
 *                  "_" means a promise has been made for this job and that the job will be canceled if the app closes
 *                  "" means there is no callback
 * @param extra - a string that can be used for any purpose by the caller
 * @param maxSleep - max number of ms allowed to wait before should give up
 * @param callback - callback function to be run when done.
 */
void Database::getNextIPFSJob(unsigned int& jobIndex, string& cid, string& sync, string& extra, unsigned int& maxSleep,
                              IPFSCallbackFunction& callback) {
    //make sure only 1 thread can call at a time
    std::lock_guard<std::mutex> lock(_mutexGetNextIPFSJob);

    //clear pauses that are done
    if (!_ipfsCurrentlyPaused.empty()) {
        //get current time
        uint64_t currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();

        //look through ram paused list and see if any ready to be unpaused
        bool removed = false;
        auto it = _ipfsCurrentlyPaused.begin();
        while (it != _ipfsCurrentlyPaused.end()) {
            if (it->second < currentTime) {
                removed = true;
                it = _ipfsCurrentlyPaused.erase(it);
            } else {
                it++;
            }
        }

        //clear from database
        if (removed) {
            sqlite3_reset(_stmtClearIPFSPause);
            sqlite3_bind_int64(_stmtClearIPFSPause, 1, currentTime);
            int rc = sqlite3_step(_stmtClearIPFSPause);
            if (rc != SQLITE_DONE) throw exceptionFailedUpdate();
        }
    }

    //lookup the next job(if there is one)
    sqlite3_reset(_stmtGetNextIPFSJob);
    int rc = sqlite3_step(_stmtGetNextIPFSJob);
    if (rc != SQLITE_ROW) {
        jobIndex = 0;//signal there are no new jobs
        return;
    }
    jobIndex = sqlite3_column_int(_stmtGetNextIPFSJob, 0);
    sync = reinterpret_cast<const char*>(sqlite3_column_text(_stmtGetNextIPFSJob, 1));
    cid = reinterpret_cast<const char*>(sqlite3_column_text(_stmtGetNextIPFSJob, 2));
    extra = reinterpret_cast<const char*>(sqlite3_column_text(_stmtGetNextIPFSJob, 3));
    string callbackSymbol = reinterpret_cast<const char*>(sqlite3_column_text(_stmtGetNextIPFSJob, 4));

    //get max time
    uint64_t currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    uint64_t maxTime = sqlite3_column_int64(_stmtGetNextIPFSJob, 5);
    if (maxTime == 0) {
        //if null return forever
        maxSleep = std::numeric_limits<uint64_t>::max();
    } else if (currentTime >= maxTime) {
        //its already expired
        maxSleep = 0;
        //todo we should cancel it and get a new one
    } else {
        maxSleep = maxTime - currentTime;
    }

    //look up callback function
    if (callbackSymbol == "_") {
        callbackSymbol += to_string(jobIndex);
    }
    callback = _ipfsCallbacks[callbackSymbol];  //get callback(note there is a default callback if empty)


    //lock the sync if not pin or non synchronized
    if ((sync == "pin") || (sync == "_") || (sync.empty())) {
        sqlite3_reset(_stmtSetIPFSLockJob);
        sqlite3_bind_int(_stmtSetIPFSLockJob, 1, jobIndex);
        rc = sqlite3_step(_stmtSetIPFSLockJob);
    } else {
        sqlite3_reset(_stmtSetIPFSLockSync);
        sqlite3_bind_text(_stmtSetIPFSLockSync, 1, sync.c_str(), sync.length(), SQLITE_STATIC);
        rc = sqlite3_step(_stmtSetIPFSLockSync);
    }
    if (rc != SQLITE_DONE) throw exceptionFailedUpdate();
}

void Database::pauseIPFSSync(unsigned int jobIndex, const string& sync, unsigned int pauseLengthInMilliSeconds) {
    //get current time
    uint64_t currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    uint64_t unpauseTime = currentTime + pauseLengthInMilliSeconds;

    //update database
    int rc;
    if ((sync == "pin") || (sync == "_") || (sync.empty())) {    //handle jobs that are non ordered
        sqlite3_reset(_stmtSetIPFSPauseJob);
        sqlite3_bind_int64(_stmtSetIPFSPauseJob, 1, unpauseTime);
        sqlite3_bind_int(_stmtSetIPFSPauseJob, 2, jobIndex);
        rc = sqlite3_step(_stmtSetIPFSPauseJob);
    } else {
        sqlite3_reset(_stmtSetIPFSPauseSync);
        sqlite3_bind_int64(_stmtSetIPFSPauseSync, 1, unpauseTime);
        sqlite3_bind_text(_stmtSetIPFSPauseSync, 2, sync.c_str(), sync.length(), SQLITE_STATIC);
        rc = sqlite3_step(_stmtSetIPFSPauseSync);

        //update ram
        if (rc == SQLITE_DONE) _ipfsCurrentlyPaused.emplace_back(sync, unpauseTime);
    }
    if (rc != SQLITE_DONE) {    //there should always be one
        string tempErrorMessage = sqlite3_errmsg(_db);
        throw exceptionFailedDelete();  //failed to check database
    }
}

/**
 * This function is only meant to be called by IPFS thread and assumes that the job is not paused since it was just executed
 * @param jobIndex
 */
void Database::removeIPFSJob(unsigned int jobIndex, const string& sync) {
    //remove from database
    sqlite3_reset(_stmtClearNextIPFSJob_a);
    sqlite3_bind_int(_stmtClearNextIPFSJob_a, 1, jobIndex);
    int rc = sqlite3_step(_stmtClearNextIPFSJob_a);
    if (rc != SQLITE_DONE) {    //there should always be one
        string tempErrorMessage = sqlite3_errmsg(_db);
        throw exceptionFailedSQLCommand();  //failed to delete or unlock
    }
    sqlite3_reset(_stmtClearNextIPFSJob_b);
    sqlite3_bind_text(_stmtClearNextIPFSJob_b, 1, sync.c_str(), sync.length(), SQLITE_STATIC);
    rc = sqlite3_step(_stmtClearNextIPFSJob_b);
    if (rc != SQLITE_DONE) {    //there should always be one
        string tempErrorMessage = sqlite3_errmsg(_db);
        throw exceptionFailedSQLCommand();  //failed to delete or unlock
    }

    //remove callback from ram if temp callback
    if (sync == "_") {
        _ipfsCallbacks.erase("_" + to_string(jobIndex));
    }
}

/**
 * Add a job to the IPFS job list
 * @param cid - cid of the file that should be downloaded or pinned
 * @param sync - string that represents what order jobs should be done in
 *                  "pin", "_", or "" can be done in any order any other value must be done in sequential order by jobIndex for matching sync values
 *                  "pin" means the job is to be pinned not downloaded
 *                  "_" means a promise has been made for this job and that the job will be canceled if the app closes
 *                  "" means there is no callback
 * @param extra - a string that can be used for any purpose by the caller
 * @param maxSleep - 0 for no max or time in ms
 * @param callbackSymbol - string representing a registered function
 * @return
 */
unsigned int
Database::addIPFSJob(const string& cid, const string& sync, const string& extra, unsigned int maxSleep,
                     const string& callbackSymbol) {
    //see if paused
    uint64_t pause = 0;
    for (pair<std::string, uint64_t> pausePair: _ipfsCurrentlyPaused) {
        //just in case there are more than one pause in ram use the greater value
        if ((pausePair.first == sync) && (pausePair.second > pause)) {
            pause = pausePair.second;
        }
    }

    //add type download to database
    sqlite3_reset(_stmtInsertIPFSJob);
    sqlite3_bind_text(_stmtInsertIPFSJob, 1, sync.c_str(), sync.length(), SQLITE_STATIC);
    sqlite3_bind_text(_stmtInsertIPFSJob, 2, cid.c_str(), cid.length(), SQLITE_STATIC);
    sqlite3_bind_text(_stmtInsertIPFSJob, 3, extra.c_str(), extra.length(), SQLITE_STATIC);
    sqlite3_bind_text(_stmtInsertIPFSJob, 4, callbackSymbol.c_str(), callbackSymbol.length(), SQLITE_STATIC);
    if (pause != 0) {
        sqlite3_bind_int64(_stmtInsertIPFSJob, 5, pause);
    } else {
        sqlite3_bind_null(_stmtInsertIPFSJob, 5);
    }
    if (maxSleep == 0) {
        sqlite3_bind_null(_stmtInsertIPFSJob, 6);
    } else {
        uint64_t currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
        sqlite3_bind_int64(_stmtInsertIPFSJob, 6, currentTime + maxSleep);
    }

    int rc = sqlite3_step(_stmtInsertIPFSJob);
    if (rc != SQLITE_DONE) {
        string tempErrorMessage = sqlite3_errmsg(_db);
        throw exceptionFailedInsert();
    }
    return sqlite3_last_insert_rowid(_db);
}

using IPFSCallbackFunction = std::function<void(const std::string&, const std::string&, const std::string&,
                                                bool failed)>;
vector<tuple<string, string, IPFSCallbackFunction>> toDoLater;

promise<string> Database::addIPFSJobPromise(const string& cid, const string& sync, unsigned int maxTime) {
    promise<string> result;

    //add job to the database
    unsigned int jobIndex = addIPFSJob(cid, sync, "", maxTime, "_");

    //register callback function
    _ipfsCallbacks["_" + to_string(jobIndex)] = [&](const string& cid, const string& extra, const string& content,
                                                    bool failed) {
        if (failed) {
            result.set_exception(std::make_exception_ptr(IPFS::exceptionTimeout()));
        } else {
            result.set_value(content);
        }
    };

    return result;
}

/*
██████╗  ██████╗ ███╗   ███╗ █████╗ ██╗███╗   ██╗
██╔══██╗██╔═══██╗████╗ ████║██╔══██╗██║████╗  ██║
██║  ██║██║   ██║██╔████╔██║███████║██║██╔██╗ ██║
██║  ██║██║   ██║██║╚██╔╝██║██╔══██║██║██║╚██╗██║
██████╔╝╚██████╔╝██║ ╚═╝ ██║██║  ██║██║██║ ╚████║
╚═════╝  ╚═════╝ ╚═╝     ╚═╝╚═╝  ╚═╝╚═╝╚═╝  ╚═══╝
 */

void Database::revokeDomain(const string& domain) {
    sqlite3_reset(_stmtRevokeDomain);
    sqlite3_bind_text(_stmtRevokeDomain, 1, domain.c_str(), domain.length(), SQLITE_STATIC);
    int rc = sqlite3_step(_stmtRevokeDomain);
    if (rc != SQLITE_DONE) {
        string tempErrorMessage = sqlite3_errmsg(_db);
        throw exceptionFailedUpdate();
    }
}

void Database::addDomain(const string& domain, const string& assetId) {
    sqlite3_reset(_stmtAddDomain);
    sqlite3_bind_text(_stmtAddDomain, 1, domain.c_str(), domain.length(), SQLITE_STATIC);
    sqlite3_bind_text(_stmtAddDomain, 2, assetId.c_str(), assetId.length(), SQLITE_STATIC);
    int rc = sqlite3_step(_stmtAddDomain);
    if (rc != SQLITE_DONE) {
        string tempErrorMessage = sqlite3_errmsg(_db);
        throw exceptionFailedUpdate();
    }
}

string Database::getDomainAssetId(const std::string& domain, bool returnErrorIfRevoked) const {
    sqlite3_reset(_stmtGetDomainAssetId);
    sqlite3_bind_text(_stmtGetDomainAssetId, 1, domain.c_str(), domain.length(), SQLITE_STATIC);
    int rc = sqlite3_step(_stmtGetDomainAssetId);

    if (rc != SQLITE_ROW) throw DigiByteDomain::exceptionUnknownDomain();
    if (returnErrorIfRevoked && (sqlite3_column_int(_stmtGetDomainAssetId, 1) == true)) {
        throw DigiByteDomain::exceptionRevokedDomain();
    }
    return reinterpret_cast<const char*>(sqlite3_column_text(_stmtGetDomainAssetId, 0));
}

bool Database::isMasterDomainAssetId(const std::string& assetId) const {
    for (const string& id: _masterDomainAssetId) {
        if (id == assetId) return true;
    }
    return false;
}

bool Database::isActiveMasterDomainAssetId(const std::string& assetId) const {
    return (_masterDomainAssetId.back() == assetId);
}

void Database::setMasterDomainAssetId(const string& assetId) {
    sqlite3_reset(_stmtSetDomainMasterAssetId_a);
    string lastDomain = _masterDomainAssetId.back();
    sqlite3_bind_text(_stmtSetDomainMasterAssetId_a, 1, lastDomain.c_str(), lastDomain.length(), SQLITE_STATIC);
    int rc = sqlite3_step(_stmtSetDomainMasterAssetId_a);
    if (rc != SQLITE_DONE) {
        string tempErrorMessage = sqlite3_errmsg(_db);
        throw exceptionFailedUpdate();
    }
    sqlite3_reset(_stmtSetDomainMasterAssetId_b);
    sqlite3_bind_text(_stmtSetDomainMasterAssetId_b, 1, assetId.c_str(), assetId.length(), SQLITE_STATIC);
    rc = sqlite3_step(_stmtSetDomainMasterAssetId_b);
    if (rc != SQLITE_DONE) {
        string tempErrorMessage = sqlite3_errmsg(_db);
        throw exceptionFailedInsert();
    }
    _masterDomainAssetId.push_back(assetId);
}

void Database::setDomainCompromised() {
    setMasterDomainAssetId("");
}

bool Database::isDomainCompromised() const {
    return (_masterDomainAssetId.back().empty());
}

std::string Database::getDomainAddress(const string& domain) const {
    string assetId = getDomainAssetId(domain);
    uint64_t assetIndex = getAssetIndex(assetId);
    vector<AssetHolder> holders = getAssetHolders(assetIndex);
    if (holders.empty()) throw DigiByteDomain::exceptionBurnedDomain();
    return holders[0].address;
}

/*
██╗  ██╗███████╗██╗     ██████╗ ███████╗██████╗ ███████╗
██║  ██║██╔════╝██║     ██╔══██╗██╔════╝██╔══██╗██╔════╝
███████║█████╗  ██║     ██████╔╝█████╗  ██████╔╝███████╗
██╔══██║██╔══╝  ██║     ██╔═══╝ ██╔══╝  ██╔══██╗╚════██║
██║  ██║███████╗███████╗██║     ███████╗██║  ██║███████║
╚═╝  ╚═╝╚══════╝╚══════╝╚═╝     ╚══════╝╚═╝  ╚═╝╚══════╝
 */
int Database::defaultCallback(void* NotUsed, int argc, char** argv, char** azColName) {
    {
        int i;
        for (i = 0; i < argc; i++) {
            printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
        }
        printf("\n");
        return 0;
    }
}

void Database::setDigiByteCore(DigiByteCore& core) {
    _dgb = &core;
}





