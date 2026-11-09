import json
import re
import unittest
from pathlib import Path


PROJECT_ROOT = Path(__file__).resolve().parents[1]
MANIFEST_PATH = PROJECT_ROOT / "assets" / "voice" / "retro_robot" / "manifest.json"


class VoicePackManifestTest(unittest.TestCase):
    def setUp(self):
        self.manifest = json.loads(MANIFEST_PATH.read_text(encoding="utf-8"))

    def test_manifest_describes_english_male_tape_robot_pack(self):
        self.assertEqual(self.manifest["packId"], "rocko_soul_robot_basic")
        self.assertIn("tape", self.manifest["style"])
        self.assertIn("soul", self.manifest["style"])
        self.assertEqual(self.manifest["generator"]["voice"], "Rocko")
        self.assertIn("tape", self.manifest["generator"]["effect"])

    def test_all_phrase_text_is_english(self):
        phrases = self.manifest["phrases"]
        self.assertEqual(len(phrases), 20)
        for phrase in phrases:
            self.assertNotRegex(phrase["text"], re.compile(r"[\u4e00-\u9fff]"))

    def test_hello_phrase_names_the_robot(self):
        phrases = {phrase["id"]: phrase for phrase in self.manifest["phrases"]}
        self.assertEqual(phrases["hello"]["text"], "Hello. I am Robot.")

    def test_phrase_ids_cover_playful_self_talk_and_basic_prompts(self):
        phrase_ids = [phrase["id"] for phrase in self.manifest["phrases"]]
        self.assertEqual(
            phrase_ids,
            [
                "hello",
                "hello_human",
                "are_you_there",
                "say_something",
                "i_heard_you",
                "listening",
                "thinking",
                "processing",
                "curious",
                "confused",
                "happy",
                "lonely",
                "self_talk",
                "wake_up",
                "standby",
                "received",
                "done",
                "battery_low",
                "error",
                "goodbye",
            ],
        )

    def test_top_level_wavs_match_manifest_files(self):
        expected = {phrase["file"] for phrase in self.manifest["phrases"]}
        actual = {path.name for path in MANIFEST_PATH.parent.glob("*.wav")}
        self.assertEqual(actual, expected)


if __name__ == "__main__":
    unittest.main()
