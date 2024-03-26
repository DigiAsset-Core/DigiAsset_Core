//
// Created by mctrivia on 20/03/24.
//

#ifndef DIGIASSET_CORE_DATABASE_LOCKEDSTATEMENT_H
#define DIGIASSET_CORE_DATABASE_LOCKEDSTATEMENT_H



#include "Blob.h"
#include <mutex>
#include <sqlite3.h>

class Statement;

class LockedStatement {
public:
    LockedStatement(Statement& statement);
    ~LockedStatement();

    void reset();

    // Bind methods remember indexes start at 1
    void bindInt(int index, int value);

    void bindInt64(int index, int64_t value);

    void bindDouble(int index, double value);

    void bindText(int index, const std::string& value);

    void bindBlob(int index, const Blob& blob);

    void bindNull(int index);

    // Get column methods remember indexes start at 0
    int getColumnInt(int index);

    int64_t getColumnInt64(int index);

    double getColumnDouble(int index);

    std::string getColumnText(int index);

    Blob getColumnBlob(int index);

    // Execute step
    int executeStep();

private:
    sqlite3_stmt* _stmt;
    std::unique_lock<std::mutex> _lock; // Automatically releases the lock when destroyed
    Statement* _creator; // Reference to the creating Statement object
    std::chrono::steady_clock::time_point _creationTime; // Time of creation
};



#endif //DIGIASSET_CORE_DATABASE_LOCKEDSTATEMENT_H
