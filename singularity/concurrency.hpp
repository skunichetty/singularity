#pragma once
#ifndef CONCURRENCY_HPP
#define CONCURRENCY_HPP

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

#include "utils.hpp"

namespace singularity::concurrency {

template <typename T>
class Buffer {
    virtual void push(T&& object) = 0;
    virtual T pop() = 0;
    [[nodiscard]] virtual size_t size() const = 0;
    [[nodiscard]] virtual bool empty() const = 0;
};

/**
 * @brief A dynamic buffer class that supports concurrent push and pop
 * operations.
 *
 * The DynamicBuffer class provides a thread-safe implementation of a dynamic
 * buffer, allowing elements to be pushed and popped concurrently. The buffer
 * grows dynamically with inputs, but threads will block if the buffer is empty.
 *
 * @tparam T The type of elements stored in the buffer.
 */
template <typename T>
class DynamicBuffer : public Buffer<T> {
   private:
    T* _storage;
    std::allocator<T> _allocator;

    size_t _size;
    size_t _capacity;
    size_t _start;
    size_t _end;

    mutable std::mutex _access;
    mutable std::condition_variable _pop;

    void _grow() {
        T* copy = _allocator.allocate(_capacity * 2);

        for (size_t index = 0; index < _capacity; ++index) {
            copy[index] = std::move(_storage[(_start + index) % _capacity]);
        }

        std::swap(copy, _storage);
        _allocator.deallocate(copy, _capacity);

        _capacity *= 2;
    }

   public:
    DynamicBuffer() : _size{0}, _capacity{8}, _start{0}, _end{0} {
        _storage = _allocator.allocate(_capacity);
    }

    DynamicBuffer(const DynamicBuffer& other)
        : _size{other.size},
          _capacity{other._capacity},
          _start{other.start},
          _end{other.end} {
        _storage = _allocator.allocate(_capacity);
        memcpy(_storage, other._storage, other._size * sizeof(T));
    }

    DynamicBuffer& operator=(const DynamicBuffer& other) {
        return (*this = DynamicBuffer(other));
    }

    DynamicBuffer(DynamicBuffer&& other) = default;
    DynamicBuffer& operator=(DynamicBuffer&& other) = default;

    ~DynamicBuffer() { _allocator.deallocate(_storage, _capacity); }

    /**
     * @brief Pushes an element into the buffer.
     *
     * Adds a new element to the buffer. If the buffer is full, it will be
     * automatically resized to accommodate the new element. This operation is
     * thread-safe.
     *
     * @param object The element to be pushed into the buffer.
     */
    void push(T&& object) override {
        std::unique_lock<std::mutex> lock(_access);

        if (_size == _capacity) _grow();
        size_t index = _end++ % _capacity;
        _storage[index] = std::forward<T>(object);
        ++_size;

        _pop.notify_one();
    }

    /**
     * @brief Pops an element from the buffer.
     *
     * Removes and returns the first element from the buffer. If the buffer is
     * empty, the calling thread will be blocked until an element becomes
     * available. This operation is thread-safe.
     *
     * @return The first element in the buffer.
     */
    T pop() override {
        std::unique_lock<std::mutex> lock(_access);
        if (_size == 0) _pop.wait(lock);
        T item = std::move(_storage[_start]);
        _start = (_start + 1) % _capacity;
        --_size;
        return item;
    }

    /**
     * @brief Returns the current number of elements in the buffer.
     *
     * This operation is thread-safe.
     *
     * @return The number of elements in the buffer.
     */
    [[nodiscard]] size_t size() const override {
        std::unique_lock<std::mutex> lock(_access);
        return _size;
    }

    /**
     * @brief Checks if the buffer is empty.
     *
     * This operation is thread-safe.
     *
     * @return True if the buffer is empty, false otherwise.
     */
    [[nodiscard]] bool empty() const override {
        std::unique_lock<std::mutex> lock(_access);
        return _size == 0;
    }
};

/**
 * @brief A fixed-size buffer implementation for storing elements of type T.
 *
 * This implementation is thread-safe, and threads will block when unable to
 * push or pop an element from the buffer
 *
 * @tparam T The type of elements to be stored in the buffer.
 * @tparam buffer_size The maximum number of elements that can be stored in the
 * buffer.
 */
template <typename T, size_t buffer_size>
class FixedBuffer : public Buffer<T> {
   private:
    T* _storage;
    std::allocator<T> _allocator;

    size_t _size;
    size_t _start;
    size_t _end;

    mutable std::mutex _data_mutex;
    mutable std::condition_variable _wait_push;
    mutable std::condition_variable _wait_pop;

    void _push(T&& object) {
        _storage[_end] = std::move(object);
        ++_size;
        _end = (_end + 1) % buffer_size;
        _wait_push.notify_one();
    }

    T _pop() {
        T object = std::move(_storage[_start]);
        --_size;
        _start = (_start + 1) % buffer_size;
        _wait_pop.notify_one();
        return object;
    }

   public:
    FixedBuffer() : _size{0}, _start{0}, _end{0} {
        static_assert(buffer_size > 0, "Buffer size must be greater than 0.");
        _storage = _allocator.allocate(buffer_size);
    }

    FixedBuffer(const FixedBuffer& other)
        : _size{other.size}, _start{other.start}, _end{other.end} {
        _storage = _allocator.allocate(buffer_size);
        memcpy(_storage, other._storage, other._size * sizeof(T));
    }

    FixedBuffer& operator=(const FixedBuffer& other) {
        return (*this = FixedBuffer(other));
    }

    FixedBuffer(FixedBuffer&& other) = default;
    FixedBuffer& operator=(FixedBuffer&& other) = default;

    ~FixedBuffer() { _allocator.deallocate(_storage, buffer_size); }

    /**
     * @brief Pushes an element into the buffer.
     *
     * If the buffer is full, the calling thread will wait until space becomes
     * available.
     *
     * @param object The element to be pushed into the buffer.
     */
    void push(T&& object) override {
        std::unique_lock<std::mutex> lock(_data_mutex);
        _push(std::forward<T>(object));
    }

    /**
     * @brief Pushes an element the buffer with a timeout.
     *
     * This function pushes the specified element into the buffer with a
     * timeout. It waits until there is space available in the buffer or the
     * timeout expires.
     *
     * @param object The element to be pushed into the buffer.
     * @param timeout The maximum duration to wait for space in the buffer.
     * @return `true` if the element was successfully pushed into the buffer,
     * `false` if the timeout expired before space became available.
     */
    bool push(T&& object, std::chrono::nanoseconds timeout) {
        std::unique_lock<std::mutex> lock(_data_mutex);

        auto status = _wait_pop.wait_for(
            lock, timeout, [this]() { return _size < buffer_size; });
        if (!status) return false;

        _push(std::forward<T>(object));
        return true;
    }

    /**
     * @brief Pops an element from the buffer.
     *
     * If the buffer is empty, the calling thread will wait until an element
     * becomes available.
     *
     * @return The element popped from the buffer.
     */
    T pop() override {
        std::unique_lock<std::mutex> lock(_data_mutex);
        _wait_push.wait(lock, [this]() { return _size > 0; });
        return _pop();
    }

    /**
     * @brief Pops an element from the buffer with a timeout.
     *
     * If the buffer is empty, the function will wait for a specified timeout
     * duration for an element to be available. If no element becomes available
     * within the timeout duration, the function returns std::nullopt.
     *
     * @param timeout The maximum duration to wait for an element to become
     * available.
     * @return An optional containing the removed element, or std::nullopt if
     * the buffer is empty and the timeout expires.
     */
    std::optional<T> pop(std::chrono::nanoseconds timeout) {
        std::unique_lock<std::mutex> lock(_data_mutex);
        auto status =
            _wait_push.wait_for(lock, timeout, [this]() { return _size > 0; });
        if (!status) return std::nullopt;
        return _pop();
    }

    /**
     * @brief Returns the current number of elements in the buffer.
     *
     * This operation is thread-safe.
     *
     * @return The number of elements in the buffer.
     */
    [[nodiscard]] size_t size() const override {
        std::unique_lock<std::mutex> lock(_data_mutex);
        return _size;
    }

    /**
     * @brief Checks if the buffer is empty.
     *
     * This operation is thread-safe.
     *
     * @return True if the buffer is empty, false otherwise.
     */
    [[nodiscard]] bool empty() const override {
        std::unique_lock<std::mutex> lock(_data_mutex);
        return _size == 0;
    }
};

}  // namespace singularity::concurrency

#endif  // CONCURRENCY_HPP