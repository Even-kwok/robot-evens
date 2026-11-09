#!/usr/bin/env python3
import argparse
import json
import time
import urllib.request
from datetime import datetime
from pathlib import Path


MOODS = [
    "neutral",
    "happy",
    "angry",
    "sad",
    "sleeping",
    "tired",
    "surprised",
    "love",
    "dizzy",
    "wink",
    "dead",
    "thinking",
    "curious",
    "sunglasses",
    "loading",
]

ENDPOINTS = {
    "status": "/api/status",
    "triggers": "/api/triggers/status",
    "idle": "/api/idle/status",
    "camera": "/api/camera/status",
    "clock": "/api/clock/status",
    "mumble": "/api/mumble/status",
    "proximity": "/api/proximity/status",
    "emotion": "/api/emotion/status",
    "emotionSoundSlots": "/api/emotion/sound-slots",
    "listening": "/api/listening/status",
    "voiceNod": "/api/voice-nod/status",
    "debugSubtitle": "/api/debug/subtitle/status",
    "motionStream": "/api/motion/stream/status",
    "screen": "/api/screen/status",
    "faceLibrary": "/api/face/library",
}

OPTIONAL_ENDPOINT_KEYS = {
    "voiceNod",
}

FACE_STYLE_FIELDS = [
    "eyeWidthScale",
    "eyeHeightScale",
    "cornerRadius",
    "eyeOffsetX",
    "eyeOffsetY",
    "eyeSpacing",
    "eyeColor",
    "glowEnabled",
    "glowStrength",
    "scanlinesEnabled",
    "scanlineSpacing",
    "scanlineAlpha",
]

EXCLUDED_EPHEMERAL_FIELDS = [
    "uptimeMs",
    "ip",
    "rssi",
    "current yaw/pitch",
    "active behavior step",
    "cooldown timers",
    "detected face memory",
    "last trigger timestamps",
    "battery and heap diagnostics",
    "current dateTimeText",
]


def default_timestamp():
    return datetime.now().astimezone().strftime("%Y-%m-%dT%H:%M:%S%z")


def filename_timestamp():
    return datetime.now().astimezone().strftime("%Y%m%d-%H%M%S")


class UrlTransport:
    def __init__(self, base_url, timeout=5):
        self.base_url = base_url.rstrip("/")
        self.timeout = timeout

    def get_json(self, path):
        with urllib.request.urlopen(self.base_url + path, timeout=self.timeout) as response:
            return json.loads(response.read().decode("utf-8"))

    def post_json(self, path, payload):
        data = json.dumps(payload).encode("utf-8")
        request = urllib.request.Request(
            self.base_url + path,
            data=data,
            headers={"Content-Type": "application/json"},
            method="POST",
        )
        with urllib.request.urlopen(request, timeout=self.timeout) as response:
            return json.loads(response.read().decode("utf-8"))


class RobotSettingsCapture:
    def __init__(self, robot_url, transport=None, now=default_timestamp, pause_seconds=0.05):
        self.robot_url = robot_url.rstrip("/")
        self.transport = transport or UrlTransport(self.robot_url)
        self.now = now
        self.pause_seconds = pause_seconds

    def capture(self):
        payload = {
            "capturedAt": self.now(),
            "source": self.robot_url,
            "endpoints": dict(ENDPOINTS),
            "endpointErrors": {},
            "excludedEphemeralFields": list(EXCLUDED_EPHEMERAL_FIELDS),
        }
        for key, path in ENDPOINTS.items():
            try:
                payload[key] = self.transport.get_json(path)
            except Exception as exc:
                if key not in OPTIONAL_ENDPOINT_KEYS:
                    raise
                payload["endpointErrors"][key] = {
                    "path": path,
                    "error": str(exc),
                }

        original_mood = payload["status"].get("face", {}).get("mood", "neutral")
        payload["moodStyles"] = self._capture_mood_styles(original_mood)
        return payload

    def _capture_mood_styles(self, original_mood):
        styles = {}
        try:
            for mood in MOODS:
                response = self.transport.post_json("/api/face/set", {"mood": mood})
                if self.pause_seconds:
                    time.sleep(self.pause_seconds)
                face = self._face_state_from_response(response)
                if not face:
                    status = self.transport.get_json("/api/status")
                    face = status.get("face", {})
                styles[mood] = {
                    key: face[key]
                    for key in FACE_STYLE_FIELDS
                    if key in face
                }
        finally:
            self.transport.post_json("/api/face/set", {"mood": original_mood or "neutral"})
        return styles

    def _face_state_from_response(self, response):
        face = response.get("face", response)
        if any(key in face for key in FACE_STYLE_FIELDS):
            return face
        return {}


def parse_args():
    parser = argparse.ArgumentParser(description="Capture live Robot Evens firmware settings.")
    parser.add_argument("--robot", default="http://deskbot.local", help="Robot base URL")
    parser.add_argument("--output", help="Output JSON path")
    parser.add_argument("--backup-dir", default="backups", help="Directory for generated backups")
    parser.add_argument("--timeout", type=float, default=5.0, help="HTTP timeout in seconds")
    parser.add_argument("--mood-pause", type=float, default=0.05, help="Pause after switching moods")
    return parser.parse_args()


def main():
    args = parse_args()
    robot_url = args.robot.rstrip("/")
    transport = UrlTransport(robot_url, timeout=args.timeout)
    capture = RobotSettingsCapture(robot_url, transport=transport, pause_seconds=args.mood_pause)
    payload = capture.capture()

    if args.output:
        output = Path(args.output)
    else:
        output = Path(args.backup_dir) / f"robot-device-settings-{filename_timestamp()}.json"
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(payload, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(output)


if __name__ == "__main__":
    main()
