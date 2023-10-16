#include "DigiByteCore.h"
#include "Database.h"
#include "ChainAnalyzer.h"
#include "IPFS.h"
#include "BitcoinRpcServer.h"
#include "Log.h"
#include "Config.h"
#include <iostream>


int main(int argc, char *argv[]) {
    if (argc<2) return 1;

    //get input
    string command=argv[1];
    Value args=Value(Json::arrayValue);
    for (int i = 2; i < argc; ++i) {
        // Try to convert the argument to a number
        char *endptr;
        double num = strtod(argv[i], &endptr);
        if (*endptr == '\0') {
            // Successfully converted to a number
            args.append(Json::Value(num));
        } else if (string(argv[i]) == "true" || string(argv[i]) == "false") {
            // Handle "true" and "false" as bool values
            args.append(Json::Value(argv[i] == string("true")));
        } else {
            // Otherwise, treat it as a string
            args.append(Json::Value(argv[i]));
        }
    }

    //ask core what it means
    DigiByteCore api = DigiByteCore();
    api.setFileName("config.cfg",true);
    api.makeConnection();
    try {
        cout << api.sendcommand(command, args) << "\n";
    } catch (const DigiByteException& e) {
        cout <<"error code: " << e.getCode() << "\n";
        cout <<"error message:\n" << e.getMessage() << "\n";
    }
}