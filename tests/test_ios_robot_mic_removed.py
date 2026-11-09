from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]
APP = ROOT / "ios" / "RobotEvensControl" / "RobotEvensControl"


class IOSRobotMicRemovalTest(unittest.TestCase):
    def test_audio_beat_control_uses_phone_microphone_only(self):
        view_model = (APP / "AppViewModel.swift").read_text(encoding="utf-8")
        content_view = (APP / "ContentView.swift").read_text(encoding="utf-8")

        for forbidden in [
            "AudioBeatMicrophoneSource",
            "audioBeatMicrophoneSource",
            "setAudioBeatMicrophoneSource",
            "robotAudioBeatTask",
            "runRobotAudioBeatLoop",
            "client.micSample",
            "机器人麦克风",
        ]:
            with self.subTest(forbidden=forbidden):
                self.assertNotIn(forbidden, view_model + content_view)

        self.assertNotIn('Picker("麦克风"', content_view)
        self.assertIn("手机麦克风", view_model + content_view)


if __name__ == "__main__":
    unittest.main()
