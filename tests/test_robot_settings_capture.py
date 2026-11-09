import unittest

from tools.capture_robot_settings import RobotSettingsCapture


class FakeTransport:
    def __init__(self):
        self.gets = []
        self.posts = []

    def get_json(self, path):
        self.gets.append(path)
        if path == "/api/status":
            return {"ok": True, "face": {"mood": "neutral", "eyeColor": "#00ff62"}}
        return {"ok": True, "path": path}

    def post_json(self, path, payload):
        self.posts.append((path, dict(payload)))
        return {"ok": True, "face": {"mood": payload.get("mood", "neutral")}}


class MissingOptionalVoiceNodTransport(FakeTransport):
    def get_json(self, path):
        if path == "/api/voice-nod/status":
            self.gets.append(path)
            raise RuntimeError("HTTP Error 404: Not Found")
        return super().get_json(path)


class FaceSetResponseTransport(FakeTransport):
    def get_json(self, path):
        self.gets.append(path)
        if path == "/api/status":
            return {"ok": True, "face": {"mood": "neutral"}}
        return {"ok": True, "path": path}

    def post_json(self, path, payload):
        self.posts.append((path, dict(payload)))
        mood = payload.get("mood", "neutral")
        return {
            "mood": mood,
            "eyeWidthScale": 0.35 if mood == "happy" else 1.28,
            "eyeHeightScale": 1.33 if mood == "happy" else 1.19,
            "cornerRadius": 53 if mood == "happy" else 36,
            "eyeOffsetX": 0,
            "eyeOffsetY": -51 if mood == "happy" else -35,
            "eyeSpacing": 105 if mood == "happy" else 112,
            "eyeColor": "#00ff62",
            "glowEnabled": False,
            "glowStrength": 3,
            "scanlinesEnabled": True,
            "scanlineSpacing": 5,
            "scanlineAlpha": 20,
        }


class RobotSettingsCaptureTest(unittest.TestCase):
    def test_capture_reads_required_endpoints_and_restores_original_mood(self):
        transport = FakeTransport()
        capture = RobotSettingsCapture(
            "http://robot.local",
            transport=transport,
            now=lambda: "2026-06-30T18:40:00+0800",
        )

        payload = capture.capture()

        self.assertEqual("http://robot.local", payload["source"])
        self.assertIn("/api/triggers/status", transport.gets)
        self.assertIn("/api/voice-nod/status", transport.gets)
        self.assertIn("/api/face/library", transport.gets)
        self.assertEqual({"ok": True, "path": "/api/face/library"}, payload["faceLibrary"])
        self.assertEqual({"ok": True, "path": "/api/voice-nod/status"}, payload["voiceNod"])
        self.assertIn("neutral", payload["moodStyles"])
        self.assertIn("sunglasses", payload["moodStyles"])
        self.assertEqual(("/api/face/set", {"mood": "neutral"}), transport.posts[-1])

    def test_capture_records_optional_endpoint_errors_and_continues(self):
        transport = MissingOptionalVoiceNodTransport()
        capture = RobotSettingsCapture(
            "http://robot.local",
            transport=transport,
            now=lambda: "2026-06-30T18:40:00+0800",
        )

        payload = capture.capture()

        self.assertIn("/api/voice-nod/status", transport.gets)
        self.assertNotIn("voiceNod", payload)
        self.assertEqual(
            {
                "path": "/api/voice-nod/status",
                "error": "HTTP Error 404: Not Found",
            },
            payload["endpointErrors"]["voiceNod"],
        )
        self.assertIn("neutral", payload["moodStyles"])

    def test_capture_uses_face_set_response_for_mood_styles(self):
        transport = FaceSetResponseTransport()
        capture = RobotSettingsCapture(
            "http://robot.local",
            transport=transport,
            now=lambda: "2026-06-30T18:40:00+0800",
        )

        payload = capture.capture()

        self.assertEqual(-51, payload["moodStyles"]["happy"]["eyeOffsetY"])
        self.assertEqual(105, payload["moodStyles"]["happy"]["eyeSpacing"])


if __name__ == "__main__":
    unittest.main()
