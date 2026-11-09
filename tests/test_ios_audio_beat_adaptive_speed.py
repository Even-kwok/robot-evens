from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]
APP = ROOT / "ios" / "RobotEvensControl" / "RobotEvensControl"


class IOSAudioBeatAdaptiveSpeedTest(unittest.TestCase):
    def test_audio_beat_ui_exposes_adaptive_servo_speed_controls(self):
        view_model = (APP / "AppViewModel.swift").read_text(encoding="utf-8")
        content_view = (APP / "ContentView.swift").read_text(encoding="utf-8")

        for snippet in [
            "isAudioBeatAdaptiveSpeedEnabled",
            "audioBeatSlowSpeed",
            "audioBeatFastSpeed",
            "setAudioBeatAdaptiveSpeedEnabled",
            "setAudioBeatSlowSpeed",
            "setAudioBeatFastSpeed",
            "isAdaptiveSpeedEnabled: isAudioBeatAdaptiveSpeedEnabled",
            "adaptiveSlowSpeed: audioBeatSlowSpeed",
            "adaptiveFastSpeed: audioBeatFastSpeed",
            "跟随节拍调速",
            "慢拍速度",
            "快拍速度",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, view_model + content_view)

    def test_audio_beat_ui_exposes_randomized_pitch_and_yaw_amplitude_switches(self):
        view_model = (APP / "AppViewModel.swift").read_text(encoding="utf-8")
        content_view = (APP / "ContentView.swift").read_text(encoding="utf-8")

        for snippet in [
            "isAudioBeatPitchAmplitudeRandomized",
            "isAudioBeatYawAmplitudeRandomized",
            "setAudioBeatPitchAmplitudeRandomized",
            "setAudioBeatYawAmplitudeRandomized",
            "isPitchAmplitudeRandomized: isAudioBeatPitchAmplitudeRandomized",
            "isYawAmplitudeRandomized: isAudioBeatYawAmplitudeRandomized",
            "点头幅度随机变化",
            "摇头幅度随机变化",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, view_model + content_view)

    def test_audio_beat_light_rgb_requests_are_temporary(self):
        view_model = (APP / "AppViewModel.swift").read_text(encoding="utf-8")

        self.assertRegex(view_model, r"client\?\.setRGB\([\s\S]*temporary: true")

    def test_audio_beat_light_restores_standby_rgb_when_stopped(self):
        view_model = (APP / "AppViewModel.swift").read_text(encoding="utf-8")

        self.assertRegex(
            view_model,
            r"func setAudioBeatLightEnabled[\s\S]*restoreAudioBeatStandbyLight\(\)"
        )
        self.assertRegex(
            view_model,
            r"private func stopAudioBeatControl\(\)[\s\S]*let shouldRestoreAudioBeatLight = isAudioBeatLightEnabled[\s\S]*restoreAudioBeatStandbyLight\(\)"
        )
        self.assertRegex(
            view_model,
            r"private func restoreAudioBeatStandbyLight\(\)[\s\S]*client\?\.restoreStandbyRGB\(\)"
        )


if __name__ == "__main__":
    unittest.main()
