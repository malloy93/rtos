#!/usr/bin/env python3
"""Receiver for Minimal UART Binary Protocol v1."""

from __future__ import annotations

import argparse
from binascii import crc_hqx
from dataclasses import dataclass
import struct
import sys
from typing import Callable, Iterable


PROTOCOL_VERSION = 1
HEADER_SIZE = 6
CRC_SIZE = 2
MAX_DECODED_PACKET_BYTES = 508
MAX_WIRE_PACKET_BYTES = 512
LOG_TEXT = 0x8000

LOG_LEVELS = {
    0: "KERNEL",
    1: "DEBUG",
    2: "INFO",
    3: "ERROR",
}


@dataclass(frozen=True)
class Message:
    message_id: int
    flags: int
    payload: bytes


def crc16_ccitt_false(data: bytes) -> int:
    return crc_hqx(data, 0xFFFF)


def cobs_decode(encoded: bytes) -> bytes:
    if not encoded:
        raise ValueError("empty COBS frame")

    decoded = bytearray()
    offset = 0
    while offset < len(encoded):
        code = encoded[offset]
        if code == 0:
            raise ValueError("zero inside COBS frame")
        offset += 1
        end = offset + code - 1
        if end > len(encoded):
            raise ValueError("truncated COBS block")
        block = encoded[offset:end]
        if 0 in block:
            raise ValueError("zero inside COBS block")
        decoded.extend(block)
        offset = end
        if code != 0xFF and offset < len(encoded):
            decoded.append(0)
    return bytes(decoded)


def decode_frame(encoded: bytes) -> Message | None:
    if not encoded or len(encoded) >= MAX_WIRE_PACKET_BYTES:
        return None
    try:
        packet = cobs_decode(encoded)
    except ValueError:
        return None

    if len(packet) < HEADER_SIZE + CRC_SIZE or len(packet) > MAX_DECODED_PACKET_BYTES:
        return None

    version, flags, message_id, payload_length = struct.unpack_from("<BBHH", packet)
    if version != PROTOCOL_VERSION:
        return None
    if len(packet) != HEADER_SIZE + payload_length + CRC_SIZE:
        return None

    expected_crc = struct.unpack_from("<H", packet, len(packet) - CRC_SIZE)[0]
    if crc16_ccitt_false(packet[:-CRC_SIZE]) != expected_crc:
        return None
    return Message(message_id, flags, packet[HEADER_SIZE:-CRC_SIZE])


class StreamParser:
    def __init__(self) -> None:
        self._buffer = bytearray()
        self._discard_until_delimiter = False

    def feed(self, data: bytes) -> Iterable[Message]:
        for byte in data:
            if byte == 0:
                if not self._discard_until_delimiter and self._buffer:
                    message = decode_frame(bytes(self._buffer))
                    if message is not None:
                        yield message
                self._buffer.clear()
                self._discard_until_delimiter = False
            elif not self._discard_until_delimiter:
                self._buffer.append(byte)
                if len(self._buffer) >= MAX_WIRE_PACKET_BYTES:
                    self._buffer.clear()
                    self._discard_until_delimiter = True


def render_log(payload: bytes) -> str | None:
    if not payload:
        return None
    level = LOG_LEVELS.get(payload[0], f"LEVEL_{payload[0]}")
    text = payload[1:].decode("utf-8", errors="replace")
    return f"[{level}] {text}"


def receive(port: str, baudrate: int, decoders: dict[int, Callable[[bytes], None]]) -> None:
    try:
        import serial  # type: ignore[import-not-found]
    except ImportError as error:
        raise SystemExit("pyserial is required: python -m pip install pyserial") from error

    parser = StreamParser()
    with serial.Serial(port, baudrate=baudrate, timeout=0.25, exclusive=True) as uart:
        while True:
            for message in parser.feed(uart.read(4096)):
                if message.message_id == LOG_TEXT:
                    rendered = render_log(message.payload)
                    if rendered is not None:
                        print(rendered, flush=True)
                elif decoder := decoders.get(message.message_id):
                    decoder(message.payload)


def main() -> int:
    argument_parser = argparse.ArgumentParser(description=__doc__)
    argument_parser.add_argument("port", help="serial port, for example /dev/ttyACM0")
    argument_parser.add_argument("--baud", type=int, default=2_000_000, help="UART baud rate")
    arguments = argument_parser.parse_args()

    try:
        receive(arguments.port, arguments.baud, decoders={})
    except KeyboardInterrupt:
        return 0
    except OSError as error:
        print(f"UART error on {arguments.port}: {error}", file=sys.stderr)
        print(
            "Stop other serial monitors (including the RTOS: UART / picocom task) "
            "and check the USB connection, then run the receiver again.",
            file=sys.stderr,
        )
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
