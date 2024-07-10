//
// Created by mctrivia on 05/09/23.
//

#ifndef DIGIASSET_CORE_CONFIG_H
#define DIGIASSET_CORE_CONFIG_H

#include <map>
#include <string>
#include <vector>

using namespace std;

class Config {
    map<string, string> _values;
    string _fileName;
    static bool isInteger(const string& value);
    static bool isBool(const string& value);

public:
    static const unsigned char UNKNOWN = 0;
    static const unsigned char STRING = 1;
    static const unsigned char INTEGER = 10;


    explicit Config(const string& fileName);
    explicit Config();


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
    protected:
        std::string _lastErrorMessage;
        mutable std::string _fullErrorMessage;

    public:
        explicit exception(const std::string& message = "Unknown") : _lastErrorMessage(message) {}

        virtual const char* what() const noexcept override {
            _fullErrorMessage = "Config Exception: " + _lastErrorMessage;
            return _fullErrorMessage.c_str();
        }
    };

    class exceptionConfigFileMissing : public exception {
    public:
        explicit exceptionConfigFileMissing()
            : exception("File missing") {}
    };

    /**
     * A value is missing or of wrong type
     */
    class exceptionCorruptConfigFile : public exception {
    public:
        explicit exceptionCorruptConfigFile(const std::string& error = "File corrupt")
            : exception(error) {}
    };

    /**
     * A value is missing
     */
    class exceptionCorruptConfigFile_Missing : public exceptionCorruptConfigFile {
    public:
        explicit exceptionCorruptConfigFile_Missing(const std::string& error = "Missing value")
            : exceptionCorruptConfigFile(error) {}
    };

    /**
     * A value is wrong type
     */
    class exceptionCorruptConfigFile_WrongType : public exceptionCorruptConfigFile {
    public:
        explicit exceptionCorruptConfigFile_WrongType(const std::string& error = "Value wrong type")
            : exceptionCorruptConfigFile(error) {}
    };

    class exceptionConfigFileCouldNotBeWritten : public exception {
    public:
        explicit exceptionConfigFileCouldNotBeWritten()
            : exception("Couldn't Write") {}
    };

    /**
     * Not used by class direct but can be used by code using class as common way of showing that a value in config is not correct
     */
    class exceptionConfigFileInvalid : public exception {
    public:
        explicit exceptionConfigFileInvalid(const std::string& error = "Values in the config file are not correct")
            : exception(error) {}
    };
};



#endif //DIGIASSET_CORE_CONFIG_H
