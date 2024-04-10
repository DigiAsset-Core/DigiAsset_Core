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
#include <algorithm>
#include <mutex>
#include <sqlite3.h>
#include <stdio.h>

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
                char* zErrMsg = nullptr;
                int rc;
                const char* sql =
                        //chain data tables
                        "BEGIN TRANSACTION;"

                        "CREATE TABLE \"assets\" (\"assetIndex\" INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, \"assetId\" TEXT NOT NULL, \"cid\" TEXT, \"issueAddress\" TEXT NOT NULL, \"rules\" BLOB, \"heightCreated\" INTEGER NOT NULL, \"heightUpdated\" INTEGER NOT NULL, \"expires\" INTEGER);"
                        "INSERT INTO \"assets\" VALUES (1,'DigiByte','QmfSVLAntanDUKrEHUnTXRh53GLUBHFfxk5x6LH4zz9PM4','STANDARD','',1,1,NULL);"

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
                        "INSERT INTO \"flags\" VALUES (\"dbVersion\",5);"

                        "CREATE TABLE \"kyc\" (\"address\" TEXT NOT NULL, \"country\" TEXT NOT NULL, \"name\" TEXT NOT NULL, \"hash\" BLOB NOT NULL, \"height\" INTEGER NOT NULL, \"revoked\" INTEGER, PRIMARY KEY(\"address\"));"

                        "CREATE TABLE \"utxos\" (\"address\" TEXT NOT NULL, \"txid\" BLOB NOT NULL, \"vout\" INTEGER NOT NULL, \"aout\" INTEGER, \"assetIndex\" Integer NOT NULL, \"amount\" INTEGER NOT NULL, \"heightCreated\" INTEGER NOT NULL, \"heightDestroyed\" INTEGER, issuance INTEGER, spentTXID BLOB DEFAULT null, PRIMARY KEY(\"address\",\"txid\",\"vout\",\"aout\"));"
                        "CREATE INDEX idx_utxos_txid_vout ON utxos(txid, vout);"
                        "CREATE INDEX idx_utxos_txid_vout_aout ON utxos(txid, vout, aout);"

                        "CREATE TABLE \"votes\" (\"assetIndex\" Integer NOT NULL, \"address\" TEXT NOT NULL, \"height\" INTEGER NOT NULL, \"count\" INTEGER NOT NULL, PRIMARY KEY(\"assetIndex\",\"address\",\"height\"));"

                        //IPFS job tables
                        "CREATE TABLE \"ipfs\" (\"jobIndex\" INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, \"sync\" TEXT NOT NULL, \"lock\" BOOL NOT NULL, \"cid\" TEXT NOT NULL, \"extra\" TEXT, \"callback\" TEXT NOT NULL, \"pause\" INTEGER, \"maxTime\" INTEGER);"
                        "INSERT INTO \"ipfs\" VALUES (1,'pin',false,'QmfSVLAntanDUKrEHUnTXRh53GLUBHFfxk5x6LH4zz9PM4','','',NULL,NULL);" //DigiByte Native Coin data
                        "INSERT INTO \"ipfs\" VALUES (2,'pin',false,'QmSAcz2H7veyeuuSyACLkSj9ts9EWm1c9v7uTqbHynsVbj','','',NULL,NULL);" //DigiByte Logo

                        //PSP tables
                        "CREATE TABLE \"pspFiles\" (\"cid\" TEXT NOT NULL,\"poolIndex\");"
                        "CREATE TABLE \"pspAssets\" (\"assetIndex\" INT NOT NUll,\"poolIndex\");"

                        //DigiByte Domain tables
                        "CREATE TABLE \"domains\" (\"domain\" TEXT NOT NULL, \"assetId\" TEXT NOT NULL, \"revoked\" BOOL NOT NULL);"
                        "CREATE TABLE \"domainsMasters\" (\"assetId\" TEXT NOT NULL, \"active\" BOOL NOT NULL);"
                        "INSERT INTO \"domainsMasters\" VALUES (\"Ua7Bd7UVtrzavSHhpHxHZ2nzS2hGaHXRMT9sqy\",true);"

                        "COMMIT;";
                rc = sqlite3_exec(_db, sql, Database::defaultCallback, nullptr, &zErrMsg);
                skipUpToVersion = 5; //tell not to execute steps until version 5 to 6 transition
                if (rc != SQLITE_OK) {
                    sqlite3_free(zErrMsg);
                    throw exceptionFailedToCreateTable();
                }
            },


            //Define what is changed from version 1 to version 2
            [&]() {
                Log* log = Log::GetInstance();
                log->addMessage("Unsupported database version.", Log::CRITICAL);
                throw runtime_error("Unsupported database version.");
            },


            //Define what is changed from version 2 to version 3
            [&]() {
                Log* log = Log::GetInstance();
                log->addMessage("Unsupported database version.", Log::CRITICAL);
                throw runtime_error("Unsupported database version.");
            },


            //Define what is changed from version 3 to version 4
            [&]() {
                Log* log = Log::GetInstance();
                log->addMessage("Unsupported database version.", Log::CRITICAL);
                throw runtime_error("Unsupported database version.");
            },


            //Define what is changed from version 4 to version 5
            [&]() {
                Log* log = Log::GetInstance();
                log->addMessage("Unsupported database version.", Log::CRITICAL);
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
    //add performance indexes for rewinding
    addPerformanceIndex("assets", "heightCreated");
    addPerformanceIndex("assets", "heightCreated");
    addPerformanceIndex("blocks", "height");
    addPerformanceIndex("exchange", "height");
    addPerformanceIndex("kyc", "height");
    addPerformanceIndex("kyc", "revoked");
    addPerformanceIndex("utxos", "heightCreated");
    addPerformanceIndex("utxos", "heightDestroyed");
    addPerformanceIndex("votes", "height");



    //statement to find a flag value
    _stmtCheckFlag.prepare(_db, "SELECT value FROM flags WHERE key LIKE ?;");

    //statement to set a flag value
    _stmtSetFlag.prepare(_db, "UPDATE flags SET value=? WHERE key LIKE ?;");


    //statement to get block height
    _stmtGetBlockHeight.prepare(_db, "SELECT height FROM blocks ORDER BY height DESC LIMIT 1;");

    //statement to set block height
    _stmtInsertBlock.prepare(_db, "INSERT INTO blocks VALUES (?,?,?,?,?);");

    //statement to get block hash
    _stmtGetBlockHash.prepare(_db, "SELECT hash FROM blocks WHERE height=? LIMIT 1;");

    //second half of prune statement.  removes blocks from list that it is no longer possible to roll back to
    _stmtRemoveNonReachable.prepare(_db, "DELETE FROM blocks where height<?;");




    //statement to create UTXO
    _stmtCreateUTXO.prepare(_db, "INSERT INTO utxos (address,txid,vout,aout,assetIndex,amount,heightCreated,issuance)  VALUES (?,?,?,?,?,?,?,?);");

    //statement to spend UTXO
    _stmtSpendUTXO.prepare(_db, "UPDATE utxos SET heightDestroyed=?, spentTXID=? WHERE txid=? AND vout=?;");

    //statement to get spending address from UTXO
    _stmtGetSpendingAddress.prepare(_db, "SELECT address FROM utxos WHERE txid=? AND vout=?");

    //statement to prune utxos
    _stmtPruneUTXOs.prepare(_db, "DELETE FROM utxos WHERE heightDestroyed<?;");

    //statement to get funds on a UTXO
    _stmtGetAssetUTXO.prepare(_db, "SELECT address,aout,assetIndex,amount FROM utxos WHERE txid=? AND vout=? ORDER BY aout ASC;");

    //statement to get asset holders
    _stmtGetAssetHolders.prepare(_db, "SELECT address,SUM(amount) FROM utxos WHERE assetIndex=? AND heightDestroyed IS NULL GROUP BY address;");

    //statement to get all assetIndexs on a specific utxo
    _stmtGetAssetIndexOnUTXO.prepare(_db, "SELECT assetIndex FROM utxos WHERE txid=? AND vout=? AND aout IS NOT NULL;");

    //statement to get permanent storage paid on a specific transaction
    _stmtGetPermanentPaid.prepare(_db, "SELECT amount,heightCreated FROM utxos where txid=? and address=? and aout is null;");

    //statement to get number of assets that exist
    _stmtGetTotalAssetCounta.prepare(_db, "SELECT sum(amount) FROM utxos where assetIndex=? and heightDestroyed is null;");
    _stmtGetTotalAssetCountb.prepare(_db, "SELECT sum(u.amount) as totalAmount\n"
                                          "FROM utxos u\n"
                                          "JOIN assets a ON u.assetIndex = a.assetIndex\n"
                                          "WHERE a.assetId = ? AND u.heightDestroyed IS NULL;");
    addPerformanceIndex("utxos", "assetIndex", "heightDestroyed");
    addPerformanceIndex("assets", "assetId");

    //statement to get number of assets that exist
    _stmtGetOriginalAssetCounta.prepare(_db, "SELECT sum(amount) FROM utxos where assetIndex=? and issuance=1;");
    _stmtGetOriginalAssetCountb.prepare(_db, "SELECT sum(u.amount) as totalAmount\n"
                                             "FROM utxos u\n"
                                             "JOIN assets a ON u.assetIndex = a.assetIndex\n"
                                             "WHERE a.assetId = ? AND u.issuance=1;");
    addPerformanceIndex("utxos", "assetIndex", "issuance");
    addPerformanceIndex("assets", "assetId");

    //statement to get issuance txids
    _stmtGetAssetIssuanceTXIDs.prepare(_db, "SELECT u.assetIndex, u.txid, sum(u.amount) as amount, u.heightCreated, a.cid\n"
                                            "FROM utxos u\n"
                                            "JOIN assets a ON u.assetIndex = a.assetIndex\n"
                                            "WHERE a.assetId = ? AND issuance = 1\n"
                                            "GROUP BY u.assetIndex\n"
                                            "ORDER BY u.heightCreated ASC");

    //statement to get tx history
    _stmtGetAssetTxHistorya.prepare(_db, "SELECT txid\n"
                                         "FROM (\n"
                                         //"    -- Select txids where the asset was received\n"
                                         "    SELECT u.txid, u.heightCreated AS height\n"
                                         "    FROM utxos u\n"
                                         "    WHERE u.assetIndex = ?\n"
                                         "    UNION\n"
                                         //"    -- Select txids where the asset was spent\n"
                                         "    SELECT u.spentTXID AS txid, u.heightDestroyed AS height\n"
                                         "    FROM utxos u\n"
                                         "    WHERE u.assetIndex = ? AND u.spentTXID IS NOT NULL\n"
                                         "    UNION\n"
                                         //"    -- find txid that created the asset"
                                         "    SELECT u2.spentTXID AS txid, u2.heightCreated AS height\n"
                                         "    FROM utxos u1\n"
                                         "    JOIN utxos u2 ON u1.txid = u2.spentTXID\n"
                                         "    WHERE u1.assetIndex = ? AND u1.issuance = 1 AND u2.spentTXID IS NOT NULL\n"
                                         ") GROUP BY txid\n"
                                         "ORDER BY height ASC;");
    _stmtGetAssetTxHistoryb.prepare(_db, "SELECT txid\n"
                                         "FROM (\n"
                                         //"    -- Select txids where the asset was received\n"
                                         "    SELECT u.txid, u.heightCreated AS height\n"
                                         "    FROM utxos u\n"
                                         "    JOIN assets a ON u.assetIndex = a.assetIndex\n"
                                         "    WHERE a.assetId = ?\n"
                                         "    UNION\n"
                                         //"    -- Select txids where the asset was spent\n"
                                         "    SELECT u.spentTXID AS txid, u.heightDestroyed AS height\n"
                                         "    FROM utxos u\n"
                                         "    JOIN assets a ON u.assetIndex = a.assetIndex\n"
                                         "    WHERE a.assetId = ? AND u.spentTXID IS NOT NULL\n"
                                         "    UNION\n"
                                         //"    -- find txid that created the asset"
                                         "    SELECT u2.spentTXID AS txid, u2.heightCreated AS height\n"
                                         "    FROM utxos u1\n"
                                         "    JOIN utxos u2 ON u1.txid = u2.spentTXID\n"
                                         "    JOIN assets a ON u1.assetIndex = a.assetIndex\n"
                                         "    WHERE a.assetId = ? AND u1.issuance = 1 AND u2.spentTXID IS NOT NULL\n"
                                         ") GROUP BY txid\n"
                                         "ORDER BY height ASC;");
    _stmtGetAddressTxHistory.prepare(_db, "SELECT tx\n"
                                          "FROM (\n"
                                          "    SELECT * FROM (\n"
                                          "        SELECT txid AS tx, heightCreated AS height\n"
                                          "        FROM utxos\n"
                                          "        WHERE address=? AND heightCreated >=? AND heightCreated <=?\n"
                                          "        LIMIT ?\n"
                                          "    )\n"
                                          "    UNION\n"
                                          "    SELECT * FROM (\n"
                                          "        SELECT spentTXID AS tx, heightDestroyed AS height\n"
                                          "        FROM utxos\n"
                                          "        WHERE address=? AND spentTXID IS NOT NULL AND heightDestroyed >= ? AND heightDestroyed <= ?\n"
                                          "        LIMIT ?\n"
                                          "    )\n"
                                          ")\n"
                                          "ORDER BY height ASC\n"
                                          "LIMIT ?;");
    addPerformanceIndex("utxos", "address", "heightCreated");
    addPerformanceIndex("utxos", "address", "heightDestroyed", "spentTXID");

    //statements to get asset holdings
    _stmtGetAddressHoldings.prepare(_db, "SELECT assetIndex,SUM(amount) FROM utxos WHERE heightDestroyed IS NULL AND address=? GROUP BY assetIndex");
    addPerformanceIndex("utxos", "address", "heightDestroyed", "assetIndex");

    //statement to get valid utxos for a given address
    _stmtGetValidUTXO.prepare(_db, "SELECT `txid`,`vout`,`aout`,`assetIndex`,`amount` FROM utxos WHERE heightDestroyed IS NULL AND address=? AND heightCreated>=? AND heightCreated<=? ORDER BY txid ASC, vout ASC, aout ASC;");

    //statement to get a list of the last n blocks
    _stmtGetLastBlocks.prepare(_db, "SELECT height, hash, time, algo FROM blocks WHERE height<=? ORDER BY height DESC LIMIT ?");

    //statement to check if exchange watch address
    _stmtIsWatchAddress.prepare(_db, "SELECT address FROM exchangeWatch WHERE address=?;");

    //statement to add exchange watch address
    _stmtAddWatchAddress.prepare(_db, "INSERT INTO exchangeWatch VALUES (?);");


    //statement to insert new exchange rate
    _stmtAddExchangeRate.prepare(_db, "INSERT INTO exchange VALUES (?,?,?,?);");

    //statement to get current exchange rates(all rates)
    _stmtExchangeRatesAtHeight.prepare(_db, "WITH cte AS (\n"
                                            "  SELECT *, ROW_NUMBER() OVER (PARTITION BY [address], [index] ORDER BY height DESC) AS row_number\n"
                                            "  FROM exchange\n"
                                            "  WHERE height <= ?\n"
                                            ")\n"
                                            "SELECT [height], [address], [index], [value]\n"
                                            "FROM cte\n"
                                            "WHERE row_number = 1;");
    addPerformanceIndex("exchange", "address", "index", "height DESC");

    //statement to delete exchange rates bellow a specific height
    _stmtPruneExchangeRate.prepare(_db, "DELETE FROM exchange WHERE height<?;");

    //statement to get accepted exchange rate at a specific height
    _stmtGetValidExchangeRate.prepare(_db, "SELECT value, height FROM exchange WHERE height<? AND address=? AND [index]=? ORDER BY height DESC;");

    //statement to get current exchange rate(1 rate)
    _stmtGetCurrentExchangeRate.prepare(_db, "SELECT value FROM exchange WHERE address=? AND [index]=? ORDER BY height DESC LIMIT 1;");


    //statement to insert new kyc record
    _stmtAddKYC.prepare(_db, "INSERT OR IGNORE INTO kyc VALUES (?,?,?,?,?,NULL);");

    //statement to revoke kyc record
    _stmtRevokeKYC.prepare(_db, "UPDATE kyc SET revoked=? WHERE address=?;");

    //statement to revoke kyc record
    _stmtGetKYC.prepare(_db, "SELECT country,name,hash,height,revoked FROM kyc WHERE address=?;");


    //statement to get total votes at specific height
    _stmtGetVoteCountAtHeight.prepare(_db, "SELECT assetIndex,address,SUM([count]),MAX([height]) FROM votes WHERE height<? GROUP BY assetIndex,address;");

    //statement to prune votes
    _stmtPruneVote.prepare(_db, "DELETE FROM votes WHERE height<?;");

    //statement to record votes
    _stmtAddVote.prepare(_db, "INSERT INTO votes (assetIndex, address, height, count) VALUES (?, ?, ?, ?) ON CONFLICT (assetIndex, address, height) DO UPDATE SET count=count+?;");

    //statement to get vote counts for an asset
    _stmtGetVoteCount.prepare(_db, "SELECT address,SUM([count]) FROM votes WHERE assetIndex=? GROUP BY address;");

    //statement to add asset
    _stmtAddAsset.prepare(_db, "INSERT INTO assets (assetId, issueAddress, cid, rules, heightCreated, heightUpdated, expires) VALUES (?, ?, ?, ?, ?, ?, ?);");

    //statement to update asset(aggregable only)
    _stmtUpdateAsset.prepare(_db, "UPDATE assets SET heightUpdated=?, cid=?, rules=?, expires=? WHERE assetIndex=?");

    //statement to get assetIndex(don't use for non-aggregable)
    _stmtGetAssetIndex.prepare(_db, "SELECT assetIndex FROM assets WHERE assetId=?");

    //statement to get assets ordered by issuance height
    _stmtGetLastAssetIssued.prepare(_db, "SELECT assetIndex, assetId, cid, heightCreated FROM assets WHERE assetIndex <= ? ORDER BY heightCreated DESC LIMIT ?;");

    //statement to get assets ordered by issuance height
    _stmtGetAssetIDsOrderedByHeight.prepare(_db, "SELECT assetIndex, assetId, cid, heightCreated FROM assets order by heightCreated asc limit ? offset ?;");

    //statement to get height created
    _stmtGetHeightAssetCreated.prepare(_db, "SELECT assetIndex,heightCreated FROM assets WHERE assetId=?");

    //statement to get asset rules
    _stmtGetAssetRules.prepare(_db, "SELECT rules FROM assets WHERE assetId=?");

    //statement to get asset
    _stmtGetAsset.prepare(_db, "SELECT assetId,cid,issueAddress,rules,heightCreated,heightUpdated FROM assets WHERE assetIndex=?");

    //statement to get asset created by an address
    _stmtGetAssetCreateByAddress.prepare(_db, "SELECT assetIndex FROM assets where issueAddress=? order by assetIndex asc;");


    //statement for IPFS
    _stmtGetNextIPFSJob.prepare(_db, "SELECT jobIndex,sync,cid,extra,callback,maxTime FROM ipfs WHERE pause is NULL AND lock is false ORDER BY jobIndex ASC LIMIT 1;");

    _stmtClearNextIPFSJob_a.prepare(_db, "DELETE FROM ipfs WHERE jobIndex=?;");
    _stmtClearNextIPFSJob_b.prepare(_db, "UPDATE ipfs set lock=false WHERE sync=?;");

    _stmtInsertIPFSJob.prepare(_db, "INSERT INTO ipfs (sync, lock, cid, extra, callback, pause, maxTime) VALUES (?,false,?,?,?,?,?);");

    _stmtSetIPFSPauseSync.prepare(_db, "UPDATE ipfs set pause=?, lock=false WHERE sync=?;");

    _stmtClearIPFSPause.prepare(_db, "UPDATE ipfs set pause=NULL WHERE pause<?;");

    _stmtSetIPFSLockSync.prepare(_db, "UPDATE ipfs set lock=true WHERE sync=?;");

    _stmtSetIPFSPauseJob.prepare(_db, "UPDATE ipfs set pause=?, lock=false WHERE jobIndex=?;");

    _stmtSetIPFSLockJob.prepare(_db, "UPDATE ipfs set lock=true WHERE jobIndex=?;");

    _stmtNumberOfIPFSJobs.prepare(_db, "SELECT count(*) FROM ipfs;");

    //DigiByte Domain statements
    _stmtGetDomainAssetId.prepare(_db, "SELECT assetId,revoked FROM domains WHERE domain=?");

    _stmtAddDomain.prepare(_db, "INSERT INTO domains VALUES (?,?,false);");

    _stmtRevokeDomain.prepare(_db, "UPDATE domains SET revoked=true WHERE domain=?;");

    _stmtSetDomainMasterAssetId_a.prepare(_db, "UPDATE domainsMasters SET active=false WHERE assetId=?;");
    _stmtSetDomainMasterAssetId_b.prepare(_db, "INSERT INTO domainsMasters VALUES (?,true);");

    //IPFS Permanent statements
    _stmtInsertPermanent.prepare(_db, "INSERT OR IGNORE INTO pspFiles (cid,poolIndex) VALUES (?,?)");

    _stmtDeletePermanent.prepare(_db, "DELETE FROM pspFiles WHERE cid=? AND poolIndex=?");

    _stmtIsInPermanent.prepare(_db, "SELECT 1 FROM pspFiles WHERE cid=?");

    _stmtRepinAssets.prepare(_db, "INSERT INTO ipfs (sync, lock, cid, extra, callback, pause, maxTime) SELECT 'pin', 0, cid, '', '', NULL, NULL FROM assets WHERE cid != '';");

    _stmtRepinPermanentSpecific.prepare(_db, "INSERT INTO ipfs (sync, lock, cid, extra, callback, pause, maxTime) SELECT 'pin', 0, cid, '', '', NULL, NULL FROM pspFiles WHERE \"poolIndex\" = ?;");

    _stmtAddAssetToPool.prepare(_db, "INSERT OR IGNORE INTO pspAssets (assetIndex,poolIndex) VALUES (?,?);");

    _stmtIsAssetInPool.prepare(_db, "SELECT 1 FROM \"pspAssets\" WHERE \"assetIndex\" = ? AND \"poolIndex\" = ?;");

    _stmtIsAssetInAPool.prepare(_db, "SELECT poolIndex FROM \"pspAssets\" WHERE \"assetIndex\" = ?;");

    _stmtPSPFileList.prepare(_db, "SELECT cid FROM \"pspFiles\" WHERE \"poolIndex\" = ? GROUP BY cid;");
    addPerformanceIndex("pspFiles", "poolIndex", "cid");

    _stmtPSPFindBadAsset.prepare(_db, "SELECT a.assetIndex, a.cid "
                                      "FROM assets a "
                                      "INNER JOIN pspAssets p ON a.assetIndex = p.assetIndex "
                                      "WHERE a.assetId = ? AND p.poolIndex = ?;");

    _stmtPSPDeleteBadAsset.prepare(_db, "DELETE FROM pspAssets "
                                        "WHERE assetIndex IN ("
                                        "    SELECT a.assetIndex "
                                        "    FROM assets a "
                                        "    INNER JOIN pspAssets p ON a.assetIndex = p.assetIndex "
                                        "    WHERE a.assetId = ? AND p.poolIndex = ?"
                                        ");");




    //initialize exchange address watch list
    sqlite3_stmt* stmt1;
    const char* sqlInt10 = "SELECT address FROM exchangeWatch WHERE 1;";
    int rc = sqlite3_prepare_v2(_db, sqlInt10, -1, &stmt1, nullptr);
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
    char* zErrMsg = nullptr;
    const char* sqlInt11 = "DELETE FROM ipfs WHERE callback LIKE \"_\";UPDATE ipfs SET pause=NULL,lock=0;";
    rc = sqlite3_exec(_db, sqlInt11, Database::defaultCallback, nullptr, &zErrMsg);
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
    rc = sqlite3_prepare_v2(_db, sqlInt12, -1, &stmt3, nullptr);
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
    }
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
    rc = sqlite3_open_v2(newFileName.c_str(), &_db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr);
    if (rc) throw exceptionFailedToOpen();

    //create needed tables
    if (firstRun) {
        buildTables();
    } else {
        //get database version number
        sqlite3_stmt* stmt;
        const char* sql = "SELECT `value` FROM `flags` WHERE `key`=\"dbVersion\";";
        rc = sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr);
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
    char* zErrMsg = nullptr;
    sqlite3_exec(_db, "BEGIN TRANSACTION", nullptr, nullptr, &zErrMsg);
}

/**
 * Finishes the batch transaction
 */
void Database::endTransaction() {
    char* zErrMsg = nullptr;
    sqlite3_exec(_db, "END TRANSACTION", nullptr, nullptr, &zErrMsg);
}

/**
 * disables write verification.  gives significant speed increase but on power failure data
 * may not all be written to the drive so must recheck at startup
 */
void Database::disableWriteVerification() {
    char* zErrMsg = nullptr;
    sqlite3_exec(_db, "PRAGMA synchronous = OFF", nullptr, nullptr, &zErrMsg);
    sqlite3_exec(_db, "PRAGMA journal_mode = MEMORY", nullptr, nullptr, &zErrMsg);
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
    char* zErrMsg = nullptr;
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

    rc = sqlite3_exec(_db, sql, Database::defaultCallback, nullptr, &zErrMsg);

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
    if (asset.isAggregable()) { //hybrid and dispersed never update so skip this leg

        //only 1 asset can exist with the same assetId so if not locked check if already exists
        if (!asset.isLocked()) {

            //get the assetIndex if the asset already exists
            LockedStatement getAssetIndex{_stmtGetAssetIndex};
            getAssetIndex.bindText(1, assetId);
            rc = getAssetIndex.executeStep();
            if (rc == SQLITE_ROW) { //if not found its new so leave assetIndex 0
                assetIndex = getAssetIndex.getColumnInt(0);
            }
        }

        //update existing asset
        if (assetIndex != 0) {
            LockedStatement getUpdateAsset{_stmtUpdateAsset};
            getUpdateAsset.bindInt(1, asset.getHeightUpdated());
            getUpdateAsset.bindText(2, asset.getCID());
            vector<uint8_t> serializedRules;
            serialize(serializedRules, asset.getRules());
            Blob rules{serializedRules};
            getUpdateAsset.bindBlob(3, rules);
            getUpdateAsset.bindInt64(4, asset.getExpiry());
            getUpdateAsset.bindInt64(5, assetIndex);
            rc = getUpdateAsset.executeStep();
            if (rc != SQLITE_DONE) {
                string tempErrorMessage = sqlite3_errmsg(_db);
                throw exceptionFailedUpdate();
            }
            return assetIndex;
        }
    }

    //insert new asset
    LockedStatement addAsset{_stmtAddAsset};
    addAsset.bindText(1, assetId);
    addAsset.bindText(2, asset.getIssuer().getAddress());
    addAsset.bindText(3, asset.getCID());
    vector<uint8_t> serializedRules;
    serialize(serializedRules, asset.getRules());
    Blob rules{serializedRules};
    addAsset.bindBlob(4, rules);
    addAsset.bindInt(5, asset.getHeightCreated());
    addAsset.bindInt(6, asset.getHeightUpdated()); //will be same as created
    addAsset.bindInt64(7, asset.getExpiry());
    rc = addAsset.executeStep();
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
DigiAssetRules Database::getRules(const string& assetId) {
    LockedStatement getAssetRules{_stmtGetAssetRules};
    getAssetRules.bindText(1, assetId);
    int rc = getAssetRules.executeStep();
    if (rc != SQLITE_ROW) return {};

    vector<uint8_t> serializedRules = getAssetRules.getColumnBlob(0).vector();
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
    LockedStatement getHeightAssetCreated{_stmtGetHeightAssetCreated};
    getHeightAssetCreated.bindText(1, assetId);
    int rc = getHeightAssetCreated.executeStep();
    if (rc == SQLITE_ROW) { //if not found its new so leave assetIndex 0
        assetIndex = getHeightAssetCreated.getColumnInt(0);
        return getHeightAssetCreated.getColumnInt(1);
    }
    return backupHeight;
}

/**
 * Gets an asset by its assetIndex
 * @param assetIndex
 * @param amount - optional number of assets to include in object
 * @return
 */
DigiAsset Database::getAsset(uint64_t assetIndex, uint64_t amount) {
    //get data in assets table
    LockedStatement getAsset{_stmtGetAsset};
    getAsset.bindInt64(1, assetIndex);
    int rc = getAsset.executeStep();
    if (rc != SQLITE_ROW) {
        string tempErrorMessage = sqlite3_errmsg(_db);
        throw exceptionFailedSelect();
    }
    string assetId = getAsset.getColumnText(0);
    string cid = getAsset.getColumnText(1);
    string issuerAddress = getAsset.getColumnText(2);
    vector<uint8_t> serializedRules = getAsset.getColumnBlob(3).vector();
    unsigned int heightCreated = getAsset.getColumnInt(4);
    unsigned int heightUpdated = getAsset.getColumnInt(5);

    //lookup kyc and rules
    KYC issuer = getAddressKYC(issuerAddress);
    DigiAssetRules rules;
    size_t i = 0;
    deserialize(serializedRules, i, rules);

    //create and return object
    return {assetIndex, assetId, cid, issuer, rules, heightCreated, heightUpdated, amount};
}

uint64_t Database::getAssetIndex(const string& assetId, const string& txid, unsigned int vout) {
    //see if the asset exists and if only 1 index
    uint64_t assetIndex;
    { //shorten life of database lock
        LockedStatement getAssetIndex{_stmtGetAssetIndex};
        getAssetIndex.bindText(1, assetId);
        int rc = getAssetIndex.executeStep();
        if (rc != SQLITE_ROW) {
            throw exceptionFailedSelect();
        }
        assetIndex = getAssetIndex.getColumnInt(0);


        //check if more than 1
        rc = getAssetIndex.executeStep();
        if (rc != SQLITE_ROW) return assetIndex; //there was only 1
    }

    //more than 1 so see if the txid and vout provided match a utxo
    if (txid.empty()) throw exceptionFailedSelect();
    vector<uint64_t> assetIndexPossibilities;
    { //shorten life of database lock
        LockedStatement getAssetIndexOnUTXO{_stmtGetAssetIndexOnUTXO};
        Blob txidBlob(txid);
        getAssetIndexOnUTXO.bindBlob(1, txidBlob);
        getAssetIndexOnUTXO.bindInt(2, vout);
        while (getAssetIndexOnUTXO.executeStep() == SQLITE_ROW) {
            assetIndexPossibilities.push_back(getAssetIndexOnUTXO.getColumnInt(0));
        }
    }

    //filter possibilities for those that are the same assetId
    for (uint64_t assetIndexTest: assetIndexPossibilities) {
        DigiAsset asset = getAsset(assetIndexTest);
        if (asset.getAssetId() == assetId) return assetIndexTest;
    }
    throw exceptionFailedSelect();
}

vector<uint64_t> Database::getAssetIndexes(const std::string& assetId) {
    vector<uint64_t> assetIndexes;

    // Reset the prepared statement to its initial state and bind the assetId parameter
    LockedStatement getAssetIndex{_stmtGetAssetIndex};
    getAssetIndex.bindText(1, assetId);

    // Execute the query and collect all matching asset indexes
    while (getAssetIndex.executeStep() == SQLITE_ROW) {
        uint64_t assetIndex = getAssetIndex.getColumnInt64(0);
        assetIndexes.push_back(assetIndex);
    }

    return assetIndexes;
}

/**
 * Returns a list of {assetIndex, assetId, cid, height} ordered by the issuance height
 * @return
 */
std::vector<AssetBasics> Database::getAssetsIssued(unsigned int amount, unsigned int offset) {
    std::vector<AssetBasics> results;
    LockedStatement getAssetIDsOrderedByHeight{_stmtGetAssetIDsOrderedByHeight};
    getAssetIDsOrderedByHeight.bindInt(1, amount);
    getAssetIDsOrderedByHeight.bindInt(2, offset);
    while (getAssetIDsOrderedByHeight.executeStep() == SQLITE_ROW) {
        AssetBasics asset;
        asset.assetIndex = getAssetIDsOrderedByHeight.getColumnInt(0);
        asset.assetId = getAssetIDsOrderedByHeight.getColumnText(1);
        asset.cid = getAssetIDsOrderedByHeight.getColumnText(2);
        asset.height = getAssetIDsOrderedByHeight.getColumnInt(3);
        results.push_back(asset);
    }
    return results;
}

/**
 * Returns a list of {assetIndex, assetId, cid, height} ordered by the issuance height
 * @return
 */
std::vector<AssetBasics> Database::getLastAssetsIssued(unsigned int amount, unsigned int firstAsset) {
    std::vector<AssetBasics> results;
    LockedStatement getAssetIDsOrderedByHeight{_stmtGetLastAssetIssued};
    getAssetIDsOrderedByHeight.bindInt64(1, firstAsset);
    getAssetIDsOrderedByHeight.bindInt(2, amount);
    while (getAssetIDsOrderedByHeight.executeStep() == SQLITE_ROW) {
        AssetBasics asset;
        asset.assetIndex = getAssetIDsOrderedByHeight.getColumnInt(0);
        asset.assetId = getAssetIDsOrderedByHeight.getColumnText(1);
        asset.cid = getAssetIDsOrderedByHeight.getColumnText(2);
        asset.height = getAssetIDsOrderedByHeight.getColumnInt(3);
        results.push_back(asset);
    }
    return results;
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
    LockedStatement checkFlag{_stmtCheckFlag};
    checkFlag.bindText(1, flag);
    int rc = checkFlag.executeStep();
    if (rc != SQLITE_ROW) { //there should always be one
        string tempErrorMessage = sqlite3_errmsg(_db);
        throw exceptionFailedSelect(); //failed to check database
    }

    //store in ram and return
    _flagState[flag] = checkFlag.getColumnInt(0);
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
    LockedStatement setFlag{_stmtSetFlag};
    setFlag.bindInt(1, state);
    setFlag.bindText(2, flag);
    int rc = setFlag.executeStep();
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
    LockedStatement insertBlock{_stmtInsertBlock};
    insertBlock.bindInt(1, height);
    Blob hashBlob = Blob(hash);
    insertBlock.bindBlob(2, hashBlob);
    insertBlock.bindInt(3, time);
    insertBlock.bindInt(4, algo);
    insertBlock.bindDouble(5, difficulty);
    int rc = insertBlock.executeStep();
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
std::string Database::getBlockHash(uint height) {
    LockedStatement getBlockHash{_stmtGetBlockHash};
    getBlockHash.bindInt(1, height);
    int rc = getBlockHash.executeStep();
    if (rc != SQLITE_ROW) {
        throw exceptionDataPruned();
    }
    Blob hash = getBlockHash.getColumnBlob(0);
    return hash.toHex();
}

/**
 * Gets the newest block height in database
 * Possible Errors:
 *  exceptionFailedSelect
 */
uint Database::getBlockHeight() {
    LockedStatement getBlockHeight{_stmtGetBlockHeight};
    int rc = getBlockHeight.executeStep();
    if (rc != SQLITE_ROW) { //there should always be one
        throw exceptionFailedSelect();
    }
    return getBlockHeight.getColumnInt(0);
}

/**
 * Clears all blocks above a specific height.
 * Used when rolling back data
 * Possible Errors:
 *  exceptionFailedDelete
 */
void Database::clearBlocksAboveHeight(uint height) {
    int rc;
    char* zErrMsg = nullptr;
    string lineEnd = to_string(height) + ";";
    const string sql = "DELETE FROM assets WHERE heightCreated>1 AND heightCreated>=" + lineEnd +
                       "DELETE FROM exchange WHERE height>=" + lineEnd +
                       "DELETE FROM kyc WHERE height>=" + lineEnd +
                       "UPDATE kyc SET revoked=NULL WHERE revoked>=" + lineEnd +
                       "DELETE FROM utxos WHERE heightCreated>=" + lineEnd +
                       "UPDATE utxos SET heightDestroyed=NULL WHERE heightDestroyed>=" + lineEnd +
                       "DELETE FROM votes WHERE height>=" + lineEnd +
                       "DELETE FROM blocks WHERE height>" + lineEnd;

    rc = sqlite3_exec(_db, sql.c_str(), Database::defaultCallback, nullptr, &zErrMsg);
    if (rc != SQLITE_OK) {
        sqlite3_free(zErrMsg);
        throw exceptionFailedDelete();
    }
}

/**
 * Get last blocks
 */
std::vector<BlockBasics> Database::getLastBlocks(unsigned int limit, unsigned int start) {
    std::vector<BlockBasics> results;
    LockedStatement getLastBlocks{_stmtGetLastBlocks};
    getLastBlocks.bindInt64(1, start);
    getLastBlocks.bindInt(2, limit);
    while (getLastBlocks.executeStep() == SQLITE_ROW) {
        BlockBasics block;
        block.height = getLastBlocks.getColumnInt(0);
        block.hash = getLastBlocks.getColumnBlob(1).toHex();
        block.time = getLastBlocks.getColumnInt(2);
        block.algo = getLastBlocks.getColumnInt(3);
        results.push_back(block);
    }
    return results;
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
 * @param issuance- 0 nothing, 1 asset, 2 block
 * Possible Errors:
 *  exceptionFailedInsert
 */
void Database::createUTXO(const AssetUTXO& value, unsigned int heightCreated, bool assetIssuance) {
    if (value.address.empty()) return; //op return don't store
    if (value.assets.empty() && getBeenPrunedNonAssetUTXOHistory()) {
        //_recentNonAssetUTXO.add(value.txid, value.vout); //keep temp cache that this is non asset utxo
        return; //non asset utxo and we aren't storing those
    }
    int rc;

    //add the main utxo
    LockedStatement createUTXO{_stmtCreateUTXO};
    createUTXO.bindText(1, value.address);
    Blob blobTXID = Blob(value.txid);
    createUTXO.bindBlob(2, blobTXID);
    createUTXO.bindInt(3, value.vout);
    createUTXO.bindNull(4);
    createUTXO.bindInt(5, 1);
    createUTXO.bindInt64(6, value.digibyte);
    createUTXO.bindInt(7, heightCreated);
    createUTXO.bindInt(8, 0);
    rc = createUTXO.executeStep();
    if (rc != SQLITE_DONE) {
        string tempErrorMessage = sqlite3_errmsg(_db); //todo have occasionally gotten no more rows error.  This eventually fixes itself but need to figure out why.  to prevent hammering of system have added a 2 second delay when this error occurs
        Log::GetInstance()->addMessage("Known Unrecoverable Database Error: "+tempErrorMessage,Log::CRITICAL);
        std::abort();   //crash the program
        throw exceptionFailedInsert();//just incase
    }

    //add any assets
    for (size_t aout = 0; aout < value.assets.size(); aout++) {
        createUTXO.reset();
        createUTXO.bindText(1, value.address);
        blobTXID = Blob(value.txid);
        createUTXO.bindBlob(2, blobTXID);
        createUTXO.bindInt(3, value.vout);
        createUTXO.bindInt(4, aout);
        createUTXO.bindInt64(5, value.assets[aout].getAssetIndex());
        createUTXO.bindInt64(6, value.assets[aout].getCount());
        createUTXO.bindInt(7, heightCreated);
        createUTXO.bindInt(8, assetIssuance);
        rc = createUTXO.executeStep();
        if (rc != SQLITE_DONE) {
            string tempErrorMessage = sqlite3_errmsg(_db);
            throw exceptionFailedInsert();
        }
    }
}

/**
 * Marks a utxo as spent
 * Possible Errors:
 *  exceptionFailedUpdate
 */
void Database::spendUTXO(const std::string& txid, unsigned int vout, unsigned int heightSpent, const string& spentTXID) {
    LockedStatement spendUTXO{_stmtSpendUTXO};
    Blob blobTXID = Blob(txid);
    Blob blobSpendingTXID = Blob(spentTXID);
    spendUTXO.bindInt(1, heightSpent);
    spendUTXO.bindBlob(2, blobSpendingTXID);
    spendUTXO.bindBlob(3, blobTXID);
    spendUTXO.bindInt(4, vout);
    int rc = spendUTXO.executeStep();
    if (rc != SQLITE_DONE) {
        string tempErrorMessage = sqlite3_errmsg(_db);
        throw exceptionFailedUpdate();
    }
}

std::string Database::getSendingAddress(const string& txid, unsigned int vout) {
    LockedStatement getSpendingAddress{_stmtGetSpendingAddress};
    Blob blobTXID = Blob(txid);
    getSpendingAddress.bindBlob(1, blobTXID);
    getSpendingAddress.bindInt(2, vout);
    int rc = getSpendingAddress.executeStep();
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
    return getSpendingAddress.getColumnText(0);
}

/**
 * Removes all spent UTXOs below(not including) height
 * @param height
 */
void Database::pruneUTXO(unsigned int height) {
    //prune UTXOs
    {
        LockedStatement pruneUTXOs{_stmtPruneUTXOs};
        pruneUTXOs.bindInt(1, height);
        int rc = pruneUTXOs.executeStep();
        if (rc != SQLITE_DONE) {
            string tempErrorMessage = sqlite3_errmsg(_db);
            throw exceptionFailedDelete();
        }
    }

    //mark blocks that can't be rolled back anymore
    {
        LockedStatement removeNonReachable{_stmtRemoveNonReachable};
        removeNonReachable.bindInt(1, height);
        int rc = removeNonReachable.executeStep();
        if (rc != SQLITE_DONE) {
            string tempErrorMessage = sqlite3_errmsg(_db);
            throw exceptionFailedUpdate();
        }
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
 * @param height - optional used for better error handling
 * @return
 */
AssetUTXO Database::getAssetUTXO(const string& txid, unsigned int vout, unsigned int height) {
    //try to get data from database
    AssetUTXO result;
    {
        LockedStatement getAssetUTXO{_stmtGetAssetUTXO};
        Blob blobTXID = Blob(txid);
        getAssetUTXO.bindBlob(1, blobTXID);
        getAssetUTXO.bindInt(2, vout);
        result.txid = txid;
        result.vout = vout;
        bool exists = false;
        while (getAssetUTXO.executeStep() == SQLITE_ROW) {
            exists = true;
            unsigned int assetIndex = getAssetUTXO.getColumnInt(2);
            uint64_t amount = getAssetUTXO.getColumnInt64(3);
            if (assetIndex == 1) {
                result.address = getAssetUTXO.getColumnText(0);
                result.digibyte = amount;
            } else {
                result.assets.emplace_back(getAsset(assetIndex, amount));
            }
        }
        if (exists) return result;
    }

    //check if we can/should get from the wallet
    //if (height<getBeenPrunedUTXOHistory()) means the data we want has been pruned and may contain asset data
    AppMain* main = AppMain::GetInstance();
    if (!main->isDigiByteCoreSet() || (static_cast<int>(height) < getBeenPrunedUTXOHistory())) throw exceptionDataPruned();

    //get tx data from wallet
    getrawtransaction_t txData = main->getDigiByteCore()->getRawTransaction(txid);
    result.txid = txid;
    result.vout = vout;
    result.digibyte = txData.vout[vout].valueS;
    result.address = txData.vout[vout].scriptPubKey.addresses[0];
    result.assets.clear();
    return result;
}

/**
 * Returns a list of UTXOs for a specific address.
 * Warning will not return all utxos if not storing non asset utxos and the address is not part of wallet
 * @param address
 * @return
 */
std::vector<AssetUTXO> Database::getAddressUTXOs(const string& address, unsigned int minConfirms, unsigned int maxConfirms) {
    vector<AssetUTXO> results;
    AssetUTXO entry;
    entry.address = address;

    //find acceptable search heights
    DigiByteCore* dgb = AppMain::GetInstance()->getDigiByteCore();
    unsigned int currentHeight = dgb->getBlockCount();
    unsigned int minConfirmHeight = currentHeight - minConfirms + 1;
    unsigned int maxConfirmHeight = 0;
    if (maxConfirms < currentHeight) maxConfirmHeight = currentHeight - maxConfirms + 1;

    //get all utxos that include DigiAssets
    Blob lastTxid{"00"};
    {
        LockedStatement getValidUTXO{_stmtGetValidUTXO};
        getValidUTXO.bindText(1, address);
        getValidUTXO.bindInt(2, maxConfirmHeight); //max confirms height will be lower bound
        getValidUTXO.bindInt(3, minConfirmHeight);
        int lastVout = -1;
        while (getValidUTXO.executeStep() == SQLITE_ROW) {
            //get next row value
            Blob txid = getValidUTXO.getColumnBlob(0);
            int vout = getValidUTXO.getColumnInt(1);
            unsigned int assetIndex = getValidUTXO.getColumnInt(3);
            uint64_t amount = getValidUTXO.getColumnInt64(4);

            //check if new utxo
            if ((lastTxid != txid) || (lastVout != vout)) {
                //save last utxo to the results
                if (lastTxid.length() != 1) {
                    results.emplace_back(entry);
                }

                //reset entry to new utxo values
                lastVout = vout;
                entry.txid = txid.toHex();
                entry.vout = vout;
                entry.address = address;
                entry.digibyte = 0;
                entry.assets = {};
            }
            lastTxid = txid;

            //add current row data
            if (assetIndex == 1) {
                //set the DGB
                entry.digibyte = amount;
            } else {
                //sql statement will always return in order
                entry.assets.push_back(getAsset(assetIndex, amount));
            }
        }
    }

    //save last entry
    if (lastTxid.length() != 1) {
        results.emplace_back(entry);
    }

    //check if there are any non asset utxo missing from the database
    if (getBeenPrunedNonAssetUTXOHistory()) {
        //see if we can get the rest of the data from the wallet
        vector<unspenttxout_t> walletUtxoData;
        try {
            walletUtxoData = dgb->listUnspent(minConfirms, maxConfirms, {address});
        } catch (...) {
            //ignore errors
        }
        for (const auto& walletUtxo: walletUtxoData) {
            //search to see if already in results
            bool isDuplicate = false;
            for (const auto& resultUtxo: results) {
                if (resultUtxo.txid == walletUtxo.txid && resultUtxo.vout == walletUtxo.n) {
                    isDuplicate = true;
                    break;
                }
            }

            //if not in results add to results
            if (!isDuplicate) {
                AssetUTXO newUtxo;
                newUtxo.txid = walletUtxo.txid;
                newUtxo.vout = walletUtxo.n;
                newUtxo.address = walletUtxo.address;
                newUtxo.digibyte = static_cast<uint64_t>(walletUtxo.amount * 100000000); // Assuming amount needs to be in satoshis
                newUtxo.assets = {};                                                     // No assets for non-asset UTXOs
                results.emplace_back(newUtxo);
            }
        }
    }

    return results;
}

/**
 * Returns a list of asset holders for a specific assetIndex
 * @param assetIndex
 * @return
 */
std::vector<AssetHolder> Database::getAssetHolders(uint64_t assetIndex) {
    LockedStatement getAssetHolders{_stmtGetAssetHolders};
    getAssetHolders.bindInt64(1, assetIndex);
    vector<AssetHolder> result;
    while (getAssetHolders.executeStep() == SQLITE_ROW) {
        string address = getAssetHolders.getColumnText(0);
        uint64_t count = getAssetHolders.getColumnInt64(1);
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
uint64_t Database::getTotalAssetCount(uint64_t assetIndex) {
    LockedStatement getTotalAssetCounta{_stmtGetTotalAssetCounta};
    getTotalAssetCounta.bindInt64(1, assetIndex);
    if (getTotalAssetCounta.executeStep() != SQLITE_ROW) throw exceptionFailedSelect();
    return getTotalAssetCounta.getColumnInt64(0);
}

/**
 * Returns the total number assets that exist of this specific type.
 * The difference between this and the other getTotalAssetCount function is this if the asset has sub types this will provide the total of all sub types
 * @param assetIndex
 * @return
 */
uint64_t Database::getTotalAssetCount(const string& assetId) {
    LockedStatement getTotalAssetCountb{_stmtGetTotalAssetCountb};
    getTotalAssetCountb.bindText(1, assetId);
    if (getTotalAssetCountb.executeStep() != SQLITE_ROW) throw exceptionFailedSelect();
    return getTotalAssetCountb.getColumnInt64(0);
}

/**
 * Returns the original number assets that exist of this specific type.
 * The difference between this and the other getOriginalAssetCount function is that if the asset has sub types this will only give total of the sub type provided
 * @param assetIndex
 * @return
 */
uint64_t Database::getOriginalAssetCount(uint64_t assetIndex) {
    if (getBeenPrunedUTXOHistory() > 0) throw exceptionDataPruned();
    LockedStatement getOriginalAssetCounta{_stmtGetOriginalAssetCounta};
    getOriginalAssetCounta.bindInt64(1, assetIndex);
    if (getOriginalAssetCounta.executeStep() != SQLITE_ROW) throw exceptionFailedSelect();
    return getOriginalAssetCounta.getColumnInt64(0);
}

/**
 * Returns the original number assets that exist of this specific type.
 * The difference between this and the other getOriginalAssetCount function is this if the asset has sub types this will provide the total of all sub types
 * @param assetIndex
 * @return
 */
uint64_t Database::getOriginalAssetCount(const string& assetId) {
    if (getBeenPrunedUTXOHistory() > -1) throw exceptionDataPruned();
    LockedStatement getOriginalAssetCountb{_stmtGetOriginalAssetCountb};
    getOriginalAssetCountb.bindText(1, assetId);
    if (getOriginalAssetCountb.executeStep() != SQLITE_ROW) throw exceptionFailedSelect();
    return getOriginalAssetCountb.getColumnInt64(0);
}

/**
 * Returns the TXIDs where the asset was created.
 * @param assetId
 * @return
 */
std::vector<IssuanceBasics> Database::getAssetIssuanceTXIDs(const string& assetId) {
    vector<IssuanceBasics> results;
    LockedStatement getAssetIssuanceTXIDs{_stmtGetAssetIssuanceTXIDs};
    getAssetIssuanceTXIDs.bindText(1, assetId);
    while (getAssetIssuanceTXIDs.executeStep() == SQLITE_ROW) {
        IssuanceBasics issuance;
        issuance.assetIndex = getAssetIssuanceTXIDs.getColumnInt(0);
        issuance.txid = getAssetIssuanceTXIDs.getColumnBlob(1).toHex();
        issuance.amount = getAssetIssuanceTXIDs.getColumnInt64(2);
        issuance.height = getAssetIssuanceTXIDs.getColumnInt(3);
        issuance.cid = getAssetIssuanceTXIDs.getColumnText(4);
        results.push_back(issuance);
    }
    return results;
}
/**
 * Returns a list of TXIDs that involve this asset in order they happened
 * The difference between this and the other getAssetTxHistory function is that if the asset has sub types this will only give history for the sub type provide
 * @param assetIndex
 * @return
 */
std::vector<std::string> Database::getAssetTxHistory(uint64_t assetIndex) {
    if (getBeenPrunedUTXOHistory() > -1) throw exceptionDataPruned();
    vector<string> results;
    LockedStatement getAssetTxHistorya{_stmtGetAssetTxHistorya};
    getAssetTxHistorya.bindInt64(1, assetIndex);
    getAssetTxHistorya.bindInt64(2, assetIndex);
    getAssetTxHistorya.bindInt64(3, assetIndex);
    while (getAssetTxHistorya.executeStep() == SQLITE_ROW) {
        Blob txid = getAssetTxHistorya.getColumnBlob(0);
        results.push_back(txid.toHex());
    }
    return results;
}

/**
 * Returns a list of TXIDs that involve this asset in order they happened
 * The difference between this and the other getAssetTxHistory function is this if the asset has sub types this will provide the history of all sub types
 * @param assetIndex
 * @return
 */
std::vector<std::string> Database::getAssetTxHistory(const string& assetId) {
    if (getBeenPrunedUTXOHistory() > -1) throw exceptionDataPruned();
    vector<string> results;
    LockedStatement getAssetTxHistoryb{_stmtGetAssetTxHistoryb};
    getAssetTxHistoryb.bindText(1, assetId);
    getAssetTxHistoryb.bindText(2, assetId);
    getAssetTxHistoryb.bindText(3, assetId);
    while (getAssetTxHistoryb.executeStep() == SQLITE_ROW) {
        Blob txid = getAssetTxHistoryb.getColumnBlob(0);
        results.push_back(txid.toHex());
    }
    return results;
}

/**
 * Returns list of TXIDs that involve an address
 * @param address
 * @param minHeight - optional minimum height to return
 * @param maxHeight - optional maximum height to return
 * @return
 */
std::vector<std::string> Database::getAddressTxList(const string& address, unsigned int minHeight, unsigned int maxHeight, unsigned int limit) {
    if (getBeenPrunedUTXOHistory() > 0) throw exceptionDataPruned();
    vector<string> results;
    LockedStatement getAddressTxHistory{_stmtGetAddressTxHistory};
    getAddressTxHistory.bindText(1, address);
    getAddressTxHistory.bindInt64(2, minHeight);
    getAddressTxHistory.bindInt64(3, maxHeight);
    getAddressTxHistory.bindInt64(4, limit * 2);
    getAddressTxHistory.bindText(5, address);
    getAddressTxHistory.bindInt64(6, minHeight);
    getAddressTxHistory.bindInt64(7, maxHeight);
    getAddressTxHistory.bindInt64(8, limit * 2);
    getAddressTxHistory.bindInt64(9, limit);
    while (getAddressTxHistory.executeStep() == SQLITE_ROW) {
        Blob txid = getAddressTxHistory.getColumnBlob(0);
        results.push_back(txid.toHex());
    }
    return results;
}

/**
 * Returns a list of assets created by the specified address
 * @param address
 * @return
 */
std::vector<uint64_t> Database::getAssetsCreatedByAddress(const string& address) {
    if (getBeenPrunedUTXOHistory() > -1) throw exceptionDataPruned();
    vector<uint64_t> results;
    LockedStatement getAssetCreateByAddress{_stmtGetAssetCreateByAddress};
    getAssetCreateByAddress.bindText(1, address);
    while (getAssetCreateByAddress.executeStep() == SQLITE_ROW) {
        results.push_back(getAssetCreateByAddress.getColumnInt64(0));
    }
    return results;
}

/**
 * Returns assetIndex and count for assets in an address.   If non asset utxo are stored also returns DigiByte as index 1
 * @param address
 * @return
 */
std::vector<AssetCount> Database::getAddressHoldings(const string& address) {
    std::vector<AssetCount> results;

    LockedStatement getAddressHoldings{_stmtGetAddressHoldings};
    getAddressHoldings.bindText(1, address);
    while (getAddressHoldings.executeStep() == SQLITE_ROW) {
        results.emplace_back(AssetCount{
                .assetIndex = static_cast<unsigned int>(getAddressHoldings.getColumnInt(0)),
                .count = static_cast<uint64_t>(getAddressHoldings.getColumnInt64(1))});
    }
    return results;
}

/*
███████╗██╗  ██╗ ██████╗██╗  ██╗ █████╗ ███╗   ██╗ ██████╗ ███████╗    ██╗    ██╗ █████╗ ████████╗ ██████╗██╗  ██╗
██╔════╝╚██╗██╔╝██╔════╝██║  ██║██╔══██╗████╗  ██║██╔════╝ ██╔════╝    ██║    ██║██╔══██╗╚══██╔══╝██╔════╝██║  ██║
█████╗   ╚███╔╝ ██║     ███████║███████║██╔██╗ ██║██║  ███╗█████╗      ██║ █╗ ██║███████║   ██║   ██║     ███████║
██╔══╝   ██╔██╗ ██║     ██╔══██║██╔══██║██║╚██╗██║██║   ██║██╔══╝      ██║███╗██║██╔══██║   ██║   ██║     ██╔══██║
███████╗██╔╝ ██╗╚██████╗██║  ██║██║  ██║██║ ╚████║╚██████╔╝███████╗    ╚███╔███╔╝██║  ██║   ██║   ╚██████╗██║  ██║
╚══════╝╚═╝  ╚═╝ ╚═════╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═══╝ ╚═════╝ ╚══════╝     ╚══╝╚══╝ ╚═╝  ╚═╝   ╚═╝    ╚═════╝╚═╝  ╚═╝
 */

bool Database::isWatchAddress(const string& address) {
    //if to many watch addresses to buffer effectively use database
    if (_exchangeWatchAddresses.empty()) {
        LockedStatement isWatchAddress{_stmtIsWatchAddress};
        isWatchAddress.bindText(1, address);
        int rc = isWatchAddress.executeStep();
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
    LockedStatement addWatchAddress{_stmtAddWatchAddress};
    addWatchAddress.bindText(1, address);
    int rc = addWatchAddress.executeStep();
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
 * Gets the excahnge rates at a specific height
 * @param height
 * @return
 */
vector<Database::exchangeRateHistoryValue> Database::getExchangeRatesAtHeight(unsigned int height) {
    vector<exchangeRateHistoryValue> firstEntry;
    LockedStatement exchangeRatesAyHeight{_stmtExchangeRatesAtHeight};
    exchangeRatesAyHeight.bindInt(1, height);
    while (exchangeRatesAyHeight.executeStep() == SQLITE_ROW) {
        firstEntry.push_back(exchangeRateHistoryValue{
                .height = (unsigned int) exchangeRatesAyHeight.getColumnInt(0),
                .address = exchangeRatesAyHeight.getColumnText(1),
                .index = (unsigned char) exchangeRatesAyHeight.getColumnInt(2),
                .value = exchangeRatesAyHeight.getColumnDouble(3)});
    }
    return firstEntry;
}

/**
 * This function should only ever be called by the chain analyzer
 */
void Database::addExchangeRate(const string& address, unsigned int index, unsigned int height, double exchangeRate) {
    LockedStatement addExchangeRate{_stmtAddExchangeRate};
    addExchangeRate.bindText(1, address);
    addExchangeRate.bindInt(2, index);
    addExchangeRate.bindInt(3, height);
    addExchangeRate.bindDouble(4, exchangeRate);
    int rc = addExchangeRate.executeStep();
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
    //we need to keep at least 1 value before prune height for each value so let's find the newest before prune height for each exchange rate
    vector<exchangeRateHistoryValue> firstEntry = getExchangeRatesAtHeight(pruneHeight - 1);

    //delete from exchange where pruneHeight<pruneHeight;
    {
        LockedStatement pruneExchangeRate{_stmtPruneExchangeRate};
        pruneExchangeRate.bindInt(1, pruneHeight);
        int rc = pruneExchangeRate.executeStep();
        if (rc != SQLITE_DONE) {
            string tempErrorMessage = sqlite3_errmsg(_db);
            throw exceptionFailedDelete();
        }
    }

    //mark blocks that can't be rolled back anymore
    {
        LockedStatement removeNonReachable{_stmtRemoveNonReachable};
        removeNonReachable.bindInt(1, pruneHeight);
        int rc = removeNonReachable.executeStep();
        if (rc != SQLITE_DONE) {
            string tempErrorMessage = sqlite3_errmsg(_db);
            throw exceptionFailedUpdate();
        }
    }

    //add first values back in to table
    for (exchangeRateHistoryValue& kept: firstEntry) {
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
double Database::getAcceptedExchangeRate(const ExchangeRate& rate, unsigned int height) {
    //see if there is an exchange rate or not
    if (!rate.enabled()) return 100000000;

    //historic errors
    if ((height >= 16562773) && (height <= 16562782) && (rate.index == 1) &&
        (rate.address == "dgb1qunxh378eltj2jrwza5sj9grvu5xud43vqvudwh")) {
        return 9032962000;
    }

    //look up exchange rate
    LockedStatement getValidExchangeRate{_stmtGetValidExchangeRate};
    getValidExchangeRate.bindInt(1, height);
    getValidExchangeRate.bindText(2, rate.address);
    getValidExchangeRate.bindInt(3, rate.index);
    double min = numeric_limits<double>::infinity();
    while (getValidExchangeRate.executeStep() == SQLITE_ROW) {
        //check if smallest
        double currentRate = getValidExchangeRate.getColumnDouble(0);
        if (currentRate < min) min = currentRate;

        //check if that was the last sample(this test is done after the sample taken because we want to always get at least 1 sample past the expiry unless sample falls exactly on the expiry)
        unsigned int sampleHeight = getValidExchangeRate.getColumnInt(1);
        if (sampleHeight <= height - DigiAsset::EXCHANGE_RATE_LENIENCY) break;
    }
    if (min == numeric_limits<double>::infinity()) throw out_of_range("Unknown Exchange Rate");
    return min;
}

double Database::getCurrentExchangeRate(const ExchangeRate& rate) {
    LockedStatement getCurrentExchangeRate{_stmtGetCurrentExchangeRate};
    getCurrentExchangeRate.bindText(1, rate.address);
    getCurrentExchangeRate.bindInt(2, rate.index);
    if (getCurrentExchangeRate.executeStep() != SQLITE_ROW) throw out_of_range("Unknown Exchange Rate");
    return getCurrentExchangeRate.getColumnDouble(0);
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
    LockedStatement addKYC{_stmtAddKYC};
    Blob blobTXID = Blob(hash);
    addKYC.bindText(1, address);
    addKYC.bindText(2, country);
    addKYC.bindText(3, name);
    addKYC.bindBlob(4, blobTXID);
    addKYC.bindInt(5, height);
    int rc = addKYC.executeStep();
    if (rc != SQLITE_DONE) {
        string tempErrorMessage = sqlite3_errmsg(_db);
        throw exceptionFailedUpdate();
    }
}

void Database::revokeKYC(const string& address, unsigned int height) {
    LockedStatement revokeKYC{_stmtRevokeKYC};
    revokeKYC.bindInt(1, height);
    revokeKYC.bindText(2, address);
    int rc = revokeKYC.executeStep();
    if (rc != SQLITE_DONE) {
        string tempErrorMessage = sqlite3_errmsg(_db);
        throw exceptionFailedUpdate();
    }
}

KYC Database::getAddressKYC(const string& address) {
    LockedStatement getKYC{_stmtGetKYC};
    getKYC.bindText(1, address);
    int rc = getKYC.executeStep();
    if (rc != SQLITE_ROW) {
        return {
                address};
    }
    Blob hash = getKYC.getColumnBlob(2);
    return {
            address,
            getKYC.getColumnText(0),
            getKYC.getColumnText(1),
            hash.toHex(),
            static_cast<unsigned int>(getKYC.getColumnInt(3)),
            getKYC.getColumnInt(4)};
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
    LockedStatement addVote{_stmtAddVote};
    addVote.bindInt(1, assetIndex);
    addVote.bindText(2, address);
    addVote.bindInt(3, height);
    addVote.bindInt64(4, count);
    addVote.bindInt64(5, count);
    int rc = addVote.executeStep();
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
    vector<entries> keep;
    {
        LockedStatement getVoteCountAtHeight{_stmtGetVoteCountAtHeight};
        getVoteCountAtHeight.bindInt(1, height);
        while (getVoteCountAtHeight.executeStep() == SQLITE_ROW) {
            keep.emplace_back(entries{
                    static_cast<unsigned int>(getVoteCountAtHeight.getColumnInt(0)),
                    getVoteCountAtHeight.getColumnText(1),
                    static_cast<unsigned int>(getVoteCountAtHeight.getColumnInt(2)),
                    static_cast<unsigned int>(getVoteCountAtHeight.getColumnInt(3))});
        }
    }

    //delete from votes where height<pruneHeight
    {
        LockedStatement pruneVote{_stmtPruneVote};
        pruneVote.bindInt(1, height);
        int rc = pruneVote.executeStep();
        if (rc != SQLITE_DONE) {
            string tempErrorMessage = sqlite3_errmsg(_db);
            throw exceptionFailedDelete();
        }
    }

    //mark blocks that can't be rolled back anymore
    {
        LockedStatement removeNonReachable{_stmtRemoveNonReachable};
        removeNonReachable.bindInt(1, height);
        int rc = removeNonReachable.executeStep();
        if (rc != SQLITE_DONE) {
            string tempErrorMessage = sqlite3_errmsg(_db);
            throw exceptionFailedUpdate();
        }
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

/**
 * Returns current vote count
 * @param assetIndex
 * @return
 */
std::vector<VoteCount> Database::getVoteCounts(unsigned int assetIndex) {
    LockedStatement getVoteCount{_stmtGetVoteCount};
    getVoteCount.bindInt(1, assetIndex);
    vector<VoteCount> votes;
    while (getVoteCount.executeStep() == SQLITE_ROW) {
        votes.emplace_back(VoteCount{
                getVoteCount.getColumnText(0),
                static_cast<unsigned int>(getVoteCount.getColumnInt64(1)),
        });
    }
    return votes;
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
    LockedStatement numberOfIPFSJobs{_stmtNumberOfIPFSJobs};
    int rc = numberOfIPFSJobs.executeStep();
    if (rc != SQLITE_ROW) {
        throw exceptionFailedSelect();
    }
    return numberOfIPFSJobs.getColumnInt(0);
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
            LockedStatement clearIPFSPause{_stmtClearIPFSPause};
            clearIPFSPause.bindInt64(1, currentTime);
            int rc = clearIPFSPause.executeStep();
            if (rc != SQLITE_DONE) throw exceptionFailedUpdate();
        }
    }

    //lookup the next job(if there is one)
    uint64_t currentTime;
    uint64_t maxTime;
    string callbackSymbol;
    {
        LockedStatement getNextIPFSJob{_stmtGetNextIPFSJob};
        int rc = getNextIPFSJob.executeStep();
        if (rc != SQLITE_ROW) {
            jobIndex = 0; //signal there are no new jobs
            return;
        }
        jobIndex = getNextIPFSJob.getColumnInt(0);
        sync = getNextIPFSJob.getColumnText(1);
        cid = getNextIPFSJob.getColumnText(2);
        extra = getNextIPFSJob.getColumnText(3);
        callbackSymbol = getNextIPFSJob.getColumnText(4);

        //get max time
        currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                              std::chrono::system_clock::now().time_since_epoch())
                              .count();
        maxTime = getNextIPFSJob.getColumnInt64(5);
    }
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
    int rc;
    if ((sync == "pin") || (sync == "_") || (sync.empty())) {
        LockedStatement setIPFSLockJob{_stmtSetIPFSLockJob};
        setIPFSLockJob.bindInt(1, jobIndex);
        rc = setIPFSLockJob.executeStep();
    } else {
        LockedStatement setIPFSLockSync{_stmtSetIPFSLockSync};
        setIPFSLockSync.bindText(1, sync);
        rc = setIPFSLockSync.executeStep();
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
        LockedStatement setIPFSPauseJob{_stmtSetIPFSPauseJob};
        setIPFSPauseJob.bindInt64(1, unpauseTime);
        setIPFSPauseJob.bindInt(2, jobIndex);
        rc = setIPFSPauseJob.executeStep();
    } else {
        LockedStatement setIPFSPauseSync{_stmtSetIPFSPauseSync};
        setIPFSPauseSync.bindInt64(1, unpauseTime);
        setIPFSPauseSync.bindText(2, sync);
        rc = setIPFSPauseSync.executeStep();

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
    {
        LockedStatement clearNextIPFSJob{_stmtClearNextIPFSJob_a};
        clearNextIPFSJob.bindInt(1, jobIndex);
        int rc = clearNextIPFSJob.executeStep();
        if (rc != SQLITE_DONE) { //there should always be one
            string tempErrorMessage = sqlite3_errmsg(_db);
            throw exceptionFailedSQLCommand(); //failed to delete or unlock
        }
    }
    {
        LockedStatement clearNextIPFSJob{_stmtClearNextIPFSJob_b};
        clearNextIPFSJob.bindText(1, sync);
        int rc = clearNextIPFSJob.executeStep();
        if (rc != SQLITE_DONE) { //there should always be one
            string tempErrorMessage = sqlite3_errmsg(_db);
            throw exceptionFailedSQLCommand(); //failed to delete or unlock
        }
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
    LockedStatement insertIPFSJob{_stmtInsertIPFSJob};
    insertIPFSJob.bindText(1, sync);
    insertIPFSJob.bindText(2, cid);
    insertIPFSJob.bindText(3, extra);
    insertIPFSJob.bindText(4, callbackSymbol);
    if (pause != 0) {
        insertIPFSJob.bindInt64(5, pause);
    } else {
        insertIPFSJob.bindNull(5);
    }
    if (maxSleep == 0) {
        insertIPFSJob.bindNull(6);
    } else {
        uint64_t currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                                       std::chrono::system_clock::now().time_since_epoch())
                                       .count();
        insertIPFSJob.bindInt64(6, currentTime + maxSleep);
    }

    int rc = insertIPFSJob.executeStep();
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
    LockedStatement revokeDomain{_stmtRevokeDomain};
    revokeDomain.bindText(1, domain);
    int rc = revokeDomain.executeStep();
    if (rc != SQLITE_DONE) {
        string tempErrorMessage = sqlite3_errmsg(_db);
        throw exceptionFailedUpdate();
    }
}

void Database::addDomain(const string& domain, const string& assetId) {
    LockedStatement addDomain{_stmtAddDomain};
    addDomain.bindText(1, domain);
    addDomain.bindText(2, assetId);
    int rc = addDomain.executeStep();
    if (rc != SQLITE_DONE) {
        string tempErrorMessage = sqlite3_errmsg(_db);
        throw exceptionFailedUpdate();
    }
}

string Database::getDomainAssetId(const std::string& domain, bool returnErrorIfRevoked) {
    LockedStatement getDomainAssetId{_stmtGetDomainAssetId};
    getDomainAssetId.bindText(1, domain);
    int rc = getDomainAssetId.executeStep();

    if (rc != SQLITE_ROW) throw DigiByteDomain::exceptionUnknownDomain();
    if (returnErrorIfRevoked && (getDomainAssetId.getColumnInt(1) == true)) {
        throw DigiByteDomain::exceptionRevokedDomain();
    }
    return getDomainAssetId.getColumnText(0);
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
    {
        LockedStatement setDomainMasterAssetId{_stmtSetDomainMasterAssetId_a};
        string lastDomain = _masterDomainAssetId.back();
        setDomainMasterAssetId.bindText(1, lastDomain);
        int rc = setDomainMasterAssetId.executeStep();
        if (rc != SQLITE_DONE) {
            string tempErrorMessage = sqlite3_errmsg(_db);
            throw exceptionFailedUpdate();
        }
    }
    LockedStatement setDomainMasterAssetId{_stmtSetDomainMasterAssetId_b};
    setDomainMasterAssetId.bindText(1, assetId);
    int rc = setDomainMasterAssetId.executeStep();
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

std::string Database::getDomainAddress(const string& domain) {
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
    {
        LockedStatement repinAssets{_stmtRepinAssets};
        int rc = repinAssets.executeStep();
        if (rc != SQLITE_DONE) {
            string tempErrorMessage = sqlite3_errmsg(_db);
            throw exceptionFailedInsert();
        }
    }

    //repin all files that are part of specific pool
    LockedStatement repinPermanentSpecific{_stmtRepinPermanentSpecific};
    repinPermanentSpecific.bindInt(1, poolIndex);
    int rc = repinPermanentSpecific.executeStep();
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


    int rc = sqlite3_exec(_db, sql.c_str(), Database::defaultCallback, nullptr, nullptr);
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
    LockedStatement insertPermanent{_stmtInsertPermanent};
    insertPermanent.bindText(1, cid);
    insertPermanent.bindInt(2, poolIndex);
    int rc = insertPermanent.executeStep();
    if (rc != SQLITE_DONE) {
        string tempErrorMessage = sqlite3_errmsg(_db);
        throw exceptionFailedInsert();
    }
}

void Database::removeFromPermanent(unsigned int poolIndex, const std::string& cid, bool unpin) {
    //remove from database
    {
        LockedStatement deletePermanent{_stmtDeletePermanent};
        deletePermanent.bindText(1, cid);
        deletePermanent.bindInt(2, poolIndex);
        deletePermanent.executeStep();
    }

    //unpin if not in database anymore
    if (!unpin) return;

    //see if asset still present
    LockedStatement isInPermanent{_stmtIsInPermanent};
    isInPermanent.bindText(1, cid);
    if (isInPermanent.executeStep() == SQLITE_ROW) return;

    //add asset to be unpinned
    addIPFSJob(cid, "unpin");
}

void Database::addAssetToPool(unsigned int poolIndex, unsigned int assetIndex) {
    LockedStatement addAssetToPool{_stmtAddAssetToPool};
    addAssetToPool.bindInt(1, assetIndex);
    addAssetToPool.bindInt(2, poolIndex);

    if (addAssetToPool.executeStep() != SQLITE_DONE) {
        throw exceptionFailedInsert();
    }
}

void Database::removeAssetFromPool(unsigned int poolIndex, const std::string& assetId, bool unpin) {
    //make list of assetIndex we will remove and there cid
    vector<pair<unsigned int, string>> toDelete;
    {
        LockedStatement pspFindBadAsset{_stmtPSPFindBadAsset};
        pspFindBadAsset.bindText(1, assetId);
        pspFindBadAsset.bindInt(2, poolIndex);
        while (pspFindBadAsset.executeStep() == SQLITE_ROW) {
            unsigned int assetIndex = pspFindBadAsset.getColumnInt(0);
            string cid = pspFindBadAsset.getColumnText(1);
            toDelete.emplace_back(make_pair(assetIndex, cid));
        }
    }

    //remove from database
    {
        LockedStatement pspDeleteBadAsset{_stmtPSPDeleteBadAsset};
        pspDeleteBadAsset.bindText(1, assetId);
        pspDeleteBadAsset.bindInt(2, poolIndex);
        pspDeleteBadAsset.executeStep();
    }

    //unpin if not in database anymore
    if (!unpin) return;
    for (auto pair: toDelete) {
        //see if asset still present
        unsigned int assetIndex = pair.first;
        LockedStatement isAssetInAPool{_stmtIsAssetInAPool};
        isAssetInAPool.bindInt(1, assetIndex);
        if (isAssetInAPool.executeStep() == SQLITE_ROW) continue;

        //add asset to be unpinned
        addIPFSJob(pair.second, "unpin");
    }
}

bool Database::isAssetInPool(unsigned int poolIndex, unsigned int assetIndex) {
    LockedStatement isAssetInPool{_stmtIsAssetInPool};
    isAssetInPool.bindInt(1, assetIndex);
    isAssetInPool.bindInt(2, poolIndex);

    return (isAssetInPool.executeStep() == SQLITE_ROW);
}
bool Database::isAssetInPool(unsigned int assetIndex) {
    LockedStatement isAssetInPool{_stmtIsAssetInAPool};
    isAssetInPool.bindInt(1, assetIndex);

    return (isAssetInPool.executeStep() == SQLITE_ROW);
}
std::vector<int> Database::listPoolsAssetIsIn(unsigned int assetIndex) {
    vector<int> pools;
    LockedStatement isAssetInPool{_stmtIsAssetInAPool};
    isAssetInPool.bindInt(1, assetIndex);

    while (isAssetInPool.executeStep() == SQLITE_ROW) {
        pools.emplace_back(isAssetInPool.getColumnInt(0));
    }
    return pools;
}
std::vector<std::string> Database::getPSPFileList(unsigned int poolIndex) {
    LockedStatement fileLister{_stmtPSPFileList};
    fileLister.bindInt(1, poolIndex);
    vector<string> results;
    while (fileLister.executeStep() == SQLITE_ROW) {
        results.push_back(fileLister.getColumnText(0));
    }
    return results;
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
    rc = sqlite3_prepare_v2(_db, sql.c_str(), -1, &stmt, nullptr);
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
            "COUNT(DISPERSED u.address) AS num_addresses_with_assets "
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
            "COUNT(DISPERSED u.address) AS num_addresses_created "
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
            "COUNT(DISPERSED u.address) AS num_addresses_used "
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
            "COUNT(DISPERSED u.address) AS total_addresses "
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
            "COUNT(DISPERSED u.address) AS num_quantum_unsafe "
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
    if (sqlite3_prepare_v2(_db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
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
    if (sqlite3_prepare_v2(_db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
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
    rc = sqlite3_prepare_v2(_db, query.c_str(), -1, &stmt, nullptr);
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

/**
 * Checks if index exists.
 * Not prepared because this gets executed during the preperation cycle
 * @param indexName
 * @return
 */
bool Database::indexExists(const string& indexName) {
    std::string sql = "SELECT name FROM sqlite_master WHERE type='index' AND name=?";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(_db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(_db) << std::endl;
        throw Database::exceptionCreatingStatement();
    }

    sqlite3_bind_text(stmt, 1, indexName.c_str(), -1, SQLITE_TRANSIENT);

    bool exists = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        exists = true; // Index exists
    }

    sqlite3_finalize(stmt);
    return exists;
}
void Database::executePerformanceIndex(int& state) {
    //find an index that needs adding or exit if none to add
    PerformanceIndex index;
    do {
        if (_performanceIndexes.empty()) return; //check if we have emptied the array
        state = ChainAnalyzer::OPTIMIZING;
        index = _performanceIndexes.back(); //get last element
        _performanceIndexes.pop_back();     //remove it
    } while (indexExists(index.name));

    //add the index
    Log* log = Log::GetInstance();
    log->addMessage("Creating performance index " + index.name);
    int rc = sqlite3_exec(_db, index.command.c_str(), nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK) {
        _performanceIndexes.emplace_back(index); //couldn't execute so push it back on to be done later
    }
    log->addMessage("Finished creating performance index " + index.name);
}
