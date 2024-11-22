//
// Created by mctrivia on 04/10/23.
//

#ifndef DIGIASSET_CORE_LOG_H
#define DIGIASSET_CORE_LOG_H



#include <fstream>
#include <iostream>
#include <mutex>
#include <string>

using namespace std;


class Log {
public:
    enum LogLevel {
        DEBUG = 0,
        INFO = 10,
        WARNING = 20,
        ERROR = 30,
        CRITICAL = 40
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
    protected:
        std::string _lastErrorMessage;
        mutable std::string _fullErrorMessage;

    public:
        explicit exception(const std::string& message = "Unknown") : _lastErrorMessage(message) {}

        virtual const char* what() const noexcept override {
            _fullErrorMessage = "Log Exception: " + _lastErrorMessage;
            return _fullErrorMessage.c_str();
        }
    };

    class exceptionFailedToOpen : public exception {
    public:
        explicit exceptionFailedToOpen()
            : exception("Couldn't open or create the log") {}
    };
};


#endif //DIGIASSET_CORE_LOG_H
