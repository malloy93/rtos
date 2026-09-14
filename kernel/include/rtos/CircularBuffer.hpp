#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

constexpr size_t CIRCULAR_BUFFER_SIZE = 4096;

class CircularBuffer
{
public:
    CircularBuffer() = default;

    bool push(std::span<char> data)
    {
        return pushAll(std::span<const uint8_t>(
            reinterpret_cast<const uint8_t*>(data.data()), data.size()));
    }

    // Enqueue the whole span or nothing. Callers must serialize producers.
    bool pushAll(std::span<const uint8_t> data)
    {
        if (data.size() > freeSpace()) return false;

        size_t write = head;
        for (const uint8_t byte : data)
        {
            buffer[write] = byte;
            write = (write + 1U) % CIRCULAR_BUFFER_SIZE;
        }

        head = write;
        return true;
    }

    bool isEmpty() const { return head == tail; }

    size_t size() const { return (head + CIRCULAR_BUFFER_SIZE - tail) % CIRCULAR_BUFFER_SIZE; }

    size_t freeSpace() const { return CIRCULAR_BUFFER_SIZE - 1U - size(); }

    uint8_t* getReadPtr() { return &buffer[tail]; }

    size_t getLinearBlockSize() const
    {
        if (head == tail) return 0;
        if (head > tail) return head - tail;
        return CIRCULAR_BUFFER_SIZE - tail;
    }

    void advanceTail(size_t amount) { tail = (tail + amount) % CIRCULAR_BUFFER_SIZE; }

    void clear()
    {
        head = 0U;
        tail = 0U;
    }

private:
    uint8_t buffer[CIRCULAR_BUFFER_SIZE] = {0};
    size_t head{0};
    size_t tail{0};
};
