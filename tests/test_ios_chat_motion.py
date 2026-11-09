from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]


class IOSChatMotionTest(unittest.TestCase):
    def test_chat_motion_ui_and_view_model_are_wired_to_soft_idle_asset(self):
        app_view_model = (ROOT / "ios/RobotEvensControl/RobotEvensControl/AppViewModel.swift").read_text(encoding="utf-8")
        content_view = (ROOT / "ios/RobotEvensControl/RobotEvensControl/ContentView.swift").read_text(encoding="utf-8")
        robot_action = (ROOT / "ios/RobotEvensCore/Sources/RobotEvensCore/RobotAction.swift").read_text(encoding="utf-8")

        for snippet in [
            "isChatMotionEnabled",
            "chatMotionStyle",
            "currentChatMotionState",
            "chatMotionSummaryText",
            "setChatMotionEnabled",
            "setChatMotionStyle",
            "startChatMotionLoopIfNeeded",
            "stopChatMotionLoop",
            "ChatMotionPlanner.cue",
            "clip.chat_soft_idle",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, app_view_model)

        for snippet in [
            "chatMotionControls",
            "聊天状态",
            "聊天动作",
            "通话风格",
            "viewModel.setChatMotionEnabled",
            "viewModel.setChatMotionStyle",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, content_view)

        for snippet in [
            "ChatMotionPlanner",
            "ChatMotionCallState",
            "ChatMotionStyle",
            "clip.chat_soft_idle",
            "listening",
            "thinking",
            "speaking",
            "settling",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, robot_action)


if __name__ == "__main__":
    unittest.main()
