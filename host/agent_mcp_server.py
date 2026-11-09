#!/usr/bin/env python3
import argparse
import json
import sys

try:
    from host.agent_workflows import build_workflow_service
except ModuleNotFoundError:
    from agent_workflows import build_workflow_service


TOOL_NAMES = [
    "robot_status",
    "robot_catalog",
    "invoke_asset",
    "invoke_safe_behavior",
    "dispatch_action",
    "nod_yes",
    "shake_no",
    "set_expression",
    "move_pose",
    "camera_step_detect",
    "face_detect_and_center",
    "find_and_greet",
    "coding_event",
]

SERVER_INSTRUCTIONS = (
    "Robot-agent controls a local desktop robot through safe tools. "
    "中文请求如“看状态/找人/打招呼/做表情/点头/摇头/移动头部” should map to these tools. "
    "Use robot_status for status, robot_catalog for assets, find_and_greet for 找人并打招呼, "
    "nod_yes for 点头/同意, shake_no for 摇头/不同意, dispatch_action for simple yes/no dispatch, "
    "face_detect_and_center for 找人/人脸居中, set_expression for 表情, move_pose for 头部姿态, "
    "and coding_event for 编程状态事件. Ask before action tools when approvals are required."
)


def tool_schema(properties=None, required=None):
    return {
        "type": "object",
        "additionalProperties": True,
        "properties": properties or {},
        "required": required or [],
    }


def build_tool_definitions():
    return [
        {"name": "robot_status", "description": "Read current robot status. 查看机器人状态。", "inputSchema": tool_schema()},
        {
            "name": "robot_catalog",
            "description": "Read robot asset and logic catalog. 查看机器人动作/表情/资源目录。",
            "inputSchema": tool_schema(),
        },
        {
            "name": "invoke_asset",
            "description": "Invoke a safe firmware asset by stable asset id. 调用安全的机器人资源/表情/动作。",
            "inputSchema": tool_schema({"asset_id": {"type": "string"}}, ["asset_id"]),
        },
        {
            "name": "invoke_safe_behavior",
            "description": "Invoke an allowlisted robot behavior through the action router. 安全调用点头、思考、开心等动作。",
            "inputSchema": tool_schema({"action_id": {"type": "string"}, "asset_id": {"type": "string"}}),
        },
        {
            "name": "dispatch_action",
            "description": "Dispatch a simple yes/no robot action from a command. 调度点头/摇头动作。",
            "inputSchema": tool_schema({"command": {"type": "string"}, "intent": {"type": "string"}}),
        },
        {
            "name": "nod_yes",
            "description": "Make the robot nod yes. 让机器人点头/表示同意。",
            "inputSchema": tool_schema(),
        },
        {
            "name": "shake_no",
            "description": "Make the robot shake no. 让机器人摇头/表示不同意。",
            "inputSchema": tool_schema(),
        },
        {
            "name": "set_expression",
            "description": "Set a robot face expression. 设置机器人表情/眼睛状态。",
            "inputSchema": tool_schema({"mood": {"type": "string"}, "color": {"type": "string"}}, ["mood"]),
        },
        {
            "name": "move_pose",
            "description": "Move robot yaw/pitch with clamped values. 移动机器人头部姿态。",
            "inputSchema": tool_schema({
                "yaw": {"type": "integer"},
                "pitch": {"type": "integer"},
                "speed": {"type": "integer"},
            }),
        },
        {
            "name": "camera_step_detect",
            "description": "Run one host-side camera face detection step. 单步人脸检测/看有没有人。",
            "inputSchema": tool_schema(),
        },
        {
            "name": "face_detect_and_center",
            "description": "Run a bounded face detect and center workflow. 找人、人脸检测并让机器人居中。",
            "inputSchema": tool_schema(),
        },
        {
            "name": "find_and_greet",
            "description": "Detect a face, center the robot, and invoke a greeting behavior. 找人，找到后打招呼。",
            "inputSchema": tool_schema(),
        },
        {
            "name": "coding_event",
            "description": "Send a coding event and invoke a matching robot reaction. 发送编程状态事件并让机器人反馈。",
            "inputSchema": tool_schema({"type": {"type": "string"}}, ["type"]),
        },
    ]


def make_result(request_id, result):
    return {"jsonrpc": "2.0", "id": request_id, "result": result}


def make_error(request_id, code, message):
    return {"jsonrpc": "2.0", "id": request_id, "error": {"code": int(code), "message": str(message)}}


def parse_jsonrpc_line(line):
    try:
        payload = json.loads(line)
    except json.JSONDecodeError as exc:
        return make_error(None, -32700, f"parse error: {exc}")
    if not isinstance(payload, dict):
        return make_error(None, -32600, "request must be a JSON object")
    return payload


class AgentMcpServer:
    def __init__(self, workflow_service, agent_config_provider=None):
        self.workflow_service = workflow_service
        self.agent_config_provider = agent_config_provider
        self.tools = {tool["name"]: tool for tool in build_tool_definitions()}
        self.dispatch = {
            "robot_status": self.workflow_service.robot_status,
            "robot_catalog": self.workflow_service.robot_catalog,
            "invoke_asset": self.workflow_service.invoke_asset,
            "invoke_safe_behavior": self.workflow_service.invoke_safe_behavior,
            "dispatch_action": self.workflow_service.dispatch_action,
            "nod_yes": self.workflow_service.nod_yes,
            "shake_no": self.workflow_service.shake_no,
            "set_expression": self.workflow_service.set_expression,
            "move_pose": self.workflow_service.move_pose,
            "camera_step_detect": self.workflow_service.camera_step_detect,
            "face_detect_and_center": self.workflow_service.face_detect_and_center,
            "find_and_greet": self.workflow_service.find_and_greet,
            "coding_event": self.workflow_service.coding_event,
        }

    def instructions(self):
        instructions = SERVER_INSTRUCTIONS
        if not callable(self.agent_config_provider):
            return instructions
        config = self.agent_config_provider() or {}
        prompt = str(config.get("systemPrompt") or "").strip()
        allowed_assets = [str(asset).strip() for asset in (config.get("allowedAssets") or []) if str(asset).strip()]
        if prompt:
            instructions += f"\n\n调度提示词:\n{prompt}"
        if allowed_assets:
            instructions += "\n\n可用资产白名单:\n" + "\n".join(f"- {asset}" for asset in allowed_assets)
        return instructions

    def handle_request(self, request):
        request_id = request.get("id")
        method = request.get("method")
        params = request.get("params") or {}
        if method == "initialize":
            return make_result(
                request_id,
                {
                    "protocolVersion": "2024-11-05",
                    "serverInfo": {"name": "robot-agent", "version": "0.1.0"},
                    "capabilities": {"tools": {}},
                    "instructions": self.instructions(),
                },
            )
        if method == "tools/list":
            return make_result(request_id, {"tools": build_tool_definitions()})
        if method == "tools/call":
            return self._call_tool(request_id, params)
        if request_id is None:
            return None
        return make_error(request_id, -32601, f"unknown method: {method}")

    def _call_tool(self, request_id, params):
        name = str((params or {}).get("name") or "")
        arguments = (params or {}).get("arguments") or {}
        if name not in self.dispatch:
            return make_error(request_id, -32602, f"unknown tool: {name}")
        if not isinstance(arguments, dict):
            return make_error(request_id, -32602, "tool arguments must be an object")
        result = self.dispatch[name](arguments)
        return make_result(
            request_id,
            {"content": [{"type": "text", "text": json.dumps(result, ensure_ascii=False, separators=(",", ":"))}]},
        )

    def serve(self, input_stream=None, output_stream=None):
        input_stream = input_stream or sys.stdin
        output_stream = output_stream or sys.stdout
        for line in input_stream:
            parsed = parse_jsonrpc_line(line)
            if "error" in parsed:
                response = parsed
            else:
                response = self.handle_request(parsed)
            if response is not None:
                output_stream.write(json.dumps(response, ensure_ascii=False, separators=(",", ":")) + "\n")
                output_stream.flush()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--robot", default=None)
    parser.add_argument("--assistant-status-url", default=None)
    args = parser.parse_args()
    AgentMcpServer(build_workflow_service(args.robot, args.assistant_status_url)).serve()


if __name__ == "__main__":
    main()
