//
// Created by mctrivia on 08/03/24.
//

#ifndef DIGIASSET_CORE_UNIQUETASKQUEUE_H
#define DIGIASSET_CORE_UNIQUETASKQUEUE_H



#include <queue>
#include <unordered_set>
#include <mutex>
#include <string>
#include <condition_variable>
class UniqueTaskQueue {
private:
    std::mutex _mutex;
    std::condition_variable _cond;
    std::queue<std::string> _queue;
    std::unordered_set<std::string> _set;

public:
    // Adds a task to the queue if it's not already present
    bool enqueue(const std::string& task);

    // Retrieves and removes the next task from the queue
    // Blocks if the queue is empty until a new task is added
    std::string dequeue();

    // Checks if the queue is empty (primarily for testing or conditional processing)
    bool isEmpty();
    unsigned int length();
};



#endif //DIGIASSET_CORE_UNIQUETASKQUEUE_H
