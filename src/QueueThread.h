//
// Created by mctrivia on 05/06/24.
//

#ifndef DIGIASSET_CORE_QUETHREAD_H
#define DIGIASSET_CORE_QUETHREAD_H


#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

template <typename T>
class QueueThread {
private:
    std::queue<T> _queue;
    std::mutex _mutex;
    std::condition_variable _condVar;
    bool _terminate = false;
    bool _isRunning = false;
    std::thread _worker;
    std::function<void(const T&)> _processFunc;

    [[noreturn]] void processQueue();

public:
    explicit QueueThread(std::function<void(const T&)> processFunc);
    ~QueueThread();
    void add(const T& item);
    void terminate();
    bool isWorkerRunning() const { return _isRunning; }
};



#endif //DIGIASSET_CORE_QUETHREAD_H
