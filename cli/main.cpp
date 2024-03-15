#include "BitcoinRpcServer.h"
#include "Config.h"
#include "Database.h"
#include "DigiByteCore.h"
#include <iostream>
#include <jsonrpccpp/client.h>


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
        cout << "error code: " << e.getCode() << "\n";
        cout << "error message:\n"
             << e.getMessage() << "\n";
    }
}