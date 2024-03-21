//
// Created by mctrivia on 20/03/24.
//

#ifndef DIGIASSET_CORE_DATABASE_STATEMENT_H
#define DIGIASSET_CORE_DATABASE_STATEMENT_H



#include "Database_LockedStatement.h"
#include <chrono>
#include <mutex>
#include <sqlite3.h>
#include <string>

class Statement {
public:
    Statement()=default;

    ~Statement();

    void prepare(sqlite3* db, const std::string& query);

    void addLockDuration(const std::chrono::microseconds& duration);

    long long getTotalLockDuration() const;
    int getLockCount() const;

    friend class LockedStatement;
private:
    sqlite3_stmt* _stmt = nullptr;
    std::mutex _mutex;
    long long _totalLockedDuration = 0;
    int _lockCount = 0;
};



#endif //DIGIASSET_CORE_DATABASE_STATEMENT_H
