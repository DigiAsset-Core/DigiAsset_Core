//
// Created by mctrivia on 12/11/23.
//

#include "local.h"
#include "AppMain.h"
using namespace std;
void local::start() {
    //not needed for local only storage
}
void local::stop() {
    //not needed for local only storage
}
void local::enable(DigiByteTransaction& tx) {
    //make sure database enabled
    loadDB();

    //normally would make changes to tx to enable on tx.  In this case we will just add it to the local database
    sqlite3_reset(_stmtEnableInPool);
    string cid = tx.getIssuedAsset().getCID();
    sqlite3_bind_text(_stmtEnableInPool, 1, cid.c_str(), cid.length(), SQLITE_STATIC);
    int rc = sqlite3_step(_stmtEnableInPool);
    if (rc != SQLITE_DONE) throw exceptionCantEnablePSP();
}
uint64_t local::getCost(const DigiByteTransaction& tx) {
    return 0; //no cost for local
}
string local::getName() {
    return "Local Storage";
}
string local::getDescription() {
    return "Stores the DigiAsset on your nodes only.  Strongly discouraged since meta data of asset may be lost if you stop running your nodes.";
}
string local::getURL() {
    return "NA";
}
/**
 * Processes a transaction and returns a value to pass to the meta data processor.
 * Should return "" if this transaction is not to be processed by the pool
 * If it is to be processed by the pool then returns a non empty string that does not contain ,: symbols
 */
string local::serializeMetaProcessor(const DigiByteTransaction& tx) {
    //check if there is a local psp.  to save time assume there is if already loaded
    if ((_db == nullptr) && (!localExists())) return "";

    //load db(does nothing if already loaded)
    loadDB();

    //check if in the database
    sqlite3_reset(_stmtCheckIfPartOfPool);
    string cid = tx.getIssuedAsset().getCID();
    sqlite3_bind_text(_stmtCheckIfPartOfPool, 1, cid.c_str(), cid.length(), SQLITE_STATIC);
    int rc = sqlite3_step(_stmtCheckIfPartOfPool);
    if (rc == SQLITE_ROW) return "1"; //found so part of PSP
    return "";
}

/**
 * Generic function to create a MetaProcessor object for this pool from the serialized Data
 * @param serializedData
 * @return
 */
unique_ptr<PermanentStoragePoolMetaProcessor> local::deserializeMetaProcessor(const string& serializedData) {
    return unique_ptr<PermanentStoragePoolMetaProcessor>(new localMetaProcessor(serializedData, _poolIndex));
}
void local::loadDB() {
    //check if already loaded
    if (_db != nullptr) return;

    //see if this is first run
    bool firstRun = localExists();

    //load or create the database
    int rc;
    rc = sqlite3_open_v2(PSP_LOCAL_DB_FILENAME, &_db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, NULL);
    if (rc) throw exceptionCantLoadPSP();

    //create needed tables
    if (firstRun) {
        buildTables();
    }

    //create needed statements
    initializeDBValues();
}
bool local::localExists() const {
    struct stat buffer {};
    return (stat(PSP_LOCAL_DB_FILENAME, &buffer) != 0);
}

void local::buildTables() {
    char* zErrMsg = 0;
    int rc;
    const char* sql =
            //chain data tables
            "CREATE TABLE \"pin\" (\"cid\" TEXT);"
            "CREATE TABLE \"bad\" (\"assetId\" TEXT);";

    rc = sqlite3_exec(_db, sql, NULL, 0, NULL);

    if (rc != SQLITE_OK) {
        sqlite3_free(zErrMsg);
        throw exceptionCantLoadPSP();
    }
}
void local::initializeDBValues() {
    const char* sql10 = "SELECT 1 FROM pin WHERE cid LIKE ?;";
    int rc = sqlite3_prepare_v2(_db, sql10, strlen(sql10), &_stmtCheckIfPartOfPool, nullptr);
    if (rc != SQLITE_OK) throw exceptionCantLoadPSP();

    const char* sql11 = "SELECT 1 FROM bad WHERE assetId LIKE ?;";
    rc = sqlite3_prepare_v2(_db, sql11, strlen(sql11), &_stmtCheckIfBad, nullptr);
    if (rc != SQLITE_OK) throw exceptionCantLoadPSP();

    const char* sql12 = "INSERT INTO pin VALUES (?);";
    rc = sqlite3_prepare_v2(_db, sql12, strlen(sql12), &_stmtEnableInPool, nullptr);
    if (rc != SQLITE_OK) throw exceptionCantLoadPSP();

    const char* sql13 = "INSERT INTO bad VALUES (?);";
    rc = sqlite3_prepare_v2(_db, sql13, strlen(sql13), &_stmtMarkBad, nullptr);
    if (rc != SQLITE_OK) throw exceptionCantLoadPSP();

    const char* sql14 = "DELETE FROM pin WHERE cid LIKE ?;";
    rc = sqlite3_prepare_v2(_db, sql14, strlen(sql14), &_stmtDiableFromPool, nullptr);
    if (rc != SQLITE_OK) throw exceptionCantLoadPSP();
}
bool local::isAssetBad(const std::string& assetId) {
    //check if there is a local psp.  to save time assume there is if already loaded
    if ((_db == nullptr) && (!localExists())) return false;

    //load db(does nothing if already loaded)
    loadDB();

    //check if in the database
    sqlite3_reset(_stmtCheckIfBad);
    sqlite3_bind_text(_stmtCheckIfBad, 1, assetId.c_str(), assetId.length(), SQLITE_STATIC);
    int rc = sqlite3_step(_stmtCheckIfBad);
    return (rc == SQLITE_ROW);
}
void local::_reportAssetBad(const std::string& assetId) {
    //load db(does nothing if already loaded)
    loadDB();

    //save assetId in database
    sqlite3_reset(_stmtMarkBad);
    sqlite3_bind_text(_stmtMarkBad, 1, assetId.c_str(), assetId.length(), SQLITE_STATIC);
    int rc = sqlite3_step(_stmtMarkBad);
    if (rc != SQLITE_DONE) throw exceptionCouldntReport();
}
void local::_reportFileBad(const string& cid) {
    //load db(does nothing if already loaded)
    loadDB();

    //save cid in database
    sqlite3_reset(_stmtDiableFromPool);
    sqlite3_bind_text(_stmtDiableFromPool, 1, cid.c_str(), cid.length(), SQLITE_STATIC);
    int rc = sqlite3_step(_stmtDiableFromPool);
    if (rc != SQLITE_DONE) throw exceptionCouldntReport();
}


localMetaProcessor::localMetaProcessor(const string& serializedData, unsigned int poolIndex) : PermanentStoragePoolMetaProcessor(poolIndex) {
    //no need to decode serialized data its just "1"
}
bool localMetaProcessor::_shouldPinFile(const std::string& name, const std::string& mimeType, const std::string& cid) {
    //local always pins everything if the transaction was in the database
    return true;
}