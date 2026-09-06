//
// Created by mctrivia on 26/07/23.
//

#include "TestHelpers.h"
#include "BitIO.h"

#include <cmath>
#include <cstdlib>
#include <exception>
#include <execinfo.h>
#include <string>
#include <unistd.h>
#include <vector>

using namespace std;

namespace {
    //if any thread dies with an uncaught exception(e.g. the intermittent "mutex lock
    //failed" during process exit) print WHERE it was thrown from before aborting -
    //the default terminate gives no backtrace which made that flake undebuggable
    [[noreturn]] void printBacktraceAndAbort() {
        void* frames[64];
        int count = backtrace(frames, 64);
        backtrace_symbols_fd(frames, count, STDERR_FILENO);
        abort();
    }
    struct TerminateDiagnosticInstaller {
        TerminateDiagnosticInstaller() { std::set_terminate(printBacktraceAndAbort); }
    } terminateDiagnosticInstaller;
} // namespace

string TestHelpers::getCSVValue(const string& line, size_t& li) {
    string result;
    while ((line[li] != '\n') && (line[li] != ',')) {
        result.push_back(line[li]);
        li++;
    }
    li++;
    return result;
}

vector<uint8_t> TestHelpers::hexToVector(const string& value) {
    const std::string charSet = BITIO_CHARSET_HEX;
    vector<uint8_t> result;
    for (size_t i = 0; i < value.size(); i += 2) {
        uint8_t hNibble = charSet.find(value[i]);
        uint8_t lNibble = charSet.find(value[i + 1]);
        result.push_back((hNibble << 4) | lNibble);
    }
    return result;
}

bool TestHelpers::approximatelyEqual(double a, double b) {
    return fabs(a - b) <= ((fabs(a) < fabs(b) ? fabs(b) : fabs(a)) * std::numeric_limits<double>::epsilon());
}
