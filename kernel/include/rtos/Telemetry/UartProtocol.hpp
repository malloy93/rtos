#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#include <board/usart.h>
#include <rtos/CircularBuffer.hpp>
#include <rtos/LogLevel.hpp>
#include <rtos/Telemetry/MessageIds.hpp>

namespace telemetry
{

constexpr uint8_t PROTOCOL_VERSION = 1U;
constexpr uint8_t RESERVED_FLAGS = 0U;
constexpr size_t HEADER_SIZE = 6U;
constexpr size_t CRC_SIZE = 2U;
constexpr size_t MAX_PAYLOAD_BYTES = 500U;
constexpr size_t MAX_DECODED_PACKET_BYTES = HEADER_SIZE + MAX_PAYLOAD_BYTES + CRC_SIZE;
constexpr size_t MAX_WIRE_PACKET_BYTES = 512U;
constexpr size_t MAX_LOG_TEXT_BYTES = 127U;

enum class OutputMode : uint8_t
{
    DATA_ONLY = 0,
    DATA_AND_LOGS = 1,
};

namespace codec
{

uint16_t crc16CcittFalse(std::span<const uint8_t> data);
size_t encodeFrame(
    MessageId id,
    std::span<const uint8_t> payload,
    std::span<uint8_t> wireFrame);

} // namespace codec

class UartProtocol
{
public:
    void init(UART_HandleTypeDef* uart, OutputMode mode);

    bool send(MessageId id, std::span<const uint8_t> payload);
    bool sendLog(core::LogLevel level, std::string_view text);

    void processTx();
    void onTxCompleteFromIsr();

    bool usesUart(const UART_HandleTypeDef* candidate) const { return uart == candidate; }
    size_t pendingBytes() const { return txBuffer.size(); }
    bool isDmaActive() const { return inFlightSize != 0U; }

private:
    UART_HandleTypeDef* uart{nullptr};
    OutputMode outputMode{OutputMode::DATA_ONLY};
    CircularBuffer txBuffer;
    size_t inFlightSize{0};
    volatile bool dmaCompleted{false};
};

} // namespace telemetry
