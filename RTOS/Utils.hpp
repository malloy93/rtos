#pragma once

#include <cstdint>
#include <limits>

namespace utils
{

constexpr uint16_t maxId{256}; // can be more in the future

uint32_t getClockFreq();

class IdGen
{
public:
    IdGen() = default;

    uint16_t getId()
    {
        if (genId == maxId)
            genId = 0;
        return genId++;
    }

private:
    uint16_t genId{0};
};

} // namespace utils