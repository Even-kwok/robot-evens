from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]


class IOSRealtimeVoiceTest(unittest.TestCase):
    def test_phone_microphone_realtime_voice_ui_and_view_model_are_wired(self):
        app_view_model = (ROOT / "ios/RobotEvensControl/RobotEvensControl/AppViewModel.swift").read_text()
        content_view = (ROOT / "ios/RobotEvensControl/RobotEvensControl/ContentView.swift").read_text()
        core_client = (ROOT / "ios/RobotEvensCore/Sources/RobotEvensCore/RobotClient.swift").read_text()

        for snippet in [
            "startRealtimeVoiceStreaming",
            "stopRealtimeVoiceStreaming",
            "VolcengineRealtimeDirectClient",
            "VolcengineRealtimeMobileConfig.defaultComputerConfig",
            "VoiceLiveAudioStreamer",
            "isRealtimeVoiceStreaming",
            "isRealtimeVoiceFrameSending",
            "realtimeVoiceStatusText",
            "realtimeVoiceFrameCount",
            "AVAudioEngine",
            "targetSampleRate",
            "pcm16",
            "enqueueRealtimeVoiceChunk",
            "sendAudioFrame(",
            "playRealtimeVoice(",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, app_view_model)

        for snippet in [
            "realtimeVoiceSection",
            "实时语音",
            "手机直连机器人",
            "开始实时收音",
            "停止实时收音",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, content_view)

        for snippet in [
            "VolcengineRealtimeMobileConfig",
            "defaultComputerConfig",
            "8061882989",
            "zh_male_yunzhou_jupiter_bigtts",
            "wss://openspeech.bytedance.com/api/v3/realtime/dialogue",
            "VolcengineRealtimeDirectClient",
            "URLSessionWebSocketTask",
            "RobotAudioConverter",
            "playRealtimeVoice(",
            "/api/mumble/upload",
            "/api/mumble/set",
            "/api/mumble/play",
            "sendAudioFrame(",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, core_client)

        for removed in [
            "realtimeVoiceHostURLString",
            "语音Host地址",
            "http://电脑IP:8768",
            "HostRealtimeVoiceEndpointBuilder",
            "HostRealtimeVoiceClient",
            "HostRealtimeVoiceLiveStatus",
            "/api/host/realtime-voice/live/status",
            "/api/host/realtime-voice/live/start",
            "/api/host/realtime-voice/live/audio",
            "/api/host/realtime-voice/live/stop",
            "停止并发送",
            "VoicePromptRecorder",
            "VoicePromptAudio",
            "sendRealtimeVoiceAudio",
            "/api/mobile/realtime-voice/audio",
        ]:
            with self.subTest(removed=removed):
                self.assertNotIn(removed, app_view_model + content_view + core_client)

    def test_realtime_voice_panel_keeps_stable_layout_while_streaming(self):
        content_view = (ROOT / "ios/RobotEvensControl/RobotEvensControl/ContentView.swift").read_text()
        realtime_section = content_view.split("private var realtimeVoiceSection", 1)[1].split(
            "private var chatMotionControls", 1
        )[0]

        self.assertNotIn("if viewModel.isRealtimeVoiceFrameSending", realtime_section)
        for snippet in [
            ".opacity(viewModel.isRealtimeVoiceFrameSending ? 1 : 0)",
            ".frame(width: 20, height: 20)",
            ".frame(minHeight: 20",
            ".frame(minHeight: 54",
            ".transaction { transaction in",
            "transaction.animation = nil",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, realtime_section)


if __name__ == "__main__":
    unittest.main()
