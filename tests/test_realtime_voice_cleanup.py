import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
HOST = ROOT / "host"
FIRMWARE = ROOT / "firmware"


class RealtimeVoiceCleanupTest(unittest.TestCase):
    def test_removed_realtime_voice_gateway_files(self):
        for path in [
            HOST / "voice_gateway.py",
            HOST / "realtime_asset_whitelist.py",
            HOST / "realtime_assets.local.json",
            HOST / "realtime_robot.py",
            HOST / "realtime_robot_client.py",
            HOST / "realtime_robot_contract.py",
            FIRMWARE / "voice_gateway_client.cpp",
            FIRMWARE / "voice_gateway_client.h",
        ]:
            with self.subTest(path=path.name):
                self.assertFalse(path.exists())

    def test_old_runtime_voice_routes_are_removed_but_realtime_voice_test_is_kept(self):
        source = "\n".join(
            path.read_text(encoding="utf-8")
            for path in [
                HOST / "voice_console_server.py",
                HOST / "voice_console.html",
                HOST / "console_server.py",
                HOST / "web_console.html",
            ]
            if path.exists()
        )

        for removed in [
            "/api/voice-gateway/",
            "/api/robot/voice-gateway/",
            "/api/host/realtime-stream/",
            "VoiceGateway",
            "RobotVoiceGateway",
            "startRobotMicGateway",
            "continuousRealtime",
            "HostRealtimeVoiceLiveService",
            "/api/host/realtime-voice/audio",
            "/api/host/realtime-voice/live/status",
            "/api/host/realtime-voice/live/start",
            "/api/host/realtime-voice/live/audio",
            "/api/host/realtime-voice/live/stop",
            "/api/host/realtime-assets/",
            "/api/host/realtime/audio-action",
            "/api/host/realtime-session/",
            "/api/host/wake-event/status",
            "/api/robot/wake-event",
            "/api/robot/listening/status",
            "/api/robot/listening/config",
            "/api/robot/listening/start",
            "/api/robot/listening/stop",
            "/api/robot/listening/wake-test",
            "语音唤醒测试",
            "固件聆听窗口",
            "SpeechRecognition",
            "webkitSpeechRecognition",
            "navigator.mediaDevices.getUserMedia",
            "startLiveVoice",
            "sendLiveVoiceChunk",
            "realtime_voice_pending",
            "Host 实时资产白名单",
        ]:
            with self.subTest(removed=removed):
                self.assertNotIn(removed, source)

        for kept in [
            "realtimeAppId",
            "realtimeAccessToken",
            "realtimeEndpoint",
            "realtimeModelVersion",
            "realtimeSpeaker",
            "realtimeRecvTimeout",
            "realtimeInsecureTls",
            "/api/host/model-config/status",
            "/api/host/model-config/save",
            "/api/host/realtime-voice/text",
            "/api/host/realtime-voice/session/start",
            "/api/host/realtime-voice/session/frame",
            "/api/host/realtime-voice/session/stop",
            "runRealtimeVoiceText",
            "startHostRealtimeVoice",
            "playRealtimePcm16",
            "/api/mobile/realtime-voice/audio",
        ]:
            with self.subTest(kept=kept):
                self.assertIn(kept, source)

    def test_firmware_realtime_voice_gateway_is_unwired(self):
        source = "\n".join(
            path.read_text(encoding="utf-8")
            for path in sorted(FIRMWARE.glob("*.*"))
            if path.suffix in {".ino", ".h", ".cpp"} and path.name != "config.local.h"
        )

        for removed in [
            "voice_gateway_client.h",
            "VoiceGatewayClient",
            "voiceGateway",
            "/api/voice-gateway/",
            "VOICE_GATEWAY_",
        ]:
            with self.subTest(removed=removed):
                self.assertNotIn(removed, source)


if __name__ == "__main__":
    unittest.main()
