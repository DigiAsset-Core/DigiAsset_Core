//
// Created by mctrivia on 18/08/23.
//

#include "Threaded.h"
#include "Log.h"
#include <future>

using namespace std;

/**
 * This function handles the new thread
 */
void Threaded::_threadFunction() {
    //startup thread.  A throw here would leave the thread function, and an exception leaving a
    //std::thread ends the process on the spot with nothing written anywhere - say what happened
    //and let the rest of the daemon carry on instead
    try {
        startupFunction();
    } catch (const std::exception& e) {
        Log::GetInstance()->addMessage(string("Background task could not start: ") + e.what(), Log::CRITICAL);
        _stopRequest = false;
        _running = false;
        return;
    } catch (...) {
        Log::GetInstance()->addMessage("Background task could not start(non standard error)", Log::CRITICAL);
        _stopRequest = false;
        _running = false;
        return;
    }

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
                    _reportResult(*it);
                    it = subThreads.erase(it);
                }
                else {
                    ++it;
                }
            }
            this_thread::sleep_for(chrono::milliseconds(10));// Avoid busy-waiting
        }
    }

    //wait for all sub threads to be done
    for (auto& future: subThreads) {
        _reportResult(future);
    }

    //shutdown
    try {
        shutdownFunction();
    } catch (const std::exception& e) {
        Log::GetInstance()->addMessage(string("Background task did not shut down cleanly: ") + e.what(), Log::ERROR);
    } catch (...) {
        Log::GetInstance()->addMessage("Background task did not shut down cleanly(non standard error)", Log::ERROR);
    }
    _stopRequest = false;
    _running = false;
}

/**
 * Collects a finished sub thread and says something if it ended badly.
 * std::async stores the exception in the future and destroying the future throws it away, so
 * before this every error escaping a mainFunction() vanished with no trace at all - a node
 * whose IPFS or analyzer work was failing looked exactly like one that was simply idle
 */
void Threaded::_reportResult(std::future<void>& result) {
    try {
        result.get();
    } catch (const std::exception& e) {
        Log::GetInstance()->addMessage(string("Uncaught error in background task: ") + e.what(), Log::ERROR);
    } catch (...) {
        Log::GetInstance()->addMessage("Uncaught non standard error in background task", Log::ERROR);
    }
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

    //a thread that ended on its own(a startup that failed) is still joinable, and assigning
    //over a joinable std::thread ends the process
    if (_thread.joinable()) _thread.join();
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
    }

    //join outside the _running check: a thread whose startupFunction failed has already cleared
    //_running but is still joinable, and destroying it unjoined ends the process
    if (_thread.joinable()) _thread.join();
    _stopRequest = false;
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

/**
 * Allows main function to check if it should allow shutdown
 * @return
 */
bool Threaded::stopRequested() const {
    return _stopRequest;
}
