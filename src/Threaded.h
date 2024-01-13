//
// Created by mctrivia on 18/08/23.
//

#ifndef DIGIASSET_CORE_THREADED_H
#define DIGIASSET_CORE_THREADED_H



#include <future>
#include <thread>
#include <vector>

class Threaded {
    std::thread _thread;
    volatile bool _running = false;
    volatile bool _stopRequest = false;
    void _threadFunction();
    size_t _parallels = 1;//if task is asynchronous allows running sub threads within thread.

protected:
    virtual void startupFunction();
    virtual void mainFunction();
    virtual void shutdownFunction();
    void setMaxParallels(size_t max = 1);

public:
    bool stopRequested();
    void start();
    void stop();
    ~Threaded();
};



#endif//DIGIASSET_CORE_THREADED_H
