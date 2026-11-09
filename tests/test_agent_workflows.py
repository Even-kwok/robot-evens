import unittest

from host.agent_workflows import RobotAgentWorkflowService


class RecordingRobotClient:
    def __init__(self):
        self.calls = []

    def status(self):
        self.calls.append(("status",))
        return {"ok": True, "hardwareReady": True}

    def catalog(self):
        self.calls.append(("catalog",))
        return {"ok": True, "version": 3, "categories": {}}

    def face_set(self, mood, color):
        self.calls.append(("face_set", mood, color))
        return {"ok": True, "mood": mood, "color": color}

    def motion_pose(self, yaw, pitch, speed):
        self.calls.append(("motion_pose", yaw, pitch, speed))
        return {"ok": True, "motion": {"yaw": yaw, "pitch": pitch, "speed": speed}}

    def asset_invoke(self, asset_id, **kwargs):
        self.calls.append(("asset_invoke", asset_id, kwargs))
        return {"ok": True, "id": asset_id, "kwargs": kwargs}


class ErrorRobotClient(RecordingRobotClient):
    def status(self):
        raise RuntimeError("robot offline")


class AgentWorkflowFoundationTest(unittest.TestCase):
    def test_robot_status_wraps_robot_client_status(self):
        robot = RecordingRobotClient()
        service = RobotAgentWorkflowService(robot)

        result = service.robot_status()

        self.assertEqual(result["ok"], True)
        self.assertEqual(result["workflow"], "robot_status")
        self.assertEqual(result["robot"]["hardwareReady"], True)
        self.assertEqual(robot.calls, [("status",)])

    def test_robot_catalog_wraps_robot_client_catalog(self):
        robot = RecordingRobotClient()
        service = RobotAgentWorkflowService(robot)

        result = service.robot_catalog()

        self.assertEqual(result["ok"], True)
        self.assertEqual(result["workflow"], "robot_catalog")
        self.assertEqual(result["catalog"]["version"], 3)
        self.assertEqual(robot.calls, [("catalog",)])

    def test_set_expression_defaults_color_and_calls_face_set(self):
        robot = RecordingRobotClient()
        service = RobotAgentWorkflowService(robot)

        result = service.set_expression({"mood": "happy"})

        self.assertEqual(result["ok"], True)
        self.assertEqual(result["workflow"], "set_expression")
        self.assertEqual(result["robot"]["mood"], "happy")
        self.assertEqual(robot.calls, [("face_set", "happy", "#00f2ff")])

    def test_set_expression_rejects_missing_mood_without_robot_call(self):
        robot = RecordingRobotClient()
        service = RobotAgentWorkflowService(robot)

        result = service.set_expression({})

        self.assertEqual(result["ok"], False)
        self.assertEqual(result["workflow"], "set_expression")
        self.assertEqual(result["stage"], "input")
        self.assertIn("mood", result["error"])
        self.assertEqual(robot.calls, [])

    def test_move_pose_clamps_values_before_calling_robot(self):
        robot = RecordingRobotClient()
        service = RobotAgentWorkflowService(robot)

        result = service.move_pose({"yaw": 9999, "pitch": -5, "speed": 5000})

        self.assertEqual(result["ok"], True)
        self.assertEqual(result["workflow"], "move_pose")
        self.assertEqual(robot.calls, [("motion_pose", 1280, 0, 1000)])

    def test_robot_errors_are_structured(self):
        service = RobotAgentWorkflowService(ErrorRobotClient())

        result = service.robot_status()

        self.assertEqual(result["ok"], False)
        self.assertEqual(result["workflow"], "robot_status")
        self.assertEqual(result["stage"], "robot_status")
        self.assertEqual(result["error"], "robot offline")
        self.assertEqual(result["recoverable"], True)


class RecordingAssistantStatusClient:
    def __init__(self):
        self.calls = []

    def post_event(self, payload):
        self.calls.append(dict(payload))
        return {"ok": True, "event": payload}


class AgentWorkflowBehaviorTest(unittest.TestCase):
    def test_dispatch_yes_no_command_routes_to_nod_behavior(self):
        robot = RecordingRobotClient()
        service = RobotAgentWorkflowService(robot)

        result = service.dispatch_action({"command": "点头", "bypass_cooldown": True})

        self.assertEqual(result["ok"], True)
        self.assertEqual(result["workflow"], "dispatch_action")
        self.assertEqual(result["dispatch"]["action_id"], "behavior.robot_action.nod_yes")
        self.assertEqual(result["behavior"]["action"]["action_id"], "behavior.robot_action.nod_yes")
        self.assertEqual(robot.calls, [("asset_invoke", "behavior.robot_action.nod_yes", {"lights": True, "sound": False})])

    def test_dispatch_yes_no_command_routes_to_shake_behavior(self):
        robot = RecordingRobotClient()
        service = RobotAgentWorkflowService(robot)

        result = service.dispatch_action({"command": "不是", "bypass_cooldown": True})

        self.assertEqual(result["ok"], True)
        self.assertEqual(result["workflow"], "dispatch_action")
        self.assertEqual(result["dispatch"]["action_id"], "behavior.robot_action.shake_no")
        self.assertEqual(result["behavior"]["action"]["action_id"], "behavior.robot_action.shake_no")
        self.assertEqual(robot.calls, [("asset_invoke", "behavior.robot_action.shake_no", {"lights": True, "sound": False})])

    def test_dispatch_rejects_unknown_command_without_robot_call(self):
        robot = RecordingRobotClient()
        service = RobotAgentWorkflowService(robot)

        result = service.dispatch_action({"command": "跳舞"})

        self.assertEqual(result["ok"], False)
        self.assertEqual(result["workflow"], "dispatch_action")
        self.assertEqual(result["stage"], "input")
        self.assertIn("unsupported", result["error"])
        self.assertEqual(robot.calls, [])

    def test_invoke_asset_allows_safe_expression_asset(self):
        robot = RecordingRobotClient()
        service = RobotAgentWorkflowService(robot)

        result = service.invoke_asset({"asset_id": "expression.happy", "duration_ms": 900})

        self.assertEqual(result["ok"], True)
        self.assertEqual(result["workflow"], "invoke_asset")
        self.assertEqual(
            robot.calls,
            [("asset_invoke", "expression.happy", {"duration_ms": 900, "lights": None, "sound": None})],
        )

    def test_invoke_asset_rejects_logic_assets_without_robot_call(self):
        robot = RecordingRobotClient()
        service = RobotAgentWorkflowService(robot)

        result = service.invoke_asset({"asset_id": "logic.idle.normal"})

        self.assertEqual(result["ok"], False)
        self.assertEqual(result["stage"], "input")
        self.assertIn("not invokable", result["error"])
        self.assertEqual(robot.calls, [])

    def test_invoke_safe_behavior_routes_through_action_router(self):
        robot = RecordingRobotClient()
        service = RobotAgentWorkflowService(robot)

        result = service.invoke_safe_behavior({"action_id": "nod_yes", "bypass_cooldown": True})

        self.assertEqual(result["ok"], True)
        self.assertEqual(result["workflow"], "invoke_safe_behavior")
        self.assertEqual(result["action"]["action_id"], "behavior.robot_action.nod_yes")
        self.assertEqual(robot.calls, [("asset_invoke", "behavior.robot_action.nod_yes", {"lights": True, "sound": False})])

    def test_invoke_safe_behavior_rejects_unknown_action_without_robot_call(self):
        robot = RecordingRobotClient()
        service = RobotAgentWorkflowService(robot)

        result = service.invoke_safe_behavior({"action_id": "dance"})

        self.assertEqual(result["ok"], False)
        self.assertEqual(result["stage"], "behavior")
        self.assertIn("unknown action", result["error"])
        self.assertEqual(robot.calls, [])

    def test_agent_asset_whitelist_blocks_unlisted_dispatch_action(self):
        robot = RecordingRobotClient()
        service = RobotAgentWorkflowService(
            robot,
            allowed_asset_ids=lambda: {"behavior.robot_action.nod_yes"},
        )

        result = service.dispatch_action({"command": "shake_no", "bypass_cooldown": True})

        self.assertEqual(result["ok"], False)
        self.assertEqual(result["workflow"], "dispatch_action")
        self.assertEqual(result["stage"], "agent_whitelist")
        self.assertIn("not allowed", result["error"])
        self.assertEqual(robot.calls, [])

    def test_agent_asset_whitelist_allows_listed_generic_asset(self):
        robot = RecordingRobotClient()
        service = RobotAgentWorkflowService(
            robot,
            allowed_asset_ids=lambda: {"behavior.emotion.happy"},
        )

        result = service.invoke_asset({"asset_id": "behavior.emotion.happy"})

        self.assertEqual(result["ok"], True)
        self.assertEqual(robot.calls, [("asset_invoke", "behavior.emotion.happy", {"duration_ms": None, "lights": None, "sound": None})])

    def test_coding_event_posts_assistant_event_and_invokes_matching_behavior(self):
        robot = RecordingRobotClient()
        assistant = RecordingAssistantStatusClient()
        service = RobotAgentWorkflowService(robot, assistant_status_client=assistant)

        result = service.coding_event({"type": "test_failed"})

        self.assertEqual(result["ok"], True)
        self.assertEqual(result["workflow"], "coding_event")
        self.assertEqual(assistant.calls, [{"type": "test_failed"}])
        self.assertEqual(robot.calls, [("asset_invoke", "behavior.emotion.confused", {"lights": True, "sound": False})])

    def test_coding_event_without_assistant_status_still_invokes_behavior(self):
        robot = RecordingRobotClient()
        service = RobotAgentWorkflowService(robot)

        result = service.coding_event({"type": "task_done", "count": 2})

        self.assertEqual(result["ok"], True)
        self.assertEqual(result["assistant"]["skipped"], True)
        self.assertEqual(robot.calls, [("asset_invoke", "behavior.emotion.happy", {"lights": True, "sound": False})])


class RecordingVisionTracker:
    def __init__(self, results):
        self.results = list(results)
        self.calls = []

    def step(self, payload):
        self.calls.append(dict(payload))
        if self.results:
            return self.results.pop(0)
        return {"ok": True, "faceDetected": False}


class AgentWorkflowVisionTest(unittest.TestCase):
    def test_camera_step_detect_calls_vision_tracker_once(self):
        robot = RecordingRobotClient()
        vision = RecordingVisionTracker([{"ok": True, "faceDetected": True, "face": {"confidence": 0.8}}])
        service = RobotAgentWorkflowService(robot, vision_tracker=vision)

        result = service.camera_step_detect({"min_confidence": 0.7, "debug_subtitle": True})

        self.assertEqual(result["ok"], True)
        self.assertEqual(result["workflow"], "camera_step_detect")
        self.assertEqual(result["vision"]["faceDetected"], True)
        self.assertEqual(
            vision.calls,
            [{"minConfidence": 0.7, "disableFirmwareLocal": True, "debugSubtitle": True, "force": True}],
        )

    def test_camera_step_detect_reports_missing_tracker(self):
        service = RobotAgentWorkflowService(RecordingRobotClient())

        result = service.camera_step_detect({})

        self.assertEqual(result["ok"], False)
        self.assertEqual(result["stage"], "vision")
        self.assertIn("not configured", result["error"])

    def test_face_detect_and_center_stops_when_face_is_found(self):
        robot = RecordingRobotClient()
        vision = RecordingVisionTracker([
            {"ok": True, "faceDetected": False},
            {"ok": True, "faceDetected": True, "face": {"confidence": 0.9}},
        ])
        service = RobotAgentWorkflowService(robot, vision_tracker=vision)

        result = service.face_detect_and_center({"max_steps": 5, "interval_ms": 0})

        self.assertEqual(result["ok"], True)
        self.assertEqual(result["workflow"], "face_detect_and_center")
        self.assertEqual(result["faceDetected"], True)
        self.assertEqual(result["steps"], 2)
        self.assertEqual(len(vision.calls), 2)

    def test_face_detect_and_center_is_bounded_when_no_face_is_found(self):
        robot = RecordingRobotClient()
        vision = RecordingVisionTracker([
            {"ok": True, "faceDetected": False},
            {"ok": True, "faceDetected": False},
            {"ok": True, "faceDetected": False},
            {"ok": True, "faceDetected": False},
        ])
        service = RobotAgentWorkflowService(robot, vision_tracker=vision)

        result = service.face_detect_and_center({"max_steps": 99, "interval_ms": 0})

        self.assertEqual(result["ok"], True)
        self.assertEqual(result["faceDetected"], False)
        self.assertEqual(result["steps"], 5)
        self.assertEqual(len(vision.calls), 5)

    def test_find_and_greet_invokes_behavior_when_face_is_found(self):
        robot = RecordingRobotClient()
        vision = RecordingVisionTracker([{"ok": True, "faceDetected": True, "face": {"confidence": 0.9}}])
        service = RobotAgentWorkflowService(robot, vision_tracker=vision)

        result = service.find_and_greet({"behavior_asset_id": "behavior.emotion.happy"})

        self.assertEqual(result["ok"], True)
        self.assertEqual(result["workflow"], "find_and_greet")
        self.assertEqual(result["detection"]["faceDetected"], True)
        self.assertEqual(result["speech"]["skipped"], True)
        self.assertEqual(robot.calls, [("asset_invoke", "behavior.emotion.happy", {"lights": True, "sound": False})])


if __name__ == "__main__":
    unittest.main()
