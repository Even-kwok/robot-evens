import unittest

from host.robot_client import RobotClient


class RobotClientTest(unittest.TestCase):
    def test_face_set_payload(self):
        client = RobotClient("http://deskbot.local")
        path, payload = client.build_face_set("happy", "#00f2ff")
        self.assertEqual(path, "/api/face/set")
        self.assertEqual(payload, {"mood": "happy", "color": "#00f2ff"})

    def test_motion_pose_payload_is_clamped(self):
        client = RobotClient("http://deskbot.local")
        path, payload = client.build_motion_pose(2000, -20, 1200)
        self.assertEqual(path, "/api/motion/pose")
        self.assertEqual(payload, {"yaw": 1280, "pitch": 0, "speed": 1000})

    def test_react_payload_is_clamped(self):
        client = RobotClient("http://deskbot.local")
        path, payload = client.build_face_react(2.0, -2.0, 1.5)
        self.assertEqual(path, "/api/face/react")
        self.assertEqual(payload, {"lookX": 1.0, "lookY": -1.0, "audioLevel": 1.0})

    def test_emotion_play_payload_uses_firmware_action_asset_name(self):
        client = RobotClient("http://deskbot.local")
        path, payload = client.build_emotion_play("nod_yes", lights=True, sound=False)
        self.assertEqual(path, "/api/emotion/play")
        self.assertEqual(payload, {"name": "nod_yes", "lights": True, "sound": False})

    def test_rgb_payload_supports_optional_brightness_and_breathing(self):
        client = RobotClient("http://deskbot.local")
        path, payload = client.build_rgb("#00ff66", index=99, brightness=300, breathing=True)
        self.assertEqual(path, "/api/rgb")
        self.assertEqual(payload, {"color": "#00ff66", "index": 11, "brightness": 255, "breathing": True})

    def test_rgb_payload_supports_manual_effects_and_presets(self):
        client = RobotClient("http://deskbot.local")
        path, payload = client.build_rgb(
            "#00ff66",
            marquee=True,
            rainbow=False,
            preset="rainbowMarquee",
        )
        self.assertEqual(path, "/api/rgb")
        self.assertEqual(
            payload,
            {
                "color": "#00ff66",
                "marquee": True,
                "rainbow": False,
                "preset": "rainbowMarquee",
            },
        )

    def test_camera_set_payload_uses_camera_api_names(self):
        client = RobotClient("http://deskbot.local")
        path, payload = client.build_camera_set(
            enabled=True,
            capture_interval_ms=500,
            detect_timeout_ms=100,
            return_settle_ms=40,
            confirm_settle_ms=5000,
            lock_hold_ms=700,
            cooldown_ms=1000,
            center_dead_zone_x=0.001,
            center_dead_zone_y=0.9,
            yaw_step_max=999,
            pitch_step_max=0,
            catchphrase_asset_id="halo",
            listen_after_catchphrase=True,
        )
        self.assertEqual(path, "/api/camera/set")
        self.assertEqual(
            payload,
            {
                "enabled": True,
                "captureIntervalMs": 3000,
                "detectTimeoutMs": 300,
                "returnSettleMs": 80,
                "confirmSettleMs": 2000,
                "lockHoldMs": 1000,
                "cooldownMs": 3000,
                "centerDeadZoneX": 0.01,
                "centerDeadZoneY": 0.45,
                "yawStepMax": 240,
                "pitchStepMax": 1,
                "catchphraseAssetId": "halo",
                "listenAfterCatchphrase": True,
            },
        )

    def test_camera_face_report_payload_requires_matching_pose_override(self):
        client = RobotClient("http://deskbot.local")
        path, payload = client.build_camera_face_report(
            scan_id=12,
            face_detected=True,
            confidence=1.4,
            yaw=2000,
            pitch=-10,
            face_center_x=2,
            face_center_y=-1,
            face_width=0.24,
            face_height=0.32,
            force=True,
        )
        self.assertEqual(path, "/api/camera/face-report")
        self.assertEqual(
            payload,
            {
                "scanId": 12,
                "faceDetected": True,
                "confidence": 1.0,
                "yaw": 1280,
                "pitch": 0,
                "faceCenterX": 1.0,
                "faceCenterY": 0.0,
                "faceWidth": 0.24,
                "faceHeight": 0.32,
                "force": True,
            },
        )
        with self.assertRaises(ValueError):
            client.build_camera_face_report(12, True, yaw=10)

    def test_camera_frame_fetch_uses_preview_bmp_route(self):
        source = RobotClient("http://deskbot.local")
        self.assertIn("/api/camera/frame.bmp", source.camera_frame_path())

    def test_camera_catchphrase_command_uses_camera_catchphrase_route(self):
        client = RobotClient("http://deskbot.local")
        path, payload = client.build_camera_catchphrase("halo", listen_after=True)
        self.assertEqual(path, "/api/camera/catchphrase")
        self.assertEqual(payload, {"assetId": "halo", "listenAfter": True})

    def test_camera_wake_and_clear_payloads(self):
        client = RobotClient("http://deskbot.local")
        self.assertEqual(client.build_camera_wake(60000), ("/api/camera/wake", {"holdMs": 30000}))
        self.assertEqual(client.build_camera_clear(), ("/api/camera/clear", {}))

    def test_debug_subtitle_payloads(self):
        client = RobotClient("http://deskbot.local")
        self.assertEqual(
            client.build_debug_subtitle_set(listening=True, api_result=False),
            ("/api/debug/subtitle/set", {"listening": True, "apiResult": False}),
        )
        self.assertEqual(
            client.build_debug_subtitle_show("apiResult", {"ok": True, "action_id": "nod_yes"}),
            ("/api/debug/subtitle/show", {"kind": "apiResult", "text": '{"ok":true,"action_id":"nod_yes"}'}),
        )

    def test_listening_mode_payloads(self):
        client = RobotClient("http://deskbot.local")
        self.assertEqual(
            client.build_listening_config(
                voice_wake_enabled=True,
                host_callback_url="http://192.168.0.2:8766/api/robot/wake-event",
                listen_timeout_ms=60000,
            ),
            (
                "/api/listening/config",
                {
                    "voiceWakeEnabled": True,
                    "hostCallbackUrl": "http://192.168.0.2:8766/api/robot/wake-event",
                    "listenTimeoutMs": 30000,
                },
            ),
        )
        self.assertEqual(client.build_listening_start(60000), ("/api/listening/start", {"listenTimeoutMs": 30000}))
        self.assertEqual(client.build_listening_stop(), ("/api/listening/stop", {}))
        self.assertEqual(client.build_listening_wake_test("panel"), ("/api/listening/wake-test", {"source": "panel"}))

    def test_mumble_play_payload_uses_full_mode_for_robot_speaker(self):
        client = RobotClient("http://deskbot.local")
        self.assertEqual(client.build_mumble_set(enabled=True), ("/api/mumble/set", {"enabled": True}))
        self.assertEqual(client.build_mumble_play(), ("/api/mumble/play", {"mode": "full"}))
        self.assertEqual(
            client.build_mumble_play("random", duration_ms=999999),
            ("/api/mumble/play", {"mode": "random", "durationMs": 30000}),
        )
        self.assertEqual(client.build_mumble_stop(), ("/api/mumble/stop", {}))


class RecordingRobotClient(RobotClient):
    def __init__(self):
        super().__init__("http://deskbot.local")
        self.get_paths = []
        self.post_calls = []
        self.multipart_calls = []

    def get(self, path):
        self.get_paths.append(path)
        return {"ok": True, "path": path}

    def post(self, path, payload, timeout=5):
        self.post_calls.append((path, payload))
        return {"ok": True, "path": path, "payload": payload}

    def post_multipart_file(self, path, field_name, filename, data, content_type="application/octet-stream", timeout=5):
        self.multipart_calls.append((path, field_name, filename, bytes(data), content_type))
        return {"ok": True, "path": path, "uploadedBytes": len(data)}


class RobotCatalogClientTest(unittest.TestCase):
    def test_catalog_helpers_use_read_only_catalog_paths(self):
        client = RecordingRobotClient()
        self.assertEqual(client.catalog()["path"], "/api/catalog")
        self.assertEqual(client.catalog_assets()["path"], "/api/catalog/assets")
        self.assertEqual(client.catalog_logic()["path"], "/api/catalog/logic")
        self.assertEqual(client.catalog_conflicts()["path"], "/api/catalog/conflicts")
        self.assertEqual(
            client.get_paths,
            [
                "/api/catalog",
                "/api/catalog/assets",
                "/api/catalog/logic",
                "/api/catalog/conflicts",
            ],
        )

    def test_asset_invoke_helper_uses_unified_asset_path(self):
        client = RecordingRobotClient()
        result = client.asset_invoke("expression.happy", duration_ms=900, speed=80, lights=False, sound=True)
        self.assertEqual(result["path"], "/api/asset/invoke")
        self.assertEqual(
            result["payload"],
            {
                "id": "expression.happy",
                "durationMs": 900,
                "speed": 80,
                "lights": False,
                "sound": True,
            },
        )
        self.assertEqual(client.post_calls, [("/api/asset/invoke", result["payload"])])

    def test_mumble_upload_and_play_helpers_use_firmware_routes(self):
        client = RecordingRobotClient()

        upload = client.mumble_upload(b"\x80\x81", filename="reply.raw")
        set_enabled = client.mumble_set(enabled=True)
        play = client.mumble_play()
        stop = client.mumble_stop()

        self.assertEqual(upload["path"], "/api/mumble/upload")
        self.assertEqual(set_enabled["path"], "/api/mumble/set")
        self.assertEqual(play["path"], "/api/mumble/play")
        self.assertEqual(stop["path"], "/api/mumble/stop")
        self.assertEqual(
            client.multipart_calls,
            [("/api/mumble/upload", "file", "reply.raw", b"\x80\x81", "application/octet-stream")],
        )
        self.assertEqual(
            client.post_calls,
            [
                ("/api/mumble/set", {"enabled": True}),
                ("/api/mumble/play", {"mode": "full"}),
                ("/api/mumble/stop", {}),
            ],
        )


if __name__ == "__main__":
    unittest.main()
