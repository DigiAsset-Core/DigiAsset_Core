//
// Created by mctrivia on 20/03/24.
//

#include "Database_Statement.h"
#include "Database_LockedStatement.h"



Statement::~Statement() {
    if (_stmt!= nullptr) sqlite3_finalize(_stmt);
}

void Statement::prepare(sqlite3* db, const std::string& query) {
    if (_stmt!= nullptr) throw std::runtime_error("Statement already prepared");    //code is wrong if this executes
    const char* tail;
    int rc = sqlite3_prepare_v2(db, query.c_str(), -1, &_stmt, &tail);
    if (rc != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare statement");
    }
}

void Statement::addLockDuration(const std::chrono::microseconds& duration) {
    _totalLockedDuration += duration.count();
    _lockCount++;
}

long long Statement::getTotalLockDuration() const { return _totalLockedDuration; }
int Statement::getLockCount() const { return _lockCount; }
