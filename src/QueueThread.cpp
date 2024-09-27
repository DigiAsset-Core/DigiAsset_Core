//
// Created by mctrivia on 05/06/24.
//

#include "QueueThread.h"
#include <iostream>

template <typename T>
QueueThread<T>::QueueThread(std::function<void(const T&)> processFunc) : _processFunc(processFunc) {}

template <typename T>
QueueThread<T>::~QueueThread() {
    terminate();
    if (_worker.joinable()) {
        _worker.join();
    }
}

template <typename T>
void QueueThread<T>::add(const T& item) {
    std::unique_lock<std::mutex> lock(_mutex);
    _queue.push(item);
    if (_isRunning) return;

    //not running so start the thread   //todo figure out how this works
    if (!_worker.joinable()) {
        _isRunning = true;
        _worker = std::thread(&QueueThread::processQueue, this);
    }
    _condVar.notify_one();
}

template <typename T>
void QueueThread<T>::terminate() {
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _terminate = true;
        _isRunning = false;
    }
    _condVar.notify_all();
    if (_worker.joinable()) {
        _worker.join();
    }
}

template <typename T>
void QueueThread<T>::processQueue() {
    std::unique_lock<std::mutex> lock(_mutex);
    while (!_terminate || !_queue.empty()) {
        if (_queue.empty() && !_terminate) {
            _condVar.wait(lock, [this] { return _terminate || !_queue.empty(); });
        }
        while (!_queue.empty()) {
            T item = _queue.front();
            _queue.pop();
            lock.unlock(); // Unlock during the long-running operation
            _processFunc(item);
            lock.lock();
        }
        if (_queue.empty()) {
            _isRunning = false;
        }
    }
    _isRunning = false;
}

// Explicit template instantiation for expected types
template class QueueThread<std::string>;
// Add more explicit instantiations if other types are used
