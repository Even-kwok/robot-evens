import os
import threading
import time
from dataclasses import dataclass
from pathlib import Path


_MODEL_URL = "https://storage.googleapis.com/mediapipe-models/face_detector/blaze_face_short_range/float16/latest/blaze_face_short_range.tflite"
DEFAULT_FACE_MODEL_PATH = Path(__file__).with_name("models") / "blaze_face_short_range.tflite"
HOST_VISION_INSTALL_HINT = (
    "python3 -m pip install mediapipe opencv-python numpy; "
    f"curl -L -o {DEFAULT_FACE_MODEL_PATH} {_MODEL_URL}"
)


def _clamp(value, low, high):
    return max(low, min(high, value))


class MissingVisionDependencyError(RuntimeError):
    pass


@dataclass(frozen=True)
class FaceBox:
    center_x: float
    center_y: float
    width: float
    height: float
    confidence: float

    def to_camera_report(self, scan_id=0, force=True):
        return {
            "scanId": int(scan_id),
            "faceDetected": True,
            "confidence": round(_clamp(float(self.confidence), 0.0, 1.0), 3),
            "faceCenterX": round(_clamp(float(self.center_x), 0.0, 1.0), 3),
            "faceCenterY": round(_clamp(float(self.center_y), 0.0, 1.0), 3),
            "faceWidth": round(_clamp(float(self.width), 0.0, 1.0), 3),
            "faceHeight": round(_clamp(float(self.height), 0.0, 1.0), 3),
            "force": bool(force),
        }


@dataclass(frozen=True)
class FaceDetection:
    face: FaceBox | None = None
    backend: str = "none"

    @property
    def face_detected(self):
        return self.face is not None


class MediaPipeFaceDetector:
    def __init__(self, min_confidence=0.55, model_selection=0, model_path=None):
        self.min_confidence = float(min_confidence)
        self.model_selection = int(model_selection)
        self.model_path = Path(model_path or DEFAULT_FACE_MODEL_PATH)
        self.backend = "mediapipe"
        self._mp = None
        self._cv2 = None
        self._np = None
        self._vision = None
        self._base_options = None
        self._mode = None

    def _load(self):
        if self._mp is not None:
            return
        try:
            os.environ.setdefault("MPLCONFIGDIR", "/private/tmp/matplotlib")
            import cv2
            import mediapipe as mp
            import numpy as np
        except ModuleNotFoundError as exc:
            raise MissingVisionDependencyError(str(exc)) from exc
        self._cv2 = cv2
        self._mp = mp
        self._np = np
        if hasattr(mp, "solutions"):
            self._mode = "solutions"
            self.backend = "mediapipe"
            return
        if not self.model_path.exists():
            raise MissingVisionDependencyError(
                f"MediaPipe face model missing: {self.model_path}. {HOST_VISION_INSTALL_HINT}"
            )
        try:
            from mediapipe.tasks import python as mp_tasks_python
            from mediapipe.tasks.python import vision
        except (ImportError, AttributeError) as exc:
            raise MissingVisionDependencyError(str(exc)) from exc
        self._base_options = mp_tasks_python.BaseOptions
        self._vision = vision
        self._mode = "tasks"
        self.backend = "mediapipe-tasks"

    def detect(self, frame_bytes):
        self._load()
        data = self._np.frombuffer(frame_bytes, dtype=self._np.uint8)
        image_bgr = self._cv2.imdecode(data, self._cv2.IMREAD_COLOR)
        if image_bgr is None:
            return FaceDetection(face=None, backend=self.backend)
        image_rgb = self._cv2.cvtColor(image_bgr, self._cv2.COLOR_BGR2RGB)

        if self._mode == "tasks":
            return self._detect_with_tasks(image_rgb)

        return self._detect_with_solutions(image_rgb)

    def _detect_with_tasks(self, image_rgb):
        mp_image = self._mp.Image(image_format=self._mp.ImageFormat.SRGB, data=image_rgb)
        options = self._vision.FaceDetectorOptions(
            base_options=self._base_options(model_asset_path=str(self.model_path)),
            min_detection_confidence=self.min_confidence,
        )
        with self._vision.FaceDetector.create_from_options(options) as detector:
            result = detector.detect(mp_image)
        detections = list(result.detections or [])
        if not detections:
            return FaceDetection(face=None, backend=self.backend)
        best = max(detections, key=lambda item: item.bounding_box.width * item.bounding_box.height)
        box = best.bounding_box
        image_h, image_w = image_rgb.shape[:2]
        score = best.categories[0].score if best.categories else self.min_confidence
        face = FaceBox(
            center_x=(box.origin_x + box.width * 0.5) / image_w,
            center_y=(box.origin_y + box.height * 0.5) / image_h,
            width=box.width / image_w,
            height=box.height / image_h,
            confidence=score,
        )
        return FaceDetection(face=face, backend=self.backend)

    def _detect_with_solutions(self, image_rgb):
        with self._mp.solutions.face_detection.FaceDetection(
            model_selection=self.model_selection,
            min_detection_confidence=self.min_confidence,
        ) as detector:
            result = detector.process(image_rgb)
        detections = list(result.detections or [])
        if not detections:
            return FaceDetection(face=None, backend=self.backend)
        best = max(detections, key=lambda item: item.location_data.relative_bounding_box.width * item.location_data.relative_bounding_box.height)
        box = best.location_data.relative_bounding_box
        score = best.score[0] if best.score else self.min_confidence
        face = FaceBox(
            center_x=box.xmin + box.width * 0.5,
            center_y=box.ymin + box.height * 0.5,
            width=box.width,
            height=box.height,
            confidence=score,
        )
        return FaceDetection(face=face, backend=self.backend)


class HostVisionTracker:
    def __init__(self, client, detector=None, sleep_fn=time.sleep):
        self.client = client
        self.detector = detector or MediaPipeFaceDetector()
        self.sleep_fn = sleep_fn
        self.active = False
        self.frame_count = 0
        self.face_count = 0
        self.last_result = None
        self.last_error = None
        self.last_face = None
        self.settings = {
            "intervalMs": 700,
            "minConfidence": 0.55,
            "disableFirmwareLocal": True,
            "debugSubtitle": False,
            "force": True,
            "reportNoFace": False,
            "timeout": 5,
        }
        self._thread = None
        self._lock = threading.Lock()

    def _update_settings(self, payload):
        payload = payload or {}
        with self._lock:
            if "intervalMs" in payload:
                self.settings["intervalMs"] = int(_clamp(int(payload["intervalMs"]), 150, 10000))
            if "minConfidence" in payload:
                min_confidence = _clamp(float(payload["minConfidence"]), 0.1, 0.95)
                self.settings["minConfidence"] = round(min_confidence, 2)
                if hasattr(self.detector, "min_confidence"):
                    self.detector.min_confidence = min_confidence
            for key in ("disableFirmwareLocal", "debugSubtitle", "force", "reportNoFace"):
                if key in payload:
                    self.settings[key] = bool(payload[key])
            if "timeout" in payload:
                self.settings["timeout"] = int(_clamp(int(payload["timeout"]), 1, 30))
            return dict(self.settings)

    def status(self):
        with self._lock:
            return {
                "ok": True,
                "active": self.active,
                "backend": getattr(self.detector, "backend", self.last_result.get("backend") if isinstance(self.last_result, dict) else "mediapipe"),
                "frameCount": self.frame_count,
                "faceCount": self.face_count,
                "lastResult": self.last_result,
                "lastError": self.last_error,
                "lastFace": self.last_face,
                "settings": dict(self.settings),
            }

    def start(self, payload=None):
        settings = self._update_settings(payload)
        already_active = False
        with self._lock:
            if self.active:
                already_active = True
            else:
                self.active = True
        if not already_active:
            self._thread = threading.Thread(target=self._run_loop, name="host_vision", daemon=True)
            self._thread.start()
        status = self.status()
        status["settings"] = settings
        return status

    def stop(self, payload=None):
        del payload
        with self._lock:
            self.active = False
        return self.status()

    def _run_loop(self):
        while True:
            with self._lock:
                if not self.active:
                    return
                interval_ms = self.settings["intervalMs"]
            self.step()
            self.sleep_fn(interval_ms / 1000.0)

    def step(self, payload=None):
        settings = self._update_settings(payload)
        try:
            if settings["disableFirmwareLocal"]:
                self.client.camera_set(enabled=False)
            frame_bytes = self.client.camera_frame_bytes(timeout=settings["timeout"])
            detection = self.detector.detect(frame_bytes)
            self.frame_count += 1
            if not detection.face_detected:
                result = {
                    "ok": True,
                    "active": self.active,
                    "backend": detection.backend,
                    "faceDetected": False,
                    "frameBytes": len(frame_bytes),
                    "frameCount": self.frame_count,
                }
                if settings["reportNoFace"]:
                    self.client.camera_face_report(0, False, confidence=0.0, force=settings["force"])
                self._record_result(result)
                return result

            face = detection.face
            report = face.to_camera_report(scan_id=0, force=settings["force"])
            if settings["debugSubtitle"]:
                self.client.debug_subtitle_show("apiResult", f"Host face {report['confidence']:.2f}")
            robot_result = self.client.camera_face_report(
                report["scanId"],
                report["faceDetected"],
                confidence=report["confidence"],
                face_center_x=report["faceCenterX"],
                face_center_y=report["faceCenterY"],
                face_width=report["faceWidth"],
                face_height=report["faceHeight"],
                force=report["force"],
            )
            self.face_count += 1
            result = {
                "ok": True,
                "active": self.active,
                "backend": detection.backend,
                "faceDetected": True,
                "face": report,
                "robot": robot_result,
                "frameBytes": len(frame_bytes),
                "frameCount": self.frame_count,
                "faceCount": self.face_count,
            }
            self._record_result(result)
            return result
        except MissingVisionDependencyError as exc:
            result = {
                "ok": False,
                "active": self.active,
                "error": str(exc),
                "installHint": HOST_VISION_INSTALL_HINT,
            }
            self._record_result(result, error=str(exc))
            return result
        except Exception as exc:
            result = {"ok": False, "active": self.active, "error": str(exc)}
            self._record_result(result, error=str(exc))
            return result

    def _record_result(self, result, error=None):
        with self._lock:
            self.last_result = result
            self.last_error = error
            self.last_face = result.get("face") if result.get("faceDetected") else None
