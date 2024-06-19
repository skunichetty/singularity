#include <iostream>
#include <thread>

#include "concurrency.hpp"

constexpr size_t NUM_PRODUCERS = 10;
constexpr size_t NUM_CONSUMERS = 10;
constexpr size_t TOTAL_ITEMS = 10000000;

struct TestStruct {
    bool yes;
    int hello;
    double hi;
    char space[100] = {0};
};

int main() {
    singularity::concurrency::FixedBuffer<TestStruct, 1000> buffer;

    std::vector<std::thread> producers(NUM_PRODUCERS);
    for (size_t index = 0; index < NUM_PRODUCERS; ++index) {
        producers.emplace_back([&buffer]() {
            for (int i = 0; i < TOTAL_ITEMS / NUM_PRODUCERS; i++) {
                buffer.push({i % 2 == 0, i, 3.14 - i + 40});
            }
        });
    }

    std::vector<std::thread> consumers(NUM_CONSUMERS);
    for (size_t index = 0; index < NUM_CONSUMERS; ++index) {
        consumers.emplace_back([&buffer]() {
            for (int i = 0; i < TOTAL_ITEMS / NUM_CONSUMERS; i++) {
                auto x = buffer.pop();
                x.hello += 1;
            }
        });
    }

    std::thread sizePrinter([&buffer]() {
        auto startTime = std::chrono::steady_clock::now();
        while (std::chrono::steady_clock::now() - startTime <
               std::chrono::seconds(7)) {
            std::cout << "Buffer size: " << buffer.size() << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
        }
    });

    sizePrinter.join();

    for (auto& thread : producers) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    for (auto& thread : consumers) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    std::cout << (buffer.size()) << std::endl;
}