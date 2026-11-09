import json
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
STANDARD_SETTINGS = ROOT / "firmware" / "standard_settings.json"


class FirmwareStandardSettingsTest(unittest.TestCase):
    def load_settings(self):
        return json.loads(STANDARD_SETTINGS.read_text(encoding="utf-8"))

    def test_standard_settings_file_captures_durable_firmware_configuration(self):
        settings = self.load_settings()

        self.assertEqual("robot-evens-standard-firmware-settings", settings["kind"])
        self.assertEqual(1, settings["schemaVersion"])
        self.assertEqual("backups/robot-device-settings-20260701-042401.json", settings["sourceSnapshot"])

        durable = settings["settings"]
        for category in [
            "faceMoodStyles",
            "rgb",
            "clock",
            "triggers",
            "idle",
            "camera",
            "mumble",
            "proximity",
            "emotionSoundSlots",
            "listening",
            "voiceNod",
            "debugSubtitle",
            "motionStream",
            "screen",
            "motion",
        ]:
            with self.subTest(category=category):
                self.assertIn(category, durable)

        self.assertEqual("#00ff62", durable["rgb"]["color"])
        self.assertEqual(60, durable["rgb"]["brightness"])
        self.assertTrue(durable["rgb"]["breathing"])
        self.assertEqual(120, durable["motion"]["restPitch"])
        self.assertTrue(durable["camera"]["listenAfterCatchphrase"])
        self.assertFalse(durable["voiceNod"]["available"])
        self.assertEqual("/api/voice-nod/status", durable["voiceNod"]["endpoint"])
        self.assertEqual(15, len(durable["faceMoodStyles"]))

    def test_standard_settings_excludes_secrets_and_ephemeral_runtime_state(self):
        settings_text = STANDARD_SETTINGS.read_text(encoding="utf-8")
        settings = json.loads(settings_text)

        for forbidden in [
            "ssid",
            "password",
            "uptimeMs",
            "ip",
            "rssi",
            "freeHeap",
            "dateTimeText",
            "lastTriggeredAtMs",
            "cooldownRemainingMs",
            "lastMoveAtMs",
            "current",
            "voltage",
        ]:
            with self.subTest(forbidden=forbidden):
                self.assertNotIn(forbidden, settings_text)

        self.assertIn("excludedEphemeralFields", settings)
        self.assertIn("Wi-Fi credentials", settings["excludedEphemeralFields"])


if __name__ == "__main__":
    unittest.main()
