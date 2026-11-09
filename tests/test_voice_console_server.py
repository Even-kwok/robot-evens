import unittest
from pathlib import Path

from host.voice_console_server import (
    MobileRealtimeVoiceService,
    VOICE_CONSOLE_PATH,
    VoiceRobotProxy,
    make_handler,
    run_server,
)


class RecordingClient:
    def __init__(self):
        self.calls = []

    def get(self, path):
        self.calls.append(("GET", path, None))
        return {"ok": True, "path": path}

    def post(self, path, payload, timeout=5):
        self.calls.append(("POST", path, payload))
        return {"ok": True, "path": path, "payload": payload}


class RecordingModelConfig:
    def __init__(self):
        self.saved_payloads = []

    def status(self):
        return {"ok": True, "saved": False, "config": {}}

    def save(self, payload):
        self.saved_payloads.append(payload)
        return {"ok": True, "saved": True, "config": dict(payload or {})}


class SavedRealtimeConfig:
    def status(self):
        return {
            "ok": True,
            "saved": True,
            "config": {
                "knownUser": "owner",
                "realtimeAppId": "app-123",
                "realtimeAccessToken": "token-123",
                "realtimeEndpoint": "wss://example.test/realtime",
                "realtimeModelVersion": "1.2.1.1",
                "realtimeSpeaker": "voice",
                "realtimeRecvTimeout": "15",
                "realtimeInsecureTls": "1",
                "realtimeFirmwareNotes": "你是桌面机器人，可以自然语音回应。",
            },
        }


class VoiceConsolePortDefaultsTest(unittest.TestCase):
    def test_voice_console_defaults_to_dedicated_port_8767(self):
        source = (Path(__file__).resolve().parents[1] / "host" / "voice_console_server.py").read_text(encoding="utf-8")

        self.assertEqual(run_server.__defaults__[0], "0.0.0.0")
        self.assertEqual(run_server.__defaults__[1], 8767)
        self.assertIn('parser.add_argument("--bind", default="0.0.0.0")', source)
        self.assertIn('parser.add_argument("--port", type=int, default=8767)', source)


class VoiceConsoleModelConfigTest(unittest.TestCase):
    def test_voice_console_keeps_shared_model_config_routes_and_controls(self):
        source = Path(VOICE_CONSOLE_PATH).read_text(encoding="utf-8")
        server_source = (Path(__file__).resolve().parents[1] / "host" / "voice_console_server.py").read_text(encoding="utf-8")

        for snippet in [
            "读取 API 配置",
            "保存 API 配置",
            "voiceRealtimeConfigState",
            "loadVoiceModelConfig",
            "saveVoiceModelConfig",
            "/api/host/model-config/status",
            "/api/host/model-config/save",
            "realtimeAppId",
            "realtimeAccessToken",
            "realtimeEndpoint",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source + server_source)

        model_config = RecordingModelConfig()
        handler = make_handler(VoiceRobotProxy(RecordingClient()), "http://robot.local", model_config=model_config)
        self.assertIs(handler.model_config, model_config)


class VoiceRobotProxyTest(unittest.TestCase):
    def test_voice_proxy_has_no_robot_voice_gateway_methods(self):
        proxy = VoiceRobotProxy(RecordingClient())

        for removed_method in [
            "listening_status",
            "listening_config",
            "listening_start",
            "listening_stop",
            "listening_wake_test",
            "voice_gateway_status",
            "voice_gateway_start",
            "voice_gateway_volume",
            "voice_gateway_stop",
            "voice_stream_status",
            "voice_stream_start",
            "voice_stream_chunk",
            "voice_stream_stop",
        ]:
            with self.subTest(removed_method=removed_method):
                self.assertFalse(hasattr(proxy, removed_method))

    def test_voice_console_runtime_and_listening_routes_are_removed(self):
        source = Path(VOICE_CONSOLE_PATH).read_text(encoding="utf-8")
        server_source = (Path(__file__).resolve().parents[1] / "host" / "voice_console_server.py").read_text(encoding="utf-8")

        for removed in [
            "/api/robot/listening/status",
            "/api/robot/listening/config",
            "/api/robot/listening/start",
            "/api/robot/listening/stop",
            "/api/robot/listening/wake-test",
            "/api/robot/wake-event/status",
            "/api/robot/wake-event",
            "VoiceWakeEventStore",
            "refreshWakeEvent",
            "/api/robot/voice-gateway/",
            "/api/voice-gateway/",
            "RobotVoiceGateway",
            "startRobotMicGateway",
            "navigator.mediaDevices",
            "getUserMedia",
            "/api/host/realtime-stream/",
            "recordVolcRealtimeAudioAction",
        ]:
            with self.subTest(removed=removed):
                self.assertNotIn(removed, source + server_source)

        handler = make_handler(VoiceRobotProxy(RecordingClient()), "http://robot.local")
        self.assertIsNotNone(handler)


class MobileRealtimeVoiceServiceTest(unittest.TestCase):
    def test_audio_request_decodes_phone_pcm_and_asks_robot_to_play_reply(self):
        captured = {}

        def runner(config, audio_pcm16, system_prompt):
            captured["config"] = config
            captured["audio"] = audio_pcm16
            captured["systemPrompt"] = system_prompt
            return {
                "ok": True,
                "chatText": "我听到了",
                "audioPcm16": b"\x01\x02\x03\x04",
                "audioSampleRate": 24_000,
                "audioFormat": "pcm_s16le",
            }

        class RobotAudio:
            def __init__(self):
                self.calls = []

            def play_realtime_voice(self, audio, sample_rate, audio_format):
                self.calls.append((audio, sample_rate, audio_format))
                return {"ok": True, "uploadedBytes": 2}

        robot_audio = RobotAudio()
        service = MobileRealtimeVoiceService(
            SavedRealtimeConfig(),
            runner=runner,
            robot_audio=robot_audio,
        )

        result = service.run_audio({"audioBase64": "AQIDBA==", "sampleRate": 16_000})

        self.assertEqual(result["ok"], True)
        self.assertEqual(result["chatText"], "我听到了")
        self.assertEqual(result["audioBase64"], "AQIDBA==")
        self.assertEqual(result["audioBytes"], 4)
        self.assertEqual(result["robotAudio"], {"ok": True, "uploadedBytes": 2})
        self.assertEqual(captured["config"].app_id, "app-123")
        self.assertEqual(captured["audio"], b"\x01\x02\x03\x04")
        self.assertIn("自然语音", captured["systemPrompt"])
        self.assertEqual(robot_audio.calls, [(b"\x01\x02\x03\x04", 24_000, "pcm_s16le")])

    def test_audio_request_rejects_empty_audio(self):
        service = MobileRealtimeVoiceService(SavedRealtimeConfig(), runner=lambda *args: {})

        result = service.run_audio({"audioBase64": ""})

        self.assertEqual(result["ok"], False)
        self.assertEqual(result["stage"], "input")

    def test_mobile_only_mode_exposes_audio_without_host_model_config_routes(self):
        server_source = (Path(__file__).resolve().parents[1] / "host" / "voice_console_server.py").read_text(encoding="utf-8")

        self.assertIn("/api/mobile/realtime-voice/audio", server_source)
        self.assertIn("--mobile-only", server_source)
        self.assertIn("make_mobile_handler", server_source)


if __name__ == "__main__":
    unittest.main()
