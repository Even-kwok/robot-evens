import json
import unittest

from host.agent_mcp_server import (
    TOOL_NAMES,
    AgentMcpServer,
    build_tool_definitions,
    parse_jsonrpc_line,
)


class RecordingWorkflowService:
    def __init__(self):
        self.calls = []

    def robot_status(self, payload=None):
        self.calls.append(("robot_status", payload or {}))
        return {"ok": True, "workflow": "robot_status"}

    def robot_catalog(self, payload=None):
        self.calls.append(("robot_catalog", payload or {}))
        return {"ok": True, "workflow": "robot_catalog"}

    def invoke_asset(self, payload):
        self.calls.append(("invoke_asset", payload))
        return {"ok": True, "workflow": "invoke_asset", "payload": payload}

    def invoke_safe_behavior(self, payload):
        self.calls.append(("invoke_safe_behavior", payload))
        return {"ok": True, "workflow": "invoke_safe_behavior", "payload": payload}

    def set_expression(self, payload):
        self.calls.append(("set_expression", payload))
        return {"ok": True, "workflow": "set_expression", "payload": payload}

    def move_pose(self, payload):
        self.calls.append(("move_pose", payload))
        return {"ok": True, "workflow": "move_pose", "payload": payload}

    def camera_step_detect(self, payload):
        self.calls.append(("camera_step_detect", payload))
        return {"ok": True, "workflow": "camera_step_detect", "payload": payload}

    def face_detect_and_center(self, payload):
        self.calls.append(("face_detect_and_center", payload))
        return {"ok": True, "workflow": "face_detect_and_center", "payload": payload}

    def find_and_greet(self, payload):
        self.calls.append(("find_and_greet", payload))
        return {"ok": True, "workflow": "find_and_greet", "payload": payload}

    def dispatch_action(self, payload):
        self.calls.append(("dispatch_action", payload))
        return {"ok": True, "workflow": "dispatch_action", "payload": payload}

    def nod_yes(self, payload):
        self.calls.append(("nod_yes", payload))
        return {"ok": True, "workflow": "dispatch_action", "payload": payload, "dispatch": {"action_id": "behavior.robot_action.nod_yes"}}

    def shake_no(self, payload):
        self.calls.append(("shake_no", payload))
        return {"ok": True, "workflow": "dispatch_action", "payload": payload, "dispatch": {"action_id": "behavior.robot_action.shake_no"}}

    def coding_event(self, payload):
        self.calls.append(("coding_event", payload))
        return {"ok": True, "workflow": "coding_event", "payload": payload}


class AgentMcpServerTest(unittest.TestCase):
    def test_tool_definitions_include_expected_names(self):
        definitions = build_tool_definitions()
        names = [tool["name"] for tool in definitions]

        self.assertEqual(names, TOOL_NAMES)
        self.assertIn("robot_status", names)
        self.assertIn("find_and_greet", names)

    def test_tool_descriptions_include_chinese_trigger_words(self):
        definitions = {tool["name"]: tool for tool in build_tool_definitions()}

        self.assertIn("找人", definitions["find_and_greet"]["description"])
        self.assertIn("打招呼", definitions["find_and_greet"]["description"])
        self.assertIn("状态", definitions["robot_status"]["description"])
        self.assertIn("点头", definitions["nod_yes"]["description"])
        self.assertIn("摇头", definitions["shake_no"]["description"])

    def test_initialize_response_contains_usage_instructions(self):
        server = AgentMcpServer(RecordingWorkflowService())

        response = server.handle_request({"jsonrpc": "2.0", "id": 0, "method": "initialize", "params": {}})

        self.assertIn("instructions", response["result"])
        self.assertIn("find_and_greet", response["result"]["instructions"])
        self.assertIn("找人", response["result"]["instructions"])
        self.assertIn("nod_yes", response["result"]["instructions"])

    def test_initialize_response_includes_configured_agent_prompt_and_allowed_assets(self):
        server = AgentMcpServer(
            RecordingWorkflowService(),
            agent_config_provider=lambda: {
                "systemPrompt": "只在肯定回答时点头。",
                "allowedAssets": ["behavior.robot_action.nod_yes"],
            },
        )

        response = server.handle_request({"jsonrpc": "2.0", "id": 0, "method": "initialize", "params": {}})

        instructions = response["result"]["instructions"]
        self.assertIn("只在肯定回答时点头。", instructions)
        self.assertIn("behavior.robot_action.nod_yes", instructions)
        self.assertNotIn("behavior.robot_action.shake_no", instructions)

    def test_tools_list_response_contains_tool_definitions(self):
        workflow = RecordingWorkflowService()
        server = AgentMcpServer(workflow)

        response = server.handle_request({"jsonrpc": "2.0", "id": 1, "method": "tools/list", "params": {}})

        self.assertEqual(response["id"], 1)
        self.assertEqual(response["result"]["tools"][0]["name"], "robot_status")

    def test_tools_call_dispatches_arguments_to_workflow(self):
        workflow = RecordingWorkflowService()
        server = AgentMcpServer(workflow)

        response = server.handle_request({
            "jsonrpc": "2.0",
            "id": 2,
            "method": "tools/call",
            "params": {"name": "set_expression", "arguments": {"mood": "happy"}},
        })

        self.assertEqual(response["result"]["content"][0]["type"], "text")
        payload = json.loads(response["result"]["content"][0]["text"])
        self.assertEqual(payload["workflow"], "set_expression")
        self.assertEqual(workflow.calls, [("set_expression", {"mood": "happy"})])

    def test_explicit_nod_tool_dispatches_to_workflow(self):
        workflow = RecordingWorkflowService()
        server = AgentMcpServer(workflow)

        response = server.handle_request({
            "jsonrpc": "2.0",
            "id": 4,
            "method": "tools/call",
            "params": {"name": "nod_yes", "arguments": {"bypass_cooldown": True}},
        })

        payload = json.loads(response["result"]["content"][0]["text"])
        self.assertEqual(payload["dispatch"]["action_id"], "behavior.robot_action.nod_yes")
        self.assertEqual(workflow.calls, [("nod_yes", {"bypass_cooldown": True})])

    def test_unknown_tool_returns_jsonrpc_error(self):
        server = AgentMcpServer(RecordingWorkflowService())

        response = server.handle_request({
            "jsonrpc": "2.0",
            "id": 3,
            "method": "tools/call",
            "params": {"name": "dance", "arguments": {}},
        })

        self.assertEqual(response["id"], 3)
        self.assertEqual(response["error"]["code"], -32602)
        self.assertIn("unknown tool", response["error"]["message"])

    def test_parse_jsonrpc_line_rejects_invalid_json(self):
        result = parse_jsonrpc_line("{not json")

        self.assertEqual(result["error"]["code"], -32700)


if __name__ == "__main__":
    unittest.main()
