//
// Created by mctrivia on 30/01/23.
//


#include "Database.h"
#include "AppMain.h"
#include "Blob.h"
#include "DigiAsset.h"
#include "DigiByteDomain.h"
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
                char* zErrMsg = 0;
                int rc;
                const char* sql =
                        //chain data tables
                        "BEGIN TRANSACTION;"
                        "CREATE TABLE \"permanent\" (\"cid\" TEXT NOT NUll UNIQUE);"
                        "UPDATE \"flags\" set \"value\"=2 WHERE \"key\"=\"dbVersion\";"
                        "COMMIT;";

                rc = sqlite3_exec(_db, sql, Database::defaultCallback, 0, &zErrMsg);

                if (rc != SQLITE_OK) {
                    sqlite3_free(zErrMsg);
                    throw exceptionFailedToCreateTable();
                }
            },


            //Define what is changed from version 2 to version 3
            [&]() {
                char* zErrMsg = 0;
                int rc;

                //alter blocks table
                const char* sql =
                        //chain data tables
                        "BEGIN TRANSACTION;"
                        "ALTER TABLE blocks ADD COLUMN time INTEGER NOT NULL DEFAULT 0;"
                        "ALTER TABLE blocks ADD COLUMN algo INTEGER NOT NULL DEFAULT 0;"
                        "ALTER TABLE blocks ADD COLUMN difficulty REAL NOT NULL DEFAULT 0;"
                        "UPDATE \"blocks\" set time=1389392876, algo=1, difficulty=0.000244140625 WHERE height=1;"
                        "UPDATE \"flags\" set \"value\"=3 WHERE \"key\"=\"dbVersion\";"
                        "COMMIT;";
                rc = sqlite3_exec(_db, sql, Database::defaultCallback, 0, &zErrMsg);
                if (rc != SQLITE_OK) {
                    sqlite3_free(zErrMsg);
                    throw exceptionFailedToCreateTable();
                }
                if (dbVersionNumber == 0) return; //no data in tables yet so no need to rebuild

                //reconstruct old blocks entries
                DigiByteCore* dgb = AppMain::GetInstance()->getDigiByteCore();

                //re go over all old blocks and fill in missing data
                sqlite3_stmt* stmt;
                const char* sqlSelect = "SELECT hash FROM blocks";
                if (sqlite3_prepare_v2(_db, sqlSelect, -1, &stmt, 0) != SQLITE_OK) throw exceptionCreatingStatement();

                const char* sqlUpdate = "UPDATE blocks SET time = ?, algo = ?, difficulty = ? WHERE hash = ?";
                sqlite3_stmt* updateStmt;
                if (sqlite3_prepare_v2(_db, sqlUpdate, -1, &updateStmt, 0) != SQLITE_OK) throw exceptionCreatingStatement();

                while (executeSqliteStepWithRetry(stmt) == SQLITE_ROW) {
                    sqlite3_reset(updateStmt);

                    //get hash of block
                    const void* hashBlob = sqlite3_column_blob(stmt, 0);
                    Blob hash = Blob(hashBlob, SHA256_LENGTH);

                    //lookup block on chain
                    blockinfo_t blockInfo = dgb->getBlock(hash.toHex());

                    //update row
                    sqlite3_bind_int(updateStmt, 1, blockInfo.time);
                    sqlite3_bind_int(updateStmt, 2, blockInfo.algo);
                    sqlite3_bind_double(updateStmt, 3, blockInfo.difficulty);
                    sqlite3_bind_blob(updateStmt, 4, hashBlob, SHA256_LENGTH, SQLITE_STATIC);
                    if (executeSqliteStepWithRetry(updateStmt) != SQLITE_DONE) throw exceptionFailedUpdate();
                }

                //finalize statements
                sqlite3_finalize(updateStmt);
                sqlite3_finalize(stmt);
            },


            //Define what is changed from version 3 to version 4
            [&]() {
                char* zErrMsg = 0;
                int rc;
                const char* sql =
                        //chain data tables
                        "BEGIN TRANSACTION;"
                        "CREATE TABLE \"pspFiles\" (\"cid\" TEXT NOT NULL,\"poolIndex\" INT NOT NUll DEFAULT 1);"
                        "INSERT INTO \"pspFiles\" (\"cid\") SELECT \"cid\" FROM \"permanent\";"
                        "CREATE TABLE \"pspAssets\" (\"assetIndex\" INT NOT NUll,\"poolIndex\" INT NOT NUll);"
                        "UPDATE \"flags\" set \"value\"=4 WHERE \"key\"=\"dbVersion\";"


                        "INSERT INTO \"domains\" VALUES (\"renzo.dgb\",\"La3dReY7GbDmeuzoQ94a9PQ37EXaRTzcdZB4Wr\",0);"
                        "INSERT INTO \"domains\" VALUES (\"mctrivia.dgb\",\"La8TuneLWXoTXG57F6qBECAekPZe48DegjfgWQ\",0);"
                        "INSERT INTO \"domains\" VALUES (\"digiassetx.dgb\",\"La6qyvPXjZneTMWxNPiYiX5Aqqdsk3KEFsH3Ho\",0);"
                        "INSERT INTO \"domains\" VALUES (\"johnnylaw.dgb\",\"La9KwXmfeGQVebob7AbtEUJ9SftW8pXT9RudtT\",0);"
                        "INSERT INTO \"domains\" VALUES (\"debo.dgb\",\"La9T1eoLSSpUggFARjMm6kZzed97caaLh7NxPC\",0);"
                        "INSERT INTO \"domains\" VALUES (\"digizoo.dgb\",\"La4n1egqSQpSfci61h7CHRBcYBVgDmcxJQi4Ep\",0);"
                        "INSERT INTO \"domains\" VALUES (\"fortunet.dgb\",\"La5oLAo3uTVpwPhfU1CVmuHUYRux9P3t1Np1Hp\",0);"
                        "INSERT INTO \"domains\" VALUES (\"finance.dgb\",\"La6ehsDAMMosV5G1R1DXxpDsXzvj3NToZavWoV\",0);"
                        "INSERT INTO \"domains\" VALUES (\"gotti.dgb\",\"La4XwycWVuT6pxgy1VUAVSRer4aPUGNpwizHvi\",0);"
                        "INSERT INTO \"domains\" VALUES (\"dgbnoob.dgb\",\"La83Pzvcgj91TwGan3BXpWv8YvJ78MbivNg9ZN\",0);"
                        "INSERT INTO \"domains\" VALUES (\"masterofleadership.dgb\",\"La6Hoe8wvshoatZvML7j5Q5mubzSwVisgcmFmq\",0);"
                        "INSERT INTO \"domains\" VALUES (\"olly.dgb\",\"La4pGodMC2Wi7oG8uwCXex4R7Zd7TTC4Z3TBZN\",0);"
                        "INSERT INTO \"domains\" VALUES (\"punk.dgb\",\"La8WhvcrgwF4VCKrndLMxgvrTBzGNgZLv7SHTG\",0);"
                        "INSERT INTO \"domains\" VALUES (\"saltedlolly.dgb\",\"La7eRbuFHCwVpSCCdGCg2aD4RgmPozuCNRcFwW\",0);"
                        "INSERT INTO \"domains\" VALUES (\"gonzoboards.dgb\",\"La8GzsvTh3xRxjgBcFuCusXekMBymVjNGwZcJf\",0);"
                        "INSERT INTO \"domains\" VALUES (\"bmess181.dgb\",\"LaA5sR6hbdpHeePPqk441VJE8jck35vciDbrzn\",0);"
                        "INSERT INTO \"domains\" VALUES (\"donate.dgb\",\"La4v2uEhvU9BKjqMoyu8pUAKShNyNq4ji9Egxm\",0);"
                        "INSERT INTO \"domains\" VALUES (\"help.dgb\",\"La39Xnve13iTuRYvNP3jjL146dQBbVY72FiFB7\",0);"
                        "INSERT INTO \"domains\" VALUES (\"jack.dgb\",\"La833SeU66nwuug2HQqBNSK7DDU3uNU8QtvGRM\",0);"
                        "INSERT INTO \"domains\" VALUES (\"guywrizzle.dgb\",\"La5yqjQwtAGGGV5pt5cobMfyNedREZMtbB5fSH\",0);"
                        "INSERT INTO \"domains\" VALUES (\"wallet.dgb\",\"La7xF2MyQuB3qB4Mju9sdquqy9K6V39bKDu57k\",0);"
                        "INSERT INTO \"domains\" VALUES (\"bob.dgb\",\"La5vowqk17c2wx6s8J3b3zf5d6qwyY8E9PkAEJ\",0);"
                        "INSERT INTO \"domains\" VALUES (\"games.dgb\",\"La67JBjVckQvuGgecx6V9eJXc8br8uuPyApLU6\",0);"
                        "INSERT INTO \"domains\" VALUES (\"elon.dgb\",\"LaA77c9byU24vTYGduKSVm16i8WGtobAFijD3R\",0);"
                        "INSERT INTO \"domains\" VALUES (\"diginode.dgb\",\"La5ccco3J3uXTU4dNVsM2cHrscscKSP9oVZeUE\",0);"
                        "INSERT INTO \"domains\" VALUES (\"gold.dgb\",\"La7Ps18NiDFUu4RHRRwrRV6pupErJtpQaAo4v4\",0);"
                        "INSERT INTO \"domains\" VALUES (\"crypto.dgb\",\"LaAGtxnUNMpPMh9MMVSyaeBSLcNkhphZsgstvm\",0);"
                        "INSERT INTO \"domains\" VALUES (\"newkid.dgb\",\"La9TdVvmRnWvKaVDTN5XxgH4f5T4mgCvZvZo1R\",0);"
                        "INSERT INTO \"domains\" VALUES (\"metaverse.dgb\",\"La3r8YchjuDcUPoLeAvfMn13FznV9oW8DmFwhV\",0);"
                        "INSERT INTO \"domains\" VALUES (\"porn.dgb\",\"La9K8ZJ188FBuaPrAnkLRgUcRbrw9GCem8oeCy\",0);"
                        "INSERT INTO \"domains\" VALUES (\"shop.dgb\",\"La9ZKD2yR3WE7u1jdgJfF4ztnsGfL62mTZdzaY\",0);"
                        "INSERT INTO \"domains\" VALUES (\"420.dgb\",\"La4u6vr9o6wfhopchKTiXhUVMxk2zE23a9ZRTg\",0);"
                        "INSERT INTO \"domains\" VALUES (\"102.dgb\",\"La615nTPzdS77mWf1FWUDsbZXtuphF77afFZEf\",0);"
                        "INSERT INTO \"domains\" VALUES (\"defi.dgb\",\"La4LDZGbjAZTBA5LTvZRU2xQb67XFczt33DJed\",0);"
                        "INSERT INTO \"domains\" VALUES (\"sendme.dgb\",\"La7Su3h957EQL1uiQgZVqv7fLrThuuhuxqfFai\",0);"
                        "INSERT INTO \"domains\" VALUES (\"batman.dgb\",\"La6MmSSi6wWXgxQv1nSmqu5K3Rg23ENhuxwSYG\",0);"
                        "INSERT INTO \"domains\" VALUES (\"apple.dgb\",\"La83TJW3REVxhYGaZh4CXSQc9psPUPdkEsJCcU\",0);"
                        "INSERT INTO \"domains\" VALUES (\"superman.dgb\",\"La3Vkc19M7sxe9jtqNPNdJZ2uj314S15xxi5Yg\",0);"
                        "INSERT INTO \"domains\" VALUES (\"trade.dgb\",\"La4Q4QtAuPPdq5jTy6wbT1eFTbd4h3SooSPimt\",0);"
                        "INSERT INTO \"domains\" VALUES (\"123.dgb\",\"La98SiQvoBD6LWaKxQCgTkUV2V2v6CKMdkhLh3\",0);"
                        "INSERT INTO \"domains\" VALUES (\"messner.dgb\",\"La6f76JFAu9R5e5VQZScWgAgu1EL7sRw311qYu\",0);"
                        "INSERT INTO \"domains\" VALUES (\"coke.dgb\",\"La8gr1aTjH7QaWT1RwqWCxNdo7RFWkpivBmcMK\",0);"
                        "INSERT INTO \"domains\" VALUES (\"moon.dgb\",\"La2xnfycChdJqGapENSpvBSCzPkiNa1Egmh76K\",0);"
                        "INSERT INTO \"domains\" VALUES (\"pay.dgb\",\"La5bJfqF3saNrjJp1KjfTFcBYirZfrUTqeNyKz\",0);"
                        "INSERT INTO \"domains\" VALUES (\"get.dgb\",\"La6pHoivkJP3VTjjV5PaLkJFJ1okRov9BMRaon\",0);"
                        "INSERT INTO \"domains\" VALUES (\"usa.dgb\",\"La8Me5D3yWBcxfaLz9CdgDf8T8NhgEdGJ75Nfo\",0);"
                        "INSERT INTO \"domains\" VALUES (\"china.dgb\",\"La4Axp6TB9KKDeYxXJVLRHtGQoqAy58si1133V\",0);"
                        "INSERT INTO \"domains\" VALUES (\"tesla.dgb\",\"La3udrrWWp6nLhkc9MJ2M5c1zr1X55ULGhNSFV\",0);"
                        "INSERT INTO \"domains\" VALUES (\"dubai.dgb\",\"La4ZKejGA2sFYDnxpqFtjoFN6jFNk1J3tnW6hm\",0);"
                        "INSERT INTO \"domains\" VALUES (\"buy.dgb\",\"La553PTFXhmHK7VBWe19SvzBhYx2eMjgDJZTPE\",0);"
                        "INSERT INTO \"domains\" VALUES (\"sell.dgb\",\"La369QB2HQqDCC7WEwpHiB7Cd4DUGF9ZBUoaHV\",0);"
                        "INSERT INTO \"domains\" VALUES (\"p2p.dgb\",\"La37cTWRg5ctGBGcvBxmAWULoRPiFZyiLpziUm\",0);"
                        "INSERT INTO \"domains\" VALUES (\"japan.dgb\",\"LaA9kygQpRriEhatvniqVMby4vTxQYeKZKhmir\",0);"
                        "INSERT INTO \"domains\" VALUES (\"art.dgb\",\"La7EX6ykEyrfNkziQaHuYGt3FNnFzqRCx5Qj9M\",0);"
                        "INSERT INTO \"domains\" VALUES (\"tom.dgb\",\"La4wHPMJahiTMD2ysbapkDezV43w8jasA3kGwn\",0);"
                        "INSERT INTO \"domains\" VALUES (\"king.dgb\",\"La3gE2ESea1VXmkQhnkEVEhAZ9eEYxqWRjttkJ\",0);"
                        "INSERT INTO \"domains\" VALUES (\"noel.dgb\",\"La7C2mNNJeVnr2C5Gt54Kbs36E4aXmBaCLWr3H\",0);"
                        "INSERT INTO \"domains\" VALUES (\"love.dgb\",\"La4fYYwyDLyGgNCsCmdSfBUToB2zoaeRPdhA6g\",0);"
                        "INSERT INTO \"domains\" VALUES (\"catholic.dgb\",\"La6PZiC8m8q2bbwCW45wsaj63tA1zLbeNnwkva\",0);"
                        "INSERT INTO \"domains\" VALUES (\"coffee.dgb\",\"La5BbEymkv4iQia9bptLfCQTVRjdKn4jyz5oGx\",0);"
                        "INSERT INTO \"domains\" VALUES (\"eat.dgb\",\"La9mEPTtokmvUyeCT18R9iTfTv1wDzH6mzWFDi\",0);"
                        "INSERT INTO \"domains\" VALUES (\"god.dgb\",\"LaAHUdqG3HwHWSTmS5t4xmH4HxwUTzuuXmMTsj\",0);"
                        "INSERT INTO \"domains\" VALUES (\"bank.dgb\",\"La8LoKJ99sE5PSFZCCjFGAsmk6kuE5ffM71aGH\",0);"
                        "INSERT INTO \"domains\" VALUES (\"hodl.dgb\",\"La7618dmS17LmrQ9TPthCgBZ1ZqeWr1vhgcWQp\",0);"
                        "INSERT INTO \"domains\" VALUES (\"bet.dgb\",\"La3fNL1iRPxtMTmTXK5iVhpEkCEnWxAjz3DW8N\",0);"
                        "INSERT INTO \"domains\" VALUES (\"001.dgb\",\"LaA6Fc4r6E5cmqTwNMcyTcp84PqL9Ub15w6rRA\",0);"
                        "INSERT INTO \"domains\" VALUES (\"tim.dgb\",\"La6kEYuyqZdpPrd2yie1H9n1BNaQ8E1wVqHAWt\",0);"
                        "INSERT INTO \"domains\" VALUES (\"sam.dgb\",\"La74fVDuu6pLzRLFdmWyLTpK4xmYfhuHwh3Ejk\",0);"
                        "INSERT INTO \"domains\" VALUES (\"swap.dgb\",\"La8VgM55yvRioUytKxpiHf6S84utPCj2wvSeKW\",0);"
                        "INSERT INTO \"domains\" VALUES (\"send.dgb\",\"La68D38zXguqgNxTVcpT4oFzURgyEFbepqbVwc\",0);"
                        "INSERT INTO \"domains\" VALUES (\"vote.dgb\",\"La3Dykh3Suozk79prQDm7dU8tU9pBFpZhzkifD\",0);"
                        "INSERT INTO \"domains\" VALUES (\"rancell.dgb\",\"La5S3xNFiN2ktKRmLamjYSfRczVcsitxB8xc9S\",0);"
                        "INSERT INTO \"domains\" VALUES (\"zoo.dgb\",\"La3Vv3SVwbwuEDM9kWP2BKMEeC5egQ1ceuMS2Z\",0);"
                        "INSERT INTO \"domains\" VALUES (\"fingyz.dgb\",\"La7UZBccYBcih5H5kcTV7a8JNrACShmuPiLkci\",0);"
                        "INSERT INTO \"domains\" VALUES (\"lgw.dgb\",\"La4PN8dZTqMnGcZU5oKLYHEWqRnFs7gh75EWPN\",0);"
                        "INSERT INTO \"domains\" VALUES (\"johnny.dgb\",\"La5wGWLFUMj1FKoXeMyRoqoLHqX1uynbRfGVLH\",0);"
                        "INSERT INTO \"domains\" VALUES (\"messer.dgb\",\"La9zXK2sE9jeSm1bjX1yvc9wUUaAkZAQMiKnJT\",0);"
                        "INSERT INTO \"domains\" VALUES (\"btc.dgb\",\"La7ZbekPHpPnENeMdx3kC2pWErnPiNePPPP22f\",0);"
                        "INSERT INTO \"domains\" VALUES (\"000.dgb\",\"La32AGmSXPYQkUtwECvU9mpPrdKR9wHERsTEg7\",0);"
                        "INSERT INTO \"domains\" VALUES (\"abc.dgb\",\"La7tBfDAhF8itDjPj5ctRmsnX29nqmXKELzzmE\",0);"
                        "INSERT INTO \"domains\" VALUES (\"google.dgb\",\"LaASBNZqKmbchjHucLjbrQSTmsLgSnURaTT8DL\",0);"
                        "INSERT INTO \"domains\" VALUES (\"amazon.dgb\",\"La6yKknHqVH6snAKWhJyEhcxbn6mNepnQfsTrM\",0);"
                        "INSERT INTO \"domains\" VALUES (\"unforgiven.dgb\",\"La5RMk2Z2BHVYP4C3oDqw4qmrYsxWTbqAuzWmS\",0);"
                        "INSERT INTO \"domains\" VALUES (\"666.dgb\",\"La2sUG62FvKCUy17y1HKHJoaAD1jPiLVGvmvTH\",0);"
                        "INSERT INTO \"domains\" VALUES (\"galatasaray.dgb\",\"La9xv9ntSv5UE8GeNnAcpZNdRi6APTJp8LABwS\",0);"
                        "INSERT INTO \"domains\" VALUES (\"n0tsat0shi.dgb\",\"La5niCoHaHGPQxV81Dz9aQ74UfczjMZkNtFcdu\",0);"
                        "INSERT INTO \"domains\" VALUES (\"fenergy.dgb\",\"La8HM5NL4LCdgMh8Vg7oj7eKM2yAPTHH1Bssf5\",0);"
                        "INSERT INTO \"domains\" VALUES (\"john.dgb\",\"La5dvy61zBEg8S8i2a1P3wos26nTkjxDHjqTvW\",0);"
                        "INSERT INTO \"domains\" VALUES (\"nft.dgb\",\"La481VkuQs26LAa6ZiDXBdxoHMewCTE4czhUBP\",0);"
                        "INSERT INTO \"domains\" VALUES (\"carolina.dgb\",\"La8sTRU7BZCNfRVGcjo4XYGZEuBvR6hNVqshaS\",0);"
                        "INSERT INTO \"domains\" VALUES (\"digibyterecords.dgb\",\"La6ivnuVs85YvEDG2wuaraPEwNvmowprQe5faA\",0);"
                        "INSERT INTO \"domains\" VALUES (\"digifaucet.dgb\",\"La4Qh9PxCjTGzTUoRjTkRnzCSYPCeLkPVZeYja\",0);"
                        "INSERT INTO \"domains\" VALUES (\"weber.dgb\",\"LaAKtxFcddA4pQXi23RsT6XxYNvRojKz82Ayh8\",0);"
                        "INSERT INTO \"domains\" VALUES (\"dev.dgb\",\"La8UmGcvHJNEucYpChRXaqP1YVHusRH9yHZbEv\",0);"
                        "INSERT INTO \"domains\" VALUES (\"majesticjay.dgb\",\"LaAMtmZfXjVdNiYGT2is7xW3oTVn3tczAnCo9u\",0);"
                        "INSERT INTO \"domains\" VALUES (\"ape.dgb\",\"LaA5ffq1dSmREjxpMsCHVubeS78vq7TMDfsC5j\",0);"
                        "INSERT INTO \"domains\" VALUES (\"sex.dgb\",\"La6XP4PGpfzTXWdjYRnGHNzWrEUMPZG8yaGjLf\",0);"
                        "INSERT INTO \"domains\" VALUES (\"game.dgb\",\"La8JNUuCei6F6sC3X6SFb4LCCuSAb2YSVPukpD\",0);"
                        "INSERT INTO \"domains\" VALUES (\"play.dgb\",\"La8JwN9d2JdPwUSjw8VAbZqBicjV6PA6XeTHC6\",0);"
                        "INSERT INTO \"domains\" VALUES (\"nfts.dgb\",\"La3RgeqdbJLHZ8wrQRRFLM94qEeUjYvk5N2Hvm\",0);"
                        "INSERT INTO \"domains\" VALUES (\"mike.dgb\",\"La6zWkgqYzoQneCEgqHeyeXmkRsAsjFyaqDaPB\",0);"
                        "INSERT INTO \"domains\" VALUES (\"dgb1q5rtsqmcw9dvfxcfj9pr84x5tnvpy82pjqdr6q6.dgb\",\"La3qh6mVw8NgPVh93zMwt4Suuk22KreyTMuGZ8\",0);"
                        "INSERT INTO \"domains\" VALUES (\"sky.dgb\",\"La4KSxpFdqYsjswWSrVjTxweJ7vzZZZ7fxtBDE\",0);"
                        "INSERT INTO \"domains\" VALUES (\"meta.dgb\",\"LaABzBkf1sFZ6fTbLx3sE4HVLV9CmD1aKHpBJX\",0);"
                        "INSERT INTO \"domains\" VALUES (\"pie.dgb\",\"La6aSzNu24hPp7r8LRUAwvbNBHjMTmKt1cBm2Y\",0);"
                        "INSERT INTO \"domains\" VALUES (\"dot.dgb\",\"La3qay6vTzvHuLBAxv2dgXugavxABwTBfzdc61\",0);"
                        "INSERT INTO \"domains\" VALUES (\"news.dgb\",\"La9BJs2RikyhcFmZTmmpGuFMmUq6yv2Patw3J1\",0);"
                        "INSERT INTO \"domains\" VALUES (\"frank.dgb\",\"La9V3J8ftobtazbUxK3eXvym3C4wvkchdgbfG5\",0);"
                        "INSERT INTO \"domains\" VALUES (\"nicki.dgb\",\"La8NUX3pxsjmBTnUeXoXyffcjDHDHVWzAUodfE\",0);"
                        "INSERT INTO \"domains\" VALUES (\"sweber.dgb\",\"La3zxYNUYzSMcnuSuApJov1qD9TRBJ9Ph2Ve4a\",0);"
                        "INSERT INTO \"domains\" VALUES (\"leonidas.dgb\",\"La8CZpvyzxMD3s81Do73ZzbHd8fZGREahBkkZu\",0);"
                        "INSERT INTO \"domains\" VALUES (\"mark.dgb\",\"La3dpUou7qqgFHqJhBH6mAG1deKEdSWWtsJZha\",0);"
                        "INSERT INTO \"domains\" VALUES (\"laura.dgb\",\"La3VtFWihD9N4EpHk7DZ6SoVqAyFoCpz6ra79U\",0);"
                        "INSERT INTO \"domains\" VALUES (\"ltlovesdigi.dgb\",\"La6wPAw4uKALVbj2adv8oTGH3s74KLDD4wLQo9\",0);"
                        "INSERT INTO \"domains\" VALUES (\"yoshi.dgb\",\"La8R9aGqzPGy1ND3miuAxyF9H9rUaGNFBd4fs5\",0);"
                        "INSERT INTO \"domains\" VALUES (\"shane.dgb\",\"La7iJqodnhBdQKXPKKe8xZRp3n8tLE5ddW6UeQ\",0);"
                        "INSERT INTO \"domains\" VALUES (\"digilover.dgb\",\"La9q8SNNf1KGeAXR3nGg1uWHzY1eNFmRG6Sck3\",0);"
                        "INSERT INTO \"domains\" VALUES (\"khan.dgb\",\"La8MRiCyAd8bLSShL4qDsrH4bzKtzfD6aZViZV\",0);"
                        "INSERT INTO \"domains\" VALUES (\"jenni.dgb\",\"La9YRVT7arzEXhSdTvoSCfTRccakG77voHYQPZ\",0);"
                        "INSERT INTO \"domains\" VALUES (\"thejudge.dgb\",\"La92tx9wZkaZJyTaSHjn7BdWK6HR6hdGvBxSFE\",0);"
                        "INSERT INTO \"domains\" VALUES (\"bmess.dgb\",\"La9cWaew4PmbGKRDUfAkfJETcrF17CE5epA4ba\",0);"
                        "INSERT INTO \"domains\" VALUES (\"one.dgb\",\"La4Q34LqA8PSia35ULA45ZtmZiuMVstdHuUvLe\",0);"
                        "INSERT INTO \"domains\" VALUES (\"amoreloveamor.dgb\",\"La6PJsxhUdoBcYQRMC9jhmfbDDwHbh3VeTGPdq\",0);"
                        "INSERT INTO \"domains\" VALUES (\"travelmoore.dgb\",\"La3j1MLKrpCSe6c1Ng9QboW6d3JyMsm1GtRxGg\",0);"
                        "INSERT INTO \"domains\" VALUES (\"michael.dgb\",\"La63o18rGQnVrkTwb9xSHTc6aW2HLSKKjCfUs3\",0);"
                        "INSERT INTO \"domains\" VALUES (\"michaeltmoore.dgb\",\"LaAR18DrM2TborVJVdsCN3vVrULDxgPDnPNXET\",0);"
                        "INSERT INTO \"domains\" VALUES (\"tavon.dgb\",\"La3QbKZLxAEZE4SKVwGdjDe4LxrAvAJBWBTU9n\",0);"
                        "INSERT INTO \"domains\" VALUES (\"umes.dgb\",\"La5Mo2843NDBtSSQkzYnoCPsvxL8HA6jUW7Zud\",0);"
                        "INSERT INTO \"domains\" VALUES (\"ewhsclassof2004.dgb\",\"La7EqVLHAn13bUYC5hm9a2YX8xfEYK5YC8SCzh\",0);"
                        "INSERT INTO \"domains\" VALUES (\"nfl.dgb\",\"La75YPCd8Vnf2dgQELEnoTGLg6sbLxZLrAb3Nn\",0);"
                        "INSERT INTO \"domains\" VALUES (\"nba.dgb\",\"La3RDL6bzdTvDhDBPkTcGTnT9J88voYqkyySLB\",0);"
                        "INSERT INTO \"domains\" VALUES (\"nascar.dgb\",\"La8rNoQtD9XKHf3JqPBEy21QwAsDyfrjF4oo2c\",0);"
                        "INSERT INTO \"domains\" VALUES (\"100.dgb\",\"La95cRN3PXhA9NrW23Jq8GtWGe4tbrbNhE2QdR\",0);"
                        "INSERT INTO \"domains\" VALUES (\"ronaldo.dgb\",\"La8PWQoWnFd3dTqPYHG6ZSdFB2acgyXgy1prZA\",0);"
                        "INSERT INTO \"domains\" VALUES (\"realmadrid.dgb\",\"La6VmzTqExSkvFFnhLFpiufCUMtSji7Bpq7t64\",0);"
                        "INSERT INTO \"domains\" VALUES (\"clintwestwood.dgb\",\"La8CbfK1jgzqB7z7PGC7MLiGnC9W2ysnSgWKZA\",0);"
                        "INSERT INTO \"domains\" VALUES (\"369.dgb\",\"La73QC5TZTAKFdyALwKWX1QeRQTdCAstGGHuco\",0);"
                        "INSERT INTO \"domains\" VALUES (\"777.dgb\",\"La9YAF8xHDNnzjQrE7kkpisVwJuPmn1MwNhrSX\",0);"
                        "INSERT INTO \"domains\" VALUES (\"888.dgb\",\"La74YPABWiB18zYc2Yw1UhtEFikiNddE3PS8qC\",0);"
                        "INSERT INTO \"domains\" VALUES (\"payme.dgb\",\"La8wbDjh4VD7Tcisry5QqrvK8yyuM5AXR3NgF7\",0);"
                        "INSERT INTO \"domains\" VALUES (\"paywith.dgb\",\"La64mThn8ARuMqnokueDkP2ugbNgdBNqauoyLe\",0);"
                        "INSERT INTO \"domains\" VALUES (\"utxo.dgb\",\"La4TzQSMNGZvJ286sbC192VkJPBHYoNF1LAyeq\",0);"
                        "INSERT INTO \"domains\" VALUES (\"development.dgb\",\"La5vy1dMkDwG8hXYRZU54ftJEF7Y5p64MgPqVZ\",0);"
                        "INSERT INTO \"domains\" VALUES (\"mywallet.dgb\",\"LaA8XnQEArTUt5PvirNBguE9VurQ38wMsKyoVg\",0);"
                        "INSERT INTO \"domains\" VALUES (\"payment.dgb\",\"La9u5Eu4iJZoAXbniaUA1xUzvaxRrruKvhgic3\",0);"
                        "INSERT INTO \"domains\" VALUES (\"jevvus1.dgb\",\"La6F2pVePkQZZcPukrLgqi6pvtqnjvY5mx4pjN\",0);"
                        "INSERT INTO \"domains\" VALUES (\"jqbuddy.dgb\",\"La7rNGCFH6fc2QGjUfFUaDYYvWxYbSsLNsNsKR\",0);"
                        "INSERT INTO \"domains\" VALUES (\"jay.dgb\",\"La4aoDvM1huUBU63jFpQe4F7dvhrzhh1Y5UaFW\",0);"
                        "INSERT INTO \"domains\" VALUES (\"tennessee.dgb\",\"La2pV4SbBQxdqRQDCcmVMaJBNTYbMajiXy6SDH\",0);"
                        "INSERT INTO \"domains\" VALUES (\"sports.dgb\",\"La6XripMyJtqE1vgFgYTAUSHTjdL7FftZqPGAt\",0);"
                        "INSERT INTO \"domains\" VALUES (\"steven.dgb\",\"La5h6oxKDFUvnMFGni2cffpoCu4Woo4Js27xwh\",0);"
                        "INSERT INTO \"domains\" VALUES (\"music.dgb\",\"LaAM78XFNWi3igKtCyJ8m5QrWnzY3NGVDWTnbx\",0);"
                        "INSERT INTO \"domains\" VALUES (\"digiasset.dgb\",\"La6Re9NzCiVmWJmVwFBwuLKFb8ipkgX3fiXe2d\",0);"
                        "INSERT INTO \"domains\" VALUES (\"digiassets.dgb\",\"La3A4CDFAsp3hvXCwujornA2WWbkfp1oEnzY5u\",0);"
                        "INSERT INTO \"domains\" VALUES (\"marketing.dgb\",\"La8DqKg67CQYLtQzeHxDUaKgefFJArkcZfgkuc\",0);"
                        "INSERT INTO \"domains\" VALUES (\"social.dgb\",\"La85hfTDJpstG5Jmrov1KwHYj8BoXXPR8PxjVw\",0);"
                        "INSERT INTO \"domains\" VALUES (\"alphabet.dgb\",\"La7tpoRGJvXpfGYsvJXTcj1rQ8Qp7FAAXWHs1y\",0);"
                        "INSERT INTO \"domains\" VALUES (\"bmw.dgb\",\"LaA3jiFLxTTUKUFbtnWRruk6sBjcQ5owEk3Ekb\",0);"
                        "INSERT INTO \"domains\" VALUES (\"samsung.dgb\",\"La57HwJUo3wRTVDcH1x8iV4VHvUWruVkFU8x4P\",0);"
                        "INSERT INTO \"domains\" VALUES (\"spotify.dgb\",\"La6emjWxXvLYFeZNcd5qSL14XjP7myoBVtacX2\",0);"
                        "INSERT INTO \"domains\" VALUES (\"paypal.dgb\",\"La4vHMf1LGAtKXcPuPLnJAw7oPAfpr3QUP4fsU\",0);"
                        "INSERT INTO \"domains\" VALUES (\"ebay.dgb\",\"La8Bw35Hjs7RLh2qANAoCR1b1cMY9wEPoq6btG\",0);"
                        "INSERT INTO \"domains\" VALUES (\"netflix.dgb\",\"La45BZ6s1GEbeuzRjgbkQvKy5WZp2KKcJxN2Q9\",0);"
                        "INSERT INTO \"domains\" VALUES (\"disney.dgb\",\"La6WmDRHAMkRGgk2AtAGu8vjTbf6AkUQYSkfEJ\",0);"
                        "INSERT INTO \"domains\" VALUES (\"hbo.dgb\",\"La85MesB6Nuk3fWr2bfjsLvytR19rbxt6YutHd\",0);"
                        "INSERT INTO \"domains\" VALUES (\"555.dgb\",\"La7sYod3xEciY8MXjKzACTPUbSTXGGDozvrwTH\",0);"
                        "INSERT INTO \"domains\" VALUES (\"999.dgb\",\"La7Jq5KYRa2PoAuqMCP7PmsdL4XurHTkQaWojF\",0);"
                        "INSERT INTO \"domains\" VALUES (\"nike.dgb\",\"La7zan6PJRSX5HMFVyrQpe2SLBMmMFdawvJkUG\",0);"
                        "INSERT INTO \"domains\" VALUES (\"moncler.dgb\",\"La5VRMLvXXvQkqz15f586xSFUritFRzhQPH6tQ\",0);"
                        "INSERT INTO \"domains\" VALUES (\"gucci.dgb\",\"La5rPJLaYDS51b3jstsNaoYwM6KMtHDsQXJvRf\",0);"
                        "INSERT INTO \"domains\" VALUES (\"prp.dgb\",\"La7YCJKE2U2peqBeU4HRJHX4MR91WPAYGxhQmy\",0);"
                        "INSERT INTO \"domains\" VALUES (\"shake.dgb\",\"La3hzRxKHW9yr9aGPrph9uY4LPDmb21vxCgdH4\",0);"
                        "INSERT INTO \"domains\" VALUES (\"jason.dgb\",\"La5UE2GvsmJ8sVWXa3uk73ec4ZVGHvZcZUTZw6\",0);"
                        "INSERT INTO \"domains\" VALUES (\"elevate.dgb\",\"La97WYqAHm66VmCHA2nynBLzYPKDHMg811AyYV\",0);"
                        "INSERT INTO \"domains\" VALUES (\"mining.dgb\",\"La4N95X1V3VBzEmmFrPH2HS2zKu1oCDEAcDviZ\",0);"
                        "INSERT INTO \"domains\" VALUES (\"cryptopilgrim1.dgb\",\"La3FxbNPcMNAWMUcAShqTqMcucQmKz4NVyrZVg\",0);"
                        "INSERT INTO \"domains\" VALUES (\"diaz.dgb\",\"La3aeiX2BepXS4QupucKcLmUqcyMgPD6E6nXTc\",0);"
                        "INSERT INTO \"domains\" VALUES (\"ali.dgb\",\"LaALqjP1xiQFrVBS9nWqUDMs55hd6n3NQCBurR\",0);"
                        "INSERT INTO \"domains\" VALUES (\"333.dgb\",\"La3RPuM8XJDCthVHXSek4LtmtqXcTYJXYuU38i\",0);"
                        "INSERT INTO \"domains\" VALUES (\"bobby.dgb\",\"La89Y7Dy2sxSy5RWHfomgLRedJzJg5YMUyo45d\",0);"
                        "INSERT INTO \"domains\" VALUES (\"robert.dgb\",\"La5iRSKEAWPeLKVF2biZBkjDn9apsFFa6UvkCs\",0);"
                        "INSERT INTO \"domains\" VALUES (\"jennifer.dgb\",\"La8RWhh2bCwJdLfwNgvhSMGhA28Ksih5wG1cV6\",0);"
                        "INSERT INTO \"domains\" VALUES (\"dannyfromtheblockchain.dgb\",\"La3Ru4aGsHcdAbY8uZwGWbow65PocrLv5a8oF8\",0);"
                        "INSERT INTO \"domains\" VALUES (\"danny.dgb\",\"La7rdbKozpnQvMA75295oA9izXZwycawxpeEFn\",0);"
                        "INSERT INTO \"domains\" VALUES (\"elf.dgb\",\"La8gYAkayGino4CqmUynjewAafsyxqYxnz2skH\",0);"
                        "INSERT INTO \"domains\" VALUES (\"anon.dgb\",\"La5LQZHXNVCF7KGRsDgwAfm1Yn23f8p4TvRHJt\",0);"
                        "INSERT INTO \"domains\" VALUES (\"beer.dgb\",\"La8oNaFbtfJtWsTyupaaaCX86BJhjmn34eB2Pm\",0);"
                        "INSERT INTO \"domains\" VALUES (\"shell.dgb\",\"La67psMPgS2bt5Arx9i9vqbDEySRgLyNC6KMj5\",0);"
                        "INSERT INTO \"domains\" VALUES (\"tip.dgb\",\"La3Pmq4R9ZGN28DNPPJ5rS66EbsCs4AKQD5Krz\",0);"
                        "INSERT INTO \"domains\" VALUES (\"black7rob.dgb\",\"La8jVDjsJgCdG5x6sBPhbWKSouXcVMwskgeHpr\",0);"
                        "INSERT INTO \"domains\" VALUES (\"222.dgb\",\"La45rf4iP854NCQtcYVr2xx8NdVL3aHjwiyEzF\",0);"
                        "INSERT INTO \"domains\" VALUES (\"thedigibytebrothers.dgb\",\"La524kK8MUzZthozfno6gAfqrLgF6pJszg6L6E\",0);"
                        "INSERT INTO \"domains\" VALUES (\"garrickhines.dgb\",\"La5UaLEJk1iPfVjwCimj6znCj7jZc1ufXfjkgq\",0);"
                        "INSERT INTO \"domains\" VALUES (\"collaborativeteamwork.dgb\",\"La4SEzeVxvk5C14MYodsqZ5qtEVpvtTGsw6SFc\",0);"
                        "INSERT INTO \"domains\" VALUES (\"arliny.dgb\",\"La6bx9b4DGYbRtDtzcJfaQZbLcYaTcAPf5WGfg\",0);"
                        "INSERT INTO \"domains\" VALUES (\"gonzo.dgb\",\"La36hcY6zxyEHLnBBMjUqUDMAKWK5cZ1gdnMy2\",0);"
                        "INSERT INTO \"domains\" VALUES (\"philippines.dgb\",\"La8DZno3UQivQZpHd9DTDe9Pmd1qtjtLP8oy4F\",0);"
                        "INSERT INTO \"domains\" VALUES (\"chris.dgb\",\"La3XmoRJw8BwmzKwzUyYHRkDfTbaEYyUMoNo4R\",0);"
                        "INSERT INTO \"domains\" VALUES (\"konecny.dgb\",\"La6qr6PWPfRGJiJCJA7V7nQgpWg8ZTL6t2K7Xn\",0);"
                        "INSERT INTO \"domains\" VALUES (\"chriskonecny.dgb\",\"La5s1UStJRUMyw73WWHcHSN4pCjZLsKJb9f3YG\",0);"
                        "INSERT INTO \"domains\" VALUES (\"renzo-diaz.dgb\",\"La8e366mdy5FY6DM5Howdbny1jiTvCejF5tVwe\",0);"
                        "INSERT INTO \"domains\" VALUES (\"recoop.dgb\",\"La9JT4FDjo9UAXGrb1y7wRVP62WCcPhyLEFePa\",0);"
                        "INSERT INTO \"domains\" VALUES (\"btcdgb.dgb\",\"La8rb44GWXp4Zsu8TJutdjsVT8aroBBwRtEsD4\",0);"
                        "INSERT INTO \"domains\" VALUES (\"jesus.dgb\",\"La4f22moktEFsGyZ9nDuCQ1gxXKT5T8275H568\",0);"
                        "INSERT INTO \"domains\" VALUES (\"dany.dgb\",\"La6kkU4G63fev6TNVGCNPTs4eQdKUZ54zbCc9r\",0);"
                        "INSERT INTO \"domains\" VALUES (\"deegebi.dgb\",\"La3BBuKNCCh3qp9uLgrpZreGt3RsE1GEFsDpuC\",0);"
                        "INSERT INTO \"domains\" VALUES (\"musiclover.dgb\",\"La81PfBN9GgeuF5MjNX267wMqvzqPW2SHErvsF\",0);"
                        "INSERT INTO \"domains\" VALUES (\"digibytedesktop.dgb\",\"La2o7PbWcfXnLvFEFnqCw8pcUc6dgJSPhLxQcK\",0);"
                        "INSERT INTO \"domains\" VALUES (\"sylvain.dgb\",\"La8K1BBcF6XCxjk331zcHFk4PoShovFLD86zqJ\",0);"
                        "INSERT INTO \"domains\" VALUES (\"sly.dgb\",\"La9h7LrR2Bp78wQBFSEvjPQssgWYXYL5mtKXEz\",0);"
                        "INSERT INTO \"domains\" VALUES (\"twenx.dgb\",\"La3a48b6DZNGJyhFkY7E42V57KvsekvXXxjQJc\",0);"
                        "INSERT INTO \"domains\" VALUES (\"energy.dgb\",\"La7NFhXATmWK6RryZytVkQZsNwUQdTLUaKyg39\",0);"
                        "INSERT INTO \"domains\" VALUES (\"oliver.dgb\",\"La9QoFwBoggG5eRPYR1Y4W9q7RC4XwYW4f8yod\",0);"
                        "INSERT INTO \"domains\" VALUES (\"amcodgb.dgb\",\"La9nUp3wYDtu7rDpcMHXJTLYxDMH2UvqixCzJ7\",0);"
                        "INSERT INTO \"domains\" VALUES (\"digiassetcore.dgb\",\"La9AbivE5NEd3eFQEJBwQc5L87qvpURUA8vSkj\",0);"
                        "INSERT INTO \"domains\" VALUES (\"alejandra.dgb\",\"La56asqbQaVjLE7XBwpTsZAPJR1Xh33FM36H99\",0);"
                        "INSERT INTO \"domains\" VALUES (\"pizzaplanet.dgb\",\"La5XnoqwzUYVE9WBnUk7KL4MuLqnLHLHiGZuz2\",0);"
                        "INSERT INTO \"domains\" VALUES (\"he-man.dgb\",\"La5nzmuAUpZJe7kFfLJMCxqgwWds54eTSfN4fU\",0);"
                        "INSERT INTO \"domains\" VALUES (\"jasonhenson.dgb\",\"LaAPTcBLzPh2Ji7xQaF6us9ba9dB7DyqBsSeCw\",0);"
                        "INSERT INTO \"domains\" VALUES (\"caden.dgb\",\"La7ARUhZUvNgviS1DrYpkS9hWhqrxGyP2jxnyv\",0);"
                        "INSERT INTO \"domains\" VALUES (\"triplezen.dgb\",\"La6tYSmcXnuf1Rc5ghb1TCE87a4AGUEZzYaXQF\",0);"
                        "INSERT INTO \"domains\" VALUES (\"knight.dgb\",\"La3hdgHoy8UsiCou6o6xzjGdf1jmJEFdGZ2Zrj\",0);"
                        "INSERT INTO \"domains\" VALUES (\"fenndoge01.dgb\",\"La9THQup4vdkbfDYbepKUX28rPeNCi24F76t46\",0);"
                        "INSERT INTO \"domains\" VALUES (\"chuka.dgb\",\"La4CDdAiFBNprETwXX6QjxGftgGrEJqHXmWWQR\",0);"
                        "INSERT INTO \"domains\" VALUES (\"vinexsky.dgb\",\"La9VSryPXBywztgsi4AR17HjJ9m4bnn4mKGUHA\",0);"
                        "INSERT INTO \"domains\" VALUES (\"vprime.dgb\",\"La8ao2JdCnhtcx6kebNPcrgKSfEJHoe7MHA4o7\",0);"
                        "INSERT INTO \"domains\" VALUES (\"zach.dgb\",\"La3WSFSNFnWxKF7gBqysn3VxwpbGz6Mu2C5Z4A\",0);"
                        "INSERT INTO \"domains\" VALUES (\"zachary.dgb\",\"La6MroddmACxCyeGx5zGDRRahcF2uMuynZCWnD\",0);"
                        "INSERT INTO \"domains\" VALUES (\"shark.dgb\",\"La9mcRaAoRo9yx8CgWMfF4TMv6AYcXsQ9cdPY9\",0);"
                        "INSERT INTO \"domains\" VALUES (\"whale.dgb\",\"La8ugqLvZbZSsDcrAn9zxpVZHLT2xhWiG6PyeR\",0);"
                        "INSERT INTO \"domains\" VALUES (\"web3.dgb\",\"La8RtfneLFUp7iTGM15rievUpBW3KNqEw9qvjV\",0);"
                        "INSERT INTO \"domains\" VALUES (\"paradigm.dgb\",\"La6j8eWHSJU4RhtY5p1XG4WWFP81Uv1ZuQ9EHQ\",0);"
                        "INSERT INTO \"domains\" VALUES (\"111.dgb\",\"La3JzajyvEe12RE7hdpz6k3TLNVbVM6gW5m3Sj\",0);"
                        "INSERT INTO \"domains\" VALUES (\"444.dgb\",\"La3bN3nvwBhbzG7npbgTN1YtRutZ9SpazhirXY\",0);"
                        "INSERT INTO \"domains\" VALUES (\"doge.dgb\",\"La7QMPxsxtJAVvv8mL2oCaMpZR6c3sYWBoH52x\",0);"
                        "INSERT INTO \"domains\" VALUES (\"digitalwealthstudios.dgb\",\"La9VzsbVAf5PiQDs3FdrcHkFN9gREj3MaonKJe\",0);"
                        "INSERT INTO \"domains\" VALUES (\"fomo.dgb\",\"La4g3c6ZNttXs8m7YC4zanf2VYqdMj8vedBoFG\",0);"
                        "INSERT INTO \"domains\" VALUES (\"meme.dgb\",\"La9WgjBCknqjzNPHAkeTKpqavgbnphYNjixZyN\",0);"
                        "INSERT INTO \"domains\" VALUES (\"pizza.dgb\",\"LaAGRXRdnXqCs7z5d3LhDezXb8ci4m4UEoG7Hj\",0);"
                        "INSERT INTO \"domains\" VALUES (\"zac.dgb\",\"La4SFqzMRsyS1cEWDbh9AEHQqFf2zwtAAW696M\",0);"
                        "INSERT INTO \"domains\" VALUES (\"rare.dgb\",\"La9QVLehiuTV9jMP9xtYzJisbGeHRwjTDTVqwq\",0);"
                        "INSERT INTO \"domains\" VALUES (\"ceo.dgb\",\"La2vHSTZTsJNb6NHNqHxnW5e6rb7awEyPrZkEB\",0);"
                        "INSERT INTO \"domains\" VALUES (\"dope.dgb\",\"La3u6KXfZehdWgwmTm9rNHUmE7c8vnMSNxoQRg\",0);"
                        "INSERT INTO \"domains\" VALUES (\"our.dgb\",\"La9Y2SYfi2yRVA3KNuF6xbhMcoRSkt7sE584AE\",0);"
                        "INSERT INTO \"domains\" VALUES (\"069.dgb\",\"La6X5fWpbkHFi6u4EiMusNxXFAiKUqL78dwudg\",0);"
                        "INSERT INTO \"domains\" VALUES (\"096.dgb\",\"La6SaK8Qigvg4AvSavea2AHwWVyvXmN4ByFwUz\",0);"
                        "INSERT INTO \"domains\" VALUES (\"digi.dgb\",\"La4qKhb2t15CpHkWwbfvLEjzBdHQ2R2WesqyAJ\",0);"
                        "INSERT INTO \"domains\" VALUES (\"mint.dgb\",\"La2qatYDhUR3VTZHtAerjnXn6CJ5y8ErxDoGxm\",0);"
                        "INSERT INTO \"domains\" VALUES (\"xxx.dgb\",\"La5ADKzFgHJFMLLp6MWdujo3DT33CvnNqKBkit\",0);"
                        "INSERT INTO \"domains\" VALUES (\"wagmi.dgb\",\"La6GTzWN6ui4qAt87ECcT6AiT4M2CyQBWrB7Pd\",0);"
                        "INSERT INTO \"domains\" VALUES (\"burn.dgb\",\"La827XFVfhgzGejAmb45qrVZgwt5TUQgdFCs2v\",0);"
                        "INSERT INTO \"domains\" VALUES (\"coin.dgb\",\"La2mBnhAhvym2QZz1bSwDQHdAMrakaynw8rCVb\",0);"
                        "INSERT INTO \"domains\" VALUES (\"023.dgb\",\"La6kbwVZ6gvb4o8VsxMn1Uyippuov1bM6aGEQv\",0);"
                        "INSERT INTO \"domains\" VALUES (\"hugo.dgb\",\"La6hZKRpPaC8CGkqY2yBGNPbPMeSiC8TfjhLi4\",0);"
                        "INSERT INTO \"domains\" VALUES (\"hell.dgb\",\"La5MeJ4uV9FoMiCYYoX8rUW2wdh76AUpCZgZKZ\",0);"
                        "INSERT INTO \"domains\" VALUES (\"xeet.dgb\",\"La6MGTz85sbyMPL5u4WeSdFeVHFQF2MVhKfAfj\",0);"
                        "INSERT INTO \"domains\" VALUES (\"dino.dgb\",\"La3S2UR5hVcjK9veqr9x3VvQRmCfAEotwZHeCf\",0);"
                        "INSERT INTO \"domains\" VALUES (\"mine.dgb\",\"La7cZ1y5onfHwqDnYNpoAHR83cyDbnUmd6MSM8\",0);"
                        "INSERT INTO \"domains\" VALUES (\"pow.dgb\",\"LaAFHdDt2wjnQzHJSFvEtfYwApX8pXRay3TViM\",0);"
                        "INSERT INTO \"domains\" VALUES (\"zip.dgb\",\"La2yuZj2MHCuLf7acT3e7qykMntSp7j6r7oDDw\",0);"
                        "INSERT INTO \"domains\" VALUES (\"blue.dgb\",\"La7LXGX6ypc9rnoQb1T6reR56FMwXV44DB3hq2\",0);"
                        "INSERT INTO \"domains\" VALUES (\"coins.dgb\",\"La4Cjtv6vRjk6Fzf2BfCLTJBwCq3NpgkyNGdwq\",0);"
                        "INSERT INTO \"domains\" VALUES (\"2014.dgb\",\"La9KFeEMr3z9BLNRVFY7jtCTKz6ZoJqR7FUtc5\",0);"
                        "INSERT INTO \"domains\" VALUES (\"2035.dgb\",\"La7M9wvvbG1TXyHdJCjy4Q6cCXYRUvmucrabqM\",0);"
                        "INSERT INTO \"domains\" VALUES (\"based.dgb\",\"La888ezrbgqrNGeBit8GNc12yGGdMbHRACCrg7\",0);"
                        "INSERT INTO \"domains\" VALUES (\"flex.dgb\",\"La8uHRS3FTQEMTXhA14ur6Z6qCjVygroJ6mCsw\",0);"
                        "INSERT INTO \"domains\" VALUES (\"yacht.dgb\",\"La6ypSSzifUZr8aSU5yMv12Lr2zvm2dvSbyC1y\",0);"
                        "INSERT INTO \"domains\" VALUES (\"mom.dgb\",\"La4ELKNkX2Y7EmL5DMWQLv8i5rguQ2FEPaUfAj\",0);"
                        "INSERT INTO \"domains\" VALUES (\"dad.dgb\",\"La8Lf2paoRrpQi8FLGQPzkWNviditp3JLUPbrJ\",0);"
                        "INSERT INTO \"domains\" VALUES (\"jess.dgb\",\"La7k6pXknaqPXsMYgG3woLEWyz8uDoYY292rgs\",0);"
                        "INSERT INTO \"domains\" VALUES (\"josh.dgb\",\"La8kJbupqB4vhZwfoKXxBfoVFXNfvBGrHPgy5B\",0);"
                        "INSERT INTO \"domains\" VALUES (\"bech.dgb\",\"La3yE2UF56HuhRiyjkzHouD7xpJuKWZPkc2qnD\",0);"
                        "INSERT INTO \"domains\" VALUES (\"legacy.dgb\",\"La7y1XGGxG8aVeMYcWRvHD1bFvxWzYkZ6BfrS9\",0);"
                        "INSERT INTO \"domains\" VALUES (\"010.dgb\",\"La86Z9q2w4ateVkVvVrmzfkP3PHDnWR5zz2zXy\",0);"
                        "INSERT INTO \"domains\" VALUES (\"101.dgb\",\"La4kz4hnLphk5ALwvqLaH9omuFESy7btwpu8GZ\",0);"
                        "INSERT INTO \"domains\" VALUES (\"ten.dgb\",\"La8iBAsoAN5e56NGudFwFqbWjxL6XmFaZ4WyWp\",0);"
                        "INSERT INTO \"domains\" VALUES (\"block.dgb\",\"La35HL3r1J4Za1ZMfH9EMNr5CYrsU2HmcW7ca9\",0);"
                        "INSERT INTO \"domains\" VALUES (\"chain.dgb\",\"La786eBAATkJjpBWD6qPDLgatMS4o3KPyYbPcX\",0);"
                        "INSERT INTO \"domains\" VALUES (\"stack.dgb\",\"La4t7ktgUhmLWiHCAccvTThuSxLc9xgQJmiJk3\",0);"
                        "INSERT INTO \"domains\" VALUES (\"hey.dgb\",\"La8XJYBaogkPppqngBVeZ2bdVUXtTkcqX5v8Dc\",0);"
                        "INSERT INTO \"domains\" VALUES (\"sat.dgb\",\"La3xFB1M4MHcGqpURtyZWwmZ79Jd7AooUSJboT\",0);"
                        "INSERT INTO \"domains\" VALUES (\"satoshis.dgb\",\"La7e3Qeju6cxXnxY6Zm1cM6rjxZM6uL3y7n4sK\",0);"
                        "INSERT INTO \"domains\" VALUES (\"darth.dgb\",\"La7FwsQexSABAPb29fDpRnJbu73T1pDeW19aDs\",0);"
                        "INSERT INTO \"domains\" VALUES (\"cum.dgb\",\"La3Si6Ayddtx2LtnrJFt7SpVpHcfcEWoSJK3AC\",0);"
                        "INSERT INTO \"domains\" VALUES (\"613.dgb\",\"La6mPq3P1NPy85hyydVmpb92grkWH54MgJN4js\",0);"
                        "INSERT INTO \"domains\" VALUES (\"360.dgb\",\"La34jvRVjYJkRxVAsNyB4b7gG5HcKiMh2J5vRF\",0);"
                        "INSERT INTO \"domains\" VALUES (\"address.dgb\",\"LaAN5ectWXFmvMNpVXMERp8YvvKLcKYMHoTjvS\",0);"
                        "INSERT INTO \"domains\" VALUES (\"home.dgb\",\"La8mCjBm8rdfTTpRXPJqKutJbXkkGrMyt3WnTJ\",0);"
                        "INSERT INTO \"domains\" VALUES (\"solo.dgb\",\"La5HGYPqxs6vHMi6o2XqF4CNLKwej9azbTQjZk\",0);"
                        "INSERT INTO \"domains\" VALUES (\"tag.dgb\",\"La5vTpXsWwSaGKxxNB7NhbRbKXS38x6ffo1a3q\",0);"
                        "INSERT INTO \"domains\" VALUES (\"ken.dgb\",\"La3wFo4VGGG9fyNucHHGXWTTyL6jbREj1Hd9ak\",0);"
                        "INSERT INTO \"domains\" VALUES (\"lfg.dgb\",\"La4EZTCis83g98Q8oXNKoHHzRFruTJXXdDZk8S\",0);"
                        "INSERT INTO \"domains\" VALUES (\"ftw.dgb\",\"La9d5NCd54EAANBC6NMqvBKcJzPZwauTnHf4iw\",0);"
                        "INSERT INTO \"domains\" VALUES (\"few.dgb\",\"La51smnUuTouj8dFrdFqeKPRyz2s2CYbFwUQ4X\",0);"
                        "INSERT INTO \"domains\" VALUES (\"asset.dgb\",\"La9JEHbVMjJTRgqH6xMecYUrubwRDnQjLdYiP7\",0);"
                        "INSERT INTO \"domains\" VALUES (\"its.dgb\",\"La38ZN73XNSFewBuVUHXmiS7jgfqHFKp34fWUp\",0);"
                        "INSERT INTO \"domains\" VALUES (\"onlyfans.dgb\",\"La3cJP65eKEm2cxDw1q8ZEE8WFVkJkAArJzrrp\",0);"
                        "INSERT INTO \"domains\" VALUES (\"only.dgb\",\"La8aLtWLqggpZAF8U9s6fVGBGpJsrAWRJ3iqpf\",0);"
                        "INSERT INTO \"domains\" VALUES (\"fan.dgb\",\"La6ZxiXcYyTCXLdPpA6S1GUk7BsMuCCcQwqfhc\",0);"
                        "INSERT INTO \"domains\" VALUES (\"maxi.dgb\",\"La6Tq3Suv7dHwqyEKKZ6vu6vjZrc6RTodyqMtj\",0);"
                        "INSERT INTO \"domains\" VALUES (\"inu.dgb\",\"La3M8vqFriXtaagwYrqXJDGY6Sz5rySx6HhQuK\",0);"
                        "INSERT INTO \"domains\" VALUES (\"pepe.dgb\",\"LaA5QKRhoBnS9SkRhwsnKbiWbSNhMP5hH3wrtW\",0);"
                        "INSERT INTO \"domains\" VALUES (\"virtual.dgb\",\"La89YTJN2NRQethUqDc4DwqHnTnpMCkm24EWZ7\",0);"
                        "INSERT INTO \"domains\" VALUES (\"iou.dgb\",\"La2ja17dYqssENTuL4dN1pWQ6wLvzqa3K4NJEY\",0);"
                        "INSERT INTO \"domains\" VALUES (\"loan.dgb\",\"La2phqxiB5qxyv1zMwAwnJkMPkbVUyKmr5kMg2\",0);"
                        "INSERT INTO \"domains\" VALUES (\"dgbcommerce.dgb\",\"La2psHHVgkvaV34qN6QDTuByRbGPYxSfj8tZzA\",0);"
                        "INSERT INTO \"domains\" VALUES (\"nfa.dgb\",\"La5wpRZVsbzgaEm9oVwZWkPFwFetfBG2AnJDcw\",0);"
                        "INSERT INTO \"domains\" VALUES (\"luv.dgb\",\"La9QqMnHiDnEDUd3Nrd8ESLnAePjzjcjNnYRbY\",0);"
                        "INSERT INTO \"domains\" VALUES (\"ilysm.dgb\",\"La4npgLKXSsyeQH6K5Sd8KRgYsvP7moKeH9STb\",0);"
                        "INSERT INTO \"domains\" VALUES (\"iykyk.dgb\",\"La2ypQtMCzgwvgsZEbrK7nuwfgghkf8z8Uz6xq\",0);"
                        "INSERT INTO \"domains\" VALUES (\"radicalrevv.dgb\",\"La6XMbf6XJ4aFPtew8qB8wDszSezL7sjQ7Noes\",0);"
                        "INSERT INTO \"domains\" VALUES (\"valentenio.dgb\",\"La52D9XbTQXehvUP6WmYfKiuncCDSA6499bkjg\",0);"
                        "INSERT INTO \"domains\" VALUES (\"junior.dgb\",\"La7ghiCGRqj5kSyL78iZkeFHeFjQsDy7e7jPU3\",0);"
                        "INSERT INTO \"domains\" VALUES (\"colebrook.dgb\",\"La9P5JuyEmf3aKyaBAYEAbJ5Fp8U6fFa7Xg3fA\",0);"
                        "INSERT INTO \"domains\" VALUES (\"brianoakes.dgb\",\"La7TELrLp56sbvhfuDCzyw4RwUWNFJDiRyzn44\",0);"
                        "INSERT INTO \"domains\" VALUES (\"mds.dgb\",\"La6y4yJ2xCsmrPUHtX5eBPVqrYKaMBMp5wWutG\",0);"


                        "COMMIT";
                rc = sqlite3_exec(_db, sql, Database::defaultCallback, 0, &zErrMsg);
                if (rc != SQLITE_OK) {
                    sqlite3_free(zErrMsg);
                    throw exceptionFailedToCreateTable();
                } //todo if under most recent version need to redownload or start over because DigiByte domain is wrong.  can as a result colapse all these changes

                //try to delete the old permanent table.
                //have had some problems with this throwing a table is locked error.
                //Would be good to figure out why, but it is not detrimental if this line fails so not checking
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
