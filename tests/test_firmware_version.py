import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
FIRMWARE = ROOT / "firmware"


class FirmwareVersionTest(unittest.TestCase):
    def test_v081_is_the_single_firmware_version_source(self):
        version = (FIRMWARE / "firmware_version.h").read_text(encoding="utf-8")
        ino = (FIRMWARE / "firmware.ino").read_text(encoding="utf-8")
        hardware = (FIRMWARE / "robot_hardware.cpp").read_text(encoding="utf-8")
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")

        self.assertIn('ROBOT_FIRMWARE_VERSION = "V0.81"', version)
        self.assertIn('#include "firmware_version.h"', ino)
        self.assertIn("Desktop Robot boot " + '\" + String(ROBOT_FIRMWARE_VERSION)', ino)
        self.assertIn('#include "firmware_version.h"', hardware)
        self.assertIn('\\"version\\":\\"', hardware)
        self.assertIn("ROBOT_FIRMWARE_VERSION", hardware)
        self.assertIn("s.firmware?.version", console)


if __name__ == "__main__":
    unittest.main()
