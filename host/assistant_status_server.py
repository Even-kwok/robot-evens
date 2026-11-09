#!/usr/bin/env python3
import argparse
import json
from datetime import datetime, timedelta
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path


ASSISTANT_STATUS_PATH = Path(__file__).with_name("assistant_status.local.json")
DEFAULT_FEATURES = {
    "dateTime": True,
    "codingDuration": True,
    "codexRemaining": True,
    "completedTasks": True,
    "voicePlayback": True,
}
DEFAULT_EVENT_TTL_SECONDS = 12
DEFAULT_SETTINGS = {
    "eventTtlSeconds": DEFAULT_EVENT_TTL_SECONDS,
    "lowCodexPercent": 20,
}
EVENT_PROMPTS = {
    "task_failed": "TASK FAILED",
    "blocked": "CODEX BLOCKED",
    "waiting": "WAIT CONFIRM",
    "build_ok": "BUILD OK",
    "build_failed": "BUILD FAIL",
    "test_ok": "TEST OK",
    "test_failed": "TEST FAIL",
}


def format_duration(seconds):
    seconds = max(0, int(seconds))
    minutes = seconds // 60
    hours = minutes // 60
    minute_part = minutes % 60
    if hours > 0:
        return f"{hours}h {minute_part}m"
    return f"{minutes}m"


def parse_iso_datetime(value):
    if not value:
        return None
    try:
        return datetime.fromisoformat(str(value))
    except ValueError:
        return None


def _positive_int(value, default):
    try:
        parsed = int(value)
    except (TypeError, ValueError):
        return default
    return parsed if parsed > 0 else default


def _clamped_int(value, default, minimum, maximum):
    parsed = _positive_int(value, default)
    return max(minimum, min(maximum, parsed))


def _event_type(payload):
    event_type = str((payload or {}).get("type") or "").strip().lower().replace("-", "_")
    if not event_type:
        raise ValueError("event type is required")
    return event_type


def assistant_event_message(payload):
    payload = payload or {}
    event_type = _event_type(payload)
    if event_type == "task_done":
        count = _positive_int(payload.get("count"), 1)
        return event_type, f"TASK DONE +{count}"
    if event_type == "codex_low":
        remaining = payload.get("remainingPercent")
        if remaining is None:
            return event_type, "LOW CODEX"
        try:
            remaining = int(remaining)
        except (TypeError, ValueError) as exc:
            raise ValueError("remainingPercent must be an integer from 0 to 100") from exc
        if remaining < 0 or remaining > 100:
            raise ValueError("remainingPercent must be an integer from 0 to 100")
        return event_type, f"LOW CODEX {remaining}%"
    if event_type in EVENT_PROMPTS:
        return event_type, EVENT_PROMPTS[event_type]
    raise ValueError("unsupported event type")


class AssistantStatusStore:
    def __init__(self, path=ASSISTANT_STATUS_PATH, now_provider=None):
        self.path = Path(path)
        self.now_provider = now_provider or (lambda: datetime.now().astimezone())
        self.state_issue = None

    def now(self):
        value = self.now_provider()
        if value.tzinfo is None:
            return value.astimezone()
        return value

    def today_key(self):
        return self.now().date().isoformat()

    def read_state(self):
        self.state_issue = None
        if not self.path.exists():
            return {"features": dict(DEFAULT_FEATURES), "days": {}}
        try:
            payload = json.loads(self.path.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError) as exc:
            self.state_issue = f"state reset: {exc}"
            return {"features": dict(DEFAULT_FEATURES), "days": {}}
        if not isinstance(payload, dict):
            self.state_issue = "state reset: root is not an object"
            return {"features": dict(DEFAULT_FEATURES), "days": {}}
        features = payload.get("features")
        days = payload.get("days")
        if not isinstance(features, dict):
            features = dict(DEFAULT_FEATURES)
        settings = payload.get("settings")
        if not isinstance(settings, dict):
            settings = dict(DEFAULT_SETTINGS)
        if not isinstance(days, dict):
            days = {}
        merged_features = dict(DEFAULT_FEATURES)
        merged_features.update({key: bool(features[key]) for key in DEFAULT_FEATURES if key in features})
        merged_settings = self.settings_state({"settings": settings})
        return {"features": merged_features, "settings": merged_settings, "days": days}

    def write_state(self, state):
        self.path.parent.mkdir(parents=True, exist_ok=True)
        self.path.write_text(json.dumps(state, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")

    def today_state(self, state):
        key = self.today_key()
        days = state.setdefault("days", {})
        day = days.get(key)
        if not isinstance(day, dict):
            day = {
                "coding": {
                    "active": True,
                    "startedAt": self.now().isoformat(timespec="seconds"),
                    "accumulatedSeconds": 0,
                },
                "codex": {
                    "enabled": True,
                    "available": False,
                    "remainingPercent": None,
                    "source": "manual",
                },
                "tasks": {
                    "completedToday": 0,
                },
            }
            days[key] = day
            self.write_state(state)
        day.setdefault("coding", {})
        day.setdefault("codex", {})
        day.setdefault("tasks", {})
        day.setdefault("event", {})
        return day

    def settings_state(self, state):
        settings = state.setdefault("settings", {})
        if not isinstance(settings, dict):
            settings = {}
            state["settings"] = settings
        merged = dict(DEFAULT_SETTINGS)
        merged["eventTtlSeconds"] = _clamped_int(settings.get("eventTtlSeconds"), DEFAULT_EVENT_TTL_SECONDS, 3, 120)
        merged["lowCodexPercent"] = _clamped_int(settings.get("lowCodexPercent"), 20, 1, 100)
        settings.update(merged)
        return merged

    def event_status(self, event, now):
        if not isinstance(event, dict):
            return {"active": False, "type": "", "message": ""}
        expires_at = parse_iso_datetime(event.get("expiresAt"))
        created_at = parse_iso_datetime(event.get("createdAt"))
        message = str(event.get("message") or "")
        event_type = str(event.get("type") or "")
        active = bool(message) and expires_at is not None and now < expires_at
        if not active:
            return {"active": False, "type": "", "message": ""}
        ttl_seconds = _positive_int(event.get("ttlSeconds"), DEFAULT_EVENT_TTL_SECONDS)
        if created_at is not None and expires_at is not None:
            ttl_seconds = max(1, int((expires_at - created_at).total_seconds()))
        remaining_seconds = max(0, int((expires_at - now).total_seconds()))
        return {
            "active": True,
            "type": event_type,
            "message": message,
            "createdAt": str(event.get("createdAt") or ""),
            "expiresAt": expires_at.isoformat(timespec="seconds"),
            "ttlSeconds": ttl_seconds,
            "remainingSeconds": remaining_seconds,
        }

    def set_event(self, day, event_type, message, ttl_seconds=None, settings=None):
        now = self.now()
        default_ttl = (settings or {}).get("eventTtlSeconds", DEFAULT_EVENT_TTL_SECONDS)
        ttl = _clamped_int(ttl_seconds, default_ttl, 3, 120)
        day["event"] = {
            "type": event_type,
            "message": str(message)[:40],
            "createdAt": now.isoformat(timespec="seconds"),
            "expiresAt": (now.replace() + timedelta(seconds=ttl)).isoformat(timespec="seconds"),
            "ttlSeconds": ttl,
        }

    def status(self):
        state = self.read_state()
        settings = self.settings_state(state)
        day = self.today_state(state)
        now = self.now()
        coding = day["coding"]
        accumulated = max(0, int(coding.get("accumulatedSeconds") or 0))
        active = bool(coding.get("active", True))
        started_at = parse_iso_datetime(coding.get("startedAt"))
        active_seconds = 0
        if active and started_at is not None:
            active_seconds = max(0, int((now - started_at).total_seconds()))
        duration_seconds = accumulated + active_seconds
        codex = day["codex"]
        tasks = day["tasks"]
        response = {
            "ok": True,
            "date": now.date().isoformat(),
            "time": now.strftime("%H:%M"),
            "weekday": now.strftime("%a"),
            "features": state["features"],
            "coding": {
                "active": active,
                "startedAt": coding.get("startedAt") or "",
                "durationSeconds": duration_seconds,
                "display": format_duration(duration_seconds),
            },
            "codex": {
                "enabled": bool(codex.get("enabled", True)),
                "remainingPercent": codex.get("remainingPercent"),
                "source": str(codex.get("source") or "manual"),
                "available": bool(codex.get("available", False)),
            },
            "tasks": {
                "completedToday": max(0, int(tasks.get("completedToday") or 0)),
            },
            "event": self.event_status(day.get("event"), now),
            "settings": settings,
        }
        if self.state_issue:
            response["stateIssue"] = self.state_issue
        return response

    def start_coding(self, payload=None):
        state = self.read_state()
        day = self.today_state(state)
        coding = day["coding"]
        if not bool(coding.get("active", True)):
            coding["active"] = True
            coding["startedAt"] = self.now().isoformat(timespec="seconds")
        self.write_state(state)
        return self.status()

    def stop_coding(self, payload=None):
        state = self.read_state()
        day = self.today_state(state)
        coding = day["coding"]
        started_at = parse_iso_datetime(coding.get("startedAt"))
        if bool(coding.get("active", True)) and started_at is not None:
            elapsed = max(0, int((self.now() - started_at).total_seconds()))
            coding["accumulatedSeconds"] = max(0, int(coding.get("accumulatedSeconds") or 0)) + elapsed
        coding["active"] = False
        coding["startedAt"] = ""
        self.write_state(state)
        return self.status()

    def task_done(self, payload=None):
        payload = payload or {}
        count = payload.get("count", 1)
        try:
            count = int(count)
        except (TypeError, ValueError):
            count = 1
        if count <= 0:
            count = 1
        state = self.read_state()
        day = self.today_state(state)
        settings = self.settings_state(state)
        tasks = day["tasks"]
        tasks["completedToday"] = max(0, int(tasks.get("completedToday") or 0)) + count
        event_type, message = assistant_event_message({"type": "task_done", "count": count})
        self.set_event(day, event_type, message, payload.get("ttlSeconds"), settings)
        self.write_state(state)
        return self.status()

    def set_codex_remaining(self, payload=None):
        payload = payload or {}
        state = self.read_state()
        settings = self.settings_state(state)
        day = self.today_state(state)
        codex = day["codex"]
        codex["enabled"] = bool(payload.get("enabled", True))
        codex["source"] = str(payload.get("source") or "manual")
        if payload.get("available") is False:
            codex["available"] = False
            codex["remainingPercent"] = None
        else:
            try:
                remaining = int(payload["remainingPercent"])
            except (KeyError, TypeError, ValueError) as exc:
                raise ValueError("remainingPercent must be an integer from 0 to 100") from exc
            if remaining < 0 or remaining > 100:
                raise ValueError("remainingPercent must be an integer from 0 to 100")
            codex["available"] = True
            codex["remainingPercent"] = remaining
            low_threshold = _positive_int(payload.get("lowThreshold"), settings["lowCodexPercent"])
            if remaining <= low_threshold:
                event_type, message = assistant_event_message({"type": "codex_low", "remainingPercent": remaining})
                self.set_event(day, event_type, message, payload.get("ttlSeconds"), settings)
        self.write_state(state)
        return self.status()

    def assistant_event(self, payload=None):
        payload = payload or {}
        state = self.read_state()
        settings = self.settings_state(state)
        day = self.today_state(state)
        event_type, message = assistant_event_message(payload)
        self.set_event(day, event_type, message, payload.get("ttlSeconds"), settings)
        self.write_state(state)
        return self.status()

    def update_settings(self, payload=None):
        payload = payload or {}
        state = self.read_state()
        settings = self.settings_state(state)
        if "eventTtlSeconds" in payload:
            settings["eventTtlSeconds"] = _clamped_int(payload.get("eventTtlSeconds"), settings["eventTtlSeconds"], 3, 120)
        if "lowCodexPercent" in payload:
            settings["lowCodexPercent"] = _clamped_int(payload.get("lowCodexPercent"), settings["lowCodexPercent"], 1, 100)
        state["settings"] = settings
        self.write_state(state)
        return self.status()

    def clear_event(self, payload=None):
        state = self.read_state()
        day = self.today_state(state)
        day["event"] = {}
        self.write_state(state)
        return self.status()


class AssistantStatusApi:
    def __init__(self, store):
        self.store = store

    def get(self, path):
        if path == "/api/assistant/status":
            return 200, self.store.status()
        if path == "/api/assistant/settings":
            return 200, self.store.status()
        return 404, {"ok": False, "error": "not found"}

    def post(self, path, payload):
        try:
            if path == "/api/assistant/coding/start":
                return 200, self.store.start_coding(payload)
            if path == "/api/assistant/coding/stop":
                return 200, self.store.stop_coding(payload)
            if path == "/api/assistant/tasks/done":
                return 200, self.store.task_done(payload)
            if path == "/api/assistant/codex/remaining":
                return 200, self.store.set_codex_remaining(payload)
            if path == "/api/assistant/events":
                return 200, self.store.assistant_event(payload)
            if path == "/api/assistant/events/clear":
                return 200, self.store.clear_event(payload)
            if path == "/api/assistant/settings":
                return 200, self.store.update_settings(payload)
        except ValueError as exc:
            return 400, {"ok": False, "error": str(exc)}
        return 404, {"ok": False, "error": "not found"}


class AssistantStatusRequestHandler(BaseHTTPRequestHandler):
    api = None

    def send_cors_headers(self):
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")

    def send_json(self, status, payload):
        body = json.dumps(payload, ensure_ascii=False).encode("utf-8")
        self.send_response(status)
        self.send_cors_headers()
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def read_json(self):
        length = int(self.headers.get("Content-Length", "0"))
        if length <= 0:
            return {}
        return json.loads(self.rfile.read(length).decode("utf-8"))

    def do_GET(self):
        status, payload = self.api.get(self.path)
        self.send_json(status, payload)

    def do_OPTIONS(self):
        self.send_response(204)
        self.send_cors_headers()
        self.end_headers()

    def do_POST(self):
        try:
            payload = self.read_json()
        except json.JSONDecodeError:
            self.send_json(400, {"ok": False, "error": "invalid json"})
            return
        status, response = self.api.post(self.path, payload)
        self.send_json(status, response)


def make_handler(store=None):
    class BoundAssistantStatusRequestHandler(AssistantStatusRequestHandler):
        pass

    BoundAssistantStatusRequestHandler.api = AssistantStatusApi(store or AssistantStatusStore())
    return BoundAssistantStatusRequestHandler


def run_server(bind="0.0.0.0", port=8770, state_path=ASSISTANT_STATUS_PATH):
    store = AssistantStatusStore(state_path)
    server = ThreadingHTTPServer((bind, port), make_handler(store))
    print(f"Assistant status: http://{bind}:{port}")
    print(f"State: {state_path}")
    server.serve_forever()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--bind", default="0.0.0.0")
    parser.add_argument("--port", type=int, default=8770)
    parser.add_argument("--state", default=str(ASSISTANT_STATUS_PATH))
    args = parser.parse_args()
    run_server(args.bind, args.port, Path(args.state))


if __name__ == "__main__":
    main()
