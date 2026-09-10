#pragma once

namespace configuration
{
struct MCUConfig
{
    uint32_t ramSizeKB{128};
    uint32_t ccramSizeKB{64};
    uint32_t sdramSizeKB{64};
};

constexpr static MCUConfig f429ziConfig{192, 64, 0};
constexpr static MCUConfig f411reConfig{128, 0, 0};
constexpr static MCUConfig l476rgConfig{96, 16, 0};
constexpr static MCUConfig h743ziConfig{512, 128, 64};
} // namespace configuration
 