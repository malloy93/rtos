#include <rtos/Telemetry/UartProtocol.hpp>

#include <array>

namespace
{

telemetry::UartProtocol* activeProtocol = nullptr;

uint16_t updateCrc16(uint16_t crc, std::span<const uint8_t> data)
{
    for (const uint8_t byte : data)
    {
        crc ^= static_cast<uint16_t>(byte) << 8U;
        for (uint8_t bit = 0U; bit < 8U; ++bit)
        {
            crc = (crc & 0x8000U) != 0U
                ? static_cast<uint16_t>((crc << 1U) ^ 0x1021U)
                : static_cast<uint16_t>(crc << 1U);
        }
    }
    return crc;
}

} // namespace

namespace telemetry::codec
{

uint16_t crc16CcittFalse(std::span<const uint8_t> data)
{
    return updateCrc16(0xFFFFU, data);
}

size_t encodeFrame(
    MessageId id,
    std::span<const uint8_t> payload,
    std::span<uint8_t> wireFrame,
    std::span<const uint8_t> prefix)
{
    if (!isValidMessageId(id) || wireFrame.empty() ||
        prefix.size() > MAX_PAYLOAD_BYTES || payload.size() > MAX_PAYLOAD_BYTES - prefix.size()) return 0U;

    const uint16_t rawId = static_cast<uint16_t>(id);
    const uint16_t payloadLength = static_cast<uint16_t>(prefix.size() + payload.size());
    const std::array<uint8_t, HEADER_SIZE> header{
        PROTOCOL_VERSION,
        RESERVED_FLAGS,
        static_cast<uint8_t>(rawId),
        static_cast<uint8_t>(rawId >> 8U),
        static_cast<uint8_t>(payloadLength),
        static_cast<uint8_t>(payloadLength >> 8U),
    };

    const uint16_t crc = updateCrc16(updateCrc16(crc16CcittFalse(header), prefix), payload);
    const std::array<uint8_t, CRC_SIZE> checksum{
        static_cast<uint8_t>(crc), static_cast<uint8_t>(crc >> 8U),
    };

    // Stream the header, payload segments, and checksum through COBS.
    const std::array parts{
        std::span<const uint8_t>(header), prefix, payload, std::span<const uint8_t>(checksum),
    };
    size_t write = 1U;
    size_t codePosition = 0U;
    uint8_t code = 1U;
    for (const auto part : parts)
    {
        for (const uint8_t byte : part)
        {
            if (write >= wireFrame.size()) return 0U;
            if (byte != 0U)
            {
                wireFrame[write++] = byte;
                ++code;
            }
            if (byte == 0U || code == 0xFFU)
            {
                wireFrame[codePosition] = code;
                codePosition = write++;
                code = 1U;
            }
        }
    }

    if (write >= wireFrame.size()) return 0U;
    wireFrame[codePosition] = code;
    wireFrame[write++] = 0U;
    return write;
}

} // namespace telemetry::codec

namespace telemetry
{

void UartProtocol::init(UART_HandleTypeDef* uartHandle, OutputMode mode)
{
    uart = uartHandle;
    outputMode = mode;
    txBuffer.clear();
    inFlightSize = 0U;
    dmaCompleted = false;
    activeProtocol = this;
}

bool UartProtocol::send(MessageId id, std::span<const uint8_t> payload)
{
    std::array<uint8_t, MAX_WIRE_PACKET_BYTES> frame{};
    const size_t frameSize = codec::encodeFrame(id, payload, frame);
    return frameSize != 0U &&
        txBuffer.pushAll(std::span<const uint8_t>(frame.data(), frameSize));
}

bool UartProtocol::sendLog(core::LogLevel level, std::string_view text)
{
    if (outputMode != OutputMode::DATA_AND_LOGS) return false;
    if (static_cast<uint8_t>(level) > static_cast<uint8_t>(core::LogLevel::ERROR)) return false;
    while (!text.empty() && (text.back() == '\r' || text.back() == '\n')) text.remove_suffix(1U);
    if (text.size() > MAX_LOG_TEXT_BYTES) return false;

    // Encode the level and the caller's text directly into a log-sized frame.
    const uint8_t levelByte = static_cast<uint8_t>(level);
    const auto textBytes = std::span<const uint8_t>(
        reinterpret_cast<const uint8_t*>(text.data()), text.size());
    std::array<uint8_t, MAX_LOG_WIRE_PACKET_BYTES> frame{};
    const size_t frameSize = codec::encodeFrame(
        MessageId::LOG_TEXT,
        textBytes,
        frame,
        std::span<const uint8_t>(&levelByte, 1U));
    return frameSize != 0U &&
        txBuffer.pushAll(std::span<const uint8_t>(frame.data(), frameSize));
}

void UartProtocol::processTx()
{
    if (uart == nullptr) return;

    if (dmaCompleted)
    {
        txBuffer.advanceTail(inFlightSize);
        inFlightSize = 0U;
        dmaCompleted = false;
    }

    if (isDmaActive()) return;

    const size_t length = txBuffer.getLinearBlockSize();
    if (length == 0U) return;

    if (HAL_UART_Transmit_DMA(
            uart,
            txBuffer.getReadPtr(),
            static_cast<uint16_t>(length)) == HAL_OK)
    {
        inFlightSize = length;
    }
}

void UartProtocol::onTxCompleteFromIsr()
{
    dmaCompleted = true;
}

} // namespace telemetry

extern "C" void HAL_UART_TxCpltCallback(UART_HandleTypeDef* uart)
{
    if (activeProtocol != nullptr && activeProtocol->usesUart(uart))
    {
        activeProtocol->onTxCompleteFromIsr();
    }
}
