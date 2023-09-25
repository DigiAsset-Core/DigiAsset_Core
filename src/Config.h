//
// Created by mctrivia on 05/09/23.
//

#ifndef DIGIASSET_CORE_CONFIG_H
#define DIGIASSET_CORE_CONFIG_H

#include <string>
#include <vector>
#include <map>

using namespace std;

class Config {
    static string _lastErrorMessage;
    map<string, string> _values;
    string _fileName;
    static bool isInteger(const string& value);
    static bool isBool(const string& value);
public:
    static const unsigned char UNKNOWN = 0;
    static const unsigned char STRING = 1;
    static const unsigned char INTEGER = 10;


    explicit Config(const string& fileName);


    string getString(const string& key) const;
    string getString(const string& key, const string& defaultValue) const;
    int getInteger(const string& key) const;
    int getInteger(const string& key, int defaultValue) const;
    bool getBool(const string& key) const;
    bool getBool(const string& key, bool defaultValue) const;
    bool isKey(const string& key, unsigned char type = UNKNOWN) const;

    map<string, string> getStringMap(const string& keyPrefix) const;
    map<string, int> getIntegerMap(const string& keyPrefix) const;
    map<string, bool> getBoolMap(const string& keyPrefix) const;

    void setString(const string& key, const string& value);
    void setInteger(const string& key, int value);
    void setBool(const string& key, bool value);
    void setStringMap(const string& key, const map<string, string>& values);
    void setIntegerMap(const string& key, const map<string, int>& values);
    void setBoolMap(const string& key, const map<string, bool>& values);

    void clear();
    void refresh();
    void write(string fileName = "") const;

/*
    ███████╗██████╗ ██████╗  ██████╗ ██████╗ ███████╗
    ██╔════╝██╔══██╗██╔══██╗██╔═══██╗██╔══██╗██╔════╝
    █████╗  ██████╔╝██████╔╝██║   ██║██████╔╝███████╗
    ██╔══╝  ██╔══██╗██╔══██╗██║   ██║██╔══██╗╚════██║
    ███████╗██║  ██║██║  ██║╚██████╔╝██║  ██║███████║
    ╚══════╝╚═╝  ╚═╝╚═╝  ╚═╝ ╚═════╝ ╚═╝  ╚═╝╚══════╝
     */
    class exception : public std::exception {
    private:
        string _message;
    public:
        exception(const string& message = "unknown error") {
            _message = "Something went wrong with DigiByte Core: " + message;
        }

        char* what() {
            _lastErrorMessage = _message;
            return const_cast<char*>(_lastErrorMessage.c_str());
        }
    };

    class exceptionConfigFileMissing : public exception {
    public:
        char* what() {
            _lastErrorMessage = "The config file could not be found";
            return const_cast<char*>(_lastErrorMessage.c_str());
        }
    };

    /**
     * A value is missing or of wrong type
     */
    class exceptionCorruptConfigFile : public exception {
    public:
        char* what() {
            _lastErrorMessage = "The config file is corrupt";
            return const_cast<char*>(_lastErrorMessage.c_str());
        }
    };

    /**
     * A value is missing
     */
    class exceptionCorruptConfigFile_Missing : public exceptionCorruptConfigFile {
    public:
        char* what() {
            _lastErrorMessage = "The config file is corrupt";
            return const_cast<char*>(_lastErrorMessage.c_str());
        }
    };

    /**
     * A value is wrong type
     */
    class exceptionCorruptConfigFile_WrongType : public exceptionCorruptConfigFile {
    public:
        char* what() {
            _lastErrorMessage = "The config file is corrupt";
            return const_cast<char*>(_lastErrorMessage.c_str());
        }
    };

    class exceptionConfigFileCouldNotBeWritten : public exception {
    public:
        char* what() {
            _lastErrorMessage = "Saving config settings faile";
            return const_cast<char*>(_lastErrorMessage.c_str());
        }
    };

    /**
     * Not used by class direct but can be used by code using class as common way of showing that a value in config is not correct
     */
    class exceptionConfigFileInvalid : public exception {
    public:
        char* what() {
            _lastErrorMessage = "Values in the config file are not correct";
            return const_cast<char*>(_lastErrorMessage.c_str());
        }
    };
};



#endif //DIGIASSET_CORE_CONFIG_H
