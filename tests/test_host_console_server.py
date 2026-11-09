import base64
import json
import threading
import time
import unittest
from tempfile import TemporaryDirectory
from unittest.mock import patch
from pathlib import Path

import host.console_server as console_server
from host.console_server import (
    AgentDispatchModel,
    AgentDispatchMonitor,
    DEFAULT_AGENT_ALLOWED_ASSETS,
    DEFAULT_AGENT_DISPATCH_PROMPT,
    HostModelConfigStore,
    HostVisionTracker,
    HostRobotProxy,
    HostRealtimeVoiceService,
    _pcm_s16le_to_raw_u8_mono,
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

    def mumble_upload(self, data, filename="realtime-api.raw", timeout=15):
        self.calls.append(("mumble_upload", bytes(data), filename))
        return {"ok": True, "path": "/api/mumble/upload", "uploadedBytes": len(data), "uploadedName": filename}

    def mumble_set(self, enabled=None, speed_percent=None, volume=None, auto_enabled=None, timeout=5):
        self.calls.append(("mumble_set", enabled, speed_percent, volume, auto_enabled))
        return {"ok": True, "path": "/api/mumble/set", "enabled": enabled, "volume": volume}

    def mumble_play(self, mode="full", duration_ms=None, timeout=10):
        self.calls.append(("mumble_play", mode, duration_ms))
        return {"ok": True, "path": "/api/mumble/play", "mode": mode, "durationMs": duration_ms}

    def mumble_stop(self, timeout=5):
        self.calls.append(("mumble_stop",))
        return {"ok": True, "path": "/api/mumble/stop"}

    def rgb(self, color, index=None):
        self.calls.append(("rgb", color, index))
        return {"ok": True}

    def face_set(self, mood, color):
        self.calls.append(("face_set", mood, color))
        return {"ok": True}

    def motion_pose(self, yaw, pitch, speed):
        self.calls.append(("motion_pose", yaw, pitch, speed))
        return {"ok": True}

    def motion_home(self, speed=300):
        self.calls.append(("motion_home", speed))
        return {"ok": True}

    def emotion_play(self, name, lights=True, sound=False):
        self.calls.append(("emotion_play", name, lights, sound))
        return {"ok": True, "action": name}

    def asset_invoke(self, asset_id, **kwargs):
        self.calls.append(("asset_invoke", asset_id, kwargs))
        return {"ok": True, "id": asset_id}


class HostConsolePortDefaultsTest(unittest.TestCase):
    def test_action_console_defaults_to_legacy_realtime_port_8766(self):
        source = (Path(__file__).resolve().parents[1] / "host" / "console_server.py").read_text(encoding="utf-8")

        self.assertEqual(run_server.__defaults__[1], 8766)
        self.assertIn('parser.add_argument("--port", type=int, default=8766)', source)


class HostRobotProxyTest(unittest.TestCase):
    def test_camera_status_proxies_to_robot_camera_status(self):
        client = RecordingClient()
        proxy = HostRobotProxy(client)
        result = proxy.camera_status()
        self.assertEqual(result["path"], "/api/camera/status")
        self.assertEqual(client.calls, [("GET", "/api/camera/status", None)])

    def test_camera_set_proxies_to_robot_camera_set(self):
        client = RecordingClient()
        proxy = HostRobotProxy(client)
        result = proxy.camera_set({"enabled": True, "captureIntervalMs": 12000})
        self.assertEqual(result["path"], "/api/camera/set")
        self.assertEqual(client.calls, [("POST", "/api/camera/set", {"enabled": True, "captureIntervalMs": 12000})])

    def test_realtime_voice_playback_converts_api_pcm_and_uses_robot_speaker(self):
        client = RecordingClient()
        proxy = HostRobotProxy(client)

        result = proxy.play_realtime_voice(b"\x00\x00\xff\x7f\x00\x80", sample_rate=16000, audio_format="pcm_s16le")

        self.assertEqual(result["ok"], True)
        self.assertEqual(result["format"], "raw-u8-16k-mono")
        self.assertEqual(result["uploadedBytes"], 3)
        self.assertEqual(
            client.calls,
            [
                ("mumble_upload", b"\x80\xff\x00", "realtime-api.raw"),
                ("mumble_set", True, None, 255, None),
                ("mumble_play", "full", None),
            ],
        )

    def test_realtime_voice_stop_stops_robot_speaker(self):
        client = RecordingClient()
        proxy = HostRobotProxy(client)

        result = proxy.stop_realtime_voice()

        self.assertEqual(result["ok"], True)
        self.assertEqual(result["path"], "/api/mumble/stop")
        self.assertEqual(client.calls, [("mumble_stop",)])

    def test_camera_catchphrase_proxies_to_robot_camera_catchphrase(self):
        client = RecordingClient()
        proxy = HostRobotProxy(client)
        result = proxy.camera_catchphrase({"assetId": "halo", "listenAfter": True})
        self.assertEqual(result["path"], "/api/camera/catchphrase")
        self.assertEqual(client.calls, [("POST", "/api/camera/catchphrase", {"assetId": "halo", "listenAfter": True})])

    def test_camera_face_report_proxies_to_robot_camera_report(self):
        client = RecordingClient()
        proxy = HostRobotProxy(client)
        payload = {"scanId": 7, "faceDetected": False, "confidence": 0}
        result = proxy.camera_face_report(payload)
        self.assertEqual(result["path"], "/api/camera/face-report")
        self.assertEqual(client.calls, [("POST", "/api/camera/face-report", payload)])

    def test_camera_wake_and_clear_proxy_to_robot_camera_actions(self):
        client = RecordingClient()
        proxy = HostRobotProxy(client)
        self.assertEqual(proxy.camera_wake({"holdMs": 4000})["path"], "/api/camera/wake")
        self.assertEqual(proxy.camera_clear()["path"], "/api/camera/clear")
        self.assertEqual(
            client.calls,
            [
                ("POST", "/api/camera/wake", {"holdMs": 4000}),
                ("POST", "/api/camera/clear", {}),
            ],
        )

    def test_host_console_exposes_host_vision_routes(self):
        source = (Path(__file__).resolve().parents[1] / "host" / "console_server.py").read_text(encoding="utf-8")
        for snippet in [
            "HostVisionTracker",
            "/api/host/vision/status",
            "/api/host/vision/step",
            "/api/host/vision/start",
            "/api/host/vision/stop",
            "vision_tracker.step",
            "vision_tracker.start",
            "vision_tracker.stop",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)

    def test_host_console_exposes_browser_realtime_voice_session_routes(self):
        source = (Path(__file__).resolve().parents[1] / "host" / "console_server.py").read_text(encoding="utf-8")
        for snippet in [
            "HostRealtimeVoiceSessionService",
            "/api/host/realtime-voice/session/status",
            "/api/host/realtime-voice/session/start",
            "/api/host/realtime-voice/session/frame",
            "/api/host/realtime-voice/session/stop",
            "realtime_voice_session.status",
            "realtime_voice_session.start",
            "realtime_voice_session.frame",
            "realtime_voice_session.stop",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)

    def test_catalog_proxy_methods_use_read_only_catalog_routes(self):
        client = RecordingClient()
        proxy = HostRobotProxy(client)
        self.assertEqual(proxy.catalog()["path"], "/api/catalog")
        self.assertEqual(proxy.catalog_assets()["path"], "/api/catalog/assets")
        self.assertEqual(proxy.catalog_logic()["path"], "/api/catalog/logic")
        self.assertEqual(proxy.catalog_conflicts()["path"], "/api/catalog/conflicts")
        self.assertEqual(
            client.calls,
            [
                ("GET", "/api/catalog", None),
                ("GET", "/api/catalog/assets", None),
                ("GET", "/api/catalog/logic", None),
                ("GET", "/api/catalog/conflicts", None),
            ],
        )

    def test_asset_invoke_proxy_uses_unified_asset_route(self):
        client = RecordingClient()
        proxy = HostRobotProxy(client)
        payload = {"id": "expression.happy", "durationMs": 900}
        result = proxy.asset_invoke(payload)
        self.assertEqual(result["path"], "/api/asset/invoke")
        self.assertEqual(client.calls, [("POST", "/api/asset/invoke", payload)])

    def test_debug_only_yes_no_asset_ports_are_removed(self):
        client = RecordingClient()
        proxy = HostRobotProxy(client)
        source = (Path(__file__).resolve().parents[1] / "host" / "console_server.py").read_text(encoding="utf-8")

        self.assertFalse(hasattr(proxy, "test_nod_yes"))
        self.assertFalse(hasattr(proxy, "test_shake_no"))
        self.assertNotIn("/api/robot/test/nod-yes", source)
        self.assertNotIn("/api/robot/test/shake-no", source)
        self.assertNotIn("/api/robot/realtime/action", source)

    def test_realtime_action_router_proxy_methods_are_removed(self):
        client = RecordingClient()
        proxy = HostRobotProxy(client)

        for method_name in (
            "model_action",
            "simple_yes_no_action",
            "volcengine_realtime_text_action",
            "volcengine_realtime_audio_action",
            "realtime_action",
            "realtime_action_ids",
            "realtime_action_notes",
            "realtime_firmware_notes",
        ):
            with self.subTest(method_name=method_name):
                self.assertFalse(hasattr(proxy, method_name))

    def test_host_console_removes_realtime_action_routes_and_asset_store(self):
        source = (Path(__file__).resolve().parents[1] / "host" / "console_server.py").read_text(encoding="utf-8")
        for removed in [
            "HostWakeEventStore",
            "HostRealtimeSessionStore",
            "HostRealtimeAssetStore",
            "ActionRouter",
            "allowed_action_ids",
            "build_robot_system_prompt",
            "parse_realtime_action_text",
            "run_volcengine_realtime_audio",
            "REALTIME_ASSET_CONFIG_PATH",
            "/api/host/realtime-assets/status",
            "/api/host/realtime-assets/save",
            "/api/host/realtime/audio-action",
            "/api/host/realtime-voice/audio",
            "/api/host/realtime-session/chunk",
            "/api/host/realtime-session/start",
            "/api/host/realtime-session/stop",
            "/api/host/wake-event/status",
            "/api/robot/wake-event",
            "/api/robot/listening/status",
            "/api/robot/listening/config",
            "/api/robot/listening/start",
            "/api/robot/listening/stop",
            "/api/robot/listening/wake-test",
            "/api/robot/debug/subtitle/status",
            "/api/robot/debug/subtitle/set",
            "/api/robot/debug/subtitle/show",
            "HostRealtimeVoiceLiveService",
            "/api/host/realtime-voice/live/status",
            "/api/host/realtime-voice/live/start",
            "/api/host/realtime-voice/live/audio",
            "/api/host/realtime-voice/live/stop",
        ]:
            with self.subTest(removed=removed):
                self.assertNotIn(removed, source)


class AgentDispatchMonitorTest(unittest.TestCase):
    def test_records_running_and_completed_dispatch_history(self):
        monitor = AgentDispatchMonitor(clock_ms=lambda: 12345)

        event_id = monitor.started(
            source="realtime_voice_session",
            mode="agent_model",
            user_text="你认识我吗？",
            robot_reply="当然认识",
        )
        running = monitor.status()

        self.assertEqual(running["state"], "running")
        self.assertEqual(running["current"]["id"], event_id)
        self.assertEqual(running["current"]["source"], "realtime_voice_session")
        self.assertEqual(running["current"]["mode"], "agent_model")
        self.assertEqual(running["current"]["userText"], "你认识我吗？")

        monitor.finished(event_id, {"ok": True, "dispatch": {"action_id": "behavior.robot_action.nod_yes"}, "decision": {"reason": "肯定回答"}})
        status = monitor.status()

        self.assertEqual(status["state"], "ok")
        self.assertEqual(status["last"]["id"], event_id)
        self.assertEqual(status["last"]["actionId"], "behavior.robot_action.nod_yes")
        self.assertEqual(status["last"]["reason"], "肯定回答")
        self.assertEqual(status["history"][0]["state"], "ok")

    def test_records_failed_dispatch_error(self):
        monitor = AgentDispatchMonitor(clock_ms=lambda: 20000)

        event_id = monitor.started(source="realtime_voice_text", mode="agent_model")
        monitor.finished(event_id, {"ok": False, "stage": "agent_model", "error": "401 Unauthorized"})
        status = monitor.status()

        self.assertEqual(status["state"], "error")
        self.assertEqual(status["last"]["stage"], "agent_model")
        self.assertEqual(status["last"]["error"], "401 Unauthorized")


class RecordingAgentWorkflow:
    def __init__(self):
        self.calls = []

    def robot_status(self, payload=None):
        self.calls.append(("robot_status", payload or {}))
        return {"ok": True, "workflow": "robot_status"}

    def invoke_safe_behavior(self, payload):
        self.calls.append(("invoke_safe_behavior", payload))
        return {"ok": True, "workflow": "invoke_safe_behavior", "payload": payload}

    def face_detect_and_center(self, payload):
        self.calls.append(("face_detect_and_center", payload))
        return {"ok": True, "workflow": "face_detect_and_center", "payload": payload}

    def find_and_greet(self, payload):
        self.calls.append(("find_and_greet", payload))
        return {"ok": True, "workflow": "find_and_greet", "payload": payload}

    def dispatch_action(self, payload):
        self.calls.append(("dispatch_action", payload))
        return {"ok": True, "workflow": "dispatch_action", "payload": payload}

    def coding_event(self, payload):
        self.calls.append(("coding_event", payload))
        return {"ok": True, "workflow": "coding_event", "payload": payload}


class HostConsoleAgentRoutesTest(unittest.TestCase):
    def test_host_console_exposes_agent_routes(self):
        source = (Path(__file__).resolve().parents[1] / "host" / "console_server.py").read_text(encoding="utf-8")
        for snippet in [
            "RobotAgentWorkflowService",
            "/api/agent/status",
            "/api/agent/dispatch-monitor",
            "/api/agent/invoke-safe-behavior",
            "/api/agent/face-detect-and-center",
            "/api/agent/find-and-greet",
            "/api/agent/dispatch-action",
            "/api/agent/coding-event",
            "agent_workflows.robot_status",
            "agent_monitor.status",
            "agent_workflows.invoke_safe_behavior",
            "agent_workflows.face_detect_and_center",
            "agent_workflows.find_and_greet",
            "agent_workflows.dispatch_action",
            "agent_workflows.coding_event",
            "AgentMcpServer",
            "agent_mcp.handle_request",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)


class HostRealtimeVoiceServiceTest(unittest.TestCase):
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

    def test_text_request_requires_saved_realtime_credentials(self):
        class EmptyConfig:
            def status(self):
                return {"ok": True, "saved": False, "config": {}}

        result = HostRealtimeVoiceService(EmptyConfig()).run_text({"text": "你好"})

        self.assertEqual(result["ok"], False)
        self.assertIn("Realtime App ID", result["error"])

    def test_pcm_s16le_to_raw_u8_mono_matches_firmware_mumble_format(self):
        self.assertEqual(
            _pcm_s16le_to_raw_u8_mono(b"\x00\x00\xff\x7f\x00\x80", source_rate=16000),
            b"\x80\xff\x00",
        )
        self.assertEqual(
            _pcm_s16le_to_raw_u8_mono(b"\x00\x00\xff\x7f\x00\x80", source_rate=24000),
            b"\x80\x7f",
        )

    def test_text_request_returns_audio_and_asks_robot_to_play_it(self):
        captured = {}

        def runner(config, text, system_prompt):
            captured["config"] = config
            captured["text"] = text
            captured["systemPrompt"] = system_prompt
            return {
                "ok": True,
                "sessionId": "session-1",
                "logId": "log-1",
                "chatText": "你好呀",
                "audioPcm16": b"\x01\x02\x03\x04",
                "audioSampleRate": 24000,
                "audioFormat": "pcm_s16le",
                "events": [],
            }

        class RobotAudio:
            def __init__(self):
                self.calls = []

            def play_realtime_voice(self, audio, sample_rate, audio_format):
                self.calls.append((audio, sample_rate, audio_format))
                return {"ok": True, "uploadedBytes": 2, "play": {"ok": True}}

        robot_audio = RobotAudio()

        result = HostRealtimeVoiceService(
            self.SavedRealtimeConfig(),
            runner=runner,
            robot_audio=robot_audio,
        ).run_text({"text": "你认识我吗？"})

        self.assertEqual(result["ok"], True)
        self.assertEqual(result["chatText"], "你好呀")
        self.assertEqual(result["audioBase64"], "AQIDBA==")
        self.assertEqual(result["audioBytes"], 4)
        self.assertEqual(result["audioSampleRate"], 24000)
        self.assertEqual(result["audioFormat"], "pcm_s16le")
        self.assertEqual(result["robotAudio"], {"ok": True, "uploadedBytes": 2, "play": {"ok": True}})
        self.assertEqual(robot_audio.calls, [(b"\x01\x02\x03\x04", 24000, "pcm_s16le")])
        self.assertEqual(captured["config"].app_id, "app-123")
        self.assertEqual(captured["config"].access_key, "token-123")
        self.assertEqual(captured["text"], "你认识我吗？")
        self.assertIn("自然语音", captured["systemPrompt"])

    def test_text_request_dispatches_affirmative_reply_to_agent_nod(self):
        order = []

        def runner(config, text, system_prompt):
            return {
                "ok": True,
                "chatText": "当然认识你呀",
                "audioPcm16": b"\x01\x02",
                "audioSampleRate": 24000,
                "events": [],
            }

        class RobotAudio:
            def play_realtime_voice(self, audio, sample_rate, audio_format):
                order.append("audio")
                return {"ok": True}

        class AgentDispatcher:
            def __init__(self):
                self.calls = []

            def dispatch_action(self, payload):
                order.append("dispatch")
                self.calls.append(dict(payload))
                return {"ok": True, "dispatch": {"action_id": "behavior.robot_action.nod_yes"}}

        agent = AgentDispatcher()

        result = HostRealtimeVoiceService(
            self.SavedRealtimeConfig(),
            runner=runner,
            robot_audio=RobotAudio(),
            agent_dispatcher=agent,
        ).run_text({"text": "你认不认识我？"})

        self.assertEqual(result["ok"], True)
        self.assertEqual(result["agentDispatch"]["dispatch"]["action_id"], "behavior.robot_action.nod_yes")
        self.assertEqual(
            agent.calls,
            [{"command": "nod_yes", "source": "realtime_voice_text", "bypass_cooldown": True}],
        )
        self.assertEqual(order, ["dispatch", "audio"])

    def test_text_request_records_agent_dispatch_monitor_status(self):
        def runner(config, text, system_prompt):
            return {
                "ok": True,
                "chatText": "当然认识你呀",
                "audioPcm16": b"",
                "audioSampleRate": 24000,
                "events": [],
            }

        class AgentDispatcher:
            def dispatch_action(self, payload):
                return {"ok": True, "dispatch": {"action_id": "behavior.robot_action.nod_yes"}}

        monitor = AgentDispatchMonitor(clock_ms=lambda: 34567)

        result = HostRealtimeVoiceService(
            self.SavedRealtimeConfig(),
            runner=runner,
            agent_dispatcher=AgentDispatcher(),
            agent_monitor=monitor,
        ).run_text({"text": "你认不认识我？"})

        status = monitor.status()
        self.assertEqual(result["ok"], True)
        self.assertEqual(status["state"], "ok")
        self.assertEqual(status["last"]["source"], "realtime_voice_text")
        self.assertEqual(status["last"]["mode"], "keyword_fallback")
        self.assertEqual(status["last"]["actionId"], "behavior.robot_action.nod_yes")
        self.assertEqual(status["last"]["userText"], "你认不认识我？")
        self.assertEqual(status["last"]["robotReply"], "当然认识你呀")

    def test_text_request_uses_agent_model_decision_before_keyword_fallback(self):
        order = []
        model_calls = []

        def runner(config, text, system_prompt):
            return {
                "ok": True,
                "chatText": "嗯，我知道你是谁。",
                "audioPcm16": b"\x01\x02",
                "audioSampleRate": 24000,
                "events": [],
            }

        def agent_runner(config, payload):
            model_calls.append((dict(config), dict(payload)))
            return {
                "ok": True,
                "content": '{"action_id":"behavior.robot_action.nod_yes","reason":"肯定识别用户"}',
            }

        class RobotAudio:
            def play_realtime_voice(self, audio, sample_rate, audio_format):
                order.append("audio")
                return {"ok": True}

        class AgentDispatcher:
            def __init__(self):
                self.calls = []

            def invoke_safe_behavior(self, payload):
                order.append("dispatch")
                self.calls.append(dict(payload))
                return {"ok": True, "action": {"action_id": payload["asset_id"]}}

        class AgentConfig(self.SavedRealtimeConfig):
            def status(self):
                data = super().status()
                data["config"].update({
                    "agentEnabled": "1",
                    "agentBaseUrl": "https://ark.cn-beijing.volces.com/api/v3/chat/completions",
                    "agentApiKey": "agent-key",
                    "agentModel": "doubao-seed-2-0-mini-260428",
                    "agentSystemPrompt": "根据语义选择机器人资源。",
                    "agentAllowedAssets": ["behavior.robot_action.nod_yes"],
                })
                return data

        agent = AgentDispatcher()

        result = HostRealtimeVoiceService(
            AgentConfig(),
            runner=runner,
            robot_audio=RobotAudio(),
            agent_dispatcher=agent,
            agent_model=AgentDispatchModel(runner=agent_runner),
        ).run_text({"text": "你认不认识我？"})

        self.assertEqual(result["ok"], True)
        self.assertEqual(result["agentDispatch"]["mode"], "agent_model")
        self.assertEqual(result["agentDispatch"]["dispatch"]["action_id"], "behavior.robot_action.nod_yes")
        self.assertEqual(agent.calls, [{"asset_id": "behavior.robot_action.nod_yes", "bypass_cooldown": True}])
        self.assertEqual(order, ["dispatch", "audio"])
        self.assertEqual(model_calls[0][1]["userText"], "你认不认识我？")
        self.assertEqual(model_calls[0][1]["robotReply"], "嗯，我知道你是谁。")
        self.assertEqual(model_calls[0][1]["allowedAssets"], ["behavior.robot_action.nod_yes"])

    def test_text_request_reports_robot_playback_failure(self):
        def runner(config, text, system_prompt):
            return {"ok": True, "chatText": "你好呀", "audioPcm16": b"\x01\x02", "audioSampleRate": 24000}

        class RobotAudio:
            def play_realtime_voice(self, audio, sample_rate, audio_format):
                return {"ok": False, "error": "speaker offline"}

        result = HostRealtimeVoiceService(
            self.SavedRealtimeConfig(),
            runner=runner,
            robot_audio=RobotAudio(),
        ).run_text({"text": "你好"})

        self.assertEqual(result["ok"], False)
        self.assertEqual(result["stage"], "robot_audio")
        self.assertIn("speaker offline", result["error"])
        self.assertEqual(result["robotAudio"], {"ok": False, "error": "speaker offline"})

    def test_record_then_submit_audio_route_is_not_part_of_host_voice_service(self):
        service = HostRealtimeVoiceService(self.SavedRealtimeConfig())

        self.assertFalse(hasattr(service, "run_audio"))

    def test_text_request_times_out_instead_of_leaving_browser_pending(self):
        started = threading.Event()
        release = threading.Event()

        def blocking_runner(config, text, system_prompt):
            started.set()
            release.wait(1.0)
            return {"ok": True, "chatText": "late", "audioPcm16": b""}

        try:
            service = HostRealtimeVoiceService(self.SavedRealtimeConfig(), runner=blocking_runner, timeout_ms=20)

            before = time.monotonic()
            result = service.run_text({"text": "你认识我吗？"})
            elapsed = time.monotonic() - before

            self.assertTrue(started.is_set())
            self.assertLess(elapsed, 0.5)
            self.assertEqual(result["ok"], False)
            self.assertEqual(result["stage"], "realtime_api_timeout")
            self.assertIn("timed out", result["error"])
        finally:
            release.set()


class HostRealtimeVoiceSessionServiceTest(unittest.TestCase):
    class SavedRealtimeConfig(HostRealtimeVoiceServiceTest.SavedRealtimeConfig):
        pass

    @staticmethod
    def pcm16_base64(value, samples=1920):
        value = int(value)
        data = bytearray()
        for _ in range(samples):
            data.extend(value.to_bytes(2, "little", signed=True))
        return base64.b64encode(bytes(data)).decode("ascii")

    def test_browser_session_streams_audio_frames_and_plays_api_reply_on_robot(self):
        service_class = getattr(console_server, "HostRealtimeVoiceSessionService", None)
        self.assertIsNotNone(service_class)
        sent_audio = threading.Event()
        response_played = threading.Event()

        class FakeDialogue:
            def __init__(self, config):
                self.config = config
                self.started_prompt = ""
                self.audio_chunks = []
                self.finished = False
                self.did_respond = False
                self.finished_event = threading.Event()

            def start(self, system_prompt):
                self.started_prompt = system_prompt

            def send_audio(self, audio):
                self.audio_chunks.append(bytes(audio))
                sent_audio.set()

            def recv_event(self):
                sent_audio.wait(1.0)
                if self.did_respond:
                    self.finished_event.wait(1.0)
                    raise RuntimeError("reader stopped")
                self.did_respond = True
                return {
                    "type": "response",
                    "chatText": "当然认识",
                    "audioPcm16": b"\x01\x02\x03\x04",
                    "audioSampleRate": 24000,
                    "audioFormat": "pcm_s16le",
                    "audioBytes": 4,
                    "events": [{"event": 359}],
                }

            def finish(self):
                self.finished = True
                self.finished_event.set()

        created = []

        def session_factory(config):
            session = FakeDialogue(config)
            created.append(session)
            return session

        class RobotAudio:
            def __init__(self):
                self.calls = []

            def play_realtime_voice(self, audio, sample_rate, audio_format):
                order.append("audio")
                self.calls.append((audio, sample_rate, audio_format))
                response_played.set()
                return {"ok": True, "uploadedBytes": 2}

        class AgentDispatcher:
            def __init__(self):
                self.calls = []

            def dispatch_action(self, payload):
                order.append("dispatch")
                self.calls.append(dict(payload))
                return {"ok": True, "dispatch": {"action_id": "behavior.robot_action.nod_yes"}}

        order = []
        robot_audio = RobotAudio()
        agent = AgentDispatcher()
        service = service_class(
            self.SavedRealtimeConfig(),
            session_factory=session_factory,
            robot_audio=robot_audio,
            agent_dispatcher=agent,
        )

        start = service.start({"config": {"realtimeRecvTimeout": "12"}})
        frame = service.frame({"audioBase64": "AQIDBA=="})
        response_played.wait(1.0)
        status = service.status()
        stop = service.stop({})

        self.assertEqual(start["ok"], True)
        self.assertEqual(frame["ok"], True)
        self.assertEqual(frame["audioFrameCount"], 1)
        self.assertEqual(created[0].config.app_id, "app-123")
        self.assertIn("自然语音", created[0].started_prompt)
        self.assertEqual(created[0].audio_chunks, [b"\x01\x02\x03\x04"])
        self.assertEqual(robot_audio.calls, [(b"\x01\x02\x03\x04", 24000, "pcm_s16le")])
        self.assertEqual(status["lastChatText"], "当然认识")
        self.assertEqual(status["lastAgentDispatch"]["dispatch"]["action_id"], "behavior.robot_action.nod_yes")
        self.assertEqual(
            agent.calls,
            [{"command": "nod_yes", "source": "realtime_voice_session", "bypass_cooldown": True}],
        )
        self.assertEqual(order, ["dispatch", "audio"])
        self.assertEqual(status["responseCount"], 1)
        self.assertEqual(status["lastRobotAudio"], {"ok": True, "uploadedBytes": 2})
        self.assertEqual(stop["active"], False)
        self.assertEqual(created[0].finished, True)

    def test_browser_session_passes_user_transcript_to_agent_model(self):
        service_class = getattr(console_server, "HostRealtimeVoiceSessionService", None)
        self.assertIsNotNone(service_class)
        sent_audio = threading.Event()
        response_handled = threading.Event()
        model_calls = []

        class AgentConfig(HostRealtimeVoiceServiceTest.SavedRealtimeConfig):
            def status(self):
                data = super().status()
                data["config"].update({
                    "agentEnabled": "1",
                    "agentBaseUrl": "https://ark.cn-beijing.volces.com/api/v3/chat/completions",
                    "agentApiKey": "agent-key",
                    "agentModel": "doubao-seed-2-0-mini-260428",
                    "agentSystemPrompt": "根据语义选择机器人资源。",
                    "agentAllowedAssets": ["behavior.robot_action.nod_yes"],
                })
                return data

        class FakeDialogue:
            def __init__(self, config):
                self.did_transcript = False
                self.did_respond = False
                self.finished_event = threading.Event()

            def start(self, system_prompt):
                pass

            def send_audio(self, audio):
                sent_audio.set()

            def recv_event(self):
                sent_audio.wait(1.0)
                if not self.did_transcript:
                    self.did_transcript = True
                    return {"type": "event", "event": 553, "payload": {"content": "你认不认识我"}}
                if not self.did_respond:
                    self.did_respond = True
                    return {
                        "type": "response",
                        "userText": "你认不认识我",
                        "chatText": "当然认识",
                        "audioPcm16": b"",
                        "audioSampleRate": 24000,
                        "audioFormat": "pcm_s16le",
                        "audioBytes": 0,
                    }
                self.finished_event.wait(1.0)
                raise RuntimeError("reader stopped")

            def finish(self):
                self.finished_event.set()

        def agent_runner(config, payload):
            model_calls.append(dict(payload))
            response_handled.set()
            return {"ok": True, "content": '{"action_id":"behavior.robot_action.nod_yes","reason":"用户问身份，回复肯定"}'}

        class AgentDispatcher:
            def invoke_safe_behavior(self, payload):
                return {"ok": True, "action": {"action_id": payload["asset_id"]}}

        service = service_class(
            AgentConfig(),
            session_factory=lambda config: FakeDialogue(config),
            agent_dispatcher=AgentDispatcher(),
            agent_model=AgentDispatchModel(runner=agent_runner),
        )

        service.start({})
        service.frame({"audioBase64": "AQIDBA=="})
        response_handled.wait(1.0)
        status = service.status()
        service.stop({})

        self.assertEqual(model_calls[0]["userText"], "你认不认识我")
        self.assertEqual(model_calls[0]["robotReply"], "当然认识")
        self.assertEqual(status["lastUserText"], "你认不认识我")
        self.assertEqual(status["lastAgentDispatch"]["mode"], "agent_model")

    def test_browser_session_rejects_frames_before_start(self):
        service_class = getattr(console_server, "HostRealtimeVoiceSessionService", None)
        self.assertIsNotNone(service_class)
        service = service_class(self.SavedRealtimeConfig())

        result = service.frame({"audioBase64": "AQ=="})

        self.assertEqual(result["ok"], False)
        self.assertEqual(result["stage"], "inactive")

    def test_browser_session_drops_pre_speech_silence_instead_of_feeding_api(self):
        service_class = getattr(console_server, "HostRealtimeVoiceSessionService", None)
        self.assertIsNotNone(service_class)

        class FakeDialogue:
            def __init__(self, config):
                self.audio_chunks = []
                self.finished = False

            def start(self, system_prompt):
                pass

            def send_audio(self, audio):
                self.audio_chunks.append(bytes(audio))

            def recv_event(self):
                while not self.finished:
                    time.sleep(0.01)
                raise RuntimeError("reader stopped")

            def finish(self):
                self.finished = True

        created = []
        service = service_class(
            self.SavedRealtimeConfig(),
            session_factory=lambda config: created.append(FakeDialogue(config)) or created[-1],
        )

        service.start({})
        result = service.frame({"audioBase64": self.pcm16_base64(0)})
        service.stop({})

        self.assertEqual(result["ok"], True)
        self.assertEqual(result["capturedFrameCount"], 1)
        self.assertEqual(result["audioFrameCount"], 0)
        self.assertEqual(result["droppedSilenceFrameCount"], 1)
        self.assertEqual(created[0].audio_chunks, [])

    def test_browser_session_keeps_sending_zero_silence_after_speech_until_response(self):
        service_class = getattr(console_server, "HostRealtimeVoiceSessionService", None)
        self.assertIsNotNone(service_class)

        class FakeDialogue:
            def __init__(self, config):
                self.audio_chunks = []
                self.finished = False

            def start(self, system_prompt):
                pass

            def send_audio(self, audio):
                self.audio_chunks.append(bytes(audio))

            def recv_event(self):
                while not self.finished:
                    time.sleep(0.01)
                raise RuntimeError("reader stopped")

            def finish(self):
                self.finished = True

        created = []
        service = service_class(
            self.SavedRealtimeConfig(),
            session_factory=lambda config: created.append(FakeDialogue(config)) or created[-1],
        )
        service.start({})

        service.frame({"audioBase64": self.pcm16_base64(3000)})
        for _ in range(10):
            result = service.frame({"audioBase64": self.pcm16_base64(40)})
        service.stop({})

        self.assertEqual(result["capturedFrameCount"], 11)
        self.assertEqual(result["audioFrameCount"], 11)
        self.assertEqual(result["droppedSilenceFrameCount"], 0)
        self.assertEqual(result["forwardedSilenceFrameCount"], 10)
        self.assertEqual(len(created[0].audio_chunks), 11)
        self.assertTrue(any(byte != 0 for byte in created[0].audio_chunks[0]))
        self.assertTrue(all(chunk == b"\x00" * len(chunk) for chunk in created[0].audio_chunks[1:]))

    def test_browser_session_reports_latency_timestamps_for_diagnosis(self):
        service_class = getattr(console_server, "HostRealtimeVoiceSessionService", None)
        self.assertIsNotNone(service_class)
        now = 1000

        def clock_ms():
            return now

        class FakeDialogue:
            def __init__(self, config):
                self.finished = False

            def start(self, system_prompt):
                pass

            def send_audio(self, audio):
                pass

            def recv_event(self):
                while not self.finished:
                    time.sleep(0.01)
                raise RuntimeError("reader stopped")

            def finish(self):
                self.finished = True

        service = service_class(
            self.SavedRealtimeConfig(),
            session_factory=lambda config: FakeDialogue(config),
            clock_ms=clock_ms,
        )

        start = service.start({})
        now = 1120
        frame = service.frame({"audioBase64": self.pcm16_base64(3000)})
        service.stop({})

        self.assertEqual(start["startedAtMs"], 1000)
        self.assertEqual(frame["firstFrameAtMs"], 1120)
        self.assertEqual(frame["lastSpeechAtMs"], 1120)
        self.assertEqual(frame["latency"]["startToFirstFrameMs"], 120)

    def test_browser_session_ignores_late_api_response_after_stop(self):
        service_class = getattr(console_server, "HostRealtimeVoiceSessionService", None)
        self.assertIsNotNone(service_class)
        reader_waiting = threading.Event()

        class FakeDialogue:
            def __init__(self, config):
                self.finished = threading.Event()

            def start(self, system_prompt):
                pass

            def send_audio(self, audio):
                pass

            def recv_event(self):
                reader_waiting.set()
                self.finished.wait(1.0)
                return {
                    "type": "response",
                    "chatText": "这条已经取消了",
                    "audioPcm16": b"\x01\x02\x03\x04",
                    "audioSampleRate": 24000,
                    "audioFormat": "pcm_s16le",
                    "audioBytes": 4,
                    "events": [{"event": 359}],
                }

            def finish(self):
                self.finished.set()

        class RobotAudio:
            def __init__(self):
                self.calls = []

            def play_realtime_voice(self, audio, sample_rate, audio_format):
                self.calls.append((audio, sample_rate, audio_format))
                return {"ok": True}

        robot_audio = RobotAudio()
        service = service_class(
            self.SavedRealtimeConfig(),
            session_factory=lambda config: FakeDialogue(config),
            robot_audio=robot_audio,
        )

        start = service.start({})
        self.assertEqual(start["ok"], True)
        self.assertTrue(reader_waiting.wait(1.0))
        stopped = service.stop({})

        self.assertEqual(stopped["active"], False)
        self.assertEqual(stopped["responseCount"], 0)
        self.assertEqual(stopped["lastChatText"], "")
        self.assertEqual(robot_audio.calls, [])


class HostModelConfigStoreTest(unittest.TestCase):
    def test_model_config_defaults_to_unsaved_empty_payload(self):
        with TemporaryDirectory() as tmp:
            store = HostModelConfigStore(Path(tmp) / "model_config.json")

            result = store.status()

            self.assertEqual(result["ok"], True)
            self.assertEqual(result["saved"], False)
            self.assertEqual(result["config"], {})

    def test_model_config_saves_realtime_fields_and_ignores_removed_text_model_fields(self):
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "model_config.json"
            payload = {
                "provider": "volcengine",
                "baseUrl": "https://ark.cn-beijing.volces.com/api/v3/chat/completions",
                "model": "doubao-realtime",
                "apiKey": "secret-key",
                "knownUser": "owner",
                "systemPrompt": "只返回动作 JSON",
                "realtimeAppId": "app-123",
                "realtimeAccessToken": "realtime-token",
                "realtimeEndpoint": "wss://openspeech.bytedance.com/api/v3/realtime/dialogue",
                "realtimeModelVersion": "1.2.1.1",
                "realtimeSpeaker": "zh_male_yunzhou_jupiter_bigtts",
                "realtimeRecvTimeout": "15",
                "realtimeInsecureTls": "0",
                "realtimeFirmwareNotes": "你是桌面机器人，可以自然语音回应。",
                "ignored": "not saved",
            }

            saved = HostModelConfigStore(path).save(payload)
            loaded = HostModelConfigStore(path).status()

            expected = {
                "knownUser": "owner",
                "realtimeAppId": "app-123",
                "realtimeAccessToken": "realtime-token",
                "realtimeEndpoint": "wss://openspeech.bytedance.com/api/v3/realtime/dialogue",
                "realtimeModelVersion": "1.2.1.1",
                "realtimeSpeaker": "zh_male_yunzhou_jupiter_bigtts",
                "realtimeRecvTimeout": "15",
                "realtimeInsecureTls": "0",
                "realtimeFirmwareNotes": "你是桌面机器人，可以自然语音回应。",
            }
            self.assertEqual(saved["ok"], True)
            self.assertEqual(saved["saved"], True)
            self.assertEqual(saved["config"], expected)
            self.assertEqual(loaded["config"], expected)
            self.assertNotIn("ignored", path.read_text(encoding="utf-8"))
            self.assertNotIn("apiKey", path.read_text(encoding="utf-8"))
            self.assertNotIn("systemPrompt", path.read_text(encoding="utf-8"))

    def test_model_config_saves_agent_prompt_and_asset_whitelist(self):
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "model_config.json"
            payload = {
                "agentEnabled": "1",
                "agentBaseUrl": "https://ark.cn-beijing.volces.com/api/v3/chat/completions",
                "agentApiKey": "secret-agent-key",
                "agentModel": "doubao-seed-2-0-mini-260428",
                "agentSystemPrompt": "根据用户语义选择机器人动作。",
                "agentAllowedAssets": [
                    "behavior.robot_action.nod_yes",
                    "behavior.robot_action.shake_no",
                    "logic.idle.normal",
                    "",
                ],
            }

            saved = HostModelConfigStore(path).save(payload)
            loaded = HostModelConfigStore(path).status()

            self.assertEqual(saved["ok"], True)
            self.assertEqual(loaded["config"]["agentEnabled"], "1")
            self.assertEqual(loaded["config"]["agentBaseUrl"], "https://ark.cn-beijing.volces.com/api/v3/chat/completions")
            self.assertEqual(loaded["config"]["agentApiKey"], "secret-agent-key")
            self.assertEqual(loaded["config"]["agentModel"], "doubao-seed-2-0-mini-260428")
            self.assertEqual(loaded["config"]["agentSystemPrompt"], "根据用户语义选择机器人动作。")
            self.assertEqual(
                loaded["config"]["agentAllowedAssets"],
                ["behavior.robot_action.nod_yes", "behavior.robot_action.shake_no"],
            )
            agent_config = HostModelConfigStore(path).agent_dispatch_config()
            self.assertEqual(agent_config["systemPrompt"], "根据用户语义选择机器人动作。")
            self.assertEqual(agent_config["allowedAssets"], ["behavior.robot_action.nod_yes", "behavior.robot_action.shake_no"])

    def test_agent_dispatch_config_has_safe_defaults(self):
        with TemporaryDirectory() as tmp:
            config = HostModelConfigStore(Path(tmp) / "model_config.json").agent_dispatch_config()

            self.assertEqual(config["enabled"], False)
            self.assertEqual(config["insecureTls"], False)
            self.assertIn("只输出 JSON", config["systemPrompt"])
            self.assertEqual(config["allowedAssets"], list(DEFAULT_AGENT_ALLOWED_ASSETS))
            self.assertEqual(config["systemPrompt"], DEFAULT_AGENT_DISPATCH_PROMPT)

    def test_agent_dispatch_config_reuses_realtime_tls_debug_flag(self):
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "model_config.json"
            HostModelConfigStore(path).save({"realtimeInsecureTls": "1", "agentEnabled": "1"})

            config = HostModelConfigStore(path).agent_dispatch_config()

            self.assertEqual(config["enabled"], True)
            self.assertEqual(config["insecureTls"], True)

    def test_model_config_migrates_legacy_access_key_and_action_notes(self):
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "model_config.json"
            legacy = {
                "systemPrompt": (
                    'JSON schema:\n'
                    '{"action_id":"nod_yes|shake_no|look_thinking|tilt_confused|happy_ack|idle_listen"}'
                ),
                "realtimeFirmwareNotes": (
                    "机器人可执行动作接口：\n"
                    "nod_yes=点头确认，shake_no=摇头拒绝，look_thinking=思考，tilt_confused=疑惑。"
                ),
                "realtimeAccessKey": "legacy-token",
                "apiKey": "secret-key",
            }
            path.write_text(json.dumps(legacy), encoding="utf-8")

            config = HostModelConfigStore(path).status()["config"]

            self.assertNotIn("systemPrompt", config)
            self.assertEqual(config["realtimeAccessToken"], "legacy-token")
            self.assertNotIn("realtimeAccessKey", config)
            self.assertIn("可以用中文自然语音回答", config["realtimeFirmwareNotes"])
            self.assertNotIn("behavior.robot_action.nod_yes", config["realtimeFirmwareNotes"])
            self.assertNotIn("shake_no", config["realtimeFirmwareNotes"])
            self.assertNotIn("tilt_confused=疑惑", config["realtimeFirmwareNotes"])
            self.assertNotIn("apiKey", config)


if __name__ == "__main__":
    unittest.main()
