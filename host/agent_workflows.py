import os

try:
    from host.host_vision import HostVisionTracker
    from host.robot_action_router import ActionRouter, RobotActionRejected, canonical_action_id
    from host.robot_client import RobotClient
except ModuleNotFoundError:
    from host_vision import HostVisionTracker
    from robot_action_router import ActionRouter, RobotActionRejected, canonical_action_id
    from robot_client import RobotClient


DEFAULT_ROBOT_URL = "http://deskbot.local"
DEFAULT_EYE_COLOR = "#00f2ff"

CODING_EVENT_BEHAVIORS = {
    "task_done": "behavior.emotion.happy",
    "test_ok": "behavior.emotion.happy",
    "build_ok": "behavior.emotion.happy",
    "test_failed": "behavior.emotion.confused",
    "build_failed": "behavior.emotion.confused",
    "waiting": "behavior.emotion.thinking",
    "blocked": "behavior.emotion.thinking",
}

YES_ACTION_ID = "behavior.robot_action.nod_yes"
NO_ACTION_ID = "behavior.robot_action.shake_no"

YES_COMMANDS = {
    "yes",
    "y",
    "true",
    "ok",
    "okay",
    "nod",
    "nod_yes",
    "点头",
    "是",
    "对",
    "对的",
    "好的",
    "可以",
    "同意",
}

NO_COMMANDS = {
    "no",
    "n",
    "false",
    "shake",
    "shake_no",
    "摇头",
    "不是",
    "不对",
    "不要",
    "不可以",
    "否",
    "不同意",
}


def clamp(value, low, high):
    return max(low, min(high, value))


def int_value(payload, key, default):
    try:
        return int((payload or {}).get(key, default))
    except (TypeError, ValueError):
        return int(default)


def bool_value(payload, key, default=False):
    value = (payload or {}).get(key, default)
    if isinstance(value, bool):
        return value
    if isinstance(value, (int, float)):
        return value != 0
    return str(value).strip().lower() in {"1", "true", "yes", "on", "开"}


def string_value(payload, key, default=""):
    value = (payload or {}).get(key, default)
    if value is None:
        return default
    return str(value)


class AssistantStatusClient:
    def __init__(self, base_url):
        self.base_url = (base_url or "").rstrip("/")

    def post_event(self, payload):
        if not self.base_url:
            return {"ok": True, "skipped": True, "reason": "assistant status url not configured"}
        client = RobotClient(self.base_url)
        event_type = string_value(payload, "type").strip()
        if event_type == "task_done":
            return client.post("/api/assistant/tasks/done", {"count": int_value(payload, "count", 1)})
        if event_type == "codex_low":
            return client.post(
                "/api/assistant/codex/remaining",
                {"remainingPercent": int_value(payload, "remainingPercent", 0)},
            )
        return client.post("/api/assistant/events", payload or {})


class RobotAgentWorkflowService:
    def __init__(
        self,
        robot_client,
        vision_tracker=None,
        assistant_status_client=None,
        action_router=None,
        sleep_fn=None,
        allowed_asset_ids=None,
    ):
        self.robot_client = robot_client
        self.vision_tracker = vision_tracker
        self.assistant_status_client = assistant_status_client
        self.action_router = action_router or ActionRouter(robot_client)
        self.sleep_fn = sleep_fn
        self.allowed_asset_ids = allowed_asset_ids

    def _ok(self, workflow, **fields):
        payload = {"ok": True, "workflow": workflow}
        payload.update(fields)
        return payload

    def _error(self, workflow, stage, error, recoverable=True, **fields):
        payload = {
            "ok": False,
            "workflow": workflow,
            "stage": stage,
            "error": str(error),
            "recoverable": bool(recoverable),
        }
        payload.update(fields)
        return payload

    def _allowed_asset_set(self):
        if self.allowed_asset_ids is None:
            return None
        values = self.allowed_asset_ids() if callable(self.allowed_asset_ids) else self.allowed_asset_ids
        return {str(value).strip() for value in (values or []) if str(value).strip()}

    def _is_asset_allowed(self, asset_id):
        allowed = self._allowed_asset_set()
        return allowed is None or str(asset_id or "").strip() in allowed

    def _asset_not_allowed(self, workflow, asset_id):
        return self._error(
            workflow,
            "agent_whitelist",
            f"{asset_id} is not allowed by agent asset whitelist",
            recoverable=False,
        )

    def robot_status(self, payload=None):
        try:
            return self._ok("robot_status", robot=self.robot_client.status())
        except Exception as exc:
            return self._error("robot_status", "robot_status", exc)

    def robot_catalog(self, payload=None):
        try:
            return self._ok("robot_catalog", catalog=self.robot_client.catalog())
        except Exception as exc:
            return self._error("robot_catalog", "catalog", exc)

    def set_expression(self, payload):
        mood = string_value(payload, "mood").strip()
        if not mood:
            return self._error("set_expression", "input", "mood is required")
        color = string_value(payload, "color", DEFAULT_EYE_COLOR).strip() or DEFAULT_EYE_COLOR
        try:
            return self._ok("set_expression", robot=self.robot_client.face_set(mood, color))
        except Exception as exc:
            return self._error("set_expression", "robot", exc)

    def move_pose(self, payload):
        yaw = clamp(int_value(payload, "yaw", 0), -1280, 1280)
        pitch = clamp(int_value(payload, "pitch", 120), 0, 900)
        speed = clamp(int_value(payload, "speed", 300), 0, 1000)
        try:
            return self._ok("move_pose", robot=self.robot_client.motion_pose(yaw, pitch, speed))
        except Exception as exc:
            return self._error("move_pose", "robot", exc)

    def invoke_asset(self, payload):
        asset_id = string_value(payload, "asset_id") or string_value(payload, "id")
        asset_id = asset_id.strip()
        if not asset_id:
            return self._error("invoke_asset", "input", "asset_id is required")
        if not asset_id.startswith(("expression.", "behavior.", "motion.", "sound.", "light.", "screen.")):
            return self._error("invoke_asset", "input", f"{asset_id} is not invokable by agent tools")
        if asset_id.startswith("logic."):
            return self._error("invoke_asset", "input", f"{asset_id} is not invokable by agent tools")
        if not self._is_asset_allowed(asset_id):
            return self._asset_not_allowed("invoke_asset", asset_id)

        duration_ms = (payload or {}).get("duration_ms")
        if duration_ms is None:
            duration_ms = (payload or {}).get("durationMs")
        kwargs = {
            "duration_ms": int(duration_ms) if duration_ms is not None else None,
            "lights": (payload or {}).get("lights"),
            "sound": (payload or {}).get("sound"),
        }
        try:
            return self._ok("invoke_asset", robot=self.robot_client.asset_invoke(asset_id, **kwargs))
        except Exception as exc:
            return self._error("invoke_asset", "robot", exc)

    def invoke_safe_behavior(self, payload):
        payload = payload or {}
        requested_action_id = string_value(payload, "asset_id") or string_value(payload, "action_id")
        action_id = canonical_action_id(requested_action_id.strip())
        if action_id and not self._is_asset_allowed(action_id):
            return self._asset_not_allowed("invoke_safe_behavior", action_id)
        try:
            action = self.action_router.route(payload)
            return self._ok("invoke_safe_behavior", action=action)
        except RobotActionRejected as exc:
            return self._error("invoke_safe_behavior", "behavior", exc)
        except Exception as exc:
            return self._error("invoke_safe_behavior", "robot", exc)

    def _dispatch_action_id(self, payload):
        payload = payload or {}
        command = (
            string_value(payload, "action_id")
            or string_value(payload, "asset_id")
            or string_value(payload, "command")
            or string_value(payload, "intent")
        )
        normalized = command.strip().lower().replace("-", "_")
        if normalized in {YES_ACTION_ID, "behavior_robot_action_nod_yes"} or normalized in YES_COMMANDS:
            return YES_ACTION_ID
        if normalized in {NO_ACTION_ID, "behavior_robot_action_shake_no"} or normalized in NO_COMMANDS:
            return NO_ACTION_ID
        return ""

    def dispatch_action(self, payload):
        payload = payload or {}
        action_id = self._dispatch_action_id(payload)
        if not action_id:
            command = string_value(payload, "command") or string_value(payload, "action_id") or string_value(payload, "intent")
            return self._error("dispatch_action", "input", f"unsupported dispatch command: {command}")
        if not self._is_asset_allowed(action_id):
            return self._asset_not_allowed("dispatch_action", action_id)

        behavior_payload = {
            "asset_id": action_id,
            "bypass_cooldown": bool_value(payload, "bypass_cooldown", bool_value(payload, "bypassCooldown", False)),
        }
        behavior = self.invoke_safe_behavior(behavior_payload)
        return self._ok(
            "dispatch_action",
            dispatch={
                "action_id": action_id,
                "source": string_value(payload, "source", "agent_dispatch") or "agent_dispatch",
            },
            behavior=behavior,
        )

    def nod_yes(self, payload=None):
        payload = dict(payload or {})
        payload["command"] = "nod_yes"
        return self.dispatch_action(payload)

    def shake_no(self, payload=None):
        payload = dict(payload or {})
        payload["command"] = "shake_no"
        return self.dispatch_action(payload)

    def _vision_payload(self, payload):
        payload = payload or {}
        return {
            "minConfidence": float(payload.get("min_confidence", payload.get("minConfidence", 0.55))),
            "disableFirmwareLocal": bool_value(payload, "disable_firmware_local", True),
            "debugSubtitle": bool_value(payload, "debug_subtitle", False),
            "force": True,
        }

    def camera_step_detect(self, payload):
        if self.vision_tracker is None:
            return self._error("camera_step_detect", "vision", "vision tracker not configured")
        try:
            return self._ok("camera_step_detect", vision=self.vision_tracker.step(self._vision_payload(payload)))
        except Exception as exc:
            return self._error("camera_step_detect", "vision", exc)

    def face_detect_and_center(self, payload):
        if self.vision_tracker is None:
            return self._error("face_detect_and_center", "vision", "vision tracker not configured")

        max_steps = clamp(int_value(payload, "max_steps", int_value(payload, "maxSteps", 3)), 1, 5)
        interval_ms = clamp(int_value(payload, "interval_ms", int_value(payload, "intervalMs", 500)), 0, 5000)
        results = []

        for step_index in range(max_steps):
            step = self.camera_step_detect(payload)
            results.append(step)
            vision = step.get("vision", {})
            if not step.get("ok", False):
                return self._error(
                    "face_detect_and_center",
                    step.get("stage", "vision"),
                    step.get("error", "vision step failed"),
                    steps=step_index + 1,
                    results=results,
                )
            if vision.get("faceDetected"):
                return self._ok(
                    "face_detect_and_center",
                    faceDetected=True,
                    steps=step_index + 1,
                    vision=vision,
                    results=results,
                )
            if interval_ms and step_index < max_steps - 1 and self.sleep_fn is not None:
                self.sleep_fn(interval_ms / 1000.0)

        return self._ok(
            "face_detect_and_center",
            faceDetected=False,
            steps=max_steps,
            results=results,
        )

    def find_and_greet(self, payload):
        payload = payload or {}
        detection = self.face_detect_and_center(payload)
        if not detection.get("ok", False):
            return self._error(
                "find_and_greet",
                detection.get("stage", "vision"),
                detection.get("error", "detection failed"),
                detection=detection,
            )

        behavior = {}
        if detection.get("faceDetected"):
            asset_id = string_value(
                payload,
                "behavior_asset_id",
                string_value(payload, "behaviorAssetId", "behavior.emotion.happy"),
            )
            behavior = self.invoke_safe_behavior({"asset_id": asset_id, "bypass_cooldown": True})
        speech = {"ok": True, "skipped": True, "reason": "speech adapter not configured in version 1"}
        return self._ok("find_and_greet", detection=detection, behavior=behavior, speech=speech)

    def coding_event(self, payload):
        payload = payload or {}
        event_type = string_value(payload, "type").strip().lower().replace("-", "_")
        if not event_type:
            return self._error("coding_event", "input", "type is required")
        if event_type not in CODING_EVENT_BEHAVIORS and event_type != "codex_low":
            return self._error("coding_event", "input", f"unsupported coding event: {event_type}")

        normalized = dict(payload)
        normalized["type"] = event_type
        if self.assistant_status_client is None:
            assistant = {"ok": True, "skipped": True, "reason": "assistant status client not configured"}
        else:
            try:
                assistant = self.assistant_status_client.post_event(normalized)
            except Exception as exc:
                assistant = {"ok": False, "error": str(exc)}

        behavior = {}
        asset_id = CODING_EVENT_BEHAVIORS.get(event_type)
        if asset_id:
            behavior = self.invoke_safe_behavior({"asset_id": asset_id, "bypass_cooldown": True})
        return self._ok("coding_event", event=normalized, assistant=assistant, behavior=behavior)


def build_workflow_service(robot_base_url=None, assistant_status_url=None):
    robot_url = robot_base_url or os.environ.get("ROBOT_BASE_URL") or DEFAULT_ROBOT_URL
    robot = RobotClient(robot_url)
    assistant_url = assistant_status_url or os.environ.get("ROBOT_AGENT_ASSISTANT_STATUS_URL", "")
    assistant = AssistantStatusClient(assistant_url) if assistant_url else None
    return RobotAgentWorkflowService(
        robot,
        vision_tracker=HostVisionTracker(robot),
        assistant_status_client=assistant,
    )
