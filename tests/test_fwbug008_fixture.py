#!/usr/bin/env python3
"""Source contracts for the destructive FWBUG-008 physical fixture."""

from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]


class Fwbug008FixtureTests(unittest.TestCase):
    def test_recovery_selects_legacy_before_mode_and_listener(self):
        source = (ROOT / "projects/fwbug008-physical/src/main.c").read_text()
        start = source.index("static const char recovery_startup[]")
        end = source.index("\n\n/* Positive host handoff", start)
        recovery = source[start:end]
        self.assertLess(recovery.index('"EMOS KEYINPUT extender'),
                        recovery.index('"EMOS LEGACY'))
        self.assertLess(recovery.index('"EMOS LEGACY'),
                        recovery.index('"VDU 22 3'))
        self.assertLess(recovery.index('"VDU 22 3'),
                        recovery.index('"EMOS sdserve --fast /'))


if __name__ == "__main__":
    unittest.main()
