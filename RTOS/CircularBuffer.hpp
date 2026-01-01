#pragma once
#include <cstdint>
#include <cstring>

namespace core
{

template <uint16_t BufferSize>
class CircularBuffer
{
public:
    CircularBuffer() : head{0}, tail{0}, count{0} {}

    // dodaj element na koniec
    bool push(uint8_t value)
    {
        if (isFull()) return false;

        buffer[tail] = value;
        tail = (tail + 1) % BufferSize;
        ++count;
        return true;
    }

    // pobierz element z przodu
    bool pop(uint8_t& value)
    {
        if (isEmpty()) return false;

        value = buffer[head];
        head = (head + 1) % BufferSize;
        --count;
        return true;
    }

    // podejrzyj element z przodu bez usuwania
    bool peek(uint8_t& value) const
    {
        if (isEmpty()) return false;
        value = buffer[head];
        return true;
    }

    // czy bufor jest pusty
    bool isEmpty() const { return count == 0; }

    // czy bufor jest pełny
    bool isFull() const { return count == BufferSize; }

    // liczba elementów w buforze
    uint16_t size() const { return count; }

    // pojemność buforu
    uint16_t capacity() const { return BufferSize; }

    // wyczyść bufor
    void clear()
    {
        head = 0;
        tail = 0;
        count = 0;
    }

private:
    uint8_t buffer[BufferSize];
    uint16_t head;   // indeks do odczytu
    uint16_t tail;   // indeks do zapisu
    uint16_t count;  // liczba elementów w buforze
};

} // namespace core
