//
// Created by mctrivia on 21/12/24.
//

#include "InstanceLock.h"
#include <iostream>
#include <csignal>
#include <fstream>
#include <string>

int InstanceLock::_lockFileDescriptor = -1;

#ifdef _WIN32

#include <windows.h>

static HANDLE g_mutexHandle = NULL;

static std::string getExecutablePath() {
    char path[MAX_PATH];
    DWORD len = GetModuleFileNameA(NULL, path, MAX_PATH);
    if (len > 0) {
        std::string exePath(path, len);
        size_t pos = exePath.find_last_of('\\');
        if (pos != std::string::npos) return exePath.substr(0, pos);
    }
    return ".";
}

InstanceLock::InstanceLock(const std::string& lockName) {
    _lockFilePath = getExecutablePath() + "\\" + lockName + ".lock";
}

bool InstanceLock::acquire() {
    // Use a named mutex for single-instance enforcement on Windows
    size_t pos = _lockFilePath.find_last_of("\\/");
    std::string baseName = (pos != std::string::npos) ? _lockFilePath.substr(pos + 1) : _lockFilePath;
    std::string mutexName = "Global\\DigiAssetCore_" + baseName;
    g_mutexHandle = CreateMutexA(NULL, TRUE, mutexName.c_str());
    if (g_mutexHandle == NULL) {
        std::cerr << "Failed to create mutex: " << GetLastError() << std::endl;
        return false;
    }
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        std::cerr << "Another instance is already running." << std::endl;
        CloseHandle(g_mutexHandle);
        g_mutexHandle = NULL;
        return false;
    }
    setupSignalHandlers();
    return true;
}

void InstanceLock::setupSignalHandlers() {
    signal(SIGINT,  cleanupOnSignal);
    signal(SIGTERM, cleanupOnSignal);
    signal(SIGABRT, cleanupOnSignal);
}

void InstanceLock::cleanupOnSignal(int sig) {
    if (g_mutexHandle != NULL) {
        ReleaseMutex(g_mutexHandle);
        CloseHandle(g_mutexHandle);
        g_mutexHandle = NULL;
    }
    exit(sig);
}

InstanceLock::~InstanceLock() { release(); }

void InstanceLock::release() {
    if (g_mutexHandle != NULL) {
        ReleaseMutex(g_mutexHandle);
        CloseHandle(g_mutexHandle);
        g_mutexHandle = NULL;
    }
}

#elif defined(__APPLE__)

#include <mach-o/dyld.h>
#include <unistd.h>
#include <csignal>
#include <sys/file.h>
#include <fcntl.h>
#include <limits.h>

static std::string getExecutablePath() {
    char path[PATH_MAX];
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) == 0) {
        std::string exePath(path);
        return exePath.substr(0, exePath.find_last_of('/'));
    }
    return ".";
}

InstanceLock::InstanceLock(const std::string& lockName) {
    _lockFilePath = getExecutablePath() + "/" + lockName + ".lock";
}

bool InstanceLock::acquire() {
    _lockFileDescriptor = open(_lockFilePath.c_str(), O_CREAT | O_RDWR, 0666);
    if (_lockFileDescriptor < 0) {
        std::cerr << "Failed to open or create lock file: " << _lockFilePath << std::endl;
        return false;
    }

    pid_t existingPid = 0;
    std::ifstream inFile(_lockFilePath);
    inFile >> existingPid;
    inFile.close();

    if (existingPid > 0 && kill(existingPid, 0) == 0) {
        std::cerr << "Another instance is already running (PID " << existingPid << ")." << std::endl;
        return false;
    }

    if (flock(_lockFileDescriptor, LOCK_EX | LOCK_NB) == -1) {
        std::cerr << "Another instance is already running (active lock)." << std::endl;
        return false;
    }

    ftruncate(_lockFileDescriptor, 0);
    dprintf(_lockFileDescriptor, "%d\n", getpid());

    setupSignalHandlers();
    return true;
}

void InstanceLock::setupSignalHandlers() {
    signal(SIGINT,  cleanupOnSignal);
    signal(SIGTERM, cleanupOnSignal);
    signal(SIGSEGV, cleanupOnSignal);
    signal(SIGABRT, cleanupOnSignal);
}

void InstanceLock::cleanupOnSignal(int sig) {
    if (_lockFileDescriptor != -1) {
        unlink(getExecutablePath().append("/digiasset_core.lock").c_str());
    }
    exit(sig);
}

InstanceLock::~InstanceLock() { release(); }

void InstanceLock::release() {
    if (_lockFileDescriptor != -1) {
        unlink(_lockFilePath.c_str());
        close(_lockFileDescriptor);
    }
}

#else // Linux

#include <unistd.h>
#include <csignal>
#include <sys/file.h>
#include <fcntl.h>
#include <limits.h>

static std::string getExecutablePath() {
    char path[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", path, PATH_MAX);
    if (count != -1) {
        std::string exePath(path, count);
        return exePath.substr(0, exePath.find_last_of('/'));
    }
    return ".";
}

InstanceLock::InstanceLock(const std::string& lockName) {
    _lockFilePath = getExecutablePath() + "/" + lockName + ".lock";
}

bool InstanceLock::acquire() {
    _lockFileDescriptor = open(_lockFilePath.c_str(), O_CREAT | O_RDWR, 0666);
    if (_lockFileDescriptor < 0) {
        std::cerr << "Failed to open or create lock file: " << _lockFilePath << std::endl;
        return false;
    }

    pid_t existingPid = 0;
    std::ifstream inFile(_lockFilePath);
    inFile >> existingPid;
    inFile.close();

    if (existingPid > 0 && kill(existingPid, 0) == 0) {
        std::cerr << "Another instance is already running (PID " << existingPid << ")." << std::endl;
        return false;
    }

    if (flock(_lockFileDescriptor, LOCK_EX | LOCK_NB) == -1) {
        std::cerr << "Another instance is already running (active lock)." << std::endl;
        return false;
    }

    ftruncate(_lockFileDescriptor, 0);
    dprintf(_lockFileDescriptor, "%d\n", getpid());

    setupSignalHandlers();
    return true;
}

void InstanceLock::setupSignalHandlers() {
    signal(SIGINT,  cleanupOnSignal);
    signal(SIGTERM, cleanupOnSignal);
    signal(SIGSEGV, cleanupOnSignal);
    signal(SIGABRT, cleanupOnSignal);
}

void InstanceLock::cleanupOnSignal(int sig) {
    if (_lockFileDescriptor != -1) {
        unlink(getExecutablePath().append("/digiasset_core.lock").c_str());
    }
    exit(sig);
}

InstanceLock::~InstanceLock() { release(); }

void InstanceLock::release() {
    if (_lockFileDescriptor != -1) {
        unlink(_lockFilePath.c_str());
        close(_lockFileDescriptor);
    }
}

#endif
