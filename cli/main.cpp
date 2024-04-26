#include "Config.h"
#include "Database.h"
#include "DigiByteCore.h"
#include "RPC/Server.h"
#include <iostream>
#include <jsonrpccpp/client.h>
#include <regex>


int main(int argc, char* argv[]) {
    if (argc < 2) return 1;

    //get command
    string command = argv[1];

    // Prepare to parse arguments into JSON
    Json::Value args = Json::arrayValue;
    Json::Reader reader;

    for (int i = 2; i < argc; ++i) {
        std::string argStr(argv[i]);
        Json::Value parsedValue;

        // Check if the argument could be a JSON array or object
        if ((argStr.front() == '[' && argStr.back() == ']') || (argStr.front() == '{' && argStr.back() == '}')) {
            bool parsingSuccessful = reader.parse(argStr, parsedValue);
            if (parsingSuccessful) {
                // If it's a JSON array or object, append it directly
                args.append(parsedValue);
            } else {
                // Parsing failed, treat as a string
                args.append(argStr);
            }
        } else {
            // For non-JSON strings, attempt to parse as number or boolean
            char* endptr;
            double num = strtod(argv[i], &endptr);
            if (*endptr == '\0') {
                args.append(num);
            } else if (argStr == "true") {
                args.append(true);
            } else if (argStr == "false") {
                args.append(false);
            } else {
                args.append(argStr);
            }
        }
    }

    //ask core what it means
    DigiByteCore dgb;
    dgb.setFileName("config.cfg", true);
    dgb.makeConnection();
    try {
        cout << dgb.sendcommand(command, args) << "\n";
    } catch (const DigiByteException& e) {
        string errorMessage=e.getMessage();

        //check if DigiAsset Core is offline
        if (errorMessage.substr(0,20)=="Could not connect to") {
            cout << "Exception: It looks like DigiAsset Core RPC Service is down.";
            return 0;
        }

        //check if command is forbiden
        regex pattern(">>.* is forbidden<<");
        if (regex_search(errorMessage, pattern)) {
            cout << "Exception: " + command + " is forbidden by config settings.";
            return 0;
        }




        //show generic error
        cout << "error code: " << e.getCode() << "\n";
        cout << "error message:\n"
             << errorMessage << "\n";
    } catch (...) {
        cout << "Exception: unexpected error.";
    }
}