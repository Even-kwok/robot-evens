#!/usr/bin/env python3
import argparse
import base64
import binascii
import json
import os
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

try:
    from host.console_server import HostModelConfigStore, HostRealtimeVoiceService, HostRobotProxy
    from host.robot_client import RobotClient
    from host.volcengine_realtime import build_voice_system_prompt, run_volcengine_realtime_audio
except ModuleNotFoundError:
    from console_server import HostModelConfigStore, HostRealtimeVoiceService, HostRobotProxy
    from robot_client import RobotClient
    from volcengine_realtime import build_voice_system_prompt, run_volcengine_realtime_audio


VOICE_CONSOLE_PATH = Path(__file__).with_name("voice_console.html")
MOBILE_REALTIME_VOICE_AUDIO_PATH = "/api/mobile/realtime-voice/audio"
MAX_MOBILE_AUDIO_REQUEST_BYTES = 4 * 1024 * 1024


class VoiceRobotProxy:
    def __init__(self, client):
        self.client = client

    def status(self):
        return self.client.get("/api/status")


class MobileRealtimeVoiceService(HostRealtimeVoiceService):
    def __init__(
        self,
        model_config,
        runner=run_volcengine_realtime_audio,
        timeout_ms=None,
        robot_audio=None,
    ):
        super().__init__(model_config, runner=runner, timeout_ms=timeout_ms, robot_audio=robot_audio)

    def run_audio(self, payload):
        payload = payload or {}
        encoded_audio = str(payload.get("audioBase64") or "").strip()
        if not encoded_audio:
            return {"ok": False, "stage": "input", "error": "audioBase64 is required"}

        try:
            audio = base64.b64decode(encoded_audio, validate=True)
        except (binascii.Error, ValueError) as exc:
            return {"ok": False, "stage": "input", "error": f"invalid audioBase64: {exc}"}
        if not audio:
            return {"ok": False, "stage": "input", "error": "audio is empty"}

        try:
            raw_config, realtime_config = self._realtime_config(payload)
            system_prompt = (raw_config.get("realtimeFirmwareNotes") or "").strip()
            if not system_prompt:
                system_prompt = build_voice_system_prompt(raw_config.get("knownUser"))
            result = self._run_with_timeout(self.runner, realtime_config, audio, system_prompt)
        except Exception as exc:
            return {"ok": False, "stage": "realtime_api", "error": str(exc)}
        return self._finalize_result(result)


class MobileVoiceRequestHandler(BaseHTTPRequestHandler):
    service = None

    def log_message(self, fmt, *args):
        return

    def send_json(self, status, payload):
        body = json.dumps(payload, ensure_ascii=False).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def read_json(self):
        length = int(self.headers.get("Content-Length", "0") or 0)
        if length > MAX_MOBILE_AUDIO_REQUEST_BYTES:
            raise ValueError("request body too large")
        if length <= 0:
            return {}
        return json.loads(self.rfile.read(length).decode("utf-8"))

    def do_GET(self):
        if self.path == "/api/ping":
            self.send_json(200, {"ok": True, "service": "mobile-realtime-voice"})
            return
        self.send_json(404, {"ok": False, "error": "not found"})

    def do_POST(self):
        if self.path != MOBILE_REALTIME_VOICE_AUDIO_PATH:
            self.send_json(404, {"ok": False, "error": "not found"})
            return
        try:
            payload = self.read_json()
        except Exception as exc:
            self.send_json(400, {"ok": False, "stage": "input", "error": str(exc)})
            return

        result = self.service.run_audio(payload)
        self.send_json(200 if result.get("ok", True) else 502, result)


class VoiceConsoleRequestHandler(BaseHTTPRequestHandler):
    proxy = None
    robot_base_url = ""
    model_config = None

    def send_json(self, status, payload):
        body = json.dumps(payload, ensure_ascii=False).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def send_html(self):
        body = VOICE_CONSOLE_PATH.read_bytes()
        self.send_response(200)
        self.send_header("Content-Type", "text/html; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def read_json(self):
        length = int(self.headers.get("Content-Length", "0"))
        if length <= 0:
            return {}
        return json.loads(self.rfile.read(length).decode("utf-8"))

    def call(self, fn):
        try:
            return fn()
        except Exception as exc:
            return {"ok": False, "error": str(exc)}

    def do_GET(self):
        routes = {
            "/api/config": lambda: {"ok": True, "robotBaseUrl": self.robot_base_url},
            "/api/robot/status": self.proxy.status,
            "/api/host/model-config/status": self.model_config.status,
        }
        if self.path == "/" or self.path == "/index.html":
            self.send_html()
            return
        if self.path not in routes:
            self.send_json(404, {"ok": False, "error": "not found"})
            return
        result = self.call(routes[self.path])
        self.send_json(200 if result.get("ok", True) else 502, result)

    def do_POST(self):
        try:
            payload = self.read_json()
        except json.JSONDecodeError:
            self.send_json(400, {"ok": False, "error": "invalid json"})
            return
        routes = {
            "/api/host/model-config/save": lambda: self.model_config.save(payload),
        }
        if self.path not in routes:
            self.send_json(404, {"ok": False, "error": "not found"})
            return
        result = self.call(routes[self.path])
        self.send_json(200 if result.get("ok", True) else 502, result)

def make_handler(proxy, robot_base_url, bridge=None, model_config=None):
    class BoundVoiceConsoleRequestHandler(VoiceConsoleRequestHandler):
        pass

    BoundVoiceConsoleRequestHandler.proxy = proxy
    BoundVoiceConsoleRequestHandler.robot_base_url = robot_base_url
    BoundVoiceConsoleRequestHandler.model_config = model_config or HostModelConfigStore()
    return BoundVoiceConsoleRequestHandler


def make_mobile_handler(service):
    class BoundMobileVoiceRequestHandler(MobileVoiceRequestHandler):
        pass

    BoundMobileVoiceRequestHandler.service = service
    return BoundMobileVoiceRequestHandler


def run_server(robot_base_url, bind="0.0.0.0", port=8767):
    proxy = VoiceRobotProxy(RobotClient(robot_base_url))
    model_config = HostModelConfigStore()
    server = ThreadingHTTPServer(
        (bind, port),
        make_handler(proxy, robot_base_url, model_config=model_config),
    )
    print(f"Voice console: http://{bind}:{port}")
    print(f"Robot: {robot_base_url}")
    server.serve_forever()


def run_mobile_server(robot_base_url, bind="127.0.0.1", port=8766):
    proxy = HostRobotProxy(RobotClient(robot_base_url))
    service = MobileRealtimeVoiceService(HostModelConfigStore(), robot_audio=proxy)
    server = ThreadingHTTPServer((bind, port), make_mobile_handler(service))
    print(f"Mobile realtime voice: http://{bind}:{port}")
    print(f"Robot: {robot_base_url}")
    server.serve_forever()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--robot", default=os.environ.get("ROBOT_BASE_URL", "http://deskbot.local"))
    parser.add_argument("--bind", default="0.0.0.0")
    parser.add_argument("--port", type=int, default=8767)
    parser.add_argument("--mobile-only", action="store_true")
    args = parser.parse_args()
    if args.mobile_only:
        run_mobile_server(args.robot, args.bind, args.port)
    else:
        run_server(args.robot, args.bind, args.port)


if __name__ == "__main__":
    main()
