//
// Created by mctrivia on 04/10/23.
//

#include "Log.h"

std::mutex Log::_mutex;
Log* Log::_pinstance = nullptr;

Log* Log::GetInstance() {
    std::lock_guard<std::mutex> lock(_mutex);
    if (_pinstance == nullptr) {
        _pinstance = new Log();
    }
    return _pinstance;
}

Log* Log::GetInstance(const string& fileName) {
    std::lock_guard<std::mutex> lock(_mutex);
    if (_pinstance == nullptr) {
        _pinstance = new Log();
    }
    _pinstance->setLogFile(fileName);
    return _pinstance;
}

void Log::setLogFile(const string& filename) {
    _logFile.open(filename, ios_base::app);
    if (!_logFile.is_open()) {
        throw exceptionFailedToOpen();
    }
}

void Log::setMinLevelToScreen(LogLevel level) {
    _minLevelToScreen = level;
}

void Log::setMinLevelToFile(LogLevel level) {
    _minLevelToFile = level;
}

void Log::addMessage(const string& message, LogLevel level) {
    lock_guard<mutex> lock(_mutex);

    string logLevelStr;
    switch (level) {
        case INFO:
            logLevelStr = "INFO";
            break;
        case WARNING:
            logLevelStr = "WARNING";
            break;
        case ERROR:
            logLevelStr = "ERROR";
            break;
        case CRITICAL:
            logLevelStr = "CRITICAL";
            break;
    }

    string logMessage = logLevelStr + ": " + message;

    if (level >= _minLevelToScreen) {
        cout << logMessage << endl;
    }

    if (level >= _minLevelToFile && _logFile.is_open()) {
        _logFile << logMessage << endl;
    }
}

Log::~Log() {
    if (_logFile.is_open()) {
        _logFile.close();
    }
}