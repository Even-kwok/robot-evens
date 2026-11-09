import json
import unittest
from datetime import datetime, timedelta, timezone
from tempfile import TemporaryDirectory
from pathlib import Path

from host.assistant_status_server import AssistantStatusApi, AssistantStatusStore, make_handler, run_server


class FakeClock:
    def __init__(self, value):
        self.value = value

    def __call__(self):
        return self.value


class AssistantStatusStoreTest(unittest.TestCase):
    def test_status_starts_today_session_when_state_is_empty(self):
        with TemporaryDirectory() as tmp:
            clock = FakeClock(datetime(2026, 6, 30, 21, 42, tzinfo=timezone.utc))
            store = AssistantStatusStore(Path(tmp) / "assistant_status.json", now_provider=clock)

            status = store.status()

            self.assertEqual(status["ok"], True)
            self.assertEqual(status["date"], "2026-06-30")
            self.assertEqual(status["time"], "21:42")
            self.assertEqual(status["coding"]["active"], True)
            self.assertEqual(status["coding"]["durationSeconds"], 0)
            self.assertEqual(status["coding"]["display"], "0m")
            self.assertEqual(status["codex"]["enabled"], True)
            self.assertEqual(status["codex"]["available"], False)
            self.assertIsNone(status["codex"]["remainingPercent"])
            self.assertEqual(status["tasks"]["completedToday"], 0)
            self.assertEqual(status["settings"]["eventTtlSeconds"], 12)
            self.assertEqual(status["settings"]["lowCodexPercent"], 20)

    def test_coding_duration_accumulates_while_active(self):
        with TemporaryDirectory() as tmp:
            clock = FakeClock(datetime(2026, 6, 30, 9, 0, tzinfo=timezone.utc))
            store = AssistantStatusStore(Path(tmp) / "assistant_status.json", now_provider=clock)
            store.status()

            clock.value = datetime(2026, 6, 30, 11, 18, tzinfo=timezone.utc)
            status = store.status()

            self.assertEqual(status["coding"]["durationSeconds"], 8280)
            self.assertEqual(status["coding"]["display"], "2h 18m")

    def test_task_done_increments_today_count(self):
        with TemporaryDirectory() as tmp:
            clock = FakeClock(datetime(2026, 6, 30, 10, 0, tzinfo=timezone.utc))
            store = AssistantStatusStore(Path(tmp) / "assistant_status.json", now_provider=clock)

            one = store.task_done({})
            four = store.task_done({"count": 3})

            self.assertEqual(one["tasks"]["completedToday"], 1)
            self.assertEqual(one["event"]["active"], True)
            self.assertEqual(one["event"]["message"], "TASK DONE +1")
            self.assertEqual(four["tasks"]["completedToday"], 4)
            self.assertEqual(four["event"]["message"], "TASK DONE +3")

    def test_codex_event_prompt_expires_after_ttl(self):
        with TemporaryDirectory() as tmp:
            clock = FakeClock(datetime(2026, 6, 30, 10, 0, tzinfo=timezone.utc))
            store = AssistantStatusStore(Path(tmp) / "assistant_status.json", now_provider=clock)

            event_status = store.assistant_event({"type": "task_failed", "ttlSeconds": 12})

            self.assertEqual(event_status["event"]["active"], True)
            self.assertEqual(event_status["event"]["type"], "task_failed")
            self.assertEqual(event_status["event"]["message"], "TASK FAILED")
            self.assertEqual(event_status["event"]["ttlSeconds"], 12)
            self.assertEqual(event_status["event"]["remainingSeconds"], 12)

            clock.value = clock.value + timedelta(seconds=13)
            expired = store.status()

            self.assertEqual(expired["event"]["active"], False)
            self.assertEqual(expired["event"]["message"], "")

    def test_event_settings_control_default_ttl_and_low_codex_threshold(self):
        with TemporaryDirectory() as tmp:
            clock = FakeClock(datetime(2026, 6, 30, 10, 0, tzinfo=timezone.utc))
            store = AssistantStatusStore(Path(tmp) / "assistant_status.json", now_provider=clock)

            settings_status = store.update_settings({"eventTtlSeconds": 30, "lowCodexPercent": 19})
            event_status = store.assistant_event({"type": "build_ok"})
            codex_status = store.set_codex_remaining({"remainingPercent": 18})

            self.assertEqual(settings_status["settings"]["eventTtlSeconds"], 30)
            self.assertEqual(settings_status["settings"]["lowCodexPercent"], 19)
            self.assertEqual(event_status["event"]["message"], "BUILD OK")
            self.assertEqual(event_status["event"]["ttlSeconds"], 30)
            self.assertEqual(codex_status["event"]["message"], "LOW CODEX 18%")

    def test_clear_event_removes_current_prompt(self):
        with TemporaryDirectory() as tmp:
            clock = FakeClock(datetime(2026, 6, 30, 10, 0, tzinfo=timezone.utc))
            store = AssistantStatusStore(Path(tmp) / "assistant_status.json", now_provider=clock)

            store.assistant_event({"type": "waiting"})
            cleared = store.clear_event()

            self.assertEqual(cleared["event"]["active"], False)
            self.assertEqual(cleared["event"]["message"], "")

    def test_codex_remaining_accepts_percent_and_clear(self):
        with TemporaryDirectory() as tmp:
            clock = FakeClock(datetime(2026, 6, 30, 10, 0, tzinfo=timezone.utc))
            store = AssistantStatusStore(Path(tmp) / "assistant_status.json", now_provider=clock)

            set_status = store.set_codex_remaining({"remainingPercent": 64})
            clear_status = store.set_codex_remaining({"available": False})

            self.assertEqual(set_status["codex"]["available"], True)
            self.assertEqual(set_status["codex"]["remainingPercent"], 64)
            self.assertEqual(set_status["codex"]["source"], "manual")
            self.assertEqual(clear_status["codex"]["available"], False)
            self.assertIsNone(clear_status["codex"]["remainingPercent"])

    def test_codex_remaining_rejects_invalid_percent(self):
        with TemporaryDirectory() as tmp:
            clock = FakeClock(datetime(2026, 6, 30, 10, 0, tzinfo=timezone.utc))
            store = AssistantStatusStore(Path(tmp) / "assistant_status.json", now_provider=clock)

            with self.assertRaises(ValueError):
                store.set_codex_remaining({"remainingPercent": 101})

    def test_corrupted_state_falls_back_with_state_issue(self):
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "assistant_status.json"
            path.write_text("{not json", encoding="utf-8")
            clock = FakeClock(datetime(2026, 6, 30, 10, 0, tzinfo=timezone.utc))
            store = AssistantStatusStore(path, now_provider=clock)

            status = store.status()

            self.assertEqual(status["ok"], True)
            self.assertIn("stateIssue", status)
            self.assertEqual(status["tasks"]["completedToday"], 0)
            saved = json.loads(path.read_text(encoding="utf-8"))
            self.assertIn("2026-06-30", saved["days"])


class AssistantStatusApiTest(unittest.TestCase):
    def test_get_status_route_returns_status_payload(self):
        with TemporaryDirectory() as tmp:
            clock = FakeClock(datetime(2026, 6, 30, 21, 42, tzinfo=timezone.utc))
            api = AssistantStatusApi(AssistantStatusStore(Path(tmp) / "assistant_status.json", now_provider=clock))

            status, payload = api.get("/api/assistant/status")

            self.assertEqual(status, 200)
            self.assertEqual(payload["ok"], True)
            self.assertEqual(payload["date"], "2026-06-30")

    def test_post_routes_return_updated_status(self):
        with TemporaryDirectory() as tmp:
            clock = FakeClock(datetime(2026, 6, 30, 9, 0, tzinfo=timezone.utc))
            api = AssistantStatusApi(AssistantStatusStore(Path(tmp) / "assistant_status.json", now_provider=clock))

            task_status, task_payload = api.post("/api/assistant/tasks/done", {"count": 2})
            codex_status, codex_payload = api.post("/api/assistant/codex/remaining", {"remainingPercent": 32})
            event_status, event_payload = api.post("/api/assistant/events", {"type": "waiting"})
            settings_status, settings_payload = api.post("/api/assistant/settings", {"eventTtlSeconds": 24})
            clear_status, clear_payload = api.post("/api/assistant/events/clear", {})
            stop_status, stop_payload = api.post("/api/assistant/coding/stop", {})
            start_status, start_payload = api.post("/api/assistant/coding/start", {})

            self.assertEqual(task_status, 200)
            self.assertEqual(task_payload["tasks"]["completedToday"], 2)
            self.assertEqual(codex_status, 200)
            self.assertEqual(codex_payload["codex"]["remainingPercent"], 32)
            self.assertEqual(event_status, 200)
            self.assertEqual(event_payload["event"]["message"], "WAIT CONFIRM")
            self.assertEqual(settings_status, 200)
            self.assertEqual(settings_payload["settings"]["eventTtlSeconds"], 24)
            self.assertEqual(clear_status, 200)
            self.assertEqual(clear_payload["event"]["active"], False)
            self.assertEqual(stop_status, 200)
            self.assertEqual(stop_payload["coding"]["active"], False)
            self.assertEqual(start_status, 200)
            self.assertEqual(start_payload["coding"]["active"], True)

    def test_bad_post_returns_json_error(self):
        with TemporaryDirectory() as tmp:
            clock = FakeClock(datetime(2026, 6, 30, 9, 0, tzinfo=timezone.utc))
            api = AssistantStatusApi(AssistantStatusStore(Path(tmp) / "assistant_status.json", now_provider=clock))

            status, payload = api.post("/api/assistant/codex/remaining", {"remainingPercent": -1})

            self.assertEqual(status, 400)
            self.assertEqual(payload["ok"], False)
            self.assertIn("remainingPercent", payload["error"])

    def test_unknown_route_returns_not_found(self):
        with TemporaryDirectory() as tmp:
            api = AssistantStatusApi(AssistantStatusStore(Path(tmp) / "assistant_status.json"))

            status, payload = api.get("/api/nope")

            self.assertEqual(status, 404)
            self.assertEqual(payload, {"ok": False, "error": "not found"})

    def test_server_defaults_to_port_8770(self):
        self.assertEqual(run_server.__defaults__[1], 8770)

    def test_request_handler_supports_cors_preflight_for_management_panel(self):
        handler = make_handler(AssistantStatusStore())

        self.assertTrue(hasattr(handler, "do_OPTIONS"))
