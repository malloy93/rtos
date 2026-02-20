#pragma once
#include <atomic>
#include <cstdint>
#include <span>

constexpr size_t CIRCULAR_BUFFER_SIZE = 4096;

class CircularBuffer
{
public:
    CircularBuffer() = default;

    bool push(std::span<char> data)
    {
        // mutex here
        for (auto byte : data)
        {
            size_t next_head = (head + 1) % CIRCULAR_BUFFER_SIZE;
            if (next_head == tail)
            {
                return false;
            }
            buffer[head] = static_cast<uint8_t>(byte);
            head = next_head;
        }
        return true;
    }

    bool isEmpty() const { return head == tail; }

    uint8_t* getReadPtr() { return &buffer[tail]; }

    size_t getLinearBlockSize()
    {
        size_t h = head;
        size_t t = tail;

        if (h == t) return 0;

        if (h > t)
        {
            return h - t;
        }
        else
        {
            return CIRCULAR_BUFFER_SIZE - t;
        }
    }
    void advanceTail(size_t amount) { tail = (tail + amount) % CIRCULAR_BUFFER_SIZE; }

private:
    uint8_t buffer[CIRCULAR_BUFFER_SIZE] = {0};
    volatile size_t head{0};
    volatile size_t tail{0};
};
