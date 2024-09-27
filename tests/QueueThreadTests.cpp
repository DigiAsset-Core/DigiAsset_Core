//
// Created by mctrivia on 05/06/24.
//

#include "QueueThread.h"
#include <gtest/gtest.h>
#include <string>
#include <thread>
#include <vector>

// Helper function to be used with the queue
void processString(const std::string& str, std::vector<std::string>& output) {
    output.push_back(str);
}

// Test Fixture for QueueThread tests
class QueueThreadTest : public ::testing::Test {
protected:
    std::vector<std::string> results;
    std::function<void(const std::string&)> func;

    // Setup before each test
    void SetUp() override {
        func = [this](const std::string& str) { processString(str, results); };
    }

    // Cleanup after each test
    void TearDown() override {
    }
};

// Test adding items to the queue and processing them
TEST_F(QueueThreadTest, ProcessItems) {
    QueueThread<std::string> queueThread(func);

    queueThread.add("test1");
    queueThread.add("test2");
    queueThread.add("test3");

    // Give some time for processing
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    queueThread.terminate();

    // Check if all items were processed
    EXPECT_EQ(results.size(), 3);
    EXPECT_EQ(results[0], "test1");
    EXPECT_EQ(results[1], "test2");
    EXPECT_EQ(results[2], "test3");
}

// Test the queue starts and stops the worker thread correctly
TEST_F(QueueThreadTest, ThreadStartStop) {
    QueueThread<std::string> queueThread(func);

    EXPECT_FALSE(queueThread.isWorkerRunning()); // Initially not running

    queueThread.add("start");
    EXPECT_TRUE(queueThread.isWorkerRunning());

    queueThread.terminate();
    EXPECT_FALSE(queueThread.isWorkerRunning());
}

// To test termination before any items are processed
TEST_F(QueueThreadTest, EarlyTermination) {
    QueueThread<std::string> queueThread(func);

    queueThread.add("early test");
    queueThread.terminate();

    // As termination is called quickly, it might preempt any processing
    EXPECT_TRUE(results.empty() || results.size() == 1);
}
