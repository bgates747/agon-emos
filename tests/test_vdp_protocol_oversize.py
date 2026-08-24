#!/usr/bin/env python3
"""Regression tests for oversized VDP-to-MOS protocol packets."""

from __future__ import annotations

import re
import unittest
from pathlib import Path


PROJECT_ROOT = Path(__file__).resolve().parents[1]
PROTOCOL_SOURCE = PROJECT_ROOT / "src" / "vdp_protocol.asm"
BUFFER_LENGTH = 16


def oversized_branch_stores_length() -> bool:
    """Read the actual assembly and identify its oversized-length behavior."""
    source = PROTOCOL_SOURCE.read_text(encoding="utf-8")
    match = re.search(
        r"vdp_protocol_state1:.*?"
        r"JR\s+C,\s+\$F(?P<oversized>.*?)"
        r"^\$\$:",
        source,
        flags=re.MULTILINE | re.DOTALL,
    )
    if match is None:
        raise AssertionError("could not locate the state-1 oversized branch")
    return bool(
        re.search(
            r"LD\s+\(_vdp_protocol_len\),\s*A",
            match.group("oversized"),
        )
    )


class ProtocolModel:
    """Minimal model of the assembly states exercised by this regression."""

    def __init__(self) -> None:
        self.state = 0
        self.command = 0
        self.remaining = 0
        self.body: list[int] = []
        self.general_poll: int | None = None
        self.store_oversized_length = oversized_branch_stores_length()

    def feed(self, value: int) -> None:
        value &= 0xFF
        if self.state == 0:
            if value >= 0x80:
                self.command = value - 0x80
                self.body = []
                self.state = 1
            return

        if self.state == 1:
            if value > BUFFER_LENGTH:
                if self.store_oversized_length:
                    self.remaining = value
                self.state = 3
                return
            self.remaining = value
            if value == 0:
                self._execute()
            else:
                self.state = 2
            return

        if self.state == 2:
            self.body.append(value)
            self.remaining = (self.remaining - 1) & 0xFF
            if self.remaining == 0:
                self._execute()
            return

        if self.state == 3:
            self.remaining = (self.remaining - 1) & 0xFF
            if self.remaining == 0:
                self.state = 0
            return

        raise AssertionError(f"invalid protocol state: {self.state}")

    def feed_all(self, values: list[int]) -> None:
        for value in values:
            self.feed(value)

    def _execute(self) -> None:
        self.state = 0
        if self.command == 0 and self.body:
            self.general_poll = self.body[0]


class OversizedPacketTests(unittest.TestCase):
    def test_oversized_branch_records_announced_length(self) -> None:
        self.assertTrue(
            oversized_branch_stores_length(),
            "state 3 otherwise decrements stale zero to 255",
        )

    def test_oversized_packet_does_not_consume_following_packet(self) -> None:
        parser = ProtocolModel()
        oversized = [0x81, 17] + [0x55] * 17
        valid_general_poll = [0x80, 1, 0xA5]
        parser.feed_all(oversized + valid_general_poll)
        self.assertEqual(parser.state, 0)
        self.assertEqual(parser.general_poll, 0xA5)

    def test_maximum_legal_packet_and_following_packet_are_accepted(self) -> None:
        parser = ProtocolModel()
        maximum_legal = [0x81, BUFFER_LENGTH] + [0x55] * BUFFER_LENGTH
        valid_general_poll = [0x80, 1, 0xA5]
        parser.feed_all(maximum_legal + valid_general_poll)
        self.assertEqual(parser.state, 0)
        self.assertEqual(parser.general_poll, 0xA5)


if __name__ == "__main__":
    unittest.main()
