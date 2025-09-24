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




// template <typename T>
// class Map
// {
// public:
//     Map() = default;

//     T at(uint16_t key)
//     {
//         for (const auto& pair : data)
//         {
//             if (pair.first == key)
//             {
//                 return pair.second;
//             }
//         }
//     }

//     void insert(uint16_t key, T value)
//     {
//         for (auto& pair : data)
//         {
//             if (pair.first == key)
//             {
//                 pair.second = value;
//                 return;
//             }
//         }
//         data.emplace_back(key, value);
//     }
//     void remove(uint16_t key)
//     {
//         for (auto it = data.begin(); it != data.end(); ++it)
//         {
//             if (it->first == key)
//             {
//                 data.erase(it);
//                 return;
//             }
//         }
//     }

//     void push_back(uint16_t key, T value)
//     {
//         data.emplace_back(key, value);
//     }

//     private:

//     std::vector<std::pair<uint16_t, T>> data;

// };


} // namespace utils