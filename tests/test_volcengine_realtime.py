import gzip
import json
import unittest

from host.volcengine_realtime import (
    EVENT_CHAT_ENDED,
    EVENT_CHAT_RESPONSE,
    EVENT_CHAT_TEXT_QUERY,
    EVENT_CHAT_TEXT_QUERY_CONFIRMED,
    EVENT_CONNECTION_STARTED,
    EVENT_SESSION_STARTED,
    EVENT_TTS_ENDED,
    EVENT_TTS_RESPONSE,
    MSG_WITH_EVENT,
    SERVER_FULL_RESPONSE,
    VolcengineRealtimeConfig,
    VolcengineRealtimeAudioSession,
    build_audio_client_event,
    build_full_client_event,
    build_text_client_event,
    build_voice_system_prompt,
    build_start_session_payload,
    encode_server_audio_event_for_test,
    encode_server_event_for_test,
    generate_header,
    parse_response_frame,
    VolcengineRealtimeTextClient,
)


class VolcengineRealtimeProtocolTest(unittest.TestCase):
    def test_generate_header_uses_volcengine_binary_protocol_bits(self):
        self.assertEqual(bytes(generate_header()), bytes([0x11, 0x14, 0x11, 0x00]))

    def test_client_event_frame_contains_event_session_and_gzip_json_payload(self):
        frame = build_full_client_event(501, {"content": "你认识我吗？"}, session_id="session-1")

        self.assertEqual(frame[:4], bytes([0x11, 0x14, 0x11, 0x00]))
        self.assertEqual(int.from_bytes(frame[4:8], "big"), 501)
        session_size = int.from_bytes(frame[8:12], "big")
        self.assertEqual(frame[12 : 12 + session_size], b"session-1")
        payload_start = 12 + session_size
        payload_size = int.from_bytes(frame[payload_start : payload_start + 4], "big")
        payload = gzip.decompress(frame[payload_start + 4 : payload_start + 4 + payload_size])
        self.assertEqual(json.loads(payload.decode("utf-8")), {"content": "你认识我吗？"})

    def test_audio_event_frame_contains_task_request_and_gzip_pcm_payload(self):
        frame = build_audio_client_event(b"\x01\x02" * 320, session_id="session-1")

        self.assertEqual(frame[:4], bytes([0x11, 0x24, 0x01, 0x00]))
        self.assertEqual(int.from_bytes(frame[4:8], "big"), 200)
        session_size = int.from_bytes(frame[8:12], "big")
        payload_start = 12 + session_size
        payload_size = int.from_bytes(frame[payload_start : payload_start + 4], "big")
        payload = gzip.decompress(frame[payload_start + 4 : payload_start + 4 + payload_size])
        self.assertEqual(payload, b"\x01\x02" * 320)

    def test_text_event_frame_contains_chat_text_query_and_gzip_text_payload(self):
        frame = build_text_client_event("你认识我吗？", session_id="session-1")

        self.assertEqual(frame[:4], bytes([0x11, 0x14, 0x11, 0x00]))
        self.assertEqual(int.from_bytes(frame[4:8], "big"), EVENT_CHAT_TEXT_QUERY)
        session_size = int.from_bytes(frame[8:12], "big")
        self.assertEqual(frame[12 : 12 + session_size], b"session-1")
        payload_start = 12 + session_size
        payload_size = int.from_bytes(frame[payload_start : payload_start + 4], "big")
        payload = gzip.decompress(frame[payload_start + 4 : payload_start + 4 + payload_size])
        self.assertEqual(json.loads(payload.decode("utf-8")), {"content": "你认识我吗？"})

    def test_response_frame_decodes_chat_response_payload(self):
        frame = encode_server_event_for_test(
            550,
            {"content": "你好呀"},
            "session-1",
        )

        result = parse_response_frame(frame)

        self.assertEqual(result["message_type"], "SERVER_FULL_RESPONSE")
        self.assertEqual(result["event"], 550)
        self.assertEqual(result["session_id"], "session-1")
        self.assertEqual(result["payload_msg"]["content"], "你好呀")

    def test_response_frame_decodes_tts_audio_payload(self):
        frame = encode_server_audio_event_for_test(352, b"\x01\x02\x03\x04", "session-1")

        result = parse_response_frame(frame)

        self.assertEqual(result["message_type"], "SERVER_AUDIO_RESPONSE")
        self.assertEqual(result["event"], 352)
        self.assertEqual(result["session_id"], "session-1")
        self.assertEqual(result["payload_msg"], b"\x01\x02\x03\x04")

    def test_text_query_collects_chat_text_and_tts_audio(self):
        frames = [
            encode_server_event_for_test(EVENT_CONNECTION_STARTED, {}, "session-1"),
            encode_server_event_for_test(EVENT_SESSION_STARTED, {}, "session-1"),
            encode_server_event_for_test(EVENT_CHAT_RESPONSE, {"content": "你好"}, "session-1"),
            encode_server_event_for_test(EVENT_CHAT_ENDED, {}, "session-1"),
            encode_server_audio_event_for_test(EVENT_TTS_RESPONSE, b"\x01\x02\x03\x04", "session-1"),
            encode_server_event_for_test(EVENT_TTS_ENDED, {}, "session-1"),
        ]

        class FakeWebSocket:
            def __init__(self, *args, **kwargs):
                self.sent = []
                self.frames = list(frames)
                self.response_headers = {"x-tt-logid": "log-1"}

            def connect(self):
                return self

            def send_binary(self, payload):
                self.sent.append(payload)

            def recv(self):
                return self.frames.pop(0)

            def close(self):
                pass

        socket_holder = {}

        def socket_factory(*args, **kwargs):
            socket_holder["socket"] = FakeWebSocket(*args, **kwargs)
            return socket_holder["socket"]

        client = VolcengineRealtimeTextClient(
            VolcengineRealtimeConfig(app_id="app", access_key="token"),
            session_id="session-1",
            socket_factory=socket_factory,
        )

        result = client.run_text_query("你认识我吗？", build_voice_system_prompt("owner"))

        self.assertEqual(result["ok"], True)
        self.assertEqual(result["chatText"], "你好")
        self.assertEqual(result["audioPcm16"], b"\x01\x02\x03\x04")
        self.assertEqual(result["audioBytes"], 4)
        self.assertEqual(result["audioSampleRate"], 24000)
        self.assertEqual(result["audioFormat"], "pcm_s16le")
        self.assertTrue(any(event["payload_msg"] == "<audio 4 bytes>" for event in result["events"]))

    def test_audio_session_streams_chunks_before_response_is_finished(self):
        frames = [
            encode_server_event_for_test(EVENT_CONNECTION_STARTED, {}, "session-1"),
            encode_server_event_for_test(EVENT_SESSION_STARTED, {}, "session-1"),
            encode_server_event_for_test(EVENT_CHAT_RESPONSE, {"content": "收到"}, "session-1"),
            encode_server_event_for_test(EVENT_CHAT_ENDED, {}, "session-1"),
            encode_server_audio_event_for_test(EVENT_TTS_RESPONSE, b"\x05\x06", "session-1"),
            encode_server_event_for_test(EVENT_TTS_ENDED, {}, "session-1"),
        ]

        class FakeWebSocket:
            def __init__(self, *args, **kwargs):
                self.sent = []
                self.frames = list(frames)
                self.response_headers = {"x-tt-logid": "log-1"}

            def connect(self):
                return self

            def send_binary(self, payload):
                self.sent.append(payload)

            def recv(self):
                return self.frames.pop(0)

            def close(self):
                pass

        socket_holder = {}

        def socket_factory(*args, **kwargs):
            socket_holder["socket"] = FakeWebSocket(*args, **kwargs)
            return socket_holder["socket"]

        session = VolcengineRealtimeAudioSession(
            VolcengineRealtimeConfig(app_id="app", access_key="token"),
            session_id="session-1",
            socket_factory=socket_factory,
        )

        session.start(build_voice_system_prompt("owner"))
        session.send_audio(b"\x01\x02" * 160)
        session.recv_event()
        session.recv_event()
        session.recv_event()
        response = session.recv_event()

        audio_frames = [frame for frame in socket_holder["socket"].sent if frame[:4] == bytes([0x11, 0x24, 0x01, 0x00])]
        self.assertEqual(len(audio_frames), 1)
        frame = audio_frames[0]
        self.assertEqual(int.from_bytes(frame[4:8], "big"), 200)
        session_size = int.from_bytes(frame[8:12], "big")
        payload_start = 12 + session_size
        payload_size = int.from_bytes(frame[payload_start : payload_start + 4], "big")
        self.assertEqual(gzip.decompress(frame[payload_start + 4 : payload_start + 4 + payload_size]), b"\x01\x02" * 160)
        self.assertEqual(response["type"], "response")
        self.assertEqual(response["chatText"], "收到")
        self.assertEqual(response["audioPcm16"], b"\x05\x06")

    def test_audio_session_collects_confirmed_user_text_for_response(self):
        frames = [
            encode_server_event_for_test(EVENT_CONNECTION_STARTED, {}, "session-1"),
            encode_server_event_for_test(EVENT_SESSION_STARTED, {}, "session-1"),
            encode_server_event_for_test(EVENT_CHAT_TEXT_QUERY_CONFIRMED, {"content": "你认不认识我"}, "session-1"),
            encode_server_event_for_test(EVENT_CHAT_RESPONSE, {"content": "当然认识"}, "session-1"),
            encode_server_audio_event_for_test(EVENT_TTS_RESPONSE, b"\x05\x06", "session-1"),
            encode_server_event_for_test(EVENT_TTS_ENDED, {}, "session-1"),
        ]

        class FakeWebSocket:
            def __init__(self, *args, **kwargs):
                self.sent = []
                self.frames = list(frames)
                self.response_headers = {"x-tt-logid": "log-1"}

            def connect(self):
                return self

            def send_binary(self, payload):
                self.sent.append(payload)

            def recv(self):
                return self.frames.pop(0)

            def close(self):
                pass

        session = VolcengineRealtimeAudioSession(
            VolcengineRealtimeConfig(app_id="app", access_key="token"),
            session_id="session-1",
            socket_factory=lambda *args, **kwargs: FakeWebSocket(*args, **kwargs),
        )

        session.start(build_voice_system_prompt("owner"))
        session.recv_event()
        session.recv_event()
        session.recv_event()
        response = session.recv_event()

        self.assertEqual(response["type"], "response")
        self.assertEqual(response["userText"], "你认不认识我")
        self.assertEqual(response["chatText"], "当然认识")

    def test_voice_system_prompt_allows_spoken_replies(self):
        prompt = build_voice_system_prompt(known_user="owner")

        self.assertIn("可以用中文自然语音回答", prompt)
        self.assertIn('"known_user":"owner"', prompt)
        self.assertIn("当然认识", prompt)
        self.assertIn("还不认识", prompt)
        self.assertNotIn("机器人不能说话", prompt)
        self.assertNotIn("只输出严格 JSON", prompt)

    def test_start_session_payload_uses_natural_voice_speaking_style(self):
        payload = build_start_session_payload("你是桌面机器人的实时语音大脑。")

        self.assertEqual(payload["dialog"]["speaking_style"], "自然、简短、口语化，用中文回答。")
        self.assertNotIn("JSON", payload["dialog"]["speaking_style"])

    def test_config_builds_official_headers(self):
        config = VolcengineRealtimeConfig(app_id="app", access_key="token")

        headers = config.headers(connect_id="cid")

        self.assertEqual(headers["X-Api-App-ID"], "app")
        self.assertEqual(headers["X-Api-Access-Key"], "token")
        self.assertEqual(headers["X-Api-Resource-Id"], "volc.speech.dialog")
        self.assertEqual(headers["X-Api-App-Key"], "PlgvMymc7f3tQnJ6")
        self.assertEqual(headers["X-Api-Connect-Id"], "cid")


if __name__ == "__main__":
    unittest.main()
