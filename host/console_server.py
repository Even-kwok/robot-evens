#!/usr/bin/env python3
import argparse
import base64
import binascii
from concurrent.futures import ThreadPoolExecutor, TimeoutError as FutureTimeoutError
import json
import os
import re
import ssl
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import urllib.request

try:
    from host.agent_mcp_server import AgentMcpServer
    from host.agent_workflows import RobotAgentWorkflowService
    from host.host_vision import HostVisionTracker
    from host.robot_client import RobotClient
    from host.volcengine_realtime import (
        VolcengineRealtimeConfig,
        VolcengineRealtimeAudioSession,
        EVENT_CHAT_RESPONSE,
        EVENT_TTS_RESPONSE,
        EVENT_TTS_ENDED,
        build_voice_system_prompt,
        extract_realtime_payload_text,
        run_volcengine_realtime_text,
    )
except ModuleNotFoundError:
    from agent_mcp_server import AgentMcpServer
    from agent_workflows import RobotAgentWorkflowService
    from host_vision import HostVisionTracker
    from robot_client import RobotClient
    from volcengine_realtime import (
        VolcengineRealtimeConfig,
        VolcengineRealtimeAudioSession,
        EVENT_CHAT_RESPONSE,
        EVENT_TTS_RESPONSE,
        EVENT_TTS_ENDED,
        build_voice_system_prompt,
        extract_realtime_payload_text,
        run_volcengine_realtime_text,
    )


WEB_CONSOLE_PATH = Path(__file__).with_name("web_console.html")
MODEL_CONFIG_PATH = Path(__file__).with_name("model_api_config.local.json")


DEFAULT_REALTIME_VOICE_PROMPT = """你是桌面机器人的实时语音大脑，可以用中文自然语音回答用户。
回答要短、温暖、像桌面小机器人一样有陪伴感；不要读出 JSON、代码块或内部调试信息。
机器人硬件会负责播放你的语音，电脑负责 API 和音频转发。
如果用户只是叫醒你，先简短回应并等待后续问题。"""

DEFAULT_AGENT_DISPATCH_PROMPT = """你是桌面机器人的动作调度层。
根据 userText、robotReply 和上下文，选择是否调用一个机器人资源。
如果 robotReply 是肯定、认识、记得、同意、可以，优先选择 behavior.robot_action.nod_yes。
如果 robotReply 是否定、不认识、不记得、拒绝、不可以，优先选择 behavior.robot_action.shake_no。
如果只是普通聊天且不需要身体语言，返回空 action_id。
只能从 allowedAssets 中选择 action_id；不需要动作时返回空字符串。
只输出 JSON，例如 {"action_id":"behavior.robot_action.nod_yes","reason":"肯定回答"} 或 {"action_id":"","reason":"无需动作"}。"""

DEFAULT_AGENT_ALLOWED_ASSETS = (
    "behavior.robot_action.nod_yes",
    "behavior.robot_action.shake_no",
    "behavior.emotion.happy",
    "behavior.emotion.thinking",
    "behavior.emotion.confused",
)

MAX_API_VOICE_VOLUME = 255


def _int_or_default(value, default):
    try:
        return int(value)
    except (TypeError, ValueError):
        return default


class HostModelConfigStore:
    FIELD_NAMES = (
        "knownUser",
        "realtimeAppId",
        "realtimeAccessToken",
        "realtimeEndpoint",
        "realtimeModelVersion",
        "realtimeSpeaker",
        "realtimeRecvTimeout",
        "realtimeInsecureTls",
        "realtimeFirmwareNotes",
        "agentEnabled",
        "agentBaseUrl",
        "agentApiKey",
        "agentModel",
        "agentSystemPrompt",
    )
    LIST_FIELD_NAMES = ("agentAllowedAssets",)

    def __init__(self, path=MODEL_CONFIG_PATH):
        self.path = Path(path)

    def _migrate_config(self, config):
        migrated = dict(config)
        realtime_notes = migrated.get("realtimeFirmwareNotes", "")
        action_note_markers = (
            "tilt_confused=疑惑",
            "look_thinking=思考",
            "nod_yes",
            "shake_no",
            "asset_id",
        )
        if any(marker in realtime_notes for marker in action_note_markers):
            migrated["realtimeFirmwareNotes"] = DEFAULT_REALTIME_VOICE_PROMPT
        return migrated

    def _read_config(self):
        if not self.path.exists():
            return {}
        try:
            payload = json.loads(self.path.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError):
            return {}
        if not isinstance(payload, dict):
            return {}
        config = {
            key: str(payload[key])
            for key in self.FIELD_NAMES
            if key in payload and payload[key] is not None
        }
        for key in self.LIST_FIELD_NAMES:
            if key in payload:
                config[key] = self._asset_list(payload.get(key))
        legacy_token = payload.get("realtimeAccessKey") or payload.get("accessToken")
        if "realtimeAccessToken" not in config and legacy_token is not None:
            config["realtimeAccessToken"] = str(legacy_token)
        return self._migrate_config(config)

    def _asset_list(self, value):
        if isinstance(value, str):
            raw_values = re.split(r"[\n,]+", value)
        elif isinstance(value, (list, tuple, set)):
            raw_values = value
        else:
            raw_values = []
        assets = []
        for item in raw_values:
            asset_id = str(item or "").strip()
            if not asset_id:
                continue
            if not asset_id.startswith(("expression.", "behavior.", "motion.", "sound.", "light.", "screen.")):
                continue
            if asset_id.startswith("logic."):
                continue
            if asset_id not in assets:
                assets.append(asset_id)
        return assets

    def status(self):
        config = self._read_config()
        return {"ok": True, "saved": bool(config), "config": config}

    def save(self, payload):
        config = self._read_config()
        config.update({
            key: str(payload[key])
            for key in self.FIELD_NAMES
            if key in payload and payload[key] is not None
        })
        for key in self.LIST_FIELD_NAMES:
            if key in payload:
                config[key] = self._asset_list(payload.get(key))
        legacy_token = payload.get("realtimeAccessKey") or payload.get("accessToken")
        if "realtimeAccessToken" not in payload and legacy_token is not None:
            config["realtimeAccessToken"] = str(legacy_token)
        config = self._migrate_config(config)
        self.path.parent.mkdir(parents=True, exist_ok=True)
        self.path.write_text(json.dumps(config, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        return {"ok": True, "saved": True, "config": config}

    def agent_dispatch_config(self):
        config = self._read_config()
        return {
            "enabled": parse_bool_flag(config.get("agentEnabled")),
            "baseUrl": (config.get("agentBaseUrl") or "https://ark.cn-beijing.volces.com/api/v3/chat/completions").strip(),
            "apiKey": (config.get("agentApiKey") or "").strip(),
            "model": (config.get("agentModel") or "doubao-seed-2-0-mini-260428").strip(),
            "insecureTls": parse_bool_flag(config.get("agentInsecureTls", config.get("realtimeInsecureTls"))),
            "systemPrompt": (config.get("agentSystemPrompt") or DEFAULT_AGENT_DISPATCH_PROMPT).strip(),
            "allowedAssets": self._asset_list(config.get("agentAllowedAssets")) or list(DEFAULT_AGENT_ALLOWED_ASSETS),
        }

    def agent_allowed_assets(self):
        return set(self.agent_dispatch_config()["allowedAssets"])


def parse_bool_flag(value):
    if isinstance(value, bool):
        return value
    if isinstance(value, (int, float)):
        return value != 0
    if value is None:
        return False
    return str(value).strip().lower() in {"1", "true", "yes", "on", "开"}


class AgentDispatchMonitor:
    def __init__(self, clock_ms=None, max_history=20):
        self.clock_ms = clock_ms or (lambda: int(time.time() * 1000))
        self.max_history = max(1, int(max_history))
        self._lock = threading.RLock()
        self._next_id = 0
        self._current = {}
        self._last = {}
        self._history = []

    def started(self, source="", mode="", user_text="", robot_reply="", command=""):
        now_ms = self._now_ms()
        with self._lock:
            self._next_id += 1
            event = {
                "id": self._next_id,
                "state": "running",
                "source": str(source or ""),
                "mode": str(mode or ""),
                "userText": self._short_text(user_text),
                "robotReply": self._short_text(robot_reply),
                "command": str(command or ""),
                "startedAtMs": now_ms,
                "updatedAtMs": now_ms,
            }
            self._current = event
            return event["id"]

    def finished(self, event_id, result):
        now_ms = self._now_ms()
        result = dict(result or {})
        with self._lock:
            event = dict(self._current if self._current.get("id") == event_id else {"id": event_id, "startedAtMs": now_ms})
            summary = self._summary(result)
            if not summary.get("mode") and event.get("mode"):
                summary["mode"] = event["mode"]
            event.update(summary)
            event["updatedAtMs"] = now_ms
            event["finishedAtMs"] = now_ms
            self._last = event
            self._history.insert(0, dict(event))
            self._history = self._history[: self.max_history]
            if self._current.get("id") == event_id:
                self._current = {}
            return self.status()

    def status(self):
        with self._lock:
            state = "running" if self._current else (self._last.get("state") or "idle")
            return {
                "ok": True,
                "state": state,
                "current": dict(self._current),
                "last": dict(self._last),
                "history": [dict(item) for item in self._history],
            }

    def _summary(self, result):
        behavior = result.get("behavior") or {}
        action = behavior.get("action") or {}
        dispatch = result.get("dispatch") or {}
        decision = result.get("decision") or {}
        action_id = (
            dispatch.get("action_id")
            or action.get("action_id")
            or action.get("asset_id")
            or behavior.get("action_id")
            or ""
        )
        if result.get("ok") is False:
            state = "error"
        elif result.get("skipped"):
            state = "skipped"
        else:
            state = "ok"
        return {
            "state": state,
            "ok": result.get("ok", True),
            "mode": str(result.get("mode") or ""),
            "stage": str(result.get("stage") or behavior.get("stage") or ""),
            "error": str(result.get("error") or behavior.get("error") or ""),
            "reason": str(decision.get("reason") or result.get("reason") or ""),
            "actionId": str(action_id or ""),
        }

    def _short_text(self, value, limit=180):
        text = str(value or "").strip()
        if len(text) <= limit:
            return text
        return text[: limit - 1] + "..."

    def _now_ms(self):
        return int(self.clock_ms())


AFFIRMATIVE_REPLY_MARKERS = (
    "当然认识",
    "认识你",
    "记得你",
    "当然记得",
    "知道你",
    "见过你",
    "是你",
)

NEGATIVE_REPLY_MARKERS = (
    "不认识",
    "还不认识",
    "不记得",
    "认不出来",
    "认不出",
    "不知道你",
    "没见过",
)


AGENT_ACTION_ALIASES = {
    "nod_yes": "behavior.robot_action.nod_yes",
    "shake_no": "behavior.robot_action.shake_no",
}


def _extract_json_object(text):
    text = str(text or "").strip()
    if not text:
        return "{}"
    fenced = re.search(r"```(?:json)?\s*(\{.*?\})\s*```", text, re.S)
    if fenced:
        return fenced.group(1)
    start = text.find("{")
    end = text.rfind("}")
    if start >= 0 and end > start:
        return text[start : end + 1]
    return text


def parse_agent_dispatch_decision(content, allowed_assets):
    allowed = {str(asset).strip() for asset in (allowed_assets or []) if str(asset).strip()}
    try:
        payload = json.loads(_extract_json_object(content))
    except json.JSONDecodeError as exc:
        return {"ok": False, "stage": "agent_model_parse", "error": f"invalid agent JSON: {exc}", "raw": str(content or "")}
    if not isinstance(payload, dict):
        return {"ok": False, "stage": "agent_model_parse", "error": "agent response must be a JSON object", "raw": str(content or "")}
    action_id = str(payload.get("action_id") or payload.get("asset_id") or "").strip()
    action_id = AGENT_ACTION_ALIASES.get(action_id, action_id)
    if action_id.lower() in {"none", "null", "skip", "no_action"}:
        action_id = ""
    reason = str(payload.get("reason") or "").strip()
    if not action_id:
        return {"ok": True, "skipped": True, "reason": reason or "agent chose no action", "decision": {"action_id": "", "reason": reason}}
    if action_id not in allowed:
        return {
            "ok": False,
            "stage": "agent_whitelist",
            "error": f"{action_id} is not allowed by agent asset whitelist",
            "decision": {"action_id": action_id, "reason": reason},
        }
    return {"ok": True, "decision": {"action_id": action_id, "reason": reason}}


def run_agent_dispatch_chat(config, payload):
    base_url = (config.get("baseUrl") or "").strip()
    api_key = (config.get("apiKey") or "").strip()
    model = (config.get("model") or "").strip()
    if not base_url or not api_key or not model:
        return {"ok": False, "stage": "agent_model_config", "error": "agent model baseUrl, apiKey and model are required"}
    body = {
        "model": model,
        "temperature": 0,
        "messages": [
            {"role": "system", "content": config.get("systemPrompt") or DEFAULT_AGENT_DISPATCH_PROMPT},
            {
                "role": "user",
                "content": json.dumps(payload, ensure_ascii=False, separators=(",", ":")),
            },
        ],
    }
    request = urllib.request.Request(
        base_url,
        data=json.dumps(body, ensure_ascii=False).encode("utf-8"),
        headers={
            "Content-Type": "application/json",
            "Authorization": f"Bearer {api_key}",
        },
        method="POST",
    )
    context = ssl._create_unverified_context() if parse_bool_flag(config.get("insecureTls")) else None
    with urllib.request.urlopen(request, timeout=15, context=context) as response:
        data = json.loads(response.read().decode("utf-8"))
    choices = data.get("choices") or []
    if choices:
        message = choices[0].get("message") or {}
        content = message.get("content")
        if content is not None:
            return {"ok": True, "content": str(content), "raw": data}
    return {"ok": False, "stage": "agent_model_response", "error": "agent model response did not include choices[0].message.content", "raw": data}


class AgentDispatchModel:
    def __init__(self, runner=run_agent_dispatch_chat):
        self.runner = runner

    def dispatch(self, config, agent_dispatcher, user_text="", robot_reply="", source="realtime_voice"):
        config = dict(config or {})
        if not parse_bool_flag(config.get("enabled")):
            return {"ok": True, "skipped": True, "reason": "agent model disabled", "mode": "keyword_fallback"}
        allowed_assets = list(config.get("allowedAssets") or DEFAULT_AGENT_ALLOWED_ASSETS)
        if not config.get("apiKey") and self.runner is run_agent_dispatch_chat:
            return {"ok": True, "skipped": True, "reason": "agent model not configured", "mode": "keyword_fallback"}
        payload = {
            "userText": str(user_text or ""),
            "robotReply": str(robot_reply or ""),
            "source": str(source or "realtime_voice"),
            "allowedAssets": allowed_assets,
        }
        try:
            response = self.runner(config, payload)
        except Exception as exc:
            return {"ok": False, "stage": "agent_model", "error": str(exc), "mode": "agent_model"}
        if not response.get("ok", True):
            result = dict(response)
            result["mode"] = "agent_model"
            return result
        decision = parse_agent_dispatch_decision(response.get("content"), allowed_assets)
        decision["mode"] = "agent_model"
        if not decision.get("ok", True) or decision.get("skipped"):
            return decision
        action_id = decision["decision"]["action_id"]
        try:
            if action_id.startswith(("behavior.robot_action.", "behavior.emotion.", "expression.")) and hasattr(agent_dispatcher, "invoke_safe_behavior"):
                behavior = agent_dispatcher.invoke_safe_behavior({"asset_id": action_id, "bypass_cooldown": True})
            elif hasattr(agent_dispatcher, "invoke_asset"):
                behavior = agent_dispatcher.invoke_asset({"asset_id": action_id})
            else:
                behavior = {"ok": False, "stage": "agent_dispatch", "error": "agent dispatcher cannot invoke assets"}
        except Exception as exc:
            behavior = {"ok": False, "stage": "agent_dispatch", "error": str(exc)}
        return {
            "ok": bool(behavior.get("ok", True)),
            "mode": "agent_model",
            "dispatch": {"action_id": action_id, "source": source},
            "decision": decision["decision"],
            "behavior": behavior,
        }


def infer_realtime_reply_command(chat_text):
    text = str(chat_text or "").strip().lower()
    if not text:
        return ""
    if any(marker in text for marker in NEGATIVE_REPLY_MARKERS):
        return "shake_no"
    if any(marker in text for marker in AFFIRMATIVE_REPLY_MARKERS):
        return "nod_yes"
    return ""


def _agent_dispatch_mode(agent_model=None, agent_config=None):
    if agent_model is None or agent_config is None:
        return "keyword_fallback"
    if parse_bool_flag((agent_config or {}).get("enabled")) and str((agent_config or {}).get("apiKey") or "").strip():
        return "agent_model"
    return "keyword_fallback"


def dispatch_realtime_reply_action(agent_dispatcher, chat_text, source, user_text="", agent_model=None, agent_config=None, agent_monitor=None):
    event_id = None
    if agent_monitor is not None:
        event_id = agent_monitor.started(
            source=source,
            mode=_agent_dispatch_mode(agent_model, agent_config),
            user_text=user_text,
            robot_reply=chat_text,
        )
    result = None
    try:
        result = _dispatch_realtime_reply_action(
            agent_dispatcher,
            chat_text,
            source,
            user_text=user_text,
            agent_model=agent_model,
            agent_config=agent_config,
        )
        return result
    finally:
        if event_id is not None:
            agent_monitor.finished(event_id, result or {"ok": False, "stage": "agent_dispatch", "error": "agent dispatch did not return a result"})


def _dispatch_realtime_reply_action(agent_dispatcher, chat_text, source, user_text="", agent_model=None, agent_config=None):
    if agent_dispatcher is None:
        return {"ok": True, "skipped": True, "reason": "agent dispatcher not configured"}
    if agent_model is not None and agent_config is not None:
        result = agent_model.dispatch(
            agent_config,
            agent_dispatcher,
            user_text=user_text,
            robot_reply=chat_text,
            source=source,
        )
        if not (result.get("skipped") and result.get("mode") == "keyword_fallback"):
            return result
    command = infer_realtime_reply_command(chat_text)
    if not command:
        return {"ok": True, "skipped": True, "reason": "no yes/no reply signal"}
    try:
        return agent_dispatcher.dispatch_action({
            "command": command,
            "source": source,
            "bypass_cooldown": True,
        })
    except Exception as exc:
        return {"ok": False, "stage": "agent_dispatch", "error": str(exc)}


def _pcm_s16le_to_raw_u8_mono(audio, source_rate=24000, target_rate=16000):
    audio = bytes(audio or b"")
    if len(audio) < 2:
        return b""
    source_rate = _int_or_default(source_rate, target_rate)
    if source_rate <= 0:
        source_rate = target_rate
    sample_count = len(audio) // 2
    samples = [
        int.from_bytes(audio[index : index + 2], "little", signed=True)
        for index in range(0, sample_count * 2, 2)
    ]
    if not samples:
        return b""
    if source_rate == target_rate:
        resampled = samples
    else:
        target_count = max(1, int(round(len(samples) * float(target_rate) / float(source_rate))))
        resampled = []
        for output_index in range(target_count):
            source_pos = output_index * float(source_rate) / float(target_rate)
            left = int(source_pos)
            if left >= len(samples) - 1:
                resampled.append(samples[-1])
                continue
            fraction = source_pos - left
            resampled.append(samples[left] * (1.0 - fraction) + samples[left + 1] * fraction)
    raw = bytearray()
    for sample in resampled:
        normalized = max(-1.0, min(1.0, float(sample) / 32768.0))
        raw.append(max(0, min(255, int(round((normalized * 0.5 + 0.5) * 255.0)))))
    return bytes(raw)


class HostRealtimeVoiceService:
    def __init__(
        self,
        model_config,
        runner=run_volcengine_realtime_text,
        timeout_ms=None,
        robot_audio=None,
        agent_dispatcher=None,
        agent_model=None,
        agent_monitor=None,
    ):
        self.model_config = model_config
        self.runner = runner
        self.timeout_ms = timeout_ms
        self.robot_audio = robot_audio
        self.agent_dispatcher = agent_dispatcher
        self.agent_model = agent_model or AgentDispatchModel()
        self.agent_monitor = agent_monitor

    def _merged_config(self, payload):
        saved = self.model_config.status().get("config", {})
        merged = dict(saved)
        request_config = (payload or {}).get("config")
        if isinstance(request_config, dict):
            merged.update({key: str(value) for key, value in request_config.items() if value is not None})
        return merged

    def _realtime_config(self, payload):
        config = self._merged_config(payload)
        app_id = (config.get("realtimeAppId") or "").strip()
        access_token = (config.get("realtimeAccessToken") or config.get("realtimeAccessKey") or "").strip()
        if not app_id or not access_token:
            raise ValueError("Realtime App ID and Access Token are required")
        return (
            config,
            VolcengineRealtimeConfig(
                app_id=app_id,
                access_key=access_token,
                endpoint=(config.get("realtimeEndpoint") or "").strip() or VolcengineRealtimeConfig.endpoint,
                model_version=(config.get("realtimeModelVersion") or "").strip() or "1.2.1.1",
                speaker=(config.get("realtimeSpeaker") or "").strip() or "zh_male_yunzhou_jupiter_bigtts",
                recv_timeout=_int_or_default(config.get("realtimeRecvTimeout"), 15),
                insecure_tls=parse_bool_flag(config.get("realtimeInsecureTls")),
            ),
        )

    def run_text(self, payload):
        payload = payload or {}
        text = str(payload.get("text") or "").strip()
        if not text:
            return {"ok": False, "stage": "input", "error": "text is required"}
        try:
            raw_config, realtime_config = self._realtime_config(payload)
            system_prompt = (raw_config.get("realtimeFirmwareNotes") or "").strip()
            if not system_prompt:
                system_prompt = build_voice_system_prompt(raw_config.get("knownUser"))
            result = self._run_with_timeout(self.runner, realtime_config, text, system_prompt)
        except Exception as exc:
            return {"ok": False, "stage": "realtime_api", "error": str(exc)}
        return self._finalize_result(result, raw_config=raw_config, user_text=text)

    def _finalize_result(self, result, raw_config=None, user_text=""):
        if not result.get("ok", True):
            return result
        audio = result.pop("audioPcm16", b"") or b""
        if not isinstance(audio, (bytes, bytearray)):
            audio = b""
        result.setdefault("ok", True)
        result["audioBase64"] = base64.b64encode(bytes(audio)).decode("ascii")
        result["audioBytes"] = len(audio)
        result.setdefault("audioSampleRate", 24000)
        result.setdefault("audioFormat", "pcm_s16le")
        result["agentDispatch"] = dispatch_realtime_reply_action(
            self.agent_dispatcher,
            result.get("chatText"),
            "realtime_voice_text",
            user_text=user_text,
            agent_model=self.agent_model,
            agent_config=self._agent_dispatch_config(raw_config),
            agent_monitor=self.agent_monitor,
        )
        if audio and self.robot_audio is not None:
            robot_result = self._play_robot_audio(bytes(audio), result["audioSampleRate"], result["audioFormat"])
            result["robotAudio"] = robot_result
            if not robot_result.get("ok", True):
                result["ok"] = False
                result["stage"] = "robot_audio"
                result["error"] = robot_result.get("error") or "robot audio playback failed"
        return result

    def _play_robot_audio(self, audio, sample_rate, audio_format):
        try:
            return self.robot_audio.play_realtime_voice(audio, sample_rate=sample_rate, audio_format=audio_format)
        except Exception as exc:
            return {"ok": False, "error": str(exc)}

    def _runner_timeout_ms(self, realtime_config):
        if self.timeout_ms is not None:
            return max(1, int(self.timeout_ms))
        return max(5000, (int(realtime_config.recv_timeout) + 8) * 1000)

    def _run_with_timeout(self, runner, realtime_config, value, system_prompt):
        timeout_ms = self._runner_timeout_ms(realtime_config)
        executor = ThreadPoolExecutor(max_workers=1)
        future = executor.submit(runner, realtime_config, value, system_prompt)
        try:
            return dict(future.result(timeout=timeout_ms / 1000))
        except FutureTimeoutError:
            future.cancel()
            return {
                "ok": False,
                "stage": "realtime_api_timeout",
                "error": f"realtime request timed out after {timeout_ms} ms",
                "timeoutMs": timeout_ms,
            }
        finally:
            executor.shutdown(wait=False, cancel_futures=True)

    def _agent_dispatch_config(self, raw_config=None):
        raw_config = dict(raw_config or {})
        if hasattr(self.model_config, "agent_dispatch_config"):
            config = dict(self.model_config.agent_dispatch_config())
            if raw_config:
                config.update({
                    "enabled": parse_bool_flag(raw_config.get("agentEnabled", config.get("enabled"))),
                    "baseUrl": (raw_config.get("agentBaseUrl") or config.get("baseUrl") or "").strip(),
                    "apiKey": (raw_config.get("agentApiKey") or config.get("apiKey") or "").strip(),
                    "model": (raw_config.get("agentModel") or config.get("model") or "").strip(),
                    "insecureTls": parse_bool_flag(raw_config.get("agentInsecureTls", raw_config.get("realtimeInsecureTls", config.get("insecureTls")))),
                    "systemPrompt": (raw_config.get("agentSystemPrompt") or config.get("systemPrompt") or DEFAULT_AGENT_DISPATCH_PROMPT).strip(),
                    "allowedAssets": HostModelConfigStore()._asset_list(raw_config.get("agentAllowedAssets")) or config.get("allowedAssets") or list(DEFAULT_AGENT_ALLOWED_ASSETS),
                })
            return config
        config = raw_config
        return {
            "enabled": parse_bool_flag(config.get("agentEnabled")),
            "baseUrl": (config.get("agentBaseUrl") or "https://ark.cn-beijing.volces.com/api/v3/chat/completions").strip(),
            "apiKey": (config.get("agentApiKey") or "").strip(),
            "model": (config.get("agentModel") or "doubao-seed-2-0-mini-260428").strip(),
            "insecureTls": parse_bool_flag(config.get("agentInsecureTls", config.get("realtimeInsecureTls"))),
            "systemPrompt": (config.get("agentSystemPrompt") or DEFAULT_AGENT_DISPATCH_PROMPT).strip(),
            "allowedAssets": HostModelConfigStore()._asset_list(config.get("agentAllowedAssets")) or list(DEFAULT_AGENT_ALLOWED_ASSETS),
        }


class HostRealtimeVoiceSessionService:
    VOICE_SAMPLE_RATE = 16000
    VOICE_RMS_THRESHOLD = 0.008
    VOICE_PEAK_THRESHOLD = 0.025
    VOICE_TAIL_SILENCE_MS = 960

    def __init__(
        self,
        model_config,
        session_factory=None,
        robot_audio=None,
        agent_dispatcher=None,
        agent_model=None,
        agent_monitor=None,
        clock_ms=None,
    ):
        self.model_config = model_config
        self.config_helper = HostRealtimeVoiceService(model_config, agent_model=agent_model)
        self.session_factory = session_factory or (lambda config: VolcengineRealtimeAudioSession(config))
        self.robot_audio = robot_audio
        self.agent_dispatcher = agent_dispatcher
        self.agent_model = agent_model or self.config_helper.agent_model
        self.agent_monitor = agent_monitor
        self.clock_ms = clock_ms or (lambda: int(time.monotonic() * 1000))
        self._lock = threading.RLock()
        self._send_lock = threading.Lock()
        self._session = None
        self._thread = None
        self._raw_config = {}
        self._active = False
        self._stage = "idle"
        self._voice_has_started = False
        self._trailing_silence_ms = 0
        self.captured_frame_count = 0
        self.audio_frame_count = 0
        self.audio_bytes_sent = 0
        self.dropped_silence_frame_count = 0
        self.forwarded_silence_frame_count = 0
        self.response_count = 0
        self.last_voice_level = 0
        self.last_voice_peak = 0
        self.last_audio_gate = "idle"
        self.last_user_text = ""
        self.last_chat_text = ""
        self.last_robot_audio = {}
        self.last_agent_dispatch = {}
        self.last_audio_bytes = 0
        self.last_error = ""
        self.last_event = {}
        self.started_at_ms = 0
        self.first_frame_at_ms = 0
        self.last_frame_at_ms = 0
        self.first_forwarded_at_ms = 0
        self.last_forwarded_at_ms = 0
        self.first_speech_at_ms = 0
        self.last_speech_at_ms = 0
        self.first_api_event_at_ms = 0
        self.first_chat_at_ms = 0
        self.first_tts_at_ms = 0
        self.tts_ended_at_ms = 0
        self.robot_play_start_at_ms = 0
        self.robot_play_end_at_ms = 0

    def status(self):
        with self._lock:
            return self._snapshot_unlocked(ok=True)

    def start(self, payload):
        payload = payload or {}
        with self._lock:
            if self._active:
                return self._snapshot_unlocked(ok=True)

        try:
            raw_config, realtime_config = self.config_helper._realtime_config(payload)
            system_prompt = (raw_config.get("realtimeFirmwareNotes") or "").strip()
            if not system_prompt:
                system_prompt = build_voice_system_prompt(raw_config.get("knownUser"))
            session = self.session_factory(realtime_config)
            session.start(system_prompt)
        except Exception as exc:
            return {"ok": False, "stage": "realtime_api", "error": str(exc)}

        with self._lock:
            self._session = session
            self._raw_config = raw_config
            self._active = True
            self._stage = "listening"
            self._voice_has_started = False
            self._trailing_silence_ms = 0
            self.captured_frame_count = 0
            self.audio_frame_count = 0
            self.audio_bytes_sent = 0
            self.dropped_silence_frame_count = 0
            self.forwarded_silence_frame_count = 0
            self.response_count = 0
            self.last_voice_level = 0
            self.last_voice_peak = 0
            self.last_audio_gate = "idle"
            self.last_user_text = ""
            self.last_chat_text = ""
            self.last_robot_audio = {}
            self.last_agent_dispatch = {}
            self.last_audio_bytes = 0
            self.last_error = ""
            self.last_event = {}
            self.started_at_ms = self._now_ms()
            self.first_frame_at_ms = 0
            self.last_frame_at_ms = 0
            self.first_forwarded_at_ms = 0
            self.last_forwarded_at_ms = 0
            self.first_speech_at_ms = 0
            self.last_speech_at_ms = 0
            self.first_api_event_at_ms = 0
            self.first_chat_at_ms = 0
            self.first_tts_at_ms = 0
            self.tts_ended_at_ms = 0
            self.robot_play_start_at_ms = 0
            self.robot_play_end_at_ms = 0
            thread = threading.Thread(
                target=self._reader_loop,
                args=(session,),
                name="host-realtime-voice-session",
                daemon=True,
            )
            self._thread = thread
            thread.start()
            return self._snapshot_unlocked(ok=True)

    def frame(self, payload):
        payload = payload or {}
        with self._lock:
            session = self._session
            if not self._active or session is None:
                return self._snapshot_unlocked(ok=False, stage="inactive")

        encoded_audio = str(payload.get("audioBase64") or "").strip()
        if not encoded_audio:
            return {"ok": False, "stage": "input", "error": "audioBase64 is required"}
        try:
            audio = base64.b64decode(encoded_audio, validate=True)
        except (binascii.Error, ValueError) as exc:
            return {"ok": False, "stage": "input", "error": f"invalid audioBase64: {exc}"}
        if not audio:
            return {"ok": False, "stage": "input", "error": "audio is empty"}

        audio_to_send = self._prepare_audio_frame(audio)
        if audio_to_send is None:
            with self._lock:
                return self._snapshot_unlocked(ok=True)

        try:
            with self._send_lock:
                session.send_audio(audio_to_send)
        except Exception as exc:
            with self._lock:
                self.last_error = str(exc)
                self._stage = "error"
            return self.status()

        with self._lock:
            self.audio_frame_count += 1
            self.audio_bytes_sent += len(audio_to_send)
            return self._snapshot_unlocked(ok=True)

    def _prepare_audio_frame(self, audio):
        now_ms = self._now_ms()
        rms, peak = self._pcm16_level(audio)
        is_voice = rms >= self.VOICE_RMS_THRESHOLD or peak >= self.VOICE_PEAK_THRESHOLD
        duration_ms = self._pcm16_duration_ms(audio)

        with self._lock:
            self.captured_frame_count += 1
            if not self.first_frame_at_ms:
                self.first_frame_at_ms = now_ms
            self.last_frame_at_ms = now_ms
            self.last_voice_level = round(rms, 4)
            self.last_voice_peak = round(peak, 4)

            if is_voice:
                self._voice_has_started = True
                self._trailing_silence_ms = 0
                if not self.first_speech_at_ms:
                    self.first_speech_at_ms = now_ms
                self.last_speech_at_ms = now_ms
                if not self.first_forwarded_at_ms:
                    self.first_forwarded_at_ms = now_ms
                self.last_forwarded_at_ms = now_ms
                self.last_audio_gate = "speech"
                return audio

            if not self._voice_has_started:
                self.dropped_silence_frame_count += 1
                self.last_audio_gate = "pre_speech_silence"
                return None

            self._trailing_silence_ms += duration_ms
            self.forwarded_silence_frame_count += 1
            if not self.first_forwarded_at_ms:
                self.first_forwarded_at_ms = now_ms
            self.last_forwarded_at_ms = now_ms
            self.last_audio_gate = "tail_silence"
            return b"\x00" * len(audio)

    def _now_ms(self):
        return int(self.clock_ms())

    def _pcm16_duration_ms(self, audio):
        sample_count = len(audio) // 2
        if sample_count <= 0:
            return 0
        return max(1, int(round(sample_count * 1000 / self.VOICE_SAMPLE_RATE)))

    def _pcm16_level(self, audio):
        sample_count = len(audio) // 2
        if sample_count <= 0:
            return 0, 0
        total_square = 0
        peak = 0
        for offset in range(0, sample_count * 2, 2):
            sample = int.from_bytes(audio[offset : offset + 2], "little", signed=True)
            absolute = abs(sample)
            total_square += sample * sample
            if absolute > peak:
                peak = absolute
        rms = (total_square / sample_count) ** 0.5 / 32768
        return rms, peak / 32768

    def stop(self, payload=None):
        with self._lock:
            session = self._session
            thread = self._thread
            self._active = False
            self._stage = "idle"

        error = ""
        if session is not None:
            try:
                session.finish()
            except Exception as exc:
                error = str(exc)
        robot_stop = self._stop_robot_audio()
        if thread is not None and thread is not threading.current_thread():
            thread.join(timeout=1.0)

        with self._lock:
            if session is self._session:
                self._session = None
                self._thread = None
                self._raw_config = {}
            if robot_stop:
                self.last_robot_audio = robot_stop
            if error:
                self.last_error = error
            return self._snapshot_unlocked(ok=not bool(error))

    def _stop_robot_audio(self):
        stop = getattr(self.robot_audio, "stop_realtime_voice", None)
        if not callable(stop):
            return {}
        try:
            return stop()
        except Exception as exc:
            return {"ok": False, "error": str(exc)}

    def _reader_loop(self, session):
        while True:
            with self._lock:
                if not self._active or session is not self._session:
                    return
            try:
                event = session.recv_event()
            except Exception as exc:
                with self._lock:
                    if session is self._session and self._active:
                        self.last_error = str(exc)
                        self._stage = "error"
                        self._active = False
                return

            if not isinstance(event, dict):
                continue
            with self._lock:
                if not self._active or session is not self._session:
                    return
            self._mark_api_event(event)
            if event.get("type") == "response":
                self._handle_response(session, event)
            else:
                user_text = self._extract_user_text_from_event(event)
                with self._lock:
                    if session is self._session:
                        if user_text:
                            self.last_user_text = user_text
                        self.last_event = event

    def _extract_user_text_from_event(self, event):
        text = str((event or {}).get("userText") or "").strip()
        if text:
            return text
        return extract_realtime_payload_text((event or {}).get("payload") or {})

    def _mark_api_event(self, event):
        now_ms = self._now_ms()
        event_id = event.get("event")
        with self._lock:
            if not self.first_api_event_at_ms:
                self.first_api_event_at_ms = now_ms
            if event_id == EVENT_CHAT_RESPONSE and not self.first_chat_at_ms:
                self.first_chat_at_ms = now_ms
            if event_id == EVENT_TTS_RESPONSE and not self.first_tts_at_ms:
                self.first_tts_at_ms = now_ms
            if event.get("type") == "response" or event_id == EVENT_TTS_ENDED:
                self.tts_ended_at_ms = now_ms
                if not self.first_tts_at_ms:
                    self.first_tts_at_ms = now_ms

    def _handle_response(self, session, event):
        audio = event.get("audioPcm16") or b""
        if not isinstance(audio, (bytes, bytearray)):
            audio = b""
        sample_rate = _int_or_default(event.get("audioSampleRate"), 24000)
        audio_format = event.get("audioFormat") or "pcm_s16le"

        with self._lock:
            if not self._active or session is not self._session:
                return
            self._stage = "speaking"
            self.response_count += 1
            response_user_text = str(event.get("userText") or "").strip()
            if response_user_text:
                self.last_user_text = response_user_text
            self.last_chat_text = event.get("chatText") or ""
            self.last_audio_bytes = int(event.get("audioBytes") or len(audio))
            self.last_event = {
                "type": "response",
                "userText": self.last_user_text,
                "chatText": self.last_chat_text,
                "audioBytes": self.last_audio_bytes,
            }
            user_text = self.last_user_text

        agent_dispatch = dispatch_realtime_reply_action(
            self.agent_dispatcher,
            event.get("chatText"),
            "realtime_voice_session",
            user_text=user_text,
            agent_model=self.agent_model,
            agent_config=self.config_helper._agent_dispatch_config(self._raw_config),
            agent_monitor=self.agent_monitor,
        )
        with self._lock:
            if session is not self._session:
                return
            self.last_agent_dispatch = agent_dispatch

        robot_result = {}
        if audio and self.robot_audio is not None:
            try:
                with self._lock:
                    if not self._active or session is not self._session:
                        return
                    self.robot_play_start_at_ms = self._now_ms()
                robot_result = self.robot_audio.play_realtime_voice(
                    bytes(audio),
                    sample_rate=sample_rate,
                    audio_format=audio_format,
                )
                with self._lock:
                    self.robot_play_end_at_ms = self._now_ms()
            except Exception as exc:
                robot_result = {"ok": False, "error": str(exc)}
                with self._lock:
                    self.robot_play_end_at_ms = self._now_ms()

        with self._lock:
            if session is not self._session:
                return
            self.last_robot_audio = robot_result
            self._voice_has_started = False
            self._trailing_silence_ms = 0
            self._stage = "listening" if self._active else self._stage
            if robot_result and not robot_result.get("ok", True):
                self.last_error = robot_result.get("error") or "robot audio playback failed"

    def _snapshot_unlocked(self, ok=True, stage=None):
        return {
            "ok": bool(ok),
            "active": bool(self._active),
            "stage": stage or self._stage,
            "capturedFrameCount": self.captured_frame_count,
            "audioFrameCount": self.audio_frame_count,
            "audioBytesSent": self.audio_bytes_sent,
            "droppedSilenceFrameCount": self.dropped_silence_frame_count,
            "forwardedSilenceFrameCount": self.forwarded_silence_frame_count,
            "lastVoiceLevel": self.last_voice_level,
            "lastVoicePeak": self.last_voice_peak,
            "lastAudioGate": self.last_audio_gate,
            "responseCount": self.response_count,
            "lastUserText": self.last_user_text,
            "lastChatText": self.last_chat_text,
            "lastRobotAudio": self.last_robot_audio,
            "lastAgentDispatch": self.last_agent_dispatch,
            "lastAudioBytes": self.last_audio_bytes,
            "lastError": self.last_error,
            "lastEvent": self.last_event,
            "startedAtMs": self.started_at_ms,
            "firstFrameAtMs": self.first_frame_at_ms,
            "lastFrameAtMs": self.last_frame_at_ms,
            "firstForwardedAtMs": self.first_forwarded_at_ms,
            "lastForwardedAtMs": self.last_forwarded_at_ms,
            "firstSpeechAtMs": self.first_speech_at_ms,
            "lastSpeechAtMs": self.last_speech_at_ms,
            "firstApiEventAtMs": self.first_api_event_at_ms,
            "firstChatAtMs": self.first_chat_at_ms,
            "firstTtsAtMs": self.first_tts_at_ms,
            "ttsEndedAtMs": self.tts_ended_at_ms,
            "robotPlayStartAtMs": self.robot_play_start_at_ms,
            "robotPlayEndAtMs": self.robot_play_end_at_ms,
            "latency": self._latency_unlocked(),
        }

    def _latency_unlocked(self):
        return {
            "startToFirstFrameMs": self._delta_ms(self.started_at_ms, self.first_frame_at_ms),
            "firstFrameToFirstSpeechMs": self._delta_ms(self.first_frame_at_ms, self.first_speech_at_ms),
            "firstSpeechToFirstApiEventMs": self._delta_ms(self.first_speech_at_ms, self.first_api_event_at_ms),
            "lastSpeechToFirstApiEventMs": self._delta_ms(self.last_speech_at_ms, self.first_api_event_at_ms),
            "lastForwardedToFirstApiEventMs": self._delta_ms(self.last_forwarded_at_ms, self.first_api_event_at_ms),
            "firstApiEventToTtsEndedMs": self._delta_ms(self.first_api_event_at_ms, self.tts_ended_at_ms),
            "lastSpeechToTtsEndedMs": self._delta_ms(self.last_speech_at_ms, self.tts_ended_at_ms),
            "ttsEndedToRobotPlayStartMs": self._delta_ms(self.tts_ended_at_ms, self.robot_play_start_at_ms),
            "robotPlayMs": self._delta_ms(self.robot_play_start_at_ms, self.robot_play_end_at_ms),
        }

    def _delta_ms(self, start, end):
        if not start or not end:
            return None
        return max(0, int(end) - int(start))


class HostRobotProxy:
    def __init__(self, client):
        self.client = client

    def status(self):
        return self.client.get("/api/status")

    def catalog(self):
        return self.client.get("/api/catalog")

    def catalog_assets(self):
        return self.client.get("/api/catalog/assets")

    def catalog_logic(self):
        return self.client.get("/api/catalog/logic")

    def catalog_conflicts(self):
        return self.client.get("/api/catalog/conflicts")

    def camera_status(self):
        return self.client.get("/api/camera/status")

    def camera_set(self, payload):
        return self.client.post("/api/camera/set", payload)

    def camera_catchphrase(self, payload):
        return self.client.post("/api/camera/catchphrase", payload)

    def camera_face_report(self, payload):
        return self.client.post("/api/camera/face-report", payload)

    def camera_wake(self, payload):
        return self.client.post("/api/camera/wake", payload)

    def camera_clear(self):
        return self.client.post("/api/camera/clear", {})

    def asset_invoke(self, payload):
        return self.client.post("/api/asset/invoke", payload)

    def play_realtime_voice(self, audio, sample_rate=24000, audio_format="pcm_s16le"):
        if str(audio_format or "").lower() != "pcm_s16le":
            return {"ok": False, "error": f"unsupported audio format: {audio_format}"}
        raw_audio = _pcm_s16le_to_raw_u8_mono(audio, source_rate=sample_rate, target_rate=16000)
        if not raw_audio:
            return {"ok": False, "error": "no realtime audio to play"}
        upload = self.client.mumble_upload(raw_audio, filename="realtime-api.raw", timeout=15)
        result = {
            "ok": bool(upload.get("ok", True)),
            "format": "raw-u8-16k-mono",
            "sourceFormat": audio_format,
            "sourceSampleRate": int(_int_or_default(sample_rate, 24000)),
            "sampleRate": 16000,
            "sourceBytes": len(audio or b""),
            "uploadedBytes": len(raw_audio),
            "upload": upload,
        }
        if not upload.get("ok", True):
            result["error"] = upload.get("error") or "robot audio upload failed"
            return result
        enabled = self.client.mumble_set(enabled=True, volume=MAX_API_VOICE_VOLUME, timeout=5)
        result["enabled"] = enabled
        if not enabled.get("ok", True):
            result["ok"] = False
            result["error"] = enabled.get("error") or "robot audio enable failed"
            return result
        play = self.client.mumble_play(mode="full", timeout=10)
        result["play"] = play
        result["ok"] = bool(play.get("ok", True))
        if not play.get("ok", True):
            result["error"] = play.get("error") or "robot audio playback failed"
        return result

    def stop_realtime_voice(self):
        return self.client.mumble_stop(timeout=5)


class HostConsoleRequestHandler(BaseHTTPRequestHandler):
    proxy = None
    robot_base_url = ""
    model_config = None
    realtime_voice = None
    realtime_voice_session = None
    vision_tracker = None
    agent_workflows = None
    agent_monitor = None
    agent_mcp = None

    def send_json(self, status, payload):
        body = json.dumps(payload, ensure_ascii=False).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def send_html(self):
        body = WEB_CONSOLE_PATH.read_bytes()
        self.send_response(200)
        self.send_header("Content-Type", "text/html; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def read_json(self):
        length = int(self.headers.get("Content-Length", "0"))
        if length <= 0:
            return {}
        raw = self.rfile.read(length).decode("utf-8")
        return json.loads(raw)

    def call_proxy(self, fn):
        try:
            return fn()
        except Exception as exc:
            return {"ok": False, "error": str(exc)}

    def dispatch_agent_action(self, payload):
        payload = payload or {}
        command = payload.get("command") or payload.get("action_id") or payload.get("asset_id") or payload.get("intent") or ""
        event_id = None
        if self.agent_monitor is not None:
            event_id = self.agent_monitor.started(
                source=payload.get("source") or "agent_dispatch",
                mode="manual",
                command=command,
            )
        result = None
        try:
            result = self.agent_workflows.dispatch_action(payload)
            return result
        finally:
            if event_id is not None:
                self.agent_monitor.finished(event_id, result or {"ok": False, "stage": "agent_dispatch", "error": "agent dispatch did not return a result"})

    def do_GET(self):
        if self.path == "/" or self.path == "/index.html":
            self.send_html()
            return
        if self.path == "/api/config":
            self.send_json(200, {"ok": True, "robotBaseUrl": self.robot_base_url})
            return
        if self.path == "/api/robot/status":
            result = self.call_proxy(self.proxy.status)
            self.send_json(200 if result.get("ok", True) else 502, result)
            return
        if self.path == "/api/robot/catalog":
            result = self.call_proxy(self.proxy.catalog)
            self.send_json(200 if result.get("ok", True) else 502, result)
            return
        if self.path == "/api/robot/catalog/assets":
            result = self.call_proxy(self.proxy.catalog_assets)
            self.send_json(200 if result.get("ok", True) else 502, result)
            return
        if self.path == "/api/robot/catalog/logic":
            result = self.call_proxy(self.proxy.catalog_logic)
            self.send_json(200 if result.get("ok", True) else 502, result)
            return
        if self.path == "/api/robot/catalog/conflicts":
            result = self.call_proxy(self.proxy.catalog_conflicts)
            self.send_json(200 if result.get("ok", True) else 502, result)
            return
        if self.path == "/api/robot/camera/status":
            result = self.call_proxy(self.proxy.camera_status)
            self.send_json(200 if result.get("ok", True) else 502, result)
            return
        if self.path == "/api/host/model-config/status":
            self.send_json(200, self.model_config.status())
            return
        if self.path == "/api/host/realtime-voice/session/status":
            self.send_json(200, self.realtime_voice_session.status())
            return
        if self.path == "/api/host/vision/status":
            self.send_json(200, self.vision_tracker.status())
            return
        if self.path == "/api/agent/status":
            result = self.call_proxy(self.agent_workflows.robot_status)
            self.send_json(200 if result.get("ok", True) else 502, result)
            return
        if self.path == "/api/agent/dispatch-monitor":
            self.send_json(200, self.agent_monitor.status())
            return
        self.send_json(404, {"ok": False, "error": "not found"})

    def do_POST(self):
        try:
            payload = self.read_json()
        except json.JSONDecodeError:
            self.send_json(400, {"ok": False, "error": "invalid json"})
            return

        if self.path == "/" and isinstance(payload, dict) and "jsonrpc" in payload:
            result = self.agent_mcp.handle_request(payload)
            if result is None:
                self.send_json(200, {})
                return
            self.send_json(200, result)
            return

        routes = {
            "/api/robot/camera/set": lambda: self.proxy.camera_set(payload),
            "/api/robot/camera/catchphrase": lambda: self.proxy.camera_catchphrase(payload),
            "/api/robot/camera/face-report": lambda: self.proxy.camera_face_report(payload),
            "/api/robot/camera/wake": lambda: self.proxy.camera_wake(payload),
            "/api/robot/camera/clear": self.proxy.camera_clear,
            "/api/robot/asset/invoke": lambda: self.proxy.asset_invoke(payload),
            "/api/host/realtime-voice/text": lambda: self.realtime_voice.run_text(payload),
            "/api/host/realtime-voice/session/start": lambda: self.realtime_voice_session.start(payload),
            "/api/host/realtime-voice/session/frame": lambda: self.realtime_voice_session.frame(payload),
            "/api/host/realtime-voice/session/stop": lambda: self.realtime_voice_session.stop(payload),
            "/api/host/vision/step": lambda: self.vision_tracker.step(payload),
            "/api/host/vision/start": lambda: self.vision_tracker.start(payload),
            "/api/host/vision/stop": lambda: self.vision_tracker.stop(payload),
            "/api/host/model-config/save": lambda: self.model_config.save(payload),
            "/api/agent/invoke-safe-behavior": lambda: self.agent_workflows.invoke_safe_behavior(payload),
            "/api/agent/face-detect-and-center": lambda: self.agent_workflows.face_detect_and_center(payload),
            "/api/agent/find-and-greet": lambda: self.agent_workflows.find_and_greet(payload),
            "/api/agent/dispatch-action": lambda: self.dispatch_agent_action(payload),
            "/api/agent/coding-event": lambda: self.agent_workflows.coding_event(payload),
        }
        if self.path not in routes:
            self.send_json(404, {"ok": False, "error": "not found"})
            return
        result = self.call_proxy(routes[self.path])
        self.send_json(200 if result.get("ok", True) else 502, result)


def make_handler(
    proxy,
    robot_base_url,
    model_config=None,
    realtime_voice=None,
    vision_tracker=None,
    agent_workflows=None,
    realtime_voice_session=None,
    agent_monitor=None,
):
    class BoundHostConsoleRequestHandler(HostConsoleRequestHandler):
        pass

    BoundHostConsoleRequestHandler.proxy = proxy
    BoundHostConsoleRequestHandler.robot_base_url = robot_base_url
    BoundHostConsoleRequestHandler.model_config = model_config or HostModelConfigStore()
    BoundHostConsoleRequestHandler.vision_tracker = vision_tracker or HostVisionTracker(proxy.client)
    BoundHostConsoleRequestHandler.agent_monitor = agent_monitor or AgentDispatchMonitor()
    BoundHostConsoleRequestHandler.agent_workflows = agent_workflows or RobotAgentWorkflowService(
        proxy.client,
        vision_tracker=BoundHostConsoleRequestHandler.vision_tracker,
        allowed_asset_ids=BoundHostConsoleRequestHandler.model_config.agent_allowed_assets
        if hasattr(BoundHostConsoleRequestHandler.model_config, "agent_allowed_assets")
        else None,
    )
    BoundHostConsoleRequestHandler.realtime_voice = realtime_voice or HostRealtimeVoiceService(
        BoundHostConsoleRequestHandler.model_config,
        robot_audio=proxy,
        agent_dispatcher=BoundHostConsoleRequestHandler.agent_workflows,
        agent_monitor=BoundHostConsoleRequestHandler.agent_monitor,
    )
    BoundHostConsoleRequestHandler.realtime_voice_session = realtime_voice_session or HostRealtimeVoiceSessionService(
        BoundHostConsoleRequestHandler.model_config,
        robot_audio=proxy,
        agent_dispatcher=BoundHostConsoleRequestHandler.agent_workflows,
        agent_monitor=BoundHostConsoleRequestHandler.agent_monitor,
    )
    BoundHostConsoleRequestHandler.agent_mcp = AgentMcpServer(
        BoundHostConsoleRequestHandler.agent_workflows,
        agent_config_provider=BoundHostConsoleRequestHandler.model_config.agent_dispatch_config
        if hasattr(BoundHostConsoleRequestHandler.model_config, "agent_dispatch_config")
        else None,
    )
    return BoundHostConsoleRequestHandler


def run_server(robot_base_url, bind="127.0.0.1", port=8766):
    proxy = HostRobotProxy(RobotClient(robot_base_url))
    model_config = HostModelConfigStore()
    vision_tracker = HostVisionTracker(proxy.client)
    agent_workflows = RobotAgentWorkflowService(
        proxy.client,
        vision_tracker=vision_tracker,
        allowed_asset_ids=model_config.agent_allowed_assets,
    )
    agent_model = AgentDispatchModel()
    agent_monitor = AgentDispatchMonitor()
    realtime_voice = HostRealtimeVoiceService(
        model_config,
        robot_audio=proxy,
        agent_dispatcher=agent_workflows,
        agent_model=agent_model,
        agent_monitor=agent_monitor,
    )
    realtime_voice_session = HostRealtimeVoiceSessionService(
        model_config,
        robot_audio=proxy,
        agent_dispatcher=agent_workflows,
        agent_model=agent_model,
        agent_monitor=agent_monitor,
    )
    server = ThreadingHTTPServer(
        (bind, port),
        make_handler(
            proxy,
            robot_base_url,
            model_config,
            realtime_voice,
            vision_tracker,
            agent_workflows,
            realtime_voice_session,
            agent_monitor,
        ),
    )
    print(f"Host console: http://{bind}:{port}")
    print(f"Robot: {robot_base_url}")
    server.serve_forever()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--robot", default=os.environ.get("ROBOT_BASE_URL", "http://deskbot.local"))
    parser.add_argument("--bind", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8766)
    args = parser.parse_args()
    run_server(args.robot, args.bind, args.port)


if __name__ == "__main__":
    main()
