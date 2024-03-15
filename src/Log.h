//
// Created by mctrivia on 04/10/23.
//

#ifndef DIGIASSET_CORE_LOG_H
#define DIGIASSET_CORE_LOG_H



#include <iostream>
#include <fstream>
#include <string>
#include <mutex>

using namespace std;


class Log {
public:
    enum LogLevel {
        DEBUG=0,
        INFO=10,
        WARNING=20,
        ERROR=30,
        CRITICAL=40
    };

/**
 * Singleton Start
 */
private:
    static Log* _pinstance;
    static std::mutex _mutex;

protected:
    Log() = default;
    ~Log();

public:
    Log(Log& other) = delete;
    void operator=(const Log&) = delete;
    static Log* GetInstance();
    static Log* GetInstance(const std::string& fileName);

/**
 * Singleton End
 */
private:
    ofstream _logFile;
    LogLevel _minLevelToScreen = INFO;
    LogLevel _minLevelToFile = INFO;

public:
    static std::string _lastErrorMessage;

    void setLogFile(const string& filename);
    void setMinLevelToScreen(LogLevel level);
    void setMinLevelToFile(LogLevel level);
    void addMessage(const string& message, LogLevel level = INFO);


    /*
    ███████╗██████╗ ██████╗  ██████╗ ██████╗ ███████╗
    ██╔════╝██╔══██╗██╔══██╗██╔═══██╗██╔══██╗██╔════╝
    █████╗  ██████╔╝██████╔╝██║   ██║██████╔╝███████╗
    ██╔══╝  ██╔══██╗██╔══██╗██║   ██║██╔══██╗╚════██║
    ███████╗██║  ██║██║  ██║╚██████╔╝██║  ██║███████║
    ╚══════╝╚═╝  ╚═╝╚═╝  ╚═╝ ╚═════╝ ╚═╝  ╚═╝╚══════╝
     */

    class exception : public std::exception {
    public:
        char* what() {
            _lastErrorMessage = "Something went wrong with Log";
            return const_cast<char*>(_lastErrorMessage.c_str());
        }
    };

    class exceptionFailedToOpen : public exception {
    public:
        char* what() {
            _lastErrorMessage = "Couldn't open or create the log";
            return const_cast<char*>(_lastErrorMessage.c_str());
        }
    };
};



#endif //DIGIASSET_CORE_LOG_H
