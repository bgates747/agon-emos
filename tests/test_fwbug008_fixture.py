#!/usr/bin/env python3
"""Source contracts for the destructive FWBUG-008 physical fixture."""

from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]


class Fwbug008FixtureTests(unittest.TestCase):
    def test_recovery_selects_legacy_before_mode_without_nested_listener(self):
        source = (ROOT / "projects/fwbug008-physical/src/main.c").read_text()
        start = source.index("static const char recovery_startup[]")
        end = source.index("\n\n/* An ordinary application", start)
        recovery = source[start:end]
        self.assertLess(recovery.index('"EMOS KEYINPUT extender'),
                        recovery.index('"EMOS LEGACY'))
        self.assertLess(recovery.index('"EMOS LEGACY'),
                        recovery.index('"VDU 22 3'))
        self.assertNotIn("sdserve", recovery)
        self.assertNotIn("mos_oscli", source)

    def test_completion_marker_and_result_share_required_run_token(self):
        source = (ROOT / "projects/fwbug008-physical/src/main.c").read_text()
        self.assertIn('"A10-RP04 COMPLETE %s\\r\\n"', source)
        self.assertIn('"token=%s\\n"', source)
        self.assertIn("accept_token(argc, argv)", source)


if __name__ == "__main__":
    unittest.main()
