import unittest
from pathlib import Path

from host.host_vision import (
    DEFAULT_FACE_MODEL_PATH,
    HOST_VISION_INSTALL_HINT,
    FaceBox,
    FaceDetection,
    HostVisionTracker,
    MissingVisionDependencyError,
)


class FakeDetector:
    def __init__(self, detection):
        self.detection = detection
        self.frames = []

    def detect(self, frame_bytes):
        self.frames.append(frame_bytes)
        return self.detection


class MissingDetector:
    def detect(self, frame_bytes):
        raise MissingVisionDependencyError("mediapipe/opencv/numpy not installed")


class FakeRobotClient:
    def __init__(self):
        self.calls = []

    def camera_set(self, **kwargs):
        self.calls.append(("camera_set", kwargs))
        return {"ok": True, "path": "/api/camera/set", "payload": kwargs}

    def camera_frame_bytes(self, timeout=5):
        self.calls.append(("camera_frame_bytes", timeout))
        return b"BMfake"

    def camera_face_report(
        self,
        scan_id,
        face_detected,
        confidence=0.0,
        yaw=None,
        pitch=None,
        face_center_x=None,
        face_center_y=None,
        face_width=None,
        face_height=None,
        force=None,
    ):
        payload = {
            "scanId": scan_id,
            "faceDetected": face_detected,
            "confidence": confidence,
            "faceCenterX": face_center_x,
            "faceCenterY": face_center_y,
            "faceWidth": face_width,
            "faceHeight": face_height,
            "force": force,
        }
        self.calls.append(("camera_face_report", payload))
        return {"ok": True, "path": "/api/camera/face-report", "payload": payload}

    def debug_subtitle_show(self, kind, text):
        self.calls.append(("debug_subtitle_show", kind, text))
        return {"ok": True}


class HostVisionTrackerTest(unittest.TestCase):
    def test_face_box_builds_forced_camera_report_payload(self):
        face = FaceBox(center_x=0.42, center_y=0.58, width=0.31, height=0.27, confidence=0.91)

        payload = face.to_camera_report(scan_id=7, force=True)

        self.assertEqual(
            payload,
            {
                "scanId": 7,
                "faceDetected": True,
                "confidence": 0.91,
                "faceCenterX": 0.42,
                "faceCenterY": 0.58,
                "faceWidth": 0.31,
                "faceHeight": 0.27,
                "force": True,
            },
        )

    def test_step_fetches_robot_frame_detects_face_and_forces_firmware_report(self):
        client = FakeRobotClient()
        detector = FakeDetector(FaceDetection(face=FaceBox(0.55, 0.48, 0.24, 0.32, 0.86), backend="fake"))
        tracker = HostVisionTracker(client, detector=detector)

        result = tracker.step({"disableFirmwareLocal": True, "debugSubtitle": True})

        self.assertEqual(result["ok"], True)
        self.assertEqual(result["faceDetected"], True)
        self.assertEqual(result["backend"], "fake")
        self.assertEqual(detector.frames, [b"BMfake"])
        self.assertEqual(client.calls[0], ("camera_set", {"enabled": False}))
        self.assertEqual(client.calls[1], ("camera_frame_bytes", 5))
        self.assertEqual(client.calls[2], ("debug_subtitle_show", "apiResult", "Host face 0.86"))
        self.assertEqual(
            client.calls[3],
            (
                "camera_face_report",
                {
                    "scanId": 0,
                    "faceDetected": True,
                    "confidence": 0.86,
                    "faceCenterX": 0.55,
                    "faceCenterY": 0.48,
                    "faceWidth": 0.24,
                    "faceHeight": 0.32,
                    "force": True,
                },
            ),
        )

    def test_debug_subtitle_is_off_by_default_so_halo_remains_visible(self):
        client = FakeRobotClient()
        detector = FakeDetector(FaceDetection(face=FaceBox(0.55, 0.48, 0.24, 0.32, 0.86), backend="fake"))
        tracker = HostVisionTracker(client, detector=detector)

        result = tracker.step({"disableFirmwareLocal": True})

        self.assertEqual(result["ok"], True)
        self.assertEqual(result["faceDetected"], True)
        self.assertNotIn("debug_subtitle_show", [call[0] for call in client.calls])
        self.assertEqual(client.calls[2][0], "camera_face_report")
        self.assertEqual(tracker.status()["settings"]["debugSubtitle"], False)

    def test_no_face_step_does_not_force_no_face_report_by_default(self):
        client = FakeRobotClient()
        detector = FakeDetector(FaceDetection(face=None, backend="fake"))
        tracker = HostVisionTracker(client, detector=detector)

        result = tracker.step({"disableFirmwareLocal": False})

        self.assertEqual(result["ok"], True)
        self.assertEqual(result["faceDetected"], False)
        self.assertEqual(client.calls, [("camera_frame_bytes", 5)])

    def test_missing_mediapipe_dependency_reports_install_hint(self):
        client = FakeRobotClient()
        tracker = HostVisionTracker(client, detector=MissingDetector())

        result = tracker.step()

        self.assertEqual(result["ok"], False)
        self.assertIn("mediapipe", result["error"])
        self.assertIn("pip install", result["installHint"])

    def test_mediapipe_tasks_model_path_is_documented_in_detector(self):
        source = (Path(__file__).resolve().parents[1] / "host" / "host_vision.py").read_text(encoding="utf-8")

        self.assertEqual(DEFAULT_FACE_MODEL_PATH.name, "blaze_face_short_range.tflite")
        self.assertIn("blaze_face_short_range.tflite", HOST_VISION_INSTALL_HINT)
        for snippet in [
            "mediapipe.tasks",
            "FaceDetectorOptions",
            "model_asset_path",
            "_detect_with_tasks",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)


if __name__ == "__main__":
    unittest.main()
