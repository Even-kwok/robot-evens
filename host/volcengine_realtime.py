import base64
import gzip
import hashlib
import json
import os
import socket
import ssl
import struct
import uuid
from dataclasses import dataclass
from urllib.parse import urlparse


PROTOCOL_VERSION = 0b0001
DEFAULT_HEADER_SIZE = 0b0001

CLIENT_FULL_REQUEST = 0b0001
CLIENT_AUDIO_ONLY_REQUEST = 0b0010

SERVER_FULL_RESPONSE = 0b1001
SERVER_AUDIO_RESPONSE = 0b1011
SERVER_ERROR_RESPONSE = 0b1111

NO_SEQUENCE = 0b0000
MSG_WITH_EVENT = 0b0100

NO_SERIALIZATION = 0b0000
JSON_SERIALIZATION = 0b0001

NO_COMPRESSION = 0b0000
GZIP_COMPRESSION = 0b0001

EVENT_START_CONNECTION = 1
EVENT_FINISH_CONNECTION = 2
EVENT_START_SESSION = 100
EVENT_FINISH_SESSION = 102
EVENT_TASK_REQUEST = 200
EVENT_CHAT_TEXT_QUERY = 501

EVENT_CONNECTION_STARTED = 50
EVENT_CONNECTION_FINISHED = 52
EVENT_SESSION_STARTED = 150
EVENT_SESSION_FINISHED = 152
EVENT_SESSION_FAILED = 153
EVENT_TTS_RESPONSE = 352
EVENT_TTS_ENDED = 359
EVENT_CHAT_RESPONSE = 550
EVENT_CHAT_TEXT_QUERY_CONFIRMED = 553
EVENT_CHAT_ENDED = 559
EVENT_DIALOG_COMMON_ERROR = 599

VOLCENGINE_REALTIME_URL = "wss://openspeech.bytedance.com/api/v3/realtime/dialogue"
VOLCENGINE_RESOURCE_ID = "volc.speech.dialog"
VOLCENGINE_APP_KEY = "PlgvMymc7f3tQnJ6"


class VolcengineRealtimeError(RuntimeError):
    pass


def generate_header(
    version=PROTOCOL_VERSION,
    message_type=CLIENT_FULL_REQUEST,
    message_type_specific_flags=MSG_WITH_EVENT,
    serial_method=JSON_SERIALIZATION,
    compression_type=GZIP_COMPRESSION,
    reserved_data=0x00,
    extension_header=b"",
):
    header = bytearray()
    header_size = int(len(extension_header) / 4) + 1
    header.append((version << 4) | header_size)
    header.append((message_type << 4) | message_type_specific_flags)
    header.append((serial_method << 4) | compression_type)
    header.append(reserved_data)
    header.extend(extension_header)
    return header


def _gzip(data):
    return gzip.compress(data, mtime=0)


def _json_bytes(payload):
    return json.dumps(payload, ensure_ascii=False, separators=(",", ":")).encode("utf-8")


def build_full_client_event(event_id, payload=None, session_id=None):
    body = bytearray(generate_header())
    body.extend(int(event_id).to_bytes(4, "big"))
    if session_id is not None:
        session_bytes = str(session_id).encode("utf-8")
        body.extend(len(session_bytes).to_bytes(4, "big"))
        body.extend(session_bytes)
    payload_bytes = _gzip(_json_bytes(payload or {}))
    body.extend(len(payload_bytes).to_bytes(4, "big"))
    body.extend(payload_bytes)
    return bytes(body)


def build_audio_client_event(audio, session_id):
    body = bytearray(
        generate_header(
            message_type=CLIENT_AUDIO_ONLY_REQUEST,
            serial_method=NO_SERIALIZATION,
        )
    )
    body.extend(int(EVENT_TASK_REQUEST).to_bytes(4, "big"))
    session_bytes = str(session_id).encode("utf-8")
    body.extend(len(session_bytes).to_bytes(4, "big"))
    body.extend(session_bytes)
    payload_bytes = _gzip(audio or b"")
    body.extend(len(payload_bytes).to_bytes(4, "big"))
    body.extend(payload_bytes)
    return bytes(body)


def build_text_client_event(text, session_id):
    return build_full_client_event(EVENT_CHAT_TEXT_QUERY, {"content": str(text or "")}, session_id=session_id)


def parse_response_frame(data):
    if isinstance(data, str):
        return {"message_type": "TEXT", "payload_msg": data}
    if not data or len(data) < 4:
        raise VolcengineRealtimeError("empty realtime frame")

    header_size = data[0] & 0x0F
    message_type = data[1] >> 4
    flags = data[1] & 0x0F
    serialization_method = data[2] >> 4
    compression_method = data[2] & 0x0F
    payload = data[header_size * 4 :]
    result = {}

    if message_type in {SERVER_FULL_RESPONSE, SERVER_AUDIO_RESPONSE}:
        result["message_type"] = "SERVER_FULL_RESPONSE" if message_type == SERVER_FULL_RESPONSE else "SERVER_AUDIO_RESPONSE"
        offset = 0
        if flags & MSG_WITH_EVENT:
            result["event"] = int.from_bytes(payload[offset : offset + 4], "big")
            offset += 4
        session_size = int.from_bytes(payload[offset : offset + 4], "big", signed=True)
        offset += 4
        if session_size > 0:
            result["session_id"] = payload[offset : offset + session_size].decode("utf-8", errors="replace")
            offset += session_size
        payload_size = int.from_bytes(payload[offset : offset + 4], "big", signed=False)
        offset += 4
        payload_msg = payload[offset : offset + payload_size]
    elif message_type == SERVER_ERROR_RESPONSE:
        result["message_type"] = "SERVER_ERROR"
        result["code"] = int.from_bytes(payload[:4], "big", signed=False)
        payload_size = int.from_bytes(payload[4:8], "big", signed=False)
        payload_msg = payload[8 : 8 + payload_size]
    else:
        result["message_type"] = f"UNKNOWN_{message_type}"
        result["payload_size"] = len(payload)
        result["payload_msg"] = payload
        return result

    if compression_method == GZIP_COMPRESSION:
        payload_msg = gzip.decompress(payload_msg)
    if serialization_method == JSON_SERIALIZATION:
        payload_msg = json.loads(payload_msg.decode("utf-8"))
    elif serialization_method != NO_SERIALIZATION:
        payload_msg = payload_msg.decode("utf-8", errors="replace")

    result["payload_size"] = payload_size
    result["payload_msg"] = payload_msg
    return result


def extract_realtime_payload_text(payload):
    if isinstance(payload, str):
        return payload.strip()
    if not isinstance(payload, dict):
        return ""
    for key in ("content", "text", "query", "transcript", "asr_text", "asrText"):
        value = payload.get(key)
        if value:
            return str(value).strip()
    for key in ("result", "payload", "data"):
        text = extract_realtime_payload_text(payload.get(key))
        if text:
            return text
    return ""


def encode_server_event_for_test(event_id, payload=None, session_id="session"):
    body = bytearray(
        generate_header(
            message_type=SERVER_FULL_RESPONSE,
            message_type_specific_flags=MSG_WITH_EVENT,
            serial_method=JSON_SERIALIZATION,
            compression_type=GZIP_COMPRESSION,
        )
    )
    body.extend(int(event_id).to_bytes(4, "big"))
    session_bytes = str(session_id).encode("utf-8")
    body.extend(len(session_bytes).to_bytes(4, "big"))
    body.extend(session_bytes)
    payload_bytes = _gzip(_json_bytes(payload or {}))
    body.extend(len(payload_bytes).to_bytes(4, "big"))
    body.extend(payload_bytes)
    return bytes(body)


def encode_server_audio_event_for_test(event_id, audio=None, session_id="session"):
    body = bytearray(
        generate_header(
            message_type=SERVER_AUDIO_RESPONSE,
            message_type_specific_flags=MSG_WITH_EVENT,
            serial_method=NO_SERIALIZATION,
            compression_type=NO_COMPRESSION,
        )
    )
    body.extend(int(event_id).to_bytes(4, "big"))
    session_bytes = str(session_id).encode("utf-8")
    body.extend(len(session_bytes).to_bytes(4, "big"))
    body.extend(session_bytes)
    payload_bytes = audio or b""
    body.extend(len(payload_bytes).to_bytes(4, "big"))
    body.extend(payload_bytes)
    return bytes(body)


def build_voice_system_prompt(known_user=None):
    context = json.dumps({"known_user": known_user or None}, ensure_ascii=False, separators=(",", ":"))
    return (
        "你是桌面机器人的实时语音大脑，可以用中文自然语音回答用户。\n"
        "回答要短、温暖、像桌面小机器人一样有陪伴感；不要读出 JSON、代码块或内部调试信息。\n"
        "机器人硬件会负责播放你的语音，电脑负责 API 和音频转发。\n"
        "如果用户只是叫醒你，先简短回应并等待后续问题。\n"
        "如果用户问你认不认识他、记不记得他、知不知道他，known_user 存在时回答必须以“当然认识”开头；"
        "known_user 不存在时回答必须以“还不认识”开头。\n"
        f"当前上下文: {context}"
    )


def build_start_session_payload(
    system_prompt,
    input_mod="audio",
    model_version="1.2.1.1",
    speaker="zh_male_yunzhou_jupiter_bigtts",
    recv_timeout=15,
):
    asr = {
        "audio_info": {
            "format": "pcm",
            "sample_rate": 16000,
            "channel": 1,
        },
        "extra": {
            "end_smooth_window_ms": 800,
        },
    }
    return {
        "asr": asr,
        "tts": {
            "speaker": speaker,
            "audio_config": {
                "channel": 1,
                "format": "pcm_s16le",
                "sample_rate": 24000,
            },
        },
        "dialog": {
            "bot_name": "桌面机器人",
            "system_role": system_prompt,
            "speaking_style": "自然、简短、口语化，用中文回答。",
            "extra": {
                "strict_audit": False,
                "recv_timeout": int(recv_timeout),
                "input_mod": input_mod,
                "model": model_version,
            },
        },
    }


@dataclass
class VolcengineRealtimeConfig:
    app_id: str
    access_key: str
    endpoint: str = VOLCENGINE_REALTIME_URL
    resource_id: str = VOLCENGINE_RESOURCE_ID
    app_key: str = VOLCENGINE_APP_KEY
    model_version: str = "1.2.1.1"
    speaker: str = "zh_male_yunzhou_jupiter_bigtts"
    recv_timeout: int = 15
    insecure_tls: bool = False

    def headers(self, connect_id=None):
        return {
            "X-Api-App-ID": self.app_id,
            "X-Api-Access-Key": self.access_key,
            "X-Api-Resource-Id": self.resource_id,
            "X-Api-App-Key": self.app_key,
            "X-Api-Connect-Id": connect_id or str(uuid.uuid4()),
        }


class StdlibWebSocket:
    GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

    def __init__(self, url, headers=None, timeout=20, insecure_tls=False):
        self.url = url
        self.headers = headers or {}
        self.timeout = timeout
        self.insecure_tls = insecure_tls
        self.sock = None
        self.response_headers = {}

    def connect(self):
        parsed = urlparse(self.url)
        if parsed.scheme != "wss":
            raise VolcengineRealtimeError("only wss:// websocket URLs are supported")
        host = parsed.hostname
        port = parsed.port or 443
        path = parsed.path or "/"
        if parsed.query:
            path += "?" + parsed.query
        raw = socket.create_connection((host, port), timeout=self.timeout)
        context = ssl._create_unverified_context() if self.insecure_tls else ssl.create_default_context()
        self.sock = context.wrap_socket(raw, server_hostname=host)
        self.sock.settimeout(self.timeout)

        key = base64.b64encode(os.urandom(16)).decode("ascii")
        lines = [
            f"GET {path} HTTP/1.1",
            f"Host: {host}",
            "Upgrade: websocket",
            "Connection: Upgrade",
            f"Sec-WebSocket-Key: {key}",
            "Sec-WebSocket-Version: 13",
        ]
        for name, value in self.headers.items():
            lines.append(f"{name}: {value}")
        request = "\r\n".join(lines) + "\r\n\r\n"
        self.sock.sendall(request.encode("utf-8"))
        response = self._read_http_response()
        status_line, headers = response
        if " 101 " not in status_line:
            raise VolcengineRealtimeError(f"websocket handshake failed: {status_line}")
        accept = headers.get("sec-websocket-accept", "")
        expected = base64.b64encode(hashlib.sha1((key + self.GUID).encode("ascii")).digest()).decode("ascii")
        if accept and accept != expected:
            raise VolcengineRealtimeError("websocket handshake accept header mismatch")
        self.response_headers = headers
        return self

    def _read_http_response(self):
        data = b""
        while b"\r\n\r\n" not in data:
            chunk = self.sock.recv(4096)
            if not chunk:
                break
            data += chunk
        text = data.decode("iso-8859-1", errors="replace")
        head = text.split("\r\n\r\n", 1)[0]
        lines = head.split("\r\n")
        headers = {}
        for line in lines[1:]:
            if ":" in line:
                key, value = line.split(":", 1)
                headers[key.strip().lower()] = value.strip()
        return lines[0], headers

    def send_binary(self, payload):
        self._send_frame(0x2, payload)

    def send_close(self):
        try:
            self._send_frame(0x8, b"")
        except OSError:
            pass

    def _send_frame(self, opcode, payload):
        payload = payload or b""
        first = 0x80 | opcode
        mask_bit = 0x80
        length = len(payload)
        header = bytearray([first])
        if length < 126:
            header.append(mask_bit | length)
        elif length < 65536:
            header.append(mask_bit | 126)
            header.extend(struct.pack("!H", length))
        else:
            header.append(mask_bit | 127)
            header.extend(struct.pack("!Q", length))
        mask = os.urandom(4)
        masked = bytes(payload[i] ^ mask[i % 4] for i in range(length))
        self.sock.sendall(bytes(header) + mask + masked)

    def recv(self):
        while True:
            frame = self._recv_frame()
            opcode = frame["opcode"]
            if opcode == 0x1:
                return frame["payload"].decode("utf-8", errors="replace")
            if opcode == 0x2:
                return frame["payload"]
            if opcode == 0x8:
                raise VolcengineRealtimeError("websocket closed by server")
            if opcode == 0x9:
                self._send_frame(0xA, frame["payload"])

    def _recv_exact(self, size):
        data = b""
        while len(data) < size:
            chunk = self.sock.recv(size - len(data))
            if not chunk:
                raise VolcengineRealtimeError("websocket connection closed")
            data += chunk
        return data

    def _recv_frame(self):
        first, second = self._recv_exact(2)
        opcode = first & 0x0F
        masked = bool(second & 0x80)
        length = second & 0x7F
        if length == 126:
            length = struct.unpack("!H", self._recv_exact(2))[0]
        elif length == 127:
            length = struct.unpack("!Q", self._recv_exact(8))[0]
        mask = self._recv_exact(4) if masked else b""
        payload = self._recv_exact(length) if length else b""
        if masked:
            payload = bytes(payload[i] ^ mask[i % 4] for i in range(length))
        return {"opcode": opcode, "payload": payload}

    def close(self):
        if self.sock:
            self.send_close()
            self.sock.close()
            self.sock = None


class VolcengineRealtimeTextClient:
    def __init__(self, config, session_id=None, socket_factory=StdlibWebSocket):
        self.config = config
        self.session_id = session_id or str(uuid.uuid4())
        self.socket_factory = socket_factory
        self.socket = None
        self.events = []

    def connect(self, system_prompt, input_mod="audio"):
        connect_id = str(uuid.uuid4())
        self.socket = self.socket_factory(
            self.config.endpoint,
            headers=self.config.headers(connect_id=connect_id),
            timeout=max(10, int(self.config.recv_timeout) + 5),
            insecure_tls=self.config.insecure_tls,
        ).connect()
        self._send(build_full_client_event(EVENT_START_CONNECTION, {}))
        self._recv_until({EVENT_CONNECTION_STARTED})
        start_payload = build_start_session_payload(
            system_prompt,
            input_mod=input_mod,
            model_version=self.config.model_version,
            speaker=self.config.speaker,
            recv_timeout=self.config.recv_timeout,
        )
        self._send(build_full_client_event(EVENT_START_SESSION, start_payload, session_id=self.session_id))
        self._recv_until({EVENT_SESSION_STARTED})

    def send_audio(self, audio):
        self._send(build_audio_client_event(audio, self.session_id))

    def send_text(self, text):
        self._send(build_text_client_event(text, self.session_id))

    def finish(self):
        if not self.socket:
            return
        try:
            self._send(build_full_client_event(EVENT_FINISH_SESSION, {}, session_id=self.session_id))
            self._send(build_full_client_event(EVENT_FINISH_CONNECTION, {}))
        finally:
            self.socket.close()
            self.socket = None

    def run_audio_query(self, audio_pcm16, system_prompt, chunk_size=640):
        try:
            self.connect(system_prompt, input_mod="audio_file")
            audio_pcm16 = audio_pcm16 or b""
            for offset in range(0, len(audio_pcm16), chunk_size):
                chunk = audio_pcm16[offset : offset + chunk_size]
                if not chunk:
                    continue
                self.send_audio(chunk)
                time.sleep(max(0.005, len(chunk) / 2 / 16000))
            response = self._recv_response()
            return {
                "ok": True,
                "sessionId": self.session_id,
                "logId": self.socket.response_headers.get("x-tt-logid") if self.socket else "",
                **response,
                "events": self.events,
            }
        finally:
            self.finish()

    def run_text_query(self, text, system_prompt):
        try:
            self.connect(system_prompt, input_mod="text")
            self.send_text(text)
            response = self._recv_response()
            return {
                "ok": True,
                "sessionId": self.session_id,
                "logId": self.socket.response_headers.get("x-tt-logid") if self.socket else "",
                **response,
                "events": self.events,
            }
        finally:
            self.finish()

    def _send(self, frame):
        self.socket.send_binary(frame)

    def _recv_event(self):
        event = parse_response_frame(self.socket.recv())
        summary = {
            "event": event.get("event"),
            "message_type": event.get("message_type"),
            "payload_msg": (
                f"<audio {len(event.get('payload_msg') or b'')} bytes>"
                if event.get("event") == EVENT_TTS_RESPONSE
                else event.get("payload_msg")
            ),
        }
        self.events.append(summary)
        if event.get("message_type") == "SERVER_ERROR" or event.get("event") in {EVENT_SESSION_FAILED, EVENT_DIALOG_COMMON_ERROR}:
            raise VolcengineRealtimeError(json.dumps(event, ensure_ascii=False))
        return event

    def _recv_until(self, wanted_events):
        while True:
            event = self._recv_event()
            if event.get("event") in wanted_events:
                return event

    def _recv_response(self):
        chunks = []
        audio_chunks = []
        while True:
            event = self._recv_event()
            event_id = event.get("event")
            payload = event.get("payload_msg") or {}
            if event_id == EVENT_CHAT_RESPONSE and isinstance(payload, dict):
                chunks.append(str(payload.get("content", "")))
            if event_id == EVENT_TTS_RESPONSE:
                if isinstance(payload, (bytes, bytearray)):
                    audio_chunks.append(bytes(payload))
            if event_id == EVENT_TTS_ENDED:
                audio = b"".join(audio_chunks)
                return {
                    "chatText": "".join(chunks).strip(),
                    "audioPcm16": audio,
                    "audioBytes": len(audio),
                    "audioSampleRate": 24000,
                    "audioFormat": "pcm_s16le",
                }


class VolcengineRealtimeAudioSession:
    def __init__(self, config, session_id=None, socket_factory=StdlibWebSocket):
        self.client = VolcengineRealtimeTextClient(
            config,
            session_id=session_id,
            socket_factory=socket_factory,
        )
        self.user_chunks = []
        self.chat_chunks = []
        self.audio_chunks = []
        self.response_count = 0

    @property
    def session_id(self):
        return self.client.session_id

    @property
    def events(self):
        return self.client.events

    def start(self, system_prompt):
        self.client.connect(system_prompt, input_mod="audio")

    def send_audio(self, audio):
        self.client.send_audio(audio)

    def recv_event(self):
        event = self.client._recv_event()
        event_id = event.get("event")
        payload = event.get("payload_msg") or {}
        user_text = ""
        if event_id == EVENT_CHAT_TEXT_QUERY_CONFIRMED:
            user_text = extract_realtime_payload_text(payload)
            if user_text:
                self.user_chunks.append(user_text)
        if event_id == EVENT_CHAT_RESPONSE and isinstance(payload, dict):
            self.chat_chunks.append(str(payload.get("content", "")))
        if event_id == EVENT_TTS_RESPONSE and isinstance(payload, (bytes, bytearray)):
            self.audio_chunks.append(bytes(payload))
        if event_id != EVENT_TTS_ENDED:
            result = {
                "type": "event",
                "event": event_id,
                "payload": payload,
                "messageType": event.get("message_type"),
            }
            if user_text:
                result["userText"] = user_text
            return result

        audio = b"".join(self.audio_chunks)
        response = {
            "type": "response",
            "ok": True,
            "sessionId": self.client.session_id,
            "logId": self.client.socket.response_headers.get("x-tt-logid") if self.client.socket else "",
            "userText": "".join(self.user_chunks).strip(),
            "chatText": "".join(self.chat_chunks).strip(),
            "audioPcm16": audio,
            "audioBytes": len(audio),
            "audioSampleRate": 24000,
            "audioFormat": "pcm_s16le",
            "events": list(self.client.events),
        }
        self.user_chunks = []
        self.chat_chunks = []
        self.audio_chunks = []
        self.response_count += 1
        return response

    def finish(self):
        self.client.finish()


def run_volcengine_realtime_audio(config, audio_pcm16, system_prompt):
    client = VolcengineRealtimeTextClient(config)
    return client.run_audio_query(audio_pcm16, system_prompt)


def run_volcengine_realtime_text(config, text, system_prompt):
    client = VolcengineRealtimeTextClient(config)
    return client.run_text_query(text, system_prompt)
