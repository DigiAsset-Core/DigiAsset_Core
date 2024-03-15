//
// Created by mctrivia on 08/03/24.
//

#include "UniqueTaskQueue.h"
#include <queue>
#include <unordered_set>
#include <mutex>
#include <string>
#include <condition_variable>

using namespace std;

// Adds a task to the queue if it's not already present
bool UniqueTaskQueue::enqueue(const string& task) {
    unique_lock<mutex> lock(_mutex);
    if (_set.find(task) == _set.end()) {
        _queue.push(task);
        _set.insert(task);
        lock.unlock();
        _cond.notify_one(); // Notify one waiting thread
        return true;
    }
    return false; // Task was already in the set
}

// Retrieves and removes the next task from the queue
// Blocks if the queue is empty until a new task is added
string UniqueTaskQueue::dequeue() {
    unique_lock<mutex> lock(_mutex);
    _cond.wait(lock, [this] { return !_queue.empty(); }); // Wait until the queue is not empty

    string task = _queue.front();
    _queue.pop();
    _set.erase(task);
    return task;
}

// Checks if the queue is empty (primarily for testing or conditional processing)
bool UniqueTaskQueue::isEmpty() {
    lock_guard<mutex> lock(_mutex);
    return _queue.empty();
}

unsigned int UniqueTaskQueue::length() {
    lock_guard<mutex> lock(_mutex);
    return _queue.size();
}
