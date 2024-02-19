//
// Created by mctrivia on 30/01/23.
//


#include "Database.h"
#include "AppMain.h"
#include "Blob.h"
#include "DigiAsset.h"
#include "DigiByteDomain.h"
#include "Log.h"
#include "PermanentStoragePool/PermanentStoragePoolList.h"
#include "utils.h"
#include <cmath>
#include <cstring>
#include <mutex>
#include <sqlite3.h>
#include <stdio.h>
#include <sys/stat.h>

using namespace std;

/**
 * Defines default IPFS callbacks
 */
std::map<std::string, IPFSCallbackFunction> Database::_ipfsCallbacks = {
        {"",
         [](const std::string&, const std::string&, const std::string&, bool) {}} //generic do nothing callback
};

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
void Database::buildTables(unsigned int dbVersionNumber) {
    unsigned int skipUpToVersion = 0;
    vector<function<void()>> lambdaFunctions = {

            //Define Version 4 table setup
            [&]() {
                char* zErrMsg = 0;
                int rc;
                const char* sql =
                        //chain data tables
                        "BEGIN TRANSACTION;"

                        "CREATE TABLE \"assets\" (\"assetIndex\" INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, \"assetId\" TEXT NOT NULL, \"cid\" TEXT, \"issueAddress\" TEXT NOT NULL, \"rules\" BLOB, \"heightCreated\" INTEGER NOT NULL, \"heightUpdated\" INTEGER NOT NULL, \"expires\" INTEGER);"
                        "INSERT INTO \"assets\" VALUES (1,'DigiByte','QmfSVLAntanDUKrEHUnTXRh53GLUBHFfxk5x6LH4zz9PM4','STANDARD','{}',1,1,NULL);"

                        "CREATE TABLE \"blocks\" (\"height\" INTEGER NOT NULL, \"hash\" BLOB NOT NULL, \"time\" INTEGER NOT NULL, \"algo\" INTEGER NOT NULL, \"difficulty\" REAL NOT NULL, PRIMARY KEY(\"height\"));"
                        "INSERT INTO \"blocks\" VALUES (1,X'4da631f2ac1bed857bd968c67c913978274d8aabed64ab2bcebc1665d7f4d3a0',1389392876,1,0.000244140625);"

                        "CREATE TABLE \"exchange\" (\"address\" TEXT NOT NULL, \"index\" INTEGER NOT NULL, \"height\" INTEGER NOT NULL, \"value\" REAL NOT NULL, PRIMARY KEY(\"address\",\"index\",\"height\"));"

                        "CREATE TABLE \"exchangeWatch\" (\"address\" TEXT NOT NULL, PRIMARY KEY(\"address\"));"
                        "INSERT INTO \"exchangeWatch\" VALUES (\"dgb1qunxh378eltj2jrwza5sj9grvu5xud43vqvudwh\");"
                        "INSERT INTO \"exchangeWatch\" VALUES (\"dgb1qlk3hldeynl3prqw259u8gv0jh7w5nwppxlvt3v\");"

                        "CREATE TABLE \"flags\" (\"key\" TEXT NOT NULL, \"value\" INTEGER NOT NULL, PRIMARY KEY(\"key\"));"
                        "INSERT INTO \"flags\" VALUES (\"wasPrunedExchangeHistory\",-1);"
                        "INSERT INTO \"flags\" VALUES (\"wasPrunedUTXOHistory\",-1);"
                        "INSERT INTO \"flags\" VALUES (\"wasPrunedVoteHistory\",-1);"
                        "INSERT INTO \"flags\" VALUES (\"wasPrunedNonAssetUTXOHistory\",0);"
                        "INSERT INTO \"flags\" VALUES (\"dbVersion\",4);"

                        "CREATE TABLE \"kyc\" (\"address\" TEXT NOT NULL, \"country\" TEXT NOT NULL, \"name\" TEXT NOT NULL, \"hash\" BLOB NOT NULL, \"height\" INTEGER NOT NULL, \"revoked\" INTEGER, PRIMARY KEY(\"address\"));"
                        "CREATE INDEX kyc_height_index ON kyc(height);"

                        "CREATE TABLE \"utxos\" (\"address\" TEXT NOT NULL, \"txid\" BLOB NOT NULL, \"vout\" INTEGER NOT NULL, \"aout\" INTEGER, \"assetIndex\" Integer NOT NULL, \"amount\" INTEGER NOT NULL, \"heightCreated\" INTEGER NOT NULL, \"heightDestroyed\" INTEGER, PRIMARY KEY(\"address\",\"txid\",\"vout\",\"aout\"));"

                        "CREATE TABLE \"votes\" (\"assetIndex\" Integer NOT NULL, \"address\" TEXT NOT NULL, \"height\" INTEGER NOT NULL, \"count\" INTEGER NOT NULL, PRIMARY KEY(\"assetIndex\",\"address\",\"height\"));"

                        //IPFS job tables
                        "CREATE TABLE \"ipfs\" (\"jobIndex\" INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, \"sync\" TEXT NOT NULL, \"lock\" BOOL NOT NULL, \"cid\" TEXT NOT NULL, \"extra\" TEXT, \"callback\" TEXT NOT NULL, \"pause\" INTEGER, \"maxTime\" INTEGER);"
                        "INSERT INTO \"ipfs\" VALUES (1,'pin',false,'QmfSVLAntanDUKrEHUnTXRh53GLUBHFfxk5x6LH4zz9PM4','','',NULL,NULL);"
                        "INSERT INTO \"ipfs\" VALUES (2,'pin',false,'QmSAcz2H7veyeuuSyACLkSj9ts9EWm1c9v7uTqbHynsVbj','','',NULL,NULL);"

                        //PSP tables
                        "CREATE TABLE \"pspFiles\" (\"cid\" TEXT NOT NULL,\"poolIndex\");"
                        "CREATE TABLE \"pspAssets\" (\"assetIndex\" INT NOT NUll,\"poolIndex\");"

                        //DigiByte Domain tables
                        "CREATE TABLE \"domains\" (\"domain\" TEXT NOT NULL, \"assetId\" TEXT NOT NULL, \"revoked\" BOOL NOT NULL);"
                        "CREATE TABLE \"domainsMasters\" (\"assetId\" TEXT NOT NULL, \"active\" BOOL NOT NULL);"
                        "INSERT INTO \"domainsMasters\" VALUES (\"Ua7Bd7UVtrzavSHhpHxHZ2nzS2hGaHXRMT9sqy\",true);"

                        "COMMIT";
                rc = sqlite3_exec(_db, sql, Database::defaultCallback, 0, &zErrMsg);
                skipUpToVersion = 4; //tell not to execute steps until version 4 to 5 transition
                if (rc != SQLITE_OK) {
                    sqlite3_free(zErrMsg);
                    throw exceptionFailedToCreateTable();
                }
            },


            //Define what is changed from version 1 to version 2
            [&]() {
                Log* log=Log::GetInstance();
                log->addMessage("Unsupported database version.",Log::CRITICAL);
                throw runtime_error("Unsupported database version.");
            },


            //Define what is changed from version 2 to version 3
            [&]() {
                Log* log=Log::GetInstance();
                log->addMessage("Unsupported database version.",Log::CRITICAL);
                throw runtime_error("Unsupported database version.");
            },


            //Define what is changed from version 3 to version 4
            [&]() {
                Log* log=Log::GetInstance();
                log->addMessage("Unsupported database version.",Log::CRITICAL);
                throw runtime_error("Unsupported database version.");
            }

            /*  To modify table structure place a comma after the last } above and then place the bellow code.
         *
            //Define what is changed from version x to version x+1
            [&]() {
                //put all code needed to modify table structure.
                //dont forget to include at the end
                //"UPDATE \"flags\" set \"value\"=x+1 WHERE \"key\"=\"dbVersion\";"
                //x+1 above is equal to the value in comment above and should be replaced with actual integer value
            }
         */
    };

    ///IF ADDING ANY MORE TABLES MAKE SURE YOU UPDATE reset();

    //make any necessary changes to database structure to bring up to current version
    for (unsigned int i = dbVersionNumber; i < lambdaFunctions.size(); ++i) {
        if (dbVersionNumber >= skipUpToVersion) lambdaFunctions[i]();
    }
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
    const char* sql21 = "INSERT INTO blocks VALUES (?,?,?,?,?);";
    rc = sqlite3_prepare_v2(_db, sql21, strlen(sql21), &_stmtInsertBlock, nullptr);
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
    const char* sql46 = "SELECT address,SUM(amount) FROM utxos WHERE assetIndex=? AND heightDestroyed IS NULL GROUP BY address;";
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

    //statement to get number of assets that exist
    const char* sql49a = "SELECT sum(amount) FROM utxos where assetIndex=? and heightDestroyed is null;";
    rc = sqlite3_prepare_v2(_db, sql49a, strlen(sql49a), &_stmtGetTotalAssetCounta, nullptr);
    if (rc != SQLITE_OK) throw exceptionCreatingStatement();
    const char* sql49b = "SELECT sum(u.amount) as totalAmount\n"
                         "FROM utxos u\n"
                         "JOIN assets a ON u.assetIndex = a.assetIndex\n"
                         "WHERE a.assetId = ? AND u.heightDestroyed IS NULL;";
    rc = sqlite3_prepare_v2(_db, sql49b, strlen(sql49b), &_stmtGetTotalAssetCountb, nullptr);
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
    const char* sql90 = "INSERT INTO assets (assetId, issueAddress, cid, rules, heightCreated, heightUpdated, expires) VALUES (?, ?, ?, ?, ?, ?, ?);";
    rc = sqlite3_prepare_v2(_db, sql90, strlen(sql90), &_stmtAddAsset, nullptr);
    if (rc != SQLITE_OK) {

        string tempErrorMessage = sqlite3_errmsg(_db);
        throw exceptionCreatingStatement();
    }

    //statement to update asset(aggregable only)
    const char* sql91 = "UPDATE assets SET heightUpdated=?, cid=?, rules=?, expires=? WHERE assetIndex=?";
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
    const char* sql95 = "SELECT assetId,cid,issueAddress,rules,heightCreated,heightUpdated FROM assets WHERE assetIndex=?";
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

    const char* sql102 = "INSERT INTO ipfs (sync, lock, cid, extra, callback, pause, maxTime) VALUES (?,false,?,?,?,?,?);";
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

    const char* sql108 = "SELECT count(*) FROM ipfs;";
    rc = sqlite3_prepare_v2(_db, sql108, strlen(sql108), &_stmtNumberOfIPFSJobs, nullptr);
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

    //IPFS Permanent statements
    const char* sql120 = "INSERT OR IGNORE INTO pspFiles (cid,poolIndex) VALUES (?,?)";
    rc = sqlite3_prepare_v2(_db, sql120, strlen(sql120), &_stmtInsertPermanent, nullptr);
    if (rc != SQLITE_OK) throw exceptionCreatingStatement();

    const char* sql120b = "DELETE FROM pspFiles WHERE cid=? AND poolIndex=?";
    rc = sqlite3_prepare_v2(_db, sql120b, strlen(sql120b), &_stmtDeletePermanent, nullptr);
    if (rc != SQLITE_OK) throw exceptionCreatingStatement();

    const char* sql120c = "SELECT 1 FROM pspFiles WHERE cid=?";
    rc = sqlite3_prepare_v2(_db, sql120c, strlen(sql120c), &_stmtIsInPermanent, nullptr);
    if (rc != SQLITE_OK) throw exceptionCreatingStatement();

    const char* sql121 = "INSERT INTO ipfs (sync, lock, cid, extra, callback, pause, maxTime) SELECT 'pin', 0, cid, '', '', NULL, NULL FROM assets WHERE cid != '';";
    rc = sqlite3_prepare_v2(_db, sql121, strlen(sql121), &_stmtRepinAssets, nullptr);
    if (rc != SQLITE_OK) throw exceptionCreatingStatement();

    const char* sql122 = "INSERT INTO ipfs (sync, lock, cid, extra, callback, pause, maxTime) SELECT 'pin', 0, cid, '', '', NULL, NULL FROM pspFiles WHERE \"poolIndex\" = ?;";
    rc = sqlite3_prepare_v2(_db, sql122, strlen(sql122), &_stmtRepinPermanentSpecific, nullptr);
    if (rc != SQLITE_OK) throw exceptionCreatingStatement();

    const char* sql124 = "INSERT OR IGNORE INTO pspAssets (assetIndex,poolIndex) VALUES (?,?);";
    rc = sqlite3_prepare_v2(_db, sql124, strlen(sql124), &_stmtAddAssetToPool, nullptr);
    if (rc != SQLITE_OK) throw exceptionCreatingStatement();

    const char* sql125 = "SELECT COUNT(*) FROM \"pspAssets\" WHERE \"assetIndex\" = ? AND \"poolIndex\" = ?;";
    rc = sqlite3_prepare_v2(_db, sql125, strlen(sql125), &_stmtIsAssetInPool, nullptr);
    if (rc != SQLITE_OK) throw exceptionCreatingStatement();

    const char* sql125b = "SELECT 1 FROM \"pspAssets\" WHERE \"assetIndex\" = ?;";
    rc = sqlite3_prepare_v2(_db, sql125b, strlen(sql125b), &_stmtIsAssetInAPool, nullptr);
    if (rc != SQLITE_OK) throw exceptionCreatingStatement();

    const char* sql131 = "SELECT a.assetIndex, a.cid "
                         "FROM assets a "
                         "INNER JOIN pspAssets p ON a.assetIndex = p.assetIndex "
                         "WHERE a.assetId = ? AND p.poolIndex = ?;";
    rc = sqlite3_prepare_v2(_db, sql131, strlen(sql131), &_stmtPSPFindBadAsset, nullptr);
    if (rc != SQLITE_OK) throw exceptionCreatingStatement();

    const char* sql132 = "DELETE FROM pspAssets "
                         "WHERE assetIndex IN ("
                         "    SELECT a.assetIndex "
                         "    FROM assets a "
                         "    INNER JOIN pspAssets p ON a.assetIndex = p.assetIndex "
                         "    WHERE a.assetId = ? AND p.poolIndex = ?"
                         ");";
    rc = sqlite3_prepare_v2(_db, sql132, strlen(sql132), &_stmtPSPDeleteBadAsset, nullptr);
    if (rc != SQLITE_OK) throw exceptionCreatingStatement();




    //initialize exchange address watch list
    sqlite3_stmt* stmt1;
    const char* sqlInt10 = "SELECT address FROM exchangeWatch WHERE 1;";
    rc = sqlite3_prepare_v2(_db, sqlInt10, -1, &stmt1, NULL);
    if (rc != SQLITE_OK) throw exceptionCreatingStatement();
    for (;;) {
        rc = executeSqliteStepWithRetry(stmt1);
        if (rc == SQLITE_DONE) {
            break;
        }
        if (rc != SQLITE_ROW) {
            throw exceptionFailedSelect();
        }
        _exchangeWatchAddresses.emplace_back(reinterpret_cast<const char*>(sqlite3_column_text(stmt1, 0)));
        if (_exchangeWatchAddresses.size() ==
            DIGIBYTECORE_DATABASE_CHAIN_WATCH_MAX) { //check if watch has grown too big to buffer
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
    while (executeSqliteStepWithRetry(stmt3) == SQLITE_ROW) {
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
Database::Database(const string& newFileName) {
    bool firstRun = !utils::fileExists(newFileName);

    //open database
    int rc;
    rc = sqlite3_open_v2(newFileName.c_str(), &_db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, NULL);
    if (rc) throw exceptionFailedToOpen();

    //create needed tables
    if (firstRun) {
        buildTables();
    } else {
        //get database version number
        sqlite3_stmt* stmt;
        const char* sql = "SELECT `value` FROM `flags` WHERE `key`=\"dbVersion\";";
        rc = sqlite3_prepare_v2(_db, sql, -1, &stmt, NULL);
        if (rc != SQLITE_OK) throw exceptionCreatingStatement();
        if (executeSqliteStepWithRetry(stmt) != SQLITE_ROW) throw exceptionFailedSelect();
        unsigned int dbVersion = sqlite3_column_int(stmt, 0);

        //make sure tables are in the newest format
        buildTables(dbVersion);
    }

    //create needed statements
    initializeClassValues();
}

Database::~Database() {
    sqlite3_close_v2(_db);
    sqlite3_finalize(_stmtCheckFlag);
    sqlite3_finalize(_stmtSetFlag);
    sqlite3_finalize(_stmtGetBlockHeight);
    sqlite3_finalize(_stmtInsertBlock);
    sqlite3_finalize(_stmtGetBlockHash);
    sqlite3_finalize(_stmtCreateUTXO);
    sqlite3_finalize(_stmtSpendUTXO);
    sqlite3_finalize(_stmtIsWatchAddress);
    sqlite3_finalize(_stmtAddWatchAddress);
    sqlite3_finalize(_stmtGetSpendingAddress);
    sqlite3_finalize(_stmtAddExchangeRate);
    sqlite3_finalize(_stmtAddKYC);
    sqlite3_finalize(_stmtRevokeKYC);
    sqlite3_finalize(_stmtPruneUTXOs);
    sqlite3_finalize(_stmtExchangeRatesAtHeight);
    sqlite3_finalize(_stmtPruneExchangeRate);
    sqlite3_finalize(_stmtGetVoteCount);
    sqlite3_finalize(_stmtPruneVote);
    sqlite3_finalize(_stmtAddVote);
    sqlite3_finalize(_stmtGetAssetUTXO);
    sqlite3_finalize(_stmtGetAssetHolders);
    sqlite3_finalize(_stmtAddAsset);
    sqlite3_finalize(_stmtUpdateAsset);
    sqlite3_finalize(_stmtGetAssetIndex);
    sqlite3_finalize(_stmtGetAssetIndexOnUTXO);
    sqlite3_finalize(_stmtGetHeightAssetCreated);
    sqlite3_finalize(_stmtGetAssetRules);
    sqlite3_finalize(_stmtGetAsset);
    sqlite3_finalize(_stmtGetKYC);
    sqlite3_finalize(_stmtGetValidExchangeRate);
    sqlite3_finalize(_stmtGetCurrentExchangeRate);
    sqlite3_finalize(_stmtGetNextIPFSJob);
    sqlite3_finalize(_stmtSetIPFSPauseSync);
    sqlite3_finalize(_stmtClearNextIPFSJob_a);
    sqlite3_finalize(_stmtClearNextIPFSJob_b);
    sqlite3_finalize(_stmtInsertIPFSJob);
    sqlite3_finalize(_stmtClearIPFSPause);
    sqlite3_finalize(_stmtSetIPFSLockSync);
    sqlite3_finalize(_stmtSetIPFSLockJob);
    sqlite3_finalize(_stmtSetIPFSPauseJob);
    sqlite3_finalize(_stmtGetDomainAssetId);
    sqlite3_finalize(_stmtAddDomain);
    sqlite3_finalize(_stmtRevokeDomain);
    sqlite3_finalize(_stmtSetDomainMasterAssetId_a);
    sqlite3_finalize(_stmtSetDomainMasterAssetId_b);
    sqlite3_finalize(_stmtGetPermanentPaid);
    sqlite3_finalize(_stmtRemoveNonReachable);
    sqlite3_finalize(_stmtInsertPermanent);
    sqlite3_finalize(_stmtRepinAssets);
    sqlite3_finalize(_stmtRepinPermanentSpecific);
    sqlite3_finalize(_stmtAddAssetToPool);
    sqlite3_finalize(_stmtIsAssetInPool);
    sqlite3_finalize(_stmtPSPFindBadAsset);
    sqlite3_finalize(_stmtPSPDeleteBadAsset);
    sqlite3_finalize(_stmtDeletePermanent);
    sqlite3_finalize(_stmtIsInPermanent);
    sqlite3_finalize(_stmtGetTotalAssetCounta);
    sqlite3_finalize(_stmtGetTotalAssetCountb);
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
                      "DELETE FROM exchangeWatch;"
                      "INSERT INTO \"exchangeWatch\" VALUES (\"dgb1qunxh378eltj2jrwza5sj9grvu5xud43vqvudwh\");"
                      "INSERT INTO \"exchangeWatch\" VALUES (\"dgb1qlk3hldeynl3prqw259u8gv0jh7w5nwppxlvt3v\");"
                      "DELETE FROM kyc;"
                      "DELETE FROM utxos;"
                      "DELETE FROM votes;"
                      "DELETE FROM assets WHERE assetIndex>1;"
                      "DELETE FROM blocks WHERE height>1;"
                      "DELETE FROM ipfs;"
                      "DELETE FROM pspFiles;"
                      "DELETE FROM pspAssets;"
                      "DELETE FROM domains;"
                      "DELETE FROM domainsMaster;"
                      "INSERT INTO \"domainsMasters\" VALUES (\"Ua7Bd7UVtrzavSHhpHxHZ2nzS2hGaHXRMT9sqy\",true);";

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
            rc = executeSqliteStepWithRetry(_stmtGetAssetIndex);
            if (rc == SQLITE_ROW) { //if not found its new so leave assetIndex 0
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
            sqlite3_bind_int64(_stmtUpdateAsset, 5, assetIndex);
            rc = executeSqliteStepWithRetry(_stmtUpdateAsset);
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
    sqlite3_bind_int(_stmtAddAsset, 6, asset.getHeightUpdated()); //will be same as created
    sqlite3_bind_int64(_stmtAddAsset, 7, asset.getExpiry());
    rc = executeSqliteStepWithRetry(_stmtAddAsset);
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
    rc = executeSqliteStepWithRetry(_stmtGetAssetRules);
    if (rc != SQLITE_ROW) return DigiAssetRules();

    vector<uint8_t> serializedRules = Blob(sqlite3_column_blob(_stmtGetAssetRules, 0),
                                           sqlite3_column_bytes(_stmtGetAssetRules, 0))
                                              .vector();
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
    rc = executeSqliteStepWithRetry(_stmtGetHeightAssetCreated);
    if (rc == SQLITE_ROW) { //if not found its new so leave assetIndex 0
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
    rc = executeSqliteStepWithRetry(_stmtGetAsset);
    if (rc != SQLITE_ROW) {
        string tempErrorMessage = sqlite3_errmsg(_db);
        throw exceptionFailedSelect();
    }
    string assetId = reinterpret_cast<const char*>(sqlite3_column_text(_stmtGetAsset, 0));
    string cid = reinterpret_cast<const char*>(sqlite3_column_text(_stmtGetAsset, 1));
    string issuerAddress = reinterpret_cast<const char*>(sqlite3_column_text(_stmtGetAsset, 2));
    vector<uint8_t> serializedRules = Blob(sqlite3_column_blob(_stmtGetAsset, 3),
                                           sqlite3_column_bytes(_stmtGetAsset, 3))
                                              .vector();
    unsigned int heightCreated = sqlite3_column_int(_stmtGetAsset, 4);
    unsigned int heightUpdated = sqlite3_column_int(_stmtGetAsset, 5);

    //lookup kyc and rules
    KYC issuer = getAddressKYC(issuerAddress);
    DigiAssetRules rules;
    size_t i = 0;
    deserialize(serializedRules, i, rules);

    //create and return object
    return {assetIndex, assetId, cid, issuer, rules, heightCreated, heightUpdated, amount};
}

uint64_t Database::getAssetIndex(const string& assetId, const string& txid, unsigned int vout) const {
    //see if the asset exists and if only 1 index
    sqlite3_reset(_stmtGetAssetIndex);
    sqlite3_bind_text(_stmtGetAssetIndex, 1, assetId.c_str(), assetId.length(), SQLITE_STATIC);
    int rc = executeSqliteStepWithRetry(_stmtGetAssetIndex);
    if (rc != SQLITE_ROW) {
        throw out_of_range("assetIndex does not exist");
    }
    uint64_t assetIndex = sqlite3_column_int(_stmtGetAssetIndex, 0);

    //check if more than 1
    rc = executeSqliteStepWithRetry(_stmtGetAssetIndex);
    if (rc != SQLITE_ROW) return assetIndex; //there was only 1

    //more than 1 so see if the txid and vout provided match a utxo
    if (txid.empty()) throw out_of_range("specific utxo needed");
    vector<uint64_t> assetIndexPossibilities;
    sqlite3_reset(_stmtGetAssetIndexOnUTXO);
    sqlite3_bind_text(_stmtGetAssetIndexOnUTXO, 1, assetId.c_str(), assetId.length(), SQLITE_STATIC);
    sqlite3_bind_text(_stmtGetAssetIndexOnUTXO, 2, txid.c_str(), txid.length(), SQLITE_STATIC);
    sqlite3_bind_int(_stmtGetAssetIndexOnUTXO, 3, vout);
    while (executeSqliteStepWithRetry(_stmtGetAssetIndexOnUTXO) == SQLITE_ROW) {
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
    rc = executeSqliteStepWithRetry(_stmtCheckFlag);
    if (rc != SQLITE_ROW) { //there should always be one
        string tempErrorMessage = sqlite3_errmsg(_db);
        throw exceptionFailedSelect(); //failed to check database
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
    if (getFlagInt(flag) == state) return; //no need to do anything

    //store in database
    int rc;
    sqlite3_reset(_stmtSetFlag);
    sqlite3_bind_int(_stmtSetFlag, 1, state);
    sqlite3_bind_text(_stmtSetFlag, 2, flag.c_str(), flag.length(), nullptr);
    rc = executeSqliteStepWithRetry(_stmtSetFlag);
    if (rc != SQLITE_DONE) { //there should always be one
        string tempErrorMessage = sqlite3_errmsg(_db);
        throw exceptionFailedUpdate(); //failed to check database
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
void Database::insertBlock(uint height, const std::string& hash, unsigned int time, unsigned char algo, double difficulty) {
    int rc;
    sqlite3_reset(_stmtInsertBlock);
    sqlite3_bind_int(_stmtInsertBlock, 1, height);
    Blob hashBlob = Blob(hash);
    sqlite3_bind_blob(_stmtInsertBlock, 2, hashBlob.data(), SHA256_LENGTH,
                      SQLITE_STATIC); //could use hashBlob.length() but always SHA256_LENGTH
    sqlite3_bind_int(_stmtInsertBlock, 3, time);
    sqlite3_bind_int(_stmtInsertBlock, 4, algo);
    sqlite3_bind_double(_stmtInsertBlock, 5, difficulty);
    rc = executeSqliteStepWithRetry(_stmtInsertBlock);
    if (rc != SQLITE_DONE) { //there should always be one
        string tempErrorMessage = sqlite3_errmsg(_db);
        throw exceptionFailedInsert(); //failed to check database
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
    rc = executeSqliteStepWithRetry(_stmtGetBlockHash);
    if (rc != SQLITE_ROW) {
        throw exceptionDataPruned();
    }
    Blob hash = Blob(sqlite3_column_blob(_stmtGetBlockHash, 0),
                     SHA256_LENGTH); // could have used sqlite3_column_bytes(_stmtGetBlockHash,0);  but always 8
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
    rc = executeSqliteStepWithRetry(_stmtGetBlockHeight);
    if (rc != SQLITE_ROW) { //there should always be one
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
    if (value.address.empty()) return; //op return don't store
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
    rc = executeSqliteStepWithRetry(_stmtCreateUTXO);
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
        rc = executeSqliteStepWithRetry(_stmtCreateUTXO);
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
    rc = executeSqliteStepWithRetry(_stmtSpendUTXO);
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
    rc = executeSqliteStepWithRetry(_stmtGetSpendingAddress);
    if (rc != SQLITE_ROW) {
        //try checking wallet
        AppMain* main = AppMain::GetInstance();
        if ((main->isDigiByteCoreSet()) && (getBeenPrunedNonAssetUTXOHistory())) {
            getrawtransaction_t txData = main->getDigiByteCore()->getRawTransaction(txid);
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
    rc = executeSqliteStepWithRetry(_stmtPruneUTXOs);
    if (rc != SQLITE_DONE) {
        string tempErrorMessage = sqlite3_errmsg(_db);
        throw exceptionFailedDelete();
    }

    //mark blocks that can't be rolled back anymore
    sqlite3_reset(_stmtRemoveNonReachable);
    sqlite3_bind_int(_stmtRemoveNonReachable, 1, height);
    rc = executeSqliteStepWithRetry(_stmtRemoveNonReachable);
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
    //try to get data from database
    sqlite3_reset(_stmtGetAssetUTXO);
    Blob blobTXID = Blob(txid);
    sqlite3_bind_blob(_stmtGetAssetUTXO, 1, blobTXID.data(), SHA256_LENGTH, SQLITE_STATIC);
    sqlite3_bind_int(_stmtGetAssetUTXO, 2, vout);
    AssetUTXO result;
    result.txid = txid;
    result.vout = vout;
    bool exists = false;
    while (executeSqliteStepWithRetry(_stmtGetAssetUTXO) == SQLITE_ROW) {
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

    //check if we can/should get from the wallet
    AppMain* main = AppMain::GetInstance();
    if (!main->isDigiByteCoreSet() || (!getBeenPrunedNonAssetUTXOHistory())) throw exceptionDataPruned();

    //get tx data from wallet
    getrawtransaction_t txData = main->getDigiByteCore()->getRawTransaction(txid);
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
    while (executeSqliteStepWithRetry(_stmtGetAssetHolders) == SQLITE_ROW) {
        string address = reinterpret_cast<const char*>(sqlite3_column_text(_stmtGetAssetHolders, 0));
        uint64_t count = sqlite3_column_int64(_stmtGetAssetHolders, 1);
        result.emplace_back(AssetHolder{
                .address = address,
                .count = count});
    }
    return result;
}

/**
 * Returns the total number assets that exist of this specific type.
 * The difference between this and the other getTotalAssetCount function is that if the asset has sub types this will only give total of the sub type provided
 * @param assetIndex
 * @return
 */
uint64_t Database::getTotalAssetCount(uint64_t assetIndex) const {
    sqlite3_reset(_stmtGetTotalAssetCounta);
    sqlite3_bind_int64(_stmtGetTotalAssetCounta, 1, assetIndex);
    if (executeSqliteStepWithRetry(_stmtGetTotalAssetCounta) != SQLITE_ROW) throw exceptionFailedSelect();
    return sqlite3_column_int64(_stmtGetTotalAssetCounta, 0);
}

/**
 * Returns the total number assets that exist of this specific type.
 * The difference between this and the other getTotalAssetCount function is this if the asset has sub types this will provide the total of all sub types
 * @param assetIndex
 * @return
 */
uint64_t Database::getTotalAssetCount(const string& assetId) const {
    sqlite3_reset(_stmtGetTotalAssetCountb);
    sqlite3_bind_text(_stmtGetTotalAssetCountb, 1, assetId.c_str(), assetId.length(), nullptr);
    if (executeSqliteStepWithRetry(_stmtGetTotalAssetCountb) != SQLITE_ROW) throw exceptionFailedSelect();
    return sqlite3_column_int64(_stmtGetTotalAssetCountb, 0);
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
        rc = executeSqliteStepWithRetry(_stmtIsWatchAddress);
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
    rc = executeSqliteStepWithRetry(_stmtAddWatchAddress);
    if (rc != SQLITE_OK) {
        string tempErrorMessage = sqlite3_errmsg(_db);
        throw exceptionFailedInsert();
    }

    //add to watch buffer if using buffer
    if (_exchangeWatchAddresses.empty()) return;
    if (_exchangeWatchAddresses.size() == DIGIBYTECORE_DATABASE_CHAIN_WATCH_MAX) {
        _exchangeWatchAddresses.clear(); //got to big won't use buffer anymore
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
void Database::addExchangeRate(const string& address, unsigned int index, unsigned int height, double exchangeRate) {
    int rc;
    sqlite3_reset(_stmtAddExchangeRate);
    sqlite3_bind_text(_stmtAddExchangeRate, 1, address.c_str(), address.length(), SQLITE_STATIC);
    sqlite3_bind_int(_stmtAddExchangeRate, 2, index);
    sqlite3_bind_int(_stmtAddExchangeRate, 3, height);
    sqlite3_bind_double(_stmtAddExchangeRate, 4, exchangeRate);
    rc = executeSqliteStepWithRetry(_stmtAddExchangeRate);
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
    while (executeSqliteStepWithRetry(_stmtExchangeRatesAtHeight) == SQLITE_ROW) {
        firstEntry.push_back(entry{
                .height = (unsigned int) sqlite3_column_int(_stmtExchangeRatesAtHeight, 0),
                .address = reinterpret_cast<const char*>(sqlite3_column_text(_stmtExchangeRatesAtHeight, 1)),
                .index = (unsigned char) sqlite3_column_int(_stmtExchangeRatesAtHeight, 2),
                .value = sqlite3_column_double(_stmtExchangeRatesAtHeight, 3)});
    }

    //delete from exchange where pruneHeight<pruneHeight;
    sqlite3_reset(_stmtPruneExchangeRate);
    sqlite3_bind_int(_stmtPruneExchangeRate, 1, pruneHeight);
    int rc = executeSqliteStepWithRetry(_stmtPruneExchangeRate);
    if (rc != SQLITE_DONE) {
        string tempErrorMessage = sqlite3_errmsg(_db);
        throw exceptionFailedDelete();
    }

    //mark blocks that can't be rolled back anymore
    sqlite3_reset(_stmtRemoveNonReachable);
    sqlite3_bind_int(_stmtRemoveNonReachable, 1, pruneHeight);
    rc = executeSqliteStepWithRetry(_stmtRemoveNonReachable);
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
    while (executeSqliteStepWithRetry(_stmtGetValidExchangeRate) == SQLITE_ROW) {
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
    if (executeSqliteStepWithRetry(_stmtGetCurrentExchangeRate) != SQLITE_ROW) throw out_of_range("Unknown Exchange Rate");
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
    rc = executeSqliteStepWithRetry(_stmtAddKYC);
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
    rc = executeSqliteStepWithRetry(_stmtRevokeKYC);
    if (rc != SQLITE_DONE) {
        string tempErrorMessage = sqlite3_errmsg(_db);
        throw exceptionFailedUpdate();
    }
}

KYC Database::getAddressKYC(const string& address) const {
    int rc;
    sqlite3_reset(_stmtGetKYC);
    sqlite3_bind_text(_stmtGetKYC, 1, address.c_str(), address.length(), SQLITE_STATIC);
    rc = executeSqliteStepWithRetry(_stmtGetKYC);
    if (rc != SQLITE_ROW) {
        return {
                address};
    }
    Blob hash(sqlite3_column_blob(_stmtGetKYC, 2), sqlite3_column_bytes(_stmtGetKYC, 2));
    return {
            address,
            reinterpret_cast<const char*>(sqlite3_column_text(_stmtGetKYC, 0)),
            reinterpret_cast<const char*>(sqlite3_column_text(_stmtGetKYC, 1)),
            hash.toHex(),
            static_cast<unsigned int>(sqlite3_column_int(_stmtGetKYC, 3)),
            sqlite3_column_int(_stmtGetKYC, 4)};
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
    rc = executeSqliteStepWithRetry(_stmtAddVote);
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
    while (executeSqliteStepWithRetry(_stmtGetVoteCount) == SQLITE_ROW) {
        keep.emplace_back(entries{
                static_cast<unsigned int>(sqlite3_column_int(_stmtGetVoteCount, 0)),
                reinterpret_cast<const char*>(sqlite3_column_text(_stmtGetVoteCount, 1)),
                static_cast<unsigned int>(sqlite3_column_int(_stmtGetVoteCount, 2)),
                static_cast<unsigned int>(sqlite3_column_int(_stmtGetVoteCount, 3))});
    }

    //delete from votes where height<pruneHeight
    sqlite3_reset(_stmtPruneVote);
    sqlite3_bind_int(_stmtPruneVote, 1, height);
    rc = executeSqliteStepWithRetry(_stmtPruneVote);
    if (rc != SQLITE_DONE) {
        string tempErrorMessage = sqlite3_errmsg(_db);
        throw exceptionFailedDelete();
    }

    //mark blocks that can't be rolled back anymore
    sqlite3_reset(_stmtRemoveNonReachable);
    sqlite3_bind_int(_stmtRemoveNonReachable, 1, height);
    rc = executeSqliteStepWithRetry(_stmtRemoveNonReachable);
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
 * Returns the number of pending jobs to be executed on IPFS network
 * @return
 */
unsigned int Database::getIPFSJobCount() {
    sqlite3_reset(_stmtNumberOfIPFSJobs);
    int rc = executeSqliteStepWithRetry(_stmtNumberOfIPFSJobs);
    if (rc != SQLITE_ROW) {
        throw exceptionFailedSelect();
    }
    return sqlite3_column_int(_stmtNumberOfIPFSJobs, 0);
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
                                       std::chrono::system_clock::now().time_since_epoch())
                                       .count();

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
            int rc = executeSqliteStepWithRetry(_stmtClearIPFSPause);
            if (rc != SQLITE_DONE) throw exceptionFailedUpdate();
        }
    }

    //lookup the next job(if there is one)
    sqlite3_reset(_stmtGetNextIPFSJob);
    int rc = executeSqliteStepWithRetry(_stmtGetNextIPFSJob);
    if (rc != SQLITE_ROW) {
        jobIndex = 0; //signal there are no new jobs
        return;
    }
    jobIndex = sqlite3_column_int(_stmtGetNextIPFSJob, 0);
    sync = reinterpret_cast<const char*>(sqlite3_column_text(_stmtGetNextIPFSJob, 1));
    cid = reinterpret_cast<const char*>(sqlite3_column_text(_stmtGetNextIPFSJob, 2));
    extra = reinterpret_cast<const char*>(sqlite3_column_text(_stmtGetNextIPFSJob, 3));
    string callbackSymbol = reinterpret_cast<const char*>(sqlite3_column_text(_stmtGetNextIPFSJob, 4));

    //get max time
    uint64_t currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                                   std::chrono::system_clock::now().time_since_epoch())
                                   .count();
    uint64_t maxTime = sqlite3_column_int64(_stmtGetNextIPFSJob, 5);
    if (maxTime == 0) {
        //if null return forever
        maxSleep = std::numeric_limits<unsigned int>::max();
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
    callback = _ipfsCallbacks[callbackSymbol]; //get callback(note there is a default callback if empty)


    //lock the sync if not pin or non synchronized
    if ((sync == "pin") || (sync == "_") || (sync.empty())) {
        sqlite3_reset(_stmtSetIPFSLockJob);
        sqlite3_bind_int(_stmtSetIPFSLockJob, 1, jobIndex);
        rc = executeSqliteStepWithRetry(_stmtSetIPFSLockJob);
    } else {
        sqlite3_reset(_stmtSetIPFSLockSync);
        sqlite3_bind_text(_stmtSetIPFSLockSync, 1, sync.c_str(), sync.length(), SQLITE_STATIC);
        rc = executeSqliteStepWithRetry(_stmtSetIPFSLockSync);
    }
    if (rc != SQLITE_DONE) throw exceptionFailedUpdate();
}

void Database::pauseIPFSSync(unsigned int jobIndex, const string& sync, unsigned int pauseLengthInMilliSeconds) {
    //get current time
    uint64_t currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                                   std::chrono::system_clock::now().time_since_epoch())
                                   .count();
    uint64_t unpauseTime = currentTime + pauseLengthInMilliSeconds;

    //update database
    int rc;
    if ((sync == "pin") || (sync == "_") || (sync.empty())) { //handle jobs that are non ordered
        sqlite3_reset(_stmtSetIPFSPauseJob);
        sqlite3_bind_int64(_stmtSetIPFSPauseJob, 1, unpauseTime);
        sqlite3_bind_int(_stmtSetIPFSPauseJob, 2, jobIndex);
        rc = executeSqliteStepWithRetry(_stmtSetIPFSPauseJob);
    } else {
        sqlite3_reset(_stmtSetIPFSPauseSync);
        sqlite3_bind_int64(_stmtSetIPFSPauseSync, 1, unpauseTime);
        sqlite3_bind_text(_stmtSetIPFSPauseSync, 2, sync.c_str(), sync.length(), SQLITE_STATIC);
        rc = executeSqliteStepWithRetry(_stmtSetIPFSPauseSync);

        //update ram
        if (rc == SQLITE_DONE) _ipfsCurrentlyPaused.emplace_back(sync, unpauseTime);
    }
    if (rc != SQLITE_DONE) { //there should always be one
        string tempErrorMessage = sqlite3_errmsg(_db);
        throw exceptionFailedDelete(); //failed to check database
    }
}

/**
 * This function is only meant to be called by IPFS thread and assumes that the job is not paused since it was just executed
 * @param jobIndex
 */
void Database::removeIPFSJob(unsigned int jobIndex, const string& sync) {
    //make sure only 1 thread can call at a time
    std::lock_guard<std::mutex> lock(_mutexRemoveIPFSJob);

    //remove from database
    sqlite3_reset(_stmtClearNextIPFSJob_a);
    sqlite3_bind_int(_stmtClearNextIPFSJob_a, 1, jobIndex);
    int rc = executeSqliteStepWithRetry(_stmtClearNextIPFSJob_a);
    if (rc != SQLITE_DONE) { //there should always be one
        string tempErrorMessage = sqlite3_errmsg(_db);
        throw exceptionFailedSQLCommand(); //failed to delete or unlock
    }
    sqlite3_reset(_stmtClearNextIPFSJob_b);
    sqlite3_bind_text(_stmtClearNextIPFSJob_b, 1, sync.c_str(), sync.length(), SQLITE_STATIC);
    rc = executeSqliteStepWithRetry(_stmtClearNextIPFSJob_b);
    if (rc != SQLITE_DONE) { //there should always be one
        string tempErrorMessage = sqlite3_errmsg(_db);
        throw exceptionFailedSQLCommand(); //failed to delete or unlock
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
 *                  "pin", "unpin", "_", or "" can be done in any order any other value must be done in sequential order by jobIndex for matching sync values
 *                  "pin" means the job is to be pinned not downloaded
 *                  "unpin" means the job is to unpinned not downloaded
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
                                       std::chrono::system_clock::now().time_since_epoch())
                                       .count();
        sqlite3_bind_int64(_stmtInsertIPFSJob, 6, currentTime + maxSleep);
    }

    int rc = executeSqliteStepWithRetry(_stmtInsertIPFSJob);
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
    int rc = executeSqliteStepWithRetry(_stmtRevokeDomain);
    if (rc != SQLITE_DONE) {
        string tempErrorMessage = sqlite3_errmsg(_db);
        throw exceptionFailedUpdate();
    }
}

void Database::addDomain(const string& domain, const string& assetId) {
    sqlite3_reset(_stmtAddDomain);
    sqlite3_bind_text(_stmtAddDomain, 1, domain.c_str(), domain.length(), SQLITE_STATIC);
    sqlite3_bind_text(_stmtAddDomain, 2, assetId.c_str(), assetId.length(), SQLITE_STATIC);
    int rc = executeSqliteStepWithRetry(_stmtAddDomain);
    if (rc != SQLITE_DONE) {
        string tempErrorMessage = sqlite3_errmsg(_db);
        throw exceptionFailedUpdate();
    }
}

string Database::getDomainAssetId(const std::string& domain, bool returnErrorIfRevoked) const {
    sqlite3_reset(_stmtGetDomainAssetId);
    sqlite3_bind_text(_stmtGetDomainAssetId, 1, domain.c_str(), domain.length(), SQLITE_STATIC);
    int rc = executeSqliteStepWithRetry(_stmtGetDomainAssetId);

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
    int rc = executeSqliteStepWithRetry(_stmtSetDomainMasterAssetId_a);
    if (rc != SQLITE_DONE) {
        string tempErrorMessage = sqlite3_errmsg(_db);
        throw exceptionFailedUpdate();
    }
    sqlite3_reset(_stmtSetDomainMasterAssetId_b);
    sqlite3_bind_text(_stmtSetDomainMasterAssetId_b, 1, assetId.c_str(), assetId.length(), SQLITE_STATIC);
    rc = executeSqliteStepWithRetry(_stmtSetDomainMasterAssetId_b);
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

/**
* Repins all files we should be pinning
*/
void Database::repinPermanent(unsigned int poolIndex) {
    //repin all asset main meta data
    sqlite3_reset(_stmtRepinAssets);
    int rc = executeSqliteStepWithRetry(_stmtRepinAssets);
    if (rc != SQLITE_DONE) {
        string tempErrorMessage = sqlite3_errmsg(_db);
        throw exceptionFailedInsert();
    }

    //repin all files that are part of specific pool
    sqlite3_reset(_stmtRepinPermanentSpecific);
    sqlite3_bind_int(_stmtRepinPermanentSpecific, 1, poolIndex);
    rc = executeSqliteStepWithRetry(_stmtRepinPermanentSpecific);
    if (rc != SQLITE_DONE) {
        string tempErrorMessage = sqlite3_errmsg(_db);
        throw exceptionFailedInsert();
    }
}

/**
* Unpins all files that are part of a specific psp but not part of psp we are still subscribed to
* @param poolIndex
*/
void Database::unpinPermanent(unsigned int poolIndex) {
    PermanentStoragePoolList* pools = AppMain::GetInstance()->getPermanentStoragePoolList();
    bool subscribedToAny = false;
    for (const auto& pool: *pools) {
        if (pool->subscribed()) {
            subscribedToAny = true;
            break;
        }
    }

    string sql = "INSERT INTO ipfs (sync, lock, cid, extra, callback, pause, maxTime) SELECT 'unpin', 0, cid, '', '', NULL, NULL FROM pspFiles WHERE \"poolIndex\" = ?";
    if (subscribedToAny) {
        sql += " AND \"cid\" NOT IN (SELECT \"cid\" FROM pspFiles WHERE ";
        for (const auto& pool: *pools) {
            if (pool->subscribed()) {
                sql += "\"poolIndex\"=? OR ";
            }
        }
        sql = sql.substr(0, sql.length() - 4); //remove extra " OR "
        sql += ")";
    }
    sql += ";";


    int rc = sqlite3_exec(_db, sql.c_str(), Database::defaultCallback, 0, nullptr);
    if (rc != SQLITE_DONE) {
        string tempErrorMessage = sqlite3_errmsg(_db);
        throw exceptionFailedInsert();
    }
}

/**
* Adds a cid to permanent list
* @param cid
*/
void Database::addToPermanent(unsigned int poolIndex, const string& cid) {
    if (cid.empty()) return;
    sqlite3_reset(_stmtInsertPermanent);
    sqlite3_bind_text(_stmtInsertPermanent, 1, cid.c_str(), cid.length(), SQLITE_STATIC);
    sqlite3_bind_int(_stmtInsertPermanent, 2, poolIndex);
    int rc = executeSqliteStepWithRetry(_stmtInsertPermanent);
    if (rc != SQLITE_DONE) {
        string tempErrorMessage = sqlite3_errmsg(_db);
        throw exceptionFailedInsert();
    }
}

void Database::removeFromPermanent(unsigned int poolIndex, const std::string& cid, bool unpin) {
    //remove from database
    sqlite3_reset(_stmtDeletePermanent);
    sqlite3_bind_text(_stmtDeletePermanent, 1, cid.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(_stmtDeletePermanent, 2, poolIndex);

    //unpin if not in database anymore
    if (!unpin) return;

    //see if asset still present
    sqlite3_reset(_stmtIsInPermanent);
    sqlite3_bind_text(_stmtIsInPermanent, 1, cid.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(_stmtIsAssetInAPool) == SQLITE_ROW) return;

    //add asset to be unpinned
    addIPFSJob(cid, "unpin");
}

void Database::addAssetToPool(unsigned int poolIndex, unsigned int assetIndex) {
    sqlite3_reset(_stmtAddAssetToPool);
    sqlite3_bind_int(_stmtAddAssetToPool, 1, assetIndex);
    sqlite3_bind_int(_stmtAddAssetToPool, 2, poolIndex);

    if (sqlite3_step(_stmtAddAssetToPool) != SQLITE_DONE) {
        throw exceptionFailedInsert();
    }
}

void Database::removeAssetFromPool(unsigned int poolIndex, const std::string& assetId, bool unpin) {
    //make list of assetIndex we will remove and there cid
    vector<pair<unsigned int, string>> toDelete;
    sqlite3_reset(_stmtPSPFindBadAsset);
    sqlite3_bind_text(_stmtPSPFindBadAsset, 1, assetId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(_stmtPSPFindBadAsset, 2, poolIndex);
    while (sqlite3_step(_stmtPSPFindBadAsset) == SQLITE_ROW) {
        unsigned int assetIndex = sqlite3_column_int(_stmtPSPFindBadAsset, 0);
        string cid = reinterpret_cast<const char*>(sqlite3_column_text(_stmtPSPFindBadAsset, 1));
        toDelete.emplace_back(make_pair(assetIndex, cid));
    }

    //remove from database
    sqlite3_reset(_stmtPSPDeleteBadAsset);
    sqlite3_bind_text(_stmtPSPDeleteBadAsset, 1, assetId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(_stmtPSPDeleteBadAsset, 2, poolIndex);

    //unpin if not in database anymore
    if (!unpin) return;
    for (auto pair: toDelete) {
        //see if asset still present
        unsigned int assetIndex = pair.first;
        sqlite3_reset(_stmtIsAssetInAPool);
        sqlite3_bind_int(_stmtIsAssetInAPool, 1, assetIndex);
        if (sqlite3_step(_stmtIsAssetInAPool) == SQLITE_ROW) continue;

        //add asset to be unpinned
        addIPFSJob(pair.second, "unpin");
    }
}

bool Database::isAssetInPool(unsigned int poolIndex, unsigned int assetIndex) {
    sqlite3_reset(_stmtIsAssetInPool);
    sqlite3_bind_int(_stmtIsAssetInPool, 1, assetIndex);
    sqlite3_bind_int(_stmtIsAssetInPool, 2, poolIndex);

    if (sqlite3_step(_stmtIsAssetInPool) == SQLITE_ROW) {
        return sqlite3_column_int(_stmtIsAssetInPool, 0) > 0;
    } else {
        throw exceptionFailedSelect();
    }
}

/*
███████╗████████╗ █████╗ ████████╗███████╗
██╔════╝╚══██╔══╝██╔══██╗╚══██╔══╝██╔════╝
███████╗   ██║   ███████║   ██║   ███████╗
╚════██║   ██║   ██╔══██║   ██║   ╚════██║
███████║   ██║   ██║  ██║   ██║   ███████║
╚══════╝   ╚═╝   ╚═╝  ╚═╝   ╚═╝   ╚══════╝
*/
bool Database::canGetAlgoStats() {
    try {
        getBlockHash(1);
        return true;
    } catch (const exceptionDataPruned& e) {
        return false;
    }
}
bool Database::canGetAddressStats() {
    if (!canGetAlgoStats()) return false;
    return !(
            (getBeenPrunedUTXOHistory() > -1) ||
            (getBeenPrunedNonAssetUTXOHistory()));
}
void Database::updateStats(unsigned int timeFrame) {
    //return if stats are not possible do to pruning being on(no stats are possible if algo stats are not possible)
    if (!canGetAlgoStats()) return;

    int rc;

    //prep key variables
    string sql;
    string timeStr = to_string(timeFrame);
    sqlite3_stmt* stmt;

    //find what height we need to start update from
    unsigned int startTime = 0;
    unsigned int beginHeight = 1;
    sql = "SELECT end_time,begin_height FROM StatsCutOffHeights_" + timeStr + " ORDER BY end_time DESC LIMIT 1;";
    rc = sqlite3_prepare_v2(_db, sql.c_str(), -1, &stmt, 0);
    if (rc == SQLITE_OK) {
        //table exists so get the highest value
        rc = executeSqliteStepWithRetry(stmt);
        if (rc != SQLITE_ROW) throw exceptionFailedSelect();
        startTime = sqlite3_column_int(stmt, 0);
        beginHeight = sqlite3_column_int(stmt, 1);
    }
    sqlite3_finalize(stmt);

    //create fresh new cutoff table
    if (startTime > 0) {
        executeSQLStatement(
                "DELETE FROM StatsCutOffHeights_" + timeStr,
                exceptionFailedDelete());
    } else {
        executeSQLStatement(
                "CREATE TABLE StatsCutOffHeights_" + timeStr + " (end_time INTEGER NOT NULL,begin_height INTEGER NOT NULL,end_height INTEGER NOT NULL,PRIMARY KEY(end_time));",
                exceptionFailedToCreateTable());
    }

    //populate cutoff table
    // clang-format off
executeSQLStatement(
"INSERT INTO StatsCutOffHeights_" + timeStr + " (end_time, begin_height, end_height) "
"SELECT "
    "end_time, "
    "MIN(begin_height) AS begin_height, "
    "MAX(end_height) AS end_height "
"FROM ("
    "SELECT "
        "CEIL(time / " + timeStr + ".0) * " + timeStr + " AS end_time, "
        "LEAD(height, 1, height) OVER (ORDER BY time ASC) - 2 AS end_height, "
        "LAG(height, 1, 1) OVER (ORDER BY time ASC) AS begin_height "
    "FROM blocks "
    "WHERE height >= " + to_string(beginHeight) + " "
") AS subquery "
"GROUP BY end_time "
"HAVING MIN(begin_height) >= " + to_string(beginHeight) + " "
"ORDER BY end_time ASC;",
exceptionFailedInsert()
);
    // clang-format on

    //create Stats Algo Table
    if (startTime == 0) {
        executeSQLStatement(
                "CREATE TABLE StatsAlgo_" + timeStr + " (end_time INTEGER NOT NULL,algo INTEGER NOT NULL,min_difficulty REAL NOT NULL, max_difficulty REAL NOT NULL, avg_difficulty REAL NOT NULL, num_blocks INTEGER NOT NULL, PRIMARY KEY(end_time, algo));",
                exceptionFailedToCreateTable());
    } else {
        //table exists delete anything that may have been partial info
        executeSQLStatement(
                "DELETE FROM StatsAlgo_" + timeStr + " WHERE end_time >= " + to_string(startTime),
                exceptionFailedToCreateTable());
    }

    //populate stats algo table
    // clang-format off
executeSQLStatement(
"INSERT INTO StatsAlgo_" + timeStr + " (end_time, algo, min_difficulty, max_difficulty, avg_difficulty, num_blocks) "
"SELECT "
    "tch.end_time, "
    "b.algo, "
    "MIN(b.difficulty) AS min_difficulty, "
    "MAX(b.difficulty) AS max_difficulty, "
    "AVG(b.difficulty) AS avg_difficulty, "
    "COUNT(*) AS num_blocks "
"FROM blocks b "
"JOIN StatsCutOffHeights_" + timeStr + " tch ON b.height >= tch.begin_height AND b.height <= tch.end_height "
"GROUP BY tch.end_time, b.algo "
"HAVING COUNT(*) >0 "
"ORDER BY tch.end_time ASC, b.algo ASC;",
exceptionFailedInsert()
);
    // clang-format on

    //update if can do address stats
    if (canGetAddressStats()) {

        //create address stats
        if (startTime == 0) {
            executeSQLStatement(
                    "CREATE TABLE StatsAddress_" + timeStr + " ( end_time INTEGER NOT NULL, total INTEGER, Count_Over_0 INTEGER, Count_Over_1 INTEGER, Count_Over_1k INTEGER, Count_Over_1m INTEGER, num_addresses_with_assets INTEGER, num_addresses_created INTEGER, num_addresses_used INTEGER, num_quantum_unsafe INTEGER, PRIMARY KEY(end_time));",
                    exceptionFailedToCreateTable());
        } else {
            //table exists delete anything that may have been partial info
            executeSQLStatement(
                    "DELETE FROM StatsAddress_" + timeStr + " WHERE end_time >= " + to_string(startTime),
                    exceptionFailedToCreateTable());
        }

        //populate stats address table with funded counts
        // clang-format off
executeSQLStatement(
    "INSERT INTO StatsAddress_" + timeStr + " (end_time, Count_Over_0, Count_Over_1, Count_Over_1k, Count_Over_1m) "
        "SELECT "
            "sub.end_time, "
            "COUNT(CASE WHEN sub.total_amount > 0 THEN 1 ELSE NULL END) AS Count_Over_0, "
            "COUNT(CASE WHEN sub.total_amount > 100000000 THEN 1 ELSE NULL END) AS Count_Over_1, "
            "COUNT(CASE WHEN sub.total_amount > 100000000000 THEN 1 ELSE NULL END) AS Count_Over_1k, "
            "COUNT(CASE WHEN sub.total_amount > 100000000000000 THEN 1 ELSE NULL END) AS Count_Over_1m "
        "FROM ("
            "SELECT "
                "tch.end_time, "
                "u.address, "
                "SUM(u.amount) AS total_amount "
            "FROM utxos u "
            "JOIN StatsCutOffHeights_" + timeStr + " tch ON u.heightCreated <= tch.end_height AND (u.heightDestroyed IS NULL OR u.heightDestroyed > tch.end_height) "
            "WHERE u.assetIndex = 1 "
            "GROUP BY tch.end_time, u.address"
        ") AS sub "
        "GROUP BY sub.end_time;",
    exceptionFailedInsert()
);
        // clang-format on

        //populate stats address table with asset counts
        // clang-format off
executeSQLStatement(
    "UPDATE StatsAddress_" + timeStr + " "
    "SET num_addresses_with_assets = COALESCE(sub.num_addresses_with_assets, 0) "
    "FROM ( "
        "SELECT "
            "tch.end_time, "
            "COUNT(DISTINCT u.address) AS num_addresses_with_assets "
        "FROM utxos u "
        "JOIN StatsCutOffHeights_" + timeStr + " tch ON u.heightCreated <= tch.end_height AND (u.heightDestroyed IS NULL OR u.heightDestroyed > tch.end_height) "
        "WHERE u.assetIndex > 1 "
        "GROUP BY tch.end_time "
    ") AS sub "
    "WHERE StatsAddress_" + timeStr + ".end_time = sub.end_time;",
    exceptionFailedUpdate()
);
        // clang-format on

        //populate StatsAddress table with address created counts
        // clang-format off
executeSQLStatement(
    "UPDATE StatsAddress_" + timeStr + " "
    "SET num_addresses_created = COALESCE(sub.num_addresses_created, 0) "
    "FROM ( "
        "SELECT "
            "tch.end_time, "
            "COUNT(DISTINCT u.address) AS num_addresses_created "
        "FROM StatsCutOffHeights_" + timeStr + " tch "
        "LEFT JOIN ( "
            "SELECT "
                "address, "
                "MIN(heightCreated) AS first_utxo_created "
            "FROM utxos "
            "GROUP BY address "
        ") AS u ON u.first_utxo_created >= tch.begin_height AND u.first_utxo_created <= tch.end_height "
        "GROUP BY tch.end_time "
    ") AS sub "
    "WHERE StatsAddress_" + timeStr + ".end_time = sub.end_time;",
    exceptionFailedUpdate()
);
        // clang-format on

        //populate StatsAddress table with number of addresses used
        // clang-format off
executeSQLStatement(
    "UPDATE StatsAddress_" + timeStr + " "
    "SET num_addresses_used = COALESCE(sub.num_addresses_used, 0) "
    "FROM ( "
        "SELECT "
            "tch.end_time, "
            "COUNT(DISTINCT u.address) AS num_addresses_used "
        "FROM StatsCutOffHeights_" + timeStr + " tch "
        "LEFT JOIN utxos u ON u.heightDestroyed >= tch.begin_height AND u.heightDestroyed <= tch.end_height "
        "GROUP BY tch.end_time "
    ") AS sub "
    "WHERE StatsAddress_" + timeStr + ".end_time = sub.end_time;",
    exceptionFailedUpdate()
);
        // clang-format on

        //populate StatsAddress table with number of addresses used
        // clang-format off
executeSQLStatement(
    "UPDATE StatsAddress_" + timeStr + " "
    "SET total = COALESCE(sub.total_addresses, 0) "
    "FROM ( "
        "SELECT "
            "tch.end_time, "
            "COUNT(DISTINCT u.address) AS total_addresses "
        "FROM StatsCutOffHeights_" + timeStr + " tch "
        "LEFT JOIN utxos u ON u.heightCreated <= tch.end_height "
        "GROUP BY tch.end_time "
    ") AS sub "
    "WHERE StatsAddress_" + timeStr + ".end_time = sub.end_time;",
    exceptionFailedUpdate()
);
        // clang-format on

        //populate StatsAddress table with number of quantum unsafe addresses
        // clang-format off
executeSQLStatement(
    "UPDATE StatsAddress_" + timeStr + " "
    "SET num_quantum_unsafe = COALESCE(sub.num_quantum_unsafe, 0) "
    "FROM ( "
        "SELECT "
            "tch.end_time, "
            "COUNT(DISTINCT u.address) AS num_quantum_unsafe "
        "FROM StatsCutOffHeights_" + timeStr + " tch "
        "LEFT JOIN ("
            "SELECT u.address, tch_inner.end_height "
            "FROM utxos u "
            "JOIN StatsCutOffHeights_" + timeStr + " tch_inner ON TRUE "
            "GROUP BY u.address, tch_inner.end_height "
            "HAVING SUM(CASE WHEN heightDestroyed <= tch_inner.end_height THEN 1 ELSE 0 END) > 0 "
            "AND SUM(CASE WHEN heightCreated <= tch_inner.end_height AND (heightDestroyed IS NULL OR heightDestroyed > tch_inner.end_height) THEN 1 ELSE 0 END) > 0 "
        ") AS u ON tch.end_height = u.end_height "
        "GROUP BY tch.end_time "
    ") AS sub "
    "WHERE StatsAddress_" + timeStr + ".end_time = sub.end_time;",
    exceptionFailedUpdate()
);
        // clang-format on
    }
}

std::vector<AlgoStats> Database::getAlgoStats(unsigned int start, unsigned int end, unsigned int timeFrame) {
    //make sure stats are upto date
    if (!canGetAlgoStats()) throw exceptionDataPruned();
    updateStats(timeFrame);

    //construct statement to get needed data
    std::vector<AlgoStats> algoStatsList;
    sqlite3_stmt* stmt;
    std::string sql = "SELECT * FROM StatsAlgo_" + std::to_string(timeFrame) + " WHERE end_time >= ? AND end_time <= ? ORDER BY end_time ASC, algo ASC";
    if (sqlite3_prepare_v2(_db, sql.c_str(), -1, &stmt, 0) != SQLITE_OK) {
        throw exceptionCreatingStatement();
    }
    sqlite3_bind_int(stmt, 1, start);
    sqlite3_bind_int64(stmt, 2, end);

    //get all needed data
    while (executeSqliteStepWithRetry(stmt) == SQLITE_ROW) {
        AlgoStats algoStats;
        algoStats.time = sqlite3_column_int(stmt, 0);
        algoStats.algo = sqlite3_column_int(stmt, 1);
        algoStats.blocks = sqlite3_column_int(stmt, 5);
        algoStats.difficultyMin = sqlite3_column_double(stmt, 2);
        algoStats.difficultyMax = sqlite3_column_double(stmt, 3);
        algoStats.difficultyAvg = sqlite3_column_double(stmt, 4);
        algoStatsList.push_back(algoStats);
    }

    //finalize and return
    sqlite3_finalize(stmt);
    return algoStatsList;
}

std::vector<AddressStats> Database::getAddressStats(unsigned int start, unsigned int end, unsigned int timeFrame) {
    //make sure stats are upto date
    if (!canGetAddressStats()) throw exceptionDataPruned();
    updateStats(timeFrame);

    //construct statement to get needed data
    std::vector<AddressStats> addressStatsList;
    sqlite3_stmt* stmt;
    std::string sql = "SELECT * FROM StatsAddress_" + std::to_string(timeFrame) + " WHERE end_time >= ? AND end_time <= ? ORDER BY end_time ASC";
    if (sqlite3_prepare_v2(_db, sql.c_str(), -1, &stmt, 0) != SQLITE_OK) {
        throw exceptionCreatingStatement();
    }
    sqlite3_bind_int(stmt, 1, start);
    sqlite3_bind_int64(stmt, 2, end);

    //get all needed data
    while (executeSqliteStepWithRetry(stmt) == SQLITE_ROW) {
        AddressStats addressStats;
        addressStats.time = sqlite3_column_int(stmt, 0);
        addressStats.created = sqlite3_column_int(stmt, 7);
        addressStats.used = sqlite3_column_int(stmt, 8);
        addressStats.withAssets = sqlite3_column_int(stmt, 6);
        addressStats.over0 = sqlite3_column_int(stmt, 2);
        addressStats.over1 = sqlite3_column_int(stmt, 3);
        addressStats.over1k = sqlite3_column_int(stmt, 4);
        addressStats.over1m = sqlite3_column_int(stmt, 5);
        addressStats.quantumInsecure = sqlite3_column_int(stmt, 9);
        addressStats.total = sqlite3_column_int(stmt, 1);
        addressStatsList.push_back(addressStats);
    }

    //finalize and return
    sqlite3_finalize(stmt);
    return addressStatsList;
}


/**
* Helper function to allow retying a command if database busy
* @param stmt
* @param maxRetries
* @param sleepDurationMs
* @return
*/
int Database::executeSqliteStepWithRetry(sqlite3_stmt* stmt, int maxRetries, int sleepDurationMs) {
    int rc;
    for (int i = 0; i < maxRetries; ++i) {
        rc = sqlite3_step(stmt);
        if (rc == SQLITE_DONE || rc == SQLITE_OK || rc == SQLITE_ROW) {
            return rc; // Successfully executed
        } else if (rc == SQLITE_LOCKED || rc == SQLITE_BUSY) {
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepDurationMs)); // Sleep before retrying
        } else {
            return rc; // Some other error occurred, return the error code
        }
    }
    return rc; // Return the last error code if maxRetries reached
}

void Database::executeSQLStatement(const string& query, const std::exception& errorToThrowOnFail) {
    int rc;
    sqlite3_stmt* stmt;
    rc = sqlite3_prepare_v2(_db, query.c_str(), -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        string tempErrorMessage = sqlite3_errmsg(_db);
        sqlite3_finalize(stmt);
        throw exceptionCreatingStatement();
    }
    rc = executeSqliteStepWithRetry(stmt);
    if (rc != SQLITE_DONE) {
        string tempErrorMessage = sqlite3_errmsg(_db);
        sqlite3_finalize(stmt);
        throw errorToThrowOnFail;
    }
    sqlite3_finalize(stmt);
}
