//
// Created by mctrivia on 18/08/23.
//

#include "Threaded.h"
#include <future>

using namespace std;

/**
 * This function handles the new thread
 */
void Threaded::_threadFunction() {
    //startup thread
    startupFunction();

    //main function
    vector<future<void>> subThreads;
    while (!_stopRequest) {
        //run sub thread task
        subThreads.push_back(async(launch::async, &Threaded::mainFunction, this));

        //wait for a sub thread to be finished if full
        while (subThreads.size() >= _parallels) {
            auto it = subThreads.begin();
            while (it != subThreads.end()) {
                if (it->wait_for(chrono::seconds(0)) == future_status::ready) {
                    it = subThreads.erase(it);
                } else {
                    ++it;
                }
            }
            this_thread::sleep_for(chrono::milliseconds(10));  // Avoid busy-waiting
        }
    }

    //wait for all sub threads to be done
    for (auto& future: subThreads) {
        future.wait();
    }

    //shutdown
    shutdownFunction();
    _stopRequest = false;
    _running = false;
}

/**
 * Override this function if there is code that should be run when the thread is started
 */
void Threaded::startupFunction() {

}

/**
 * Override this function with the code that should be run continuously
 * It will keep getting executed until the thread is shut down
 */
void Threaded::mainFunction() {
    // if using sub treads and want to make sure 2 don't execute a part at the same time add
    //mutex _mutex; to your private section in header file and the following to the mainFunction where
    //you wish to prevent time overlap
    //unique_lock<mutex> lock(_mutex);
    //critical code
    //lock.unlock();
}

/**
 * Override this function if there is code that should be run when the thread is shut down
 */
void Threaded::shutdownFunction() {

}

/**
 * Starts the thread
 */
void Threaded::start() {
    //don't allow loop to run twice
    if (_running) return;
    _running = true;

    //load loop in thread
    _thread = thread(&Threaded::_threadFunction, this);
}

/**
 * Ends the thread
 */
void Threaded::stop() {
    if (_running) {
        _stopRequest = true;
        while (_running) {
            chrono::milliseconds dura(100);
            this_thread::sleep_for(dura);
        }
        _thread.join();
        _stopRequest = false;
    }
}

/**
 * makes sure the thread shuts down correctly
 */
Threaded::~Threaded() {
    stop();
}

void Threaded::setMaxParallels(size_t max) {
    _parallels = max;
}