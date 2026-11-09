import unittest

from host.robot_action_router import ActionRouter, RobotActionRejected, allowed_action_ids


class RecordingRobotClient:
    def __init__(self):
        self.calls = []

    def rgb(self, color, index=None):
        self.calls.append(("rgb", color, index))
        return {"ok": True}

    def face_set(self, mood, color):
        self.calls.append(("face_set", mood, color))
        return {"ok": True}

    def motion_pose(self, yaw, pitch, speed):
        self.calls.append(("motion_pose", yaw, pitch, speed))
        return {"ok": True}

    def motion_home(self, speed=300):
        self.calls.append(("motion_home", speed))
        return {"ok": True}

    def emotion_play(self, name, lights=True, sound=False):
        self.calls.append(("emotion_play", name, lights, sound))
        return {"ok": True, "action": name}

    def asset_invoke(self, asset_id, **kwargs):
        self.calls.append(("asset_invoke", asset_id, kwargs))
        return {"ok": True, "id": asset_id}


class StaticClock:
    def __init__(self, now=100.0):
        self.now = float(now)

    def __call__(self):
        return self.now

    def advance(self, seconds):
        self.now += float(seconds)


class ActionRouterTest(unittest.TestCase):
    def test_primary_allowed_actions_are_firmware_asset_ids(self):
        action_ids = allowed_action_ids()

        self.assertIn("behavior.robot_action.nod_yes", action_ids)
        self.assertIn("behavior.robot_action.shake_no", action_ids)
        self.assertIn("behavior.emotion.confused", action_ids)
        self.assertNotIn("nod_yes", action_ids)
        self.assertNotIn("tilt_confused", action_ids)
        for action_id in action_ids:
            self.assertTrue(
                action_id.startswith(("behavior.", "expression.", "motion.", "light.")),
                action_id,
            )

    def test_nod_yes_plays_firmware_action_asset(self):
        client = RecordingRobotClient()
        router = ActionRouter(client, clock=StaticClock())

        result = router.route({"action_id": "nod_yes", "intensity": 0.8, "duration_ms": 1200})

        self.assertEqual(result["ok"], True)
        self.assertEqual(result["action_id"], "behavior.robot_action.nod_yes")
        self.assertEqual(result["asset_id"], "behavior.robot_action.nod_yes")
        self.assertEqual(result["legacy_action_id"], "nod_yes")
        self.assertEqual(client.calls, [("asset_invoke", "behavior.robot_action.nod_yes", {"lights": True, "sound": False})])

    def test_shake_no_plays_firmware_action_asset(self):
        client = RecordingRobotClient()
        router = ActionRouter(client, clock=StaticClock())

        result = router.route({"action_id": "shake_no", "intensity": 0.8, "duration_ms": 1200})

        self.assertEqual(result["ok"], True)
        self.assertEqual(result["action_id"], "behavior.robot_action.shake_no")
        self.assertEqual(result["asset_id"], "behavior.robot_action.shake_no")
        self.assertEqual(result["legacy_action_id"], "shake_no")
        self.assertEqual(client.calls, [("asset_invoke", "behavior.robot_action.shake_no", {"lights": True, "sound": False})])

    def test_firmware_asset_id_routes_directly(self):
        client = RecordingRobotClient()
        router = ActionRouter(client, clock=StaticClock())

        result = router.route({"asset_id": "behavior.emotion.thinking", "intensity": 0.8, "duration_ms": 1200})

        self.assertEqual(result["ok"], True)
        self.assertEqual(result["action_id"], "behavior.emotion.thinking")
        self.assertEqual(result["asset_id"], "behavior.emotion.thinking")
        self.assertNotIn("legacy_action_id", result)
        self.assertEqual(client.calls, [("asset_invoke", "behavior.emotion.thinking", {"lights": True, "sound": False})])

    def test_legacy_confused_alias_routes_to_existing_firmware_asset(self):
        client = RecordingRobotClient()
        router = ActionRouter(client, clock=StaticClock())

        result = router.route({"action_id": "tilt_confused", "intensity": 0.8, "duration_ms": 1200})

        self.assertEqual(result["ok"], True)
        self.assertEqual(result["action_id"], "behavior.emotion.confused")
        self.assertEqual(result["asset_id"], "behavior.emotion.confused")
        self.assertEqual(result["legacy_action_id"], "tilt_confused")
        self.assertEqual(client.calls, [("asset_invoke", "behavior.emotion.confused", {"lights": True, "sound": False})])

    def test_unknown_action_is_rejected_without_robot_calls(self):
        client = RecordingRobotClient()
        router = ActionRouter(client, clock=StaticClock())

        with self.assertRaises(RobotActionRejected):
            router.route({"action_id": "dance_freeform"})

        self.assertEqual(client.calls, [])

    def test_intensity_and_duration_are_clamped_in_result(self):
        client = RecordingRobotClient()
        router = ActionRouter(client, clock=StaticClock())

        result = router.route({"action_id": "behavior.emotion.thinking", "intensity": 5.0, "duration_ms": 999999})

        self.assertEqual(result["intensity"], 1.0)
        self.assertEqual(result["duration_ms"], 3000)

    def test_cooldown_blocks_repeated_action(self):
        client = RecordingRobotClient()
        clock = StaticClock()
        router = ActionRouter(client, clock=clock)

        router.route({"action_id": "nod_yes"})
        with self.assertRaises(RobotActionRejected):
            router.route({"action_id": "nod_yes"})

        clock.advance(2.1)
        result = router.route({"action_id": "nod_yes"})
        self.assertEqual(result["ok"], True)

    def test_bypass_cooldown_allows_realtime_stream_to_repeat_actions_immediately(self):
        client = RecordingRobotClient()
        router = ActionRouter(client, clock=StaticClock())

        first = router.route({"action_id": "nod_yes", "bypass_cooldown": True})
        second = router.route({"action_id": "nod_yes", "bypass_cooldown": True})

        self.assertEqual(first["ok"], True)
        self.assertEqual(second["ok"], True)
        self.assertEqual(len(client.calls), 2)

    def test_context_is_passed_without_blocking_generic_yes_actions(self):
        client = RecordingRobotClient()
        router = ActionRouter(client, clock=StaticClock())

        result = router.route({"action_id": "nod_yes"}, context={"known_user": None})
        self.assertEqual(result["ok"], True)


if __name__ == "__main__":
    unittest.main()
