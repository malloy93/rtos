#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include <rtos/CircularBuffer.hpp>
#include <rtos/Logger.hpp>
#include <rtos/Telemetry/UartProtocol.hpp>

namespace
{

HAL_StatusTypeDef dmaResult = HAL_OK;
size_t dmaCalls = 0U;
const uint8_t* dmaPointer = nullptr;
uint16_t dmaSize = 0U;
std::vector<uint8_t> wireBytes;

void resetDmaMock()
{
    dmaResult = HAL_OK;
    dmaCalls = 0U;
    dmaPointer = nullptr;
    dmaSize = 0U;
    wireBytes.clear();
}

void completeDma(UART_HandleTypeDef* uart)
{
    ASSERT_NE(dmaPointer, nullptr);
    wireBytes.insert(wireBytes.end(), dmaPointer, dmaPointer + dmaSize);
    dmaPointer = nullptr;
    dmaSize = 0U;
    HAL_UART_TxCpltCallback(uart);
}

std::vector<uint8_t> encode(
    telemetry::MessageId id,
    std::span<const uint8_t> payload)
{
    std::array<uint8_t, telemetry::MAX_WIRE_PACKET_BYTES> frame{};
    const size_t size = telemetry::codec::encodeFrame(id, payload, frame);
    return {frame.begin(), frame.begin() + static_cast<std::ptrdiff_t>(size)};
}

struct TestPacket
{
    uint8_t flags{0U};
    uint16_t messageId{0U};
    std::vector<uint8_t> payload;
};

bool decodeCobs(std::span<const uint8_t> encoded, std::vector<uint8_t>& decoded)
{
    decoded.clear();
    size_t read = 0U;
    while (read < encoded.size())
    {
        const uint8_t code = encoded[read++];
        if (code == 0U || static_cast<size_t>(code - 1U) > encoded.size() - read) return false;
        for (uint8_t i = 1U; i < code; ++i) decoded.push_back(encoded[read++]);
        if (code != 0xFFU && read < encoded.size()) decoded.push_back(0U);
    }
    return !encoded.empty();
}

bool decodeWire(std::span<const uint8_t> wire, TestPacket& packet)
{
    if (wire.empty() || wire.back() != 0U) return false;
    std::vector<uint8_t> raw;
    if (!decodeCobs(wire.first(wire.size() - 1U), raw) ||
        raw.size() < telemetry::HEADER_SIZE + telemetry::CRC_SIZE ||
        raw[0] != telemetry::PROTOCOL_VERSION)
    {
        return false;
    }

    const uint16_t length = static_cast<uint16_t>(raw[4]) |
        (static_cast<uint16_t>(raw[5]) << 8U);
    if (raw.size() != telemetry::HEADER_SIZE + length + telemetry::CRC_SIZE) return false;
    const uint16_t expectedCrc = static_cast<uint16_t>(raw[raw.size() - 2U]) |
        (static_cast<uint16_t>(raw.back()) << 8U);
    if (telemetry::codec::crc16CcittFalse(
            std::span<const uint8_t>(raw.data(), raw.size() - telemetry::CRC_SIZE)) != expectedCrc)
    {
        return false;
    }

    packet.flags = raw[1];
    packet.messageId = static_cast<uint16_t>(raw[2]) |
        (static_cast<uint16_t>(raw[3]) << 8U);
    packet.payload.assign(
        raw.begin() + static_cast<std::ptrdiff_t>(telemetry::HEADER_SIZE),
        raw.end() - static_cast<std::ptrdiff_t>(telemetry::CRC_SIZE));
    return true;
}

} // namespace

HAL_StatusTypeDef HAL_UART_Transmit_DMA(
    UART_HandleTypeDef*,
    const uint8_t* data,
    uint16_t size)
{
    ++dmaCalls;
    if (dmaResult == HAL_OK)
    {
        dmaPointer = data;
        dmaSize = size;
    }
    return dmaResult;
}

TEST(UartProtocolCodec, PreservesV1WireFormat)
{
    constexpr std::array<uint8_t, 7> payload{0U, 1U, 0U, 2U, 3U, 0U, 4U};
    const std::vector<uint8_t> expected{
        0x02, 0x01, 0x04, 0x34, 0x12, 0x07, 0x01, 0x02, 0x01,
        0x03, 0x02, 0x03, 0x04, 0x04, 0xC4, 0x89, 0x00,
    };
    EXPECT_EQ(encode(static_cast<telemetry::MessageId>(0x1234U), payload), expected);
}

TEST(UartProtocolCodec, UsesKnownCrcVector)
{
    constexpr std::array<uint8_t, 9> input{'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    EXPECT_EQ(telemetry::codec::crc16CcittFalse(input), 0x29B1U);
}

TEST(UartProtocolCodec, PrefixMatchesAContiguousPayload)
{
    constexpr auto id = static_cast<telemetry::MessageId>(0x1234U);
    constexpr std::array<uint8_t, 7> payload{0U, 1U, 0U, 2U, 3U, 0U, 4U};
    const auto expected = encode(id, payload);
    for (size_t split = 0U; split <= payload.size(); ++split)
    {
        std::array<uint8_t, telemetry::MAX_WIRE_PACKET_BYTES> output{};
        const auto bytes = std::span<const uint8_t>(payload);
        const size_t size = telemetry::codec::encodeFrame(
            id, bytes.subspan(split), output, bytes.first(split));
        ASSERT_EQ(size, expected.size());
        EXPECT_TRUE(std::equal(expected.begin(), expected.end(), output.begin()));
    }
}

TEST(UartProtocolCodec, CountsPrefixAgainstPayloadLimit)
{
    constexpr auto id = static_cast<telemetry::MessageId>(1U);
    std::array<uint8_t, telemetry::MAX_PAYLOAD_BYTES + 1U> bytes{};
    std::array<uint8_t, telemetry::MAX_WIRE_PACKET_BYTES> output{};
    const auto data = std::span<const uint8_t>(bytes);
    EXPECT_NE(telemetry::codec::encodeFrame(id, data.first(499U), output, data.first(1U)), 0U);
    EXPECT_EQ(telemetry::codec::encodeFrame(id, data.first(500U), output, data.first(1U)), 0U);
    EXPECT_EQ(telemetry::codec::encodeFrame(id, {}, output, data), 0U);
}

TEST(UartProtocolCodec, EncodesEmptyPayloadAndLittleEndianHeader)
{
    constexpr auto id = static_cast<telemetry::MessageId>(0x1234U);
    const auto frame = encode(id, {});
    ASSERT_FALSE(frame.empty());
    EXPECT_EQ(frame.back(), 0U);

    std::vector<uint8_t> decoded;
    ASSERT_TRUE(decodeCobs(
        std::span<const uint8_t>(frame.data(), frame.size() - 1U), decoded));
    ASSERT_EQ(decoded.size(), telemetry::HEADER_SIZE + telemetry::CRC_SIZE);
    EXPECT_EQ(decoded[0], telemetry::PROTOCOL_VERSION);
    EXPECT_EQ(decoded[1], 0U);
    EXPECT_EQ(decoded[2], 0x34U);
    EXPECT_EQ(decoded[3], 0x12U);
    EXPECT_EQ(decoded[4], 0U);
    EXPECT_EQ(decoded[5], 0U);
}

TEST(UartProtocolCodec, RoundTripsPayloadContainingZeros)
{
    constexpr auto id = static_cast<telemetry::MessageId>(0x0102U);
    constexpr std::array<uint8_t, 7> payload{0U, 1U, 0U, 2U, 3U, 0U, 4U};
    const auto frame = encode(id, payload);
    ASSERT_FALSE(frame.empty());
    EXPECT_EQ(std::count(frame.begin(), frame.end() - 1, 0U), 0);

    TestPacket packet;
    ASSERT_TRUE(decodeWire(frame, packet));
    EXPECT_EQ(packet.messageId, static_cast<uint16_t>(id));
    EXPECT_TRUE(std::equal(packet.payload.begin(), packet.payload.end(), payload.begin(), payload.end()));
}

TEST(UartProtocolCodec, EnforcesPayloadAndWireLimits)
{
    constexpr auto id = static_cast<telemetry::MessageId>(1U);
    std::array<uint8_t, telemetry::MAX_PAYLOAD_BYTES> maximumPayload{};
    for (size_t i = 0U; i < maximumPayload.size(); ++i)
    {
        maximumPayload[i] = static_cast<uint8_t>((i % 255U) + 1U);
    }

    const auto frame = encode(id, maximumPayload);
    ASSERT_FALSE(frame.empty());
    EXPECT_LE(frame.size(), telemetry::MAX_WIRE_PACKET_BYTES);

    std::array<uint8_t, telemetry::MAX_PAYLOAD_BYTES + 1U> oversized{};
    std::array<uint8_t, telemetry::MAX_WIRE_PACKET_BYTES> output{};
    EXPECT_EQ(telemetry::codec::encodeFrame(id, oversized, output), 0U);

    // Include empty, undersized, and exactly fitting output spans.
    for (size_t capacity = 0U; capacity <= frame.size(); ++capacity)
    {
        output.fill(0xA5U);
        const size_t size = telemetry::codec::encodeFrame(
            id, maximumPayload, std::span<uint8_t>(output).first(capacity));
        EXPECT_EQ(size, capacity == frame.size() ? frame.size() : 0U);
        for (size_t i = capacity; i < output.size(); ++i) EXPECT_EQ(output[i], 0xA5U);
    }
}

TEST(UartProtocolCodec, RejectsInvalidIds)
{
    std::array<uint8_t, telemetry::MAX_WIRE_PACKET_BYTES> output{};
    EXPECT_EQ(telemetry::codec::encodeFrame(telemetry::MessageId::INVALID, {}, output), 0U);
    EXPECT_EQ(telemetry::codec::encodeFrame(static_cast<telemetry::MessageId>(0xFFFFU), {}, output), 0U);
}

TEST(CircularBuffer, PushAllLeavesDataUnchangedWhenFull)
{
    CircularBuffer buffer;
    std::vector<uint8_t> fill(CIRCULAR_BUFFER_SIZE - 1U, 0xA5U);
    ASSERT_TRUE(buffer.pushAll(fill));
    const size_t sizeBefore = buffer.size();

    constexpr std::array<uint8_t, 2> extra{1U, 2U};
    EXPECT_FALSE(buffer.pushAll(extra));
    EXPECT_EQ(buffer.size(), sizeBefore);
    EXPECT_EQ(buffer.getReadPtr()[0], 0xA5U);
}

TEST(CircularBuffer, ExposesWrappedDataAsTwoLinearBlocks)
{
    CircularBuffer buffer;
    std::vector<uint8_t> first(4000U);
    for (size_t i = 0U; i < first.size(); ++i) first[i] = static_cast<uint8_t>(i);
    ASSERT_TRUE(buffer.pushAll(first));
    buffer.advanceTail(3900U);

    std::vector<uint8_t> second(200U);
    for (size_t i = 0U; i < second.size(); ++i) second[i] = static_cast<uint8_t>(i + 17U);
    ASSERT_TRUE(buffer.pushAll(second));

    ASSERT_EQ(buffer.getLinearBlockSize(), 196U);
    std::vector<uint8_t> actual(buffer.getReadPtr(), buffer.getReadPtr() + 196U);
    buffer.advanceTail(196U);
    ASSERT_EQ(buffer.getLinearBlockSize(), 104U);
    actual.insert(actual.end(), buffer.getReadPtr(), buffer.getReadPtr() + 104U);

    std::vector<uint8_t> expected(first.end() - 100, first.end());
    expected.insert(expected.end(), second.begin(), second.end());
    EXPECT_EQ(actual, expected);
}

TEST(UartProtocolLogs, HonorsModeAndEncodesLevelWithoutLineEnding)
{
    resetDmaMock();
    UART_HandleTypeDef uart{};
    telemetry::UartProtocol protocol;
    protocol.init(&uart, telemetry::OutputMode::DATA_ONLY);
    EXPECT_FALSE(protocol.sendLog(core::LogLevel::INFO, "hidden"));
    EXPECT_EQ(protocol.pendingBytes(), 0U);

    protocol.init(&uart, telemetry::OutputMode::DATA_AND_LOGS);
    ASSERT_TRUE(protocol.sendLog(core::LogLevel::DEBUG, "message\r\n"));
    protocol.processTx();
    ASSERT_EQ(dmaCalls, 1U);
    completeDma(&uart);
    protocol.processTx();

    TestPacket packet;
    ASSERT_TRUE(decodeWire(wireBytes, packet));
    ASSERT_EQ(packet.messageId, static_cast<uint16_t>(telemetry::MessageId::LOG_TEXT));
    ASSERT_EQ(packet.payload.size(), 8U);
    EXPECT_EQ(packet.payload[0], static_cast<uint8_t>(core::LogLevel::DEBUG));
    EXPECT_EQ(std::string(packet.payload.begin() + 1, packet.payload.end()), "message");
}

TEST(UartProtocolLogs, SmallFramePreservesEmptyAndMaximumLengthLogs)
{
    UART_HandleTypeDef uart{};
    telemetry::UartProtocol protocol;
    const std::array texts{
        std::string{},
        std::string(telemetry::MAX_LOG_TEXT_BYTES, 'x'),
        std::string(telemetry::MAX_LOG_TEXT_BYTES, '\0'),
    };
    for (const auto& text : texts)
    {
        resetDmaMock();
        protocol.init(&uart, telemetry::OutputMode::DATA_AND_LOGS);
        ASSERT_TRUE(protocol.sendLog(core::LogLevel::KERNEL, text + "\r\n"));
        const size_t pending = protocol.pendingBytes();
        EXPECT_LE(pending, telemetry::MAX_LOG_WIRE_PACKET_BYTES);
        EXPECT_FALSE(protocol.sendLog(
            core::LogLevel::INFO, std::string(telemetry::MAX_LOG_TEXT_BYTES + 1U, 'x')));
        EXPECT_EQ(protocol.pendingBytes(), pending);
        protocol.processTx();
        completeDma(&uart);
        protocol.processTx();

        TestPacket packet;
        ASSERT_TRUE(decodeWire(wireBytes, packet));
        ASSERT_EQ(packet.messageId, static_cast<uint16_t>(telemetry::MessageId::LOG_TEXT));
        ASSERT_EQ(packet.payload.size(), text.size() + 1U);
        EXPECT_EQ(packet.payload[0], static_cast<uint8_t>(core::LogLevel::KERNEL));
        EXPECT_EQ(std::string(packet.payload.begin() + 1, packet.payload.end()), text);
    }
}

TEST(UartProtocolDma, RetainsDataUntilCompletionAndHandlesBusy)
{
    resetDmaMock();
    UART_HandleTypeDef uart{};
    telemetry::UartProtocol protocol;
    protocol.init(&uart, telemetry::OutputMode::DATA_ONLY);
    constexpr std::array<uint8_t, 3> payload{1U, 2U, 3U};
    ASSERT_TRUE(protocol.send(static_cast<telemetry::MessageId>(1U), payload));
    const size_t pending = protocol.pendingBytes();

    dmaResult = HAL_BUSY;
    protocol.processTx();
    EXPECT_EQ(dmaCalls, 1U);
    EXPECT_FALSE(protocol.isDmaActive());
    EXPECT_EQ(protocol.pendingBytes(), pending);

    dmaResult = HAL_OK;
    protocol.processTx();
    EXPECT_TRUE(protocol.isDmaActive());
    EXPECT_EQ(protocol.pendingBytes(), pending);
    protocol.processTx();
    EXPECT_EQ(dmaCalls, 2U);

    completeDma(&uart);
    protocol.processTx();
    EXPECT_FALSE(protocol.isDmaActive());
    EXPECT_EQ(protocol.pendingBytes(), 0U);
}

TEST(UartProtocolDma, SendsAFrameCrossingTheRingEndInTwoTransfers)
{
    resetDmaMock();
    UART_HandleTypeDef uart{};
    telemetry::UartProtocol protocol;
    protocol.init(&uart, telemetry::OutputMode::DATA_ONLY);

    std::vector<uint8_t> expectedWire;
    bool sawSplitFrame = false;
    for (uint16_t message = 1U; message <= 24U; ++message)
    {
        std::array<uint8_t, 204> payload{};
        for (size_t i = 0U; i < payload.size(); ++i)
        {
            payload[i] = static_cast<uint8_t>((i + message) & 0xFFU);
        }
        const auto expectedFrame = encode(
            static_cast<telemetry::MessageId>(message), payload);
        expectedWire.insert(expectedWire.end(), expectedFrame.begin(), expectedFrame.end());

        const size_t callsBefore = dmaCalls;
        ASSERT_TRUE(protocol.send(static_cast<telemetry::MessageId>(message), payload));
        protocol.processTx();
        ASSERT_GT(dmaCalls, callsBefore);
        if (dmaSize < expectedFrame.size()) sawSplitFrame = true;

        while (protocol.pendingBytes() > 0U)
        {
            completeDma(&uart);
            protocol.processTx();
        }
    }

    EXPECT_TRUE(sawSplitFrame);
    EXPECT_EQ(wireBytes, expectedWire);
}

TEST(Logger, FormatsTextWithoutRawStartupNullOrLineEnding)
{
    resetDmaMock();
    UART_HandleTypeDef uart{};
    core::Logger::init(
        &uart,
        core::LogLevel::DEBUG,
        telemetry::OutputMode::DATA_AND_LOGS);
    core::Logger* logger = core::Logger::getInstance();
    ASSERT_NE(logger, nullptr);
    EXPECT_EQ(dmaCalls, 0U);

    logger->log(core::LogLevel::INFO, "value=%d\r\n", 42);
    logger->send();
    ASSERT_EQ(dmaCalls, 1U);
    completeDma(&uart);
    logger->send();

    TestPacket packet;
    ASSERT_TRUE(decodeWire(wireBytes, packet));
    ASSERT_EQ(packet.messageId, static_cast<uint16_t>(telemetry::MessageId::LOG_TEXT));
    ASSERT_FALSE(packet.payload.empty());
    EXPECT_EQ(packet.payload[0], static_cast<uint8_t>(core::LogLevel::INFO));
    const std::string text(packet.payload.begin() + 1, packet.payload.end());
    EXPECT_EQ(text, "value=42");
    EXPECT_EQ(std::count(packet.payload.begin(), packet.payload.end(), 0U), 0);
    EXPECT_EQ(text.find("\r\n"), std::string::npos);

    wireBytes.clear();
    const std::string longText(300U, 'x');
    logger->log(core::LogLevel::INFO, "%s", longText.c_str());
    logger->send();
    completeDma(&uart);
    logger->send();
    EXPECT_EQ(wireBytes.size(), telemetry::MAX_LOG_WIRE_PACKET_BYTES);
    ASSERT_TRUE(decodeWire(wireBytes, packet));
    ASSERT_EQ(packet.payload.size(), telemetry::MAX_LOG_TEXT_BYTES + 1U);
    EXPECT_EQ(std::string(packet.payload.begin() + 1, packet.payload.end()),
              longText.substr(0U, telemetry::MAX_LOG_TEXT_BYTES));
}
