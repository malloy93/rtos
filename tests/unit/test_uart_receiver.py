import importlib.util
from contextlib import redirect_stderr
from io import StringIO
from pathlib import Path
import struct
import sys
import unittest
from unittest.mock import MagicMock, patch


RECEIVER_PATH = Path(__file__).parents[2] / "tools" / "uart_receiver.py"
SPEC = importlib.util.spec_from_file_location("uart_receiver", RECEIVER_PATH)
assert SPEC is not None and SPEC.loader is not None
receiver = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = receiver
SPEC.loader.exec_module(receiver)


def cobs_encode(decoded: bytes) -> bytes:
    output = bytearray([0])
    code_index = 0
    code = 1
    for byte in decoded:
        if byte == 0:
            output[code_index] = code
            code_index = len(output)
            output.append(0)
            code = 1
        else:
            output.append(byte)
            code += 1
            if code == 0xFF:
                output[code_index] = code
                code_index = len(output)
                output.append(0)
                code = 1
    output[code_index] = code
    return bytes(output)


def frame(message_id: int, payload: bytes, *, flags: int = 0) -> bytes:
    packet = struct.pack("<BBHH", 1, flags, message_id, len(payload)) + payload
    packet += struct.pack("<H", receiver.crc16_ccitt_false(packet))
    return cobs_encode(packet) + b"\0"


class ReceiverTests(unittest.TestCase):
    def test_crc_vector(self) -> None:
        self.assertEqual(receiver.crc16_ccitt_false(b"123456789"), 0x29B1)

    def test_decodes_log_frame(self) -> None:
        message = receiver.decode_frame(bytes.fromhex("020101038006090168656c6c6f373e"))
        self.assertIsNotNone(message)
        assert message is not None
        self.assertEqual(message.message_id, 0x8000)
        self.assertEqual(message.payload, b"\x01hello")
        self.assertEqual(receiver.render_log(message.payload), "[DEBUG] hello")

    def test_rejects_bad_crc_and_resynchronizes(self) -> None:
        bad = bytearray(frame(7, b"bad"))
        bad[3] ^= 0x20
        good = frame(8, b"good")
        messages = list(receiver.StreamParser().feed(bytes(bad) + good))
        self.assertEqual(messages, [receiver.Message(8, 0, b"good")])

    def test_discards_oversized_frame_until_delimiter(self) -> None:
        good = frame(9, b"ok")
        stream = b"\x01" * receiver.MAX_WIRE_PACKET_BYTES + b"\0" + good
        messages = list(receiver.StreamParser().feed(stream))
        self.assertEqual(messages, [receiver.Message(9, 0, b"ok")])


class ReceiverCliTests(unittest.TestCase):
    def test_reports_open_and_read_failures(self) -> None:
        for stage in ("open", "read"):
            with self.subTest(stage=stage):
                serial = MagicMock()
                error = OSError(f"serial {stage} failed")
                if stage == "open":
                    serial.Serial.side_effect = error
                else:
                    serial.Serial.return_value.__enter__.return_value.read.side_effect = error
                stderr = StringIO()

                with patch.dict(sys.modules, {"serial": serial}), \
                     patch.object(sys, "argv", ["uart_receiver.py", "/dev/test"]), \
                     redirect_stderr(stderr):
                    self.assertEqual(receiver.main(), 1)

                self.assertIn(f"UART error on /dev/test: {error}", stderr.getvalue())
                self.assertIn("picocom", stderr.getvalue())
                serial.Serial.assert_called_once_with(
                    "/dev/test", baudrate=2_000_000, timeout=0.25, exclusive=True)
                if stage == "read":
                    serial.Serial.return_value.__exit__.assert_called_once()

    def test_ctrl_c_closes_port_and_exits_successfully(self) -> None:
        serial = MagicMock()
        serial.Serial.return_value.__enter__.return_value.read.side_effect = KeyboardInterrupt
        with patch.dict(sys.modules, {"serial": serial}), \
             patch.object(sys, "argv", ["uart_receiver.py", "/dev/test"]):
            self.assertEqual(receiver.main(), 0)
        serial.Serial.return_value.__exit__.assert_called_once()


if __name__ == "__main__":
    unittest.main()
