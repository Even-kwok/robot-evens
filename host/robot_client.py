#!/usr/bin/env python3
import argparse
import json
import urllib.request
import uuid


def clamp(value, low, high):
    return max(low, min(high, value))


class RobotClient:
    def __init__(self, host):
        self.host = host.rstrip("/")

    def build_face_set(self, mood, color):
        return "/api/face/set", {"mood": mood, "color": color}

    def build_face_react(self, look_x, look_y, audio_level):
        return "/api/face/react", {
            "lookX": round(clamp(float(look_x), -1.0, 1.0), 3),
            "lookY": round(clamp(float(look_y), -1.0, 1.0), 3),
            "audioLevel": round(clamp(float(audio_level), 0.0, 1.0), 3),
        }

    def build_motion_pose(self, yaw, pitch, speed):
        return "/api/motion/pose", {
            "yaw": int(clamp(int(yaw), -1280, 1280)),
            "pitch": int(clamp(int(pitch), 0, 900)),
            "speed": int(clamp(int(speed), 0, 1000)),
        }

    def build_motion_rotate(self, velocity):
        return "/api/motion/rotate", {
            "velocity": int(clamp(int(velocity), -1000, 1000)),
        }

    def build_rgb(self, color, index=None, brightness=None, breathing=None, marquee=None, rainbow=None, preset=None):
        payload = {"color": color}
        if index is not None:
            payload["index"] = int(clamp(int(index), 0, 11))
        if brightness is not None:
            payload["brightness"] = int(clamp(int(brightness), 0, 255))
        if breathing is not None:
            payload["breathing"] = bool(breathing)
        if marquee is not None:
            payload["marquee"] = bool(marquee)
        if rainbow is not None:
            payload["rainbow"] = bool(rainbow)
        if preset is not None:
            payload["preset"] = str(preset)
        return "/api/rgb", payload

    def build_emotion_play(self, name, lights=True, sound=True):
        return "/api/emotion/play", {
            "name": str(name),
            "lights": bool(lights),
            "sound": bool(sound),
        }

    def build_asset_invoke(self, asset_id, **kwargs):
        payload = {"id": str(asset_id)}
        field_names = {
            "duration_ms": "durationMs",
            "eye_color": "eyeColor",
        }
        for key, value in kwargs.items():
            if value is None:
                continue
            payload[field_names.get(key, key)] = value
        return "/api/asset/invoke", payload

    def build_camera_set(
        self,
        enabled=None,
        capture_interval_ms=None,
        detect_timeout_ms=None,
        return_settle_ms=None,
        confirm_settle_ms=None,
        lock_hold_ms=None,
        cooldown_ms=None,
        center_dead_zone_x=None,
        center_dead_zone_y=None,
        yaw_step_max=None,
        pitch_step_max=None,
        catchphrase_asset_id=None,
        listen_after_catchphrase=None,
        scan_enabled=None,
        scan_interval_min_ms=None,
        scan_interval_max_ms=None,
        capture_hold_ms=None,
        reaction=None,
        stare_min_ms=None,
        stare_max_ms=None,
        memory_ttl_ms=None,
    ):
        payload = {}
        if enabled is not None:
            payload["enabled"] = bool(enabled)
        if capture_interval_ms is None and scan_interval_min_ms is not None:
            capture_interval_ms = scan_interval_min_ms
        if detect_timeout_ms is None and capture_hold_ms is not None:
            detect_timeout_ms = capture_hold_ms
        if lock_hold_ms is None and stare_min_ms is not None:
            lock_hold_ms = stare_min_ms
        if cooldown_ms is None and memory_ttl_ms is not None:
            cooldown_ms = memory_ttl_ms
        if capture_interval_ms is not None:
            payload["captureIntervalMs"] = int(clamp(int(capture_interval_ms), 3000, 180000))
        if detect_timeout_ms is not None:
            payload["detectTimeoutMs"] = int(clamp(int(detect_timeout_ms), 300, 5000))
        if return_settle_ms is not None:
            payload["returnSettleMs"] = int(clamp(int(return_settle_ms), 80, 2000))
        if confirm_settle_ms is not None:
            payload["confirmSettleMs"] = int(clamp(int(confirm_settle_ms), 80, 2000))
        if lock_hold_ms is not None:
            payload["lockHoldMs"] = int(clamp(int(lock_hold_ms), 1000, 30000))
        if cooldown_ms is not None:
            payload["cooldownMs"] = int(clamp(int(cooldown_ms), 3000, 600000))
        if center_dead_zone_x is not None:
            payload["centerDeadZoneX"] = round(clamp(float(center_dead_zone_x), 0.01, 0.45), 3)
        if center_dead_zone_y is not None:
            payload["centerDeadZoneY"] = round(clamp(float(center_dead_zone_y), 0.01, 0.45), 3)
        if yaw_step_max is not None:
            payload["yawStepMax"] = int(clamp(int(yaw_step_max), 1, 240))
        if pitch_step_max is not None:
            payload["pitchStepMax"] = int(clamp(int(pitch_step_max), 1, 180))
        if catchphrase_asset_id is not None:
            payload["catchphraseAssetId"] = str(catchphrase_asset_id or "halo")
        if listen_after_catchphrase is not None:
            payload["listenAfterCatchphrase"] = bool(listen_after_catchphrase)
        if scan_enabled is not None or scan_interval_max_ms is not None or reaction is not None or stare_max_ms is not None:
            pass
        return "/api/camera/set", payload

    def build_camera_face_report(self, scan_id, face_detected, confidence=0.0, yaw=None, pitch=None, face_center_x=None, face_center_y=None, face_width=None, face_height=None, force=None):
        if (yaw is None) != (pitch is None):
            raise ValueError("yaw and pitch must be provided together")
        payload = {
            "scanId": int(scan_id),
            "faceDetected": bool(face_detected),
            "confidence": round(clamp(float(confidence), 0.0, 1.0), 3),
        }
        if yaw is not None and pitch is not None:
            payload["yaw"] = int(clamp(int(yaw), -1280, 1280))
            payload["pitch"] = int(clamp(int(pitch), 0, 900))
        if face_center_x is not None:
            payload["faceCenterX"] = round(clamp(float(face_center_x), 0.0, 1.0), 3)
        if face_center_y is not None:
            payload["faceCenterY"] = round(clamp(float(face_center_y), 0.0, 1.0), 3)
        if face_width is not None:
            payload["faceWidth"] = round(clamp(float(face_width), 0.0, 1.0), 3)
        if face_height is not None:
            payload["faceHeight"] = round(clamp(float(face_height), 0.0, 1.0), 3)
        if force is not None:
            payload["force"] = bool(force)
        return "/api/camera/face-report", payload

    def camera_frame_path(self):
        return "/api/camera/frame.bmp"

    def build_camera_catchphrase(self, asset_id, listen_after=True):
        return "/api/camera/catchphrase", {
            "assetId": str(asset_id or "halo"),
            "listenAfter": bool(listen_after),
        }

    def build_camera_wake(self, hold_ms=None):
        payload = {}
        if hold_ms is not None:
            payload["holdMs"] = int(clamp(int(hold_ms), 1000, 30000))
        return "/api/camera/wake", payload

    def build_camera_clear(self):
        return "/api/camera/clear", {}

    def build_debug_subtitle_set(self, listening=None, api_result=None):
        payload = {}
        if listening is not None:
            payload["listening"] = bool(listening)
        if api_result is not None:
            payload["apiResult"] = bool(api_result)
        return "/api/debug/subtitle/set", payload

    def build_debug_subtitle_show(self, kind, text):
        if kind not in {"listening", "apiResult"}:
            raise ValueError("kind must be listening or apiResult")
        if isinstance(text, (dict, list)):
            text = json.dumps(text, ensure_ascii=False, separators=(",", ":"))
        return "/api/debug/subtitle/show", {"kind": kind, "text": str(text)}

    def build_listening_config(
        self,
        voice_wake_enabled=None,
        host_callback_url=None,
        listen_timeout_ms=None,
    ):
        payload = {}
        if voice_wake_enabled is not None:
            payload["voiceWakeEnabled"] = bool(voice_wake_enabled)
        if host_callback_url is not None:
            payload["hostCallbackUrl"] = str(host_callback_url)
        if listen_timeout_ms is not None:
            payload["listenTimeoutMs"] = int(clamp(int(listen_timeout_ms), 3000, 30000))
        return "/api/listening/config", payload

    def build_listening_start(self, listen_timeout_ms=None):
        payload = {}
        if listen_timeout_ms is not None:
            payload["listenTimeoutMs"] = int(clamp(int(listen_timeout_ms), 3000, 30000))
        return "/api/listening/start", payload

    def build_listening_stop(self):
        return "/api/listening/stop", {}

    def build_listening_wake_test(self, source="host"):
        return "/api/listening/wake-test", {"source": str(source)}

    def build_mumble_set(self, enabled=None, speed_percent=None, volume=None, auto_enabled=None):
        payload = {}
        if enabled is not None:
            payload["enabled"] = bool(enabled)
        if speed_percent is not None:
            payload["speedPercent"] = int(clamp(int(speed_percent), 25, 400))
        if volume is not None:
            payload["volume"] = int(clamp(int(volume), 0, 255))
        if auto_enabled is not None:
            payload["autoEnabled"] = bool(auto_enabled)
        return "/api/mumble/set", payload

    def build_mumble_play(self, mode="full", duration_ms=None):
        payload = {"mode": str(mode or "full")}
        if duration_ms is not None:
            payload["durationMs"] = int(clamp(int(duration_ms), 0, 30000))
        return "/api/mumble/play", payload

    def build_mumble_stop(self):
        return "/api/mumble/stop", {}

    def get(self, path, timeout=5):
        with urllib.request.urlopen(self.host + path, timeout=timeout) as response:
            return json.loads(response.read().decode("utf-8"))

    def post(self, path, payload, timeout=5):
        data = json.dumps(payload).encode("utf-8")
        req = urllib.request.Request(
            self.host + path,
            data=data,
            headers={"Content-Type": "application/json"},
            method="POST",
        )
        with urllib.request.urlopen(req, timeout=timeout) as response:
            return json.loads(response.read().decode("utf-8"))

    def post_binary(self, path, data, headers=None, timeout=5):
        request_headers = {"Content-Type": "application/octet-stream"}
        request_headers.update(headers or {})
        req = urllib.request.Request(
            self.host + path,
            data=bytes(data),
            headers=request_headers,
            method="POST",
        )
        with urllib.request.urlopen(req, timeout=timeout) as response:
            return json.loads(response.read().decode("utf-8"))

    def post_multipart_file(self, path, field_name, filename, data, content_type="application/octet-stream", timeout=5):
        boundary = "----RobotEvensFormBoundary" + uuid.uuid4().hex
        body = bytearray()
        body.extend(f"--{boundary}\r\n".encode("ascii"))
        body.extend(
            (
                f'Content-Disposition: form-data; name="{field_name}"; filename="{filename}"\r\n'
                f"Content-Type: {content_type}\r\n\r\n"
            ).encode("utf-8")
        )
        body.extend(bytes(data))
        body.extend(f"\r\n--{boundary}--\r\n".encode("ascii"))
        req = urllib.request.Request(
            self.host + path,
            data=bytes(body),
            headers={"Content-Type": f"multipart/form-data; boundary={boundary}"},
            method="POST",
        )
        with urllib.request.urlopen(req, timeout=timeout) as response:
            return json.loads(response.read().decode("utf-8"))

    def status(self):
        return self.get("/api/status")

    def catalog(self):
        return self.get("/api/catalog")

    def catalog_assets(self):
        return self.get("/api/catalog/assets")

    def catalog_logic(self):
        return self.get("/api/catalog/logic")

    def catalog_conflicts(self):
        return self.get("/api/catalog/conflicts")

    def face_set(self, mood, color):
        return self.post(*self.build_face_set(mood, color))

    def face_react(self, look_x, look_y, audio_level):
        return self.post(*self.build_face_react(look_x, look_y, audio_level))

    def motion_pose(self, yaw, pitch, speed):
        return self.post(*self.build_motion_pose(yaw, pitch, speed))

    def motion_rotate(self, velocity):
        return self.post(*self.build_motion_rotate(velocity))

    def motion_home(self, speed=300):
        return self.post("/api/motion/home", {"speed": int(clamp(int(speed), 0, 1000))})

    def rgb(self, color, index=None, brightness=None, breathing=None, marquee=None, rainbow=None, preset=None):
        return self.post(*self.build_rgb(color, index, brightness, breathing, marquee, rainbow, preset))

    def emotion_play(self, name, lights=True, sound=True):
        return self.post(*self.build_emotion_play(name, lights=lights, sound=sound))

    def asset_invoke(self, asset_id, **kwargs):
        return self.post(*self.build_asset_invoke(asset_id, **kwargs))

    def camera_status(self):
        return self.get("/api/camera/status")

    def camera_set(self, **kwargs):
        return self.post(*self.build_camera_set(**kwargs))

    def camera_frame_bytes(self, timeout=5):
        with urllib.request.urlopen(self.host + self.camera_frame_path(), timeout=timeout) as response:
            return response.read()

    def camera_face_report(self, scan_id, face_detected, confidence=0.0, yaw=None, pitch=None, face_center_x=None, face_center_y=None, face_width=None, face_height=None, force=None):
        return self.post(*self.build_camera_face_report(scan_id, face_detected, confidence, yaw, pitch, face_center_x, face_center_y, face_width, face_height, force))

    def camera_catchphrase(self, asset_id, listen_after=True):
        return self.post(*self.build_camera_catchphrase(asset_id, listen_after))

    def camera_wake(self, hold_ms=None):
        return self.post(*self.build_camera_wake(hold_ms))

    def camera_clear(self):
        return self.post(*self.build_camera_clear())

    def debug_subtitle_status(self):
        return self.get("/api/debug/subtitle/status")

    def debug_subtitle_set(self, listening=None, api_result=None):
        return self.post(*self.build_debug_subtitle_set(listening=listening, api_result=api_result))

    def debug_subtitle_show(self, kind, text):
        return self.post(*self.build_debug_subtitle_show(kind, text))

    def listening_status(self):
        return self.get("/api/listening/status")

    def listening_config(
        self,
        voice_wake_enabled=None,
        host_callback_url=None,
        listen_timeout_ms=None,
    ):
        return self.post(
            *self.build_listening_config(
                voice_wake_enabled=voice_wake_enabled,
                host_callback_url=host_callback_url,
                listen_timeout_ms=listen_timeout_ms,
            )
        )

    def listening_start(self, listen_timeout_ms=None):
        return self.post(*self.build_listening_start(listen_timeout_ms))

    def listening_stop(self):
        return self.post(*self.build_listening_stop())

    def listening_wake_test(self, source="host"):
        return self.post(*self.build_listening_wake_test(source))

    def mumble_upload(self, data, filename="realtime-api.raw", timeout=15):
        return self.post_multipart_file(
            "/api/mumble/upload",
            "file",
            filename,
            data,
            content_type="application/octet-stream",
            timeout=timeout,
        )

    def mumble_set(self, enabled=None, speed_percent=None, volume=None, auto_enabled=None, timeout=5):
        return self.post(*self.build_mumble_set(enabled, speed_percent, volume, auto_enabled), timeout=timeout)

    def mumble_play(self, mode="full", duration_ms=None, timeout=10):
        return self.post(*self.build_mumble_play(mode, duration_ms), timeout=timeout)

    def mumble_stop(self, timeout=5):
        return self.post(*self.build_mumble_stop(), timeout=timeout)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--host", required=True)
    sub = parser.add_subparsers(dest="cmd", required=True)
    sub.add_parser("status")
    sub.add_parser("catalog")
    sub.add_parser("catalog-assets")
    sub.add_parser("catalog-logic")
    sub.add_parser("catalog-conflicts")
    asset = sub.add_parser("asset-invoke")
    asset.add_argument("asset_id")
    asset.add_argument("--duration-ms", type=int)
    asset.add_argument("--speed", type=int)
    asset.add_argument("--lights", type=int, choices=[0, 1])
    asset.add_argument("--sound", type=int, choices=[0, 1])
    asset.add_argument("--color")
    asset.add_argument("--eye-color")
    face = sub.add_parser("face")
    face.add_argument("mood")
    face.add_argument("--color", default="#00f2ff")
    react = sub.add_parser("react")
    react.add_argument("--look-x", type=float, default=0.0)
    react.add_argument("--look-y", type=float, default=0.0)
    react.add_argument("--audio", type=float, default=0.0)
    pose = sub.add_parser("pose")
    pose.add_argument("--yaw", type=int, default=0)
    pose.add_argument("--pitch", type=int, default=120)
    pose.add_argument("--speed", type=int, default=300)
    rotate = sub.add_parser("rotate")
    rotate.add_argument("velocity", type=int)
    home = sub.add_parser("home")
    home.add_argument("--speed", type=int, default=300)
    rgb = sub.add_parser("rgb")
    rgb.add_argument("color")
    rgb.add_argument("--index", type=int)
    rgb.add_argument("--brightness", type=int)
    rgb.add_argument("--breathing", type=int, choices=[0, 1])
    rgb.add_argument("--marquee", type=int, choices=[0, 1])
    rgb.add_argument("--rainbow", type=int, choices=[0, 1])
    rgb.add_argument("--preset")
    sub.add_parser("camera-status")
    camera_set = sub.add_parser("camera-set")
    camera_set.add_argument("--enabled", type=int, choices=[0, 1])
    camera_set.add_argument("--capture-interval-ms", type=int)
    camera_set.add_argument("--detect-timeout-ms", type=int)
    camera_set.add_argument("--return-settle-ms", type=int)
    camera_set.add_argument("--confirm-settle-ms", type=int)
    camera_set.add_argument("--lock-hold-ms", type=int)
    camera_set.add_argument("--cooldown-ms", type=int)
    camera_set.add_argument("--center-dead-zone-x", type=float)
    camera_set.add_argument("--center-dead-zone-y", type=float)
    camera_set.add_argument("--yaw-step-max", type=int)
    camera_set.add_argument("--pitch-step-max", type=int)
    camera_set.add_argument("--catchphrase-asset-id")
    camera_set.add_argument("--listen-after-catchphrase", type=int, choices=[0, 1])
    camera_report = sub.add_parser("camera-face-report")
    camera_report.add_argument("--scan-id", type=int, required=True)
    camera_report.add_argument("--detected", type=int, choices=[0, 1], required=True)
    camera_report.add_argument("--confidence", type=float, default=0.0)
    camera_report.add_argument("--yaw", type=int)
    camera_report.add_argument("--pitch", type=int)
    camera_report.add_argument("--face-center-x", type=float)
    camera_report.add_argument("--face-center-y", type=float)
    camera_report.add_argument("--face-width", type=float)
    camera_report.add_argument("--face-height", type=float)
    camera_catchphrase_parser = sub.add_parser("camera-catchphrase")
    camera_catchphrase_parser.add_argument("--asset-id", default="halo")
    camera_catchphrase_parser.add_argument("--listen-after", type=int, choices=[0, 1], default=1)
    camera_wake = sub.add_parser("camera-wake")
    camera_wake.add_argument("--hold-ms", type=int)
    sub.add_parser("camera-clear")
    args = parser.parse_args()
    client = RobotClient(args.host)
    if args.cmd == "status":
        result = client.status()
    elif args.cmd == "catalog":
        result = client.catalog()
    elif args.cmd == "catalog-assets":
        result = client.catalog_assets()
    elif args.cmd == "catalog-logic":
        result = client.catalog_logic()
    elif args.cmd == "catalog-conflicts":
        result = client.catalog_conflicts()
    elif args.cmd == "asset-invoke":
        result = client.asset_invoke(
            args.asset_id,
            duration_ms=args.duration_ms,
            speed=args.speed,
            lights=None if args.lights is None else bool(args.lights),
            sound=None if args.sound is None else bool(args.sound),
            color=args.color,
            eye_color=args.eye_color,
        )
    elif args.cmd == "face":
        result = client.face_set(args.mood, args.color)
    elif args.cmd == "react":
        result = client.face_react(args.look_x, args.look_y, args.audio)
    elif args.cmd == "pose":
        result = client.motion_pose(args.yaw, args.pitch, args.speed)
    elif args.cmd == "rotate":
        result = client.motion_rotate(args.velocity)
    elif args.cmd == "home":
        result = client.motion_home(args.speed)
    elif args.cmd == "camera-status":
        result = client.camera_status()
    elif args.cmd == "camera-set":
        result = client.camera_set(
            enabled=None if args.enabled is None else bool(args.enabled),
            capture_interval_ms=args.capture_interval_ms,
            detect_timeout_ms=args.detect_timeout_ms,
            return_settle_ms=args.return_settle_ms,
            confirm_settle_ms=args.confirm_settle_ms,
            lock_hold_ms=args.lock_hold_ms,
            cooldown_ms=args.cooldown_ms,
            center_dead_zone_x=args.center_dead_zone_x,
            center_dead_zone_y=args.center_dead_zone_y,
            yaw_step_max=args.yaw_step_max,
            pitch_step_max=args.pitch_step_max,
            catchphrase_asset_id=args.catchphrase_asset_id,
            listen_after_catchphrase=None if args.listen_after_catchphrase is None else bool(args.listen_after_catchphrase),
        )
    elif args.cmd == "camera-face-report":
        result = client.camera_face_report(
            args.scan_id,
            bool(args.detected),
            args.confidence,
            args.yaw,
            args.pitch,
            args.face_center_x,
            args.face_center_y,
            args.face_width,
            args.face_height,
        )
    elif args.cmd == "camera-catchphrase":
        result = client.camera_catchphrase(args.asset_id, bool(args.listen_after))
    elif args.cmd == "camera-wake":
        result = client.camera_wake(args.hold_ms)
    elif args.cmd == "camera-clear":
        result = client.camera_clear()
    else:
        result = client.rgb(
            args.color,
            args.index,
            args.brightness,
            None if args.breathing is None else bool(args.breathing),
            None if args.marquee is None else bool(args.marquee),
            None if args.rainbow is None else bool(args.rainbow),
            args.preset,
        )
    print(json.dumps(result, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
