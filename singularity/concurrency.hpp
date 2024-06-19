#pragma once
#ifndef CONCURRENCY_HPP
#define CONCURRENCY_HPP

#include <atomic>
#include <mutex>
#include <queue>

#include "utils.hpp"

namespace singularity::concurrency {

template <typename T>
class DynamicBuffer {
   private:
    T* _storage;
    size_t _size;
    size_t _capacity;

    size_t _start;
    size_t _end;

    std::mutex _access;
    std::condition_variable _pop;

    void _grow() {
        // critical - this function does not assume mutual exclusion
        T* copy = new T[_capacity * 2];

        // manual unwind of _start and _end pointers
        for (size_t index = 0; index < _capacity; ++index) {
            copy[index] = std::move(_storage[(_start + index) % _capacity]);
        }

        std::swap(copy, _storage);
        delete[] copy;

        _capacity *= 2;
    }

   public:
    DynamicBuffer() : _size{0}, _capacity{8}, _start{0}, _end{0} {
        _storage = new T[_capacity];
    }

    DynamicBuffer(const DynamicBuffer& other)
        : _size{other.size},
          _capacity{other._capacity},
          _start{other.start},
          _end{other.end} {
        _storage = new T[_capacity];
        memcpy(_storage, other._storage, other._size * sizeof(T));
    }
    DynamicBuffer& operator=(const DynamicBuffer& other) {
        return *this = DynamicBuffer(other);
    }

    DynamicBuffer(DynamicBuffer&& other) = default;
    DynamicBuffer& operator=(DynamicBuffer&& other) = default;

    ~DynamicBuffer() { delete[] _storage; }

    void push(T&& object) {
        std::unique_lock<std::mutex> lock(_access);

        if (_size == _capacity) _grow();
        size_t index = _end++ % _capacity;
        _storage[index] = std::forward<T>(object);
        ++_size;

        _pop.notify_one();
    }

    T pop() {
        std::unique_lock<std::mutex> lock(_access);
        if (_size == 0) _pop.wait(lock);
        T item = std::move(_storage[_start]);
        _start = ++_start % _capacity;
        --_size;
        return item;
    }

    [[nodiscard]] size_t size() {
        std::unique_lock<std::mutex> lock(_access);
        return _size;
    }
    [[nodiscard]] bool empty() {
        std::unique_lock<std::mutex> lock(_access);
        return _size == 0;
    }
};

template <typename T, size_t buffer_size>
class FixedBuffer {
   private:
    T _storage[buffer_size];
    size_t _size;

    size_t _start;
    size_t _end;

    std::mutex _data_mutex;
    std::condition_variable _wait_push;
    std::condition_variable _wait_pop;

   public:
    FixedBuffer() : _size{0}, _start{0}, _end{0} {
        static_assert(buffer_size > 0, "Buffer size must be greater than 0.");
    }

    FixedBuffer(const FixedBuffer& other)
        : _size{other.size}, _start{other.start}, _end{other.end} {
        memcpy(_storage, other._storage, other._size * sizeof(T));
    }
    FixedBuffer& operator=(const FixedBuffer& other) {
        return *this = FixedBuffer(other);
    }

    FixedBuffer(FixedBuffer&& other) = default;
    FixedBuffer& operator=(FixedBuffer&& other) = default;

    void push(T&& object) {
        std::unique_lock<std::mutex> lock(_data_mutex);
        while (_size == buffer_size) _wait_pop.wait(lock);
        _storage[_end] = std::forward<T>(object);
        ++_size;
        _end = (_end + 1) % buffer_size;
        _wait_push.notify_all();
    }

    T pop() {
        std::unique_lock<std::mutex> lock(_data_mutex);
        while (_size == 0) _wait_push.wait(lock);
        T object = std::move(_storage[_start]);
        --_size;

        _start = (_start + 1) % buffer_size;
        _wait_pop.notify_all();
        return object;
    }

    [[nodiscard]] size_t size() {
        std::unique_lock<std::mutex> lock(_data_mutex);
        return _size;
    }

    [[nodiscard]] bool empty() {
        std::unique_lock<std::mutex> lock(_data_mutex);
        return _size == 0;
    }
};

}  // namespace singularity::concurrency

#endif  // CONCURRENCY_HPP