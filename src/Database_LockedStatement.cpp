//
// Created by mctrivia on 20/03/24.
//

#include "Database_Statement.h"
#include "Database_LockedStatement.h"

LockedStatement::LockedStatement(Statement& statement)
    : _creator(&statement), _lock(statement._mutex), _creationTime(std::chrono::steady_clock::now()) {
    _stmt = statement._stmt;
    // The lock is acquired as soon as an object of this class is created
    reset();
}

LockedStatement::~LockedStatement() {
    auto duration = std::chrono::steady_clock::now() - _creationTime;
    _creator->addLockDuration(std::chrono::duration_cast<std::chrono::microseconds>(duration));
}

void LockedStatement::reset() {
    sqlite3_reset(_stmt);
}

// Bind methods remember indexes start at 1
void LockedStatement::bindInt(int index, int value) {
    sqlite3_bind_int(_stmt, index, value);
}

void LockedStatement::bindInt64(int index, int64_t value) {
    sqlite3_bind_int64(_stmt, index, value);
}

void LockedStatement::bindDouble(int index, double value) {
    sqlite3_bind_double(_stmt, index, value);
}

void LockedStatement::bindText(int index, const std::string& value) {
    sqlite3_bind_text(_stmt, index, value.c_str(), -1, SQLITE_TRANSIENT);
}

void LockedStatement::bindBlob(int index, const Blob& blob) {
    sqlite3_bind_blob(_stmt, index, blob.data(), blob.length(), SQLITE_TRANSIENT);
}

void LockedStatement::bindNull(int index) {
    sqlite3_bind_null(_stmt, index);
}

// Get column methods remember indexes start at 0
int LockedStatement::getColumnInt(int index) {
    return sqlite3_column_int(_stmt, index);
}

int64_t LockedStatement::getColumnInt64(int index) {
    return sqlite3_column_int64(_stmt, index);
}

double LockedStatement::getColumnDouble(int index) {
    return sqlite3_column_double(_stmt, index);
}

std::string LockedStatement::getColumnText(int index) {
    const unsigned char* text = sqlite3_column_text(_stmt, index);
    return std::string(reinterpret_cast<const char*>(text));
}

Blob LockedStatement::getColumnBlob(int index) {
    const void* data = sqlite3_column_blob(_stmt, index);
    int length = sqlite3_column_bytes(_stmt, index);
    return Blob(data, length);
}

// Execute step
int LockedStatement::executeStep() {
    return sqlite3_step(_stmt);
}