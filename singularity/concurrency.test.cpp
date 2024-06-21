#include "concurrency.hpp"

#include <gtest/gtest.h>

#include <thread>

using namespace singularity;

TEST(DynamicBufferTestSingleThread, Size) {
    concurrency::DynamicBuffer<int> queue;
    EXPECT_EQ(queue.size(), 0);
    queue.push(1);
    queue.push(2);
    queue.push(3);

    EXPECT_EQ(queue.size(), 3);
    queue.pop();
    EXPECT_EQ(queue.size(), 2);
    queue.pop();
    queue.pop();
    EXPECT_EQ(queue.size(), 0);
}

TEST(DynamicBufferTestSingleThread, Empty) {
    concurrency::DynamicBuffer<int> queue;
    EXPECT_TRUE(queue.empty());
    queue.push(1);
    EXPECT_FALSE(queue.empty());
    queue.pop();
    EXPECT_TRUE(queue.empty());
}

TEST(DynamicBufferTestSingleThread, FIFO) {
    concurrency::DynamicBuffer<int> queue;
    EXPECT_EQ(queue.size(), 0);
    queue.push(1);
    queue.push(2);
    queue.push(3);

    size_t index = 0;
    std::array<int, 3> expected_order = {1, 2, 3};
    while (!queue.empty()) {
        EXPECT_EQ(queue.pop(), expected_order[index++]);
    }
}

TEST(DynamicBufferTestSingleThread, InterleaveTest) {
    concurrency::DynamicBuffer<int> queue;
}

TEST(DynamicBufferTestMultiThread, Size) {
    concurrency::DynamicBuffer<int> queue;

    std::vector<std::thread> backing(10);
    for (size_t index = 0; index < 10; ++index) {
        backing.emplace_back([&queue]() {
            for (int i = 0; i < 100; i++) {
                queue.push(i - 50);
            }
        });
    }

    for (auto& thread : backing) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    backing.clear();

    EXPECT_EQ(queue.size(), 1000);

    for (size_t index = 0; index < 10; ++index) {
        backing.emplace_back([&queue]() {
            for (int i = 0; i < 100; i++) {
                queue.pop();
            }
        });
    }

    for (auto& thread : backing) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    EXPECT_EQ(queue.size(), 0);
}

TEST(DynamicBufferTestMultiThread, Empty) {
    concurrency::DynamicBuffer<int> queue;
    EXPECT_TRUE(queue.empty());

    std::vector<std::thread> backing(10);
    for (size_t index = 0; index < 10; ++index) {
        backing.emplace_back([&queue]() {
            for (int i = 0; i < 100; i++) {
                queue.push(i - 50);
            }
        });
    }

    for (auto& thread : backing) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    EXPECT_FALSE(queue.empty());
}

TEST(FixedBufferTest, SimpleTest) {
    concurrency::FixedBuffer<int, 3> buffer;
    EXPECT_EQ(buffer.size(), 0);
    buffer.push(1);
    buffer.push(2);
    buffer.push(3);
    EXPECT_EQ(buffer.size(), 3);

    EXPECT_EQ(1, buffer.pop());
    buffer.push(4);
    EXPECT_EQ(2, buffer.pop());
    buffer.push(5);
    EXPECT_EQ(3, buffer.pop());
    buffer.push(6);

    EXPECT_EQ(buffer.size(), 3);

    size_t index = 0;
    std::array<int, 3> expected_order = {4, 5, 6};
    while (!buffer.empty()) {
        EXPECT_EQ(buffer.pop(), expected_order[index++]);
    }
}

TEST(FixedBufferTest, TimeoutPop) {
    concurrency::FixedBuffer<int, 3> buffer;
    auto elt = buffer.pop(std::chrono::milliseconds(10));
    EXPECT_EQ(elt, std::nullopt);
}

TEST(FixedBufferTest, TimeoutPush) {
    concurrency::FixedBuffer<int, 3> buffer;
    buffer.push(1);
    buffer.push(2);
    buffer.push(3);
    EXPECT_FALSE(buffer.push(4, std::chrono::milliseconds(10)));
}

TEST(FixedBufferTest, MultithreadedTest) {
    concurrency::FixedBuffer<int, 1000> queue;

    std::vector<std::thread> backing(10);
    for (size_t index = 0; index < 10; ++index) {
        backing.emplace_back([&queue]() {
            for (int i = 0; i < 100; i++) {
                queue.push(i - 50);
            }
        });
    }

    for (auto& thread : backing) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    backing.clear();

    EXPECT_EQ(queue.size(), 1000);

    for (size_t index = 0; index < 10; ++index) {
        backing.emplace_back([&queue]() {
            for (int i = 0; i < 100; i++) {
                queue.pop();
            }
        });
    }

    for (auto& thread : backing) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    EXPECT_EQ(queue.size(), 0);
}