from dataclasses import dataclass
import time


def clamp(value, low, high):
    return max(low, min(high, value))


class RobotActionRejected(ValueError):
    pass


@dataclass(frozen=True)
class MotionPose:
    yaw: int
    pitch: int
    speed: int


@dataclass(frozen=True)
class RobotActionDefinition:
    action_id: str
    label: str
    mood: str
    color: str
    poses: tuple
    asset_id: str = ""
    min_cooldown_ms: int = 1000
    min_duration_ms: int = 300
    max_duration_ms: int = 3000
    home_after: bool = False


DEFAULT_ACTIONS = {
    "behavior.robot_action.nod_yes": RobotActionDefinition(
        action_id="behavior.robot_action.nod_yes",
        label="Nod yes",
        mood="happy",
        color="#00f2ff",
        poses=(MotionPose(0, 70, 420), MotionPose(0, 150, 420), MotionPose(0, 105, 360)),
        asset_id="behavior.robot_action.nod_yes",
        min_cooldown_ms=2000,
    ),
    "behavior.robot_action.shake_no": RobotActionDefinition(
        action_id="behavior.robot_action.shake_no",
        label="Shake no",
        mood="curious",
        color="#ff5a7a",
        poses=(MotionPose(-180, 110, 420), MotionPose(180, 110, 420), MotionPose(0, 105, 360)),
        asset_id="behavior.robot_action.shake_no",
        min_cooldown_ms=1800,
    ),
    "behavior.emotion.thinking": RobotActionDefinition(
        action_id="behavior.emotion.thinking",
        label="Look thinking",
        mood="thinking",
        color="#ffd166",
        poses=(MotionPose(-90, 120, 260), MotionPose(80, 110, 260), MotionPose(0, 115, 240)),
        asset_id="behavior.emotion.thinking",
        min_cooldown_ms=1200,
    ),
    "behavior.emotion.confused": RobotActionDefinition(
        action_id="behavior.emotion.confused",
        label="Tilt confused",
        mood="confused",
        color="#b388ff",
        poses=(MotionPose(-120, 125, 260), MotionPose(0, 115, 240)),
        asset_id="behavior.emotion.confused",
        min_cooldown_ms=1200,
    ),
    "behavior.emotion.happy": RobotActionDefinition(
        action_id="behavior.emotion.happy",
        label="Happy acknowledgement",
        mood="happy",
        color="#52ffa8",
        poses=(MotionPose(0, 95, 300), MotionPose(0, 120, 260)),
        asset_id="behavior.emotion.happy",
        min_cooldown_ms=1000,
    ),
    "behavior.emotion.shy": RobotActionDefinition(
        action_id="behavior.emotion.shy",
        label="Shy acknowledgement",
        mood="tired",
        color="#ffb3c7",
        poses=(MotionPose(120, 150, 220), MotionPose(0, 125, 220)),
        asset_id="behavior.emotion.shy",
        min_cooldown_ms=1200,
    ),
    "behavior.emotion.surprised": RobotActionDefinition(
        action_id="behavior.emotion.surprised",
        label="Surprised acknowledgement",
        mood="surprised",
        color="#ffffff",
        poses=(MotionPose(0, 60, 500), MotionPose(0, 110, 320)),
        asset_id="behavior.emotion.surprised",
        min_cooldown_ms=1400,
    ),
    "expression.sleeping": RobotActionDefinition(
        action_id="expression.sleeping",
        label="Sleepy ignore",
        mood="sleeping",
        color="#274060",
        poses=(MotionPose(0, 175, 180),),
        asset_id="expression.sleeping",
        min_cooldown_ms=3000,
    ),
    "expression.curious": RobotActionDefinition(
        action_id="expression.curious",
        label="Idle listen",
        mood="curious",
        color="#00f2ff",
        poses=(MotionPose(0, 105, 240),),
        asset_id="expression.curious",
        min_cooldown_ms=600,
    ),
    "expression.neutral": RobotActionDefinition(
        action_id="expression.neutral",
        label="Neutral face",
        mood="neutral",
        color="#00f2ff",
        poses=(),
        asset_id="expression.neutral",
        min_cooldown_ms=300,
    ),
}


LEGACY_ACTION_ALIASES = {
    "nod_yes": "behavior.robot_action.nod_yes",
    "shake_no": "behavior.robot_action.shake_no",
    "look_thinking": "behavior.emotion.thinking",
    "tilt_confused": "behavior.emotion.confused",
    "happy_ack": "behavior.emotion.happy",
    "shy_ack": "behavior.emotion.shy",
    "surprised_ack": "behavior.emotion.surprised",
    "sleepy_ignore": "expression.sleeping",
    "idle_listen": "expression.curious",
    "stop_motion": "expression.neutral",
}


def canonical_action_id(action_id):
    return LEGACY_ACTION_ALIASES.get(action_id, action_id)


class ActionRouter:
    def __init__(self, robot_client, clock=None, actions=None):
        self.robot_client = robot_client
        self.clock = clock or time.monotonic
        self.actions = actions or DEFAULT_ACTIONS
        self.last_action_at = {}

    def route(self, payload, context=None):
        context = context or {}
        requested_action_id = str(payload.get("asset_id") or payload.get("action_id", "")).strip()
        action_id = canonical_action_id(requested_action_id)
        if action_id not in self.actions:
            raise RobotActionRejected(f"unknown action_id: {requested_action_id}")

        definition = self.actions[action_id]
        now = float(self.clock())
        bypass_cooldown = bool(payload.get("bypass_cooldown") or payload.get("bypassCooldown"))
        last = self.last_action_at.get(action_id)
        cooldown_seconds = definition.min_cooldown_ms / 1000.0
        if not bypass_cooldown and last is not None and now - last < cooldown_seconds:
            raise RobotActionRejected(f"{action_id} is cooling down")

        intensity = clamp(float(payload.get("intensity", 0.7)), 0.0, 1.0)
        duration_ms = int(clamp(int(payload.get("duration_ms", 1200)), definition.min_duration_ms, definition.max_duration_ms))

        if definition.asset_id:
            self.robot_client.asset_invoke(definition.asset_id, lights=True, sound=False)
        else:
            self.robot_client.rgb(definition.color)
            self.robot_client.face_set(definition.mood, definition.color)
            for pose in definition.poses:
                self.robot_client.motion_pose(pose.yaw, pose.pitch, pose.speed)
            if definition.home_after:
                self.robot_client.motion_home()

        self.last_action_at[action_id] = now
        result = {
            "ok": True,
            "action_id": action_id,
            "asset_id": definition.asset_id or action_id,
            "label": definition.label,
            "intensity": intensity,
            "duration_ms": duration_ms,
        }
        if requested_action_id != action_id:
            result["legacy_action_id"] = requested_action_id
        return result


def allowed_action_ids():
    return tuple(DEFAULT_ACTIONS.keys())
