import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
HOST = ROOT / "host"


class HostConsoleUiTest(unittest.TestCase):
    def test_camera_panel_controls_and_actions_are_present(self):
        html = (HOST / "web_console.html").read_text(encoding="utf-8")
        for snippet in [
            "摄像头人脸",
            'id="cameraEnabled"',
            'id="cameraCaptureInterval"',
            'id="cameraDetectTimeout"',
            'id="cameraReturnSettle"',
            'id="cameraConfirmSettle"',
            'id="cameraLockHold"',
            'id="cameraCooldown"',
            'id="cameraCatchphraseAsset"',
            'id="cameraListenAfterCatchphrase"',
            'id="cameraSnapshotId"',
            'id="cameraDetecting"',
            'id="cameraFaceBox"',
            "refreshCamera",
            "saveCamera",
            "saveCameraCatchphrase",
            "reportCameraFace",
            "reportCameraNoFace",
            "wakeCamera",
            "clearCamera",
            "Host 视觉",
            'id="hostVisionIntervalMs"',
            'id="hostVisionMinConfidence"',
            'id="hostVisionDebugSubtitle"',
            'id="hostVisionState"',
            'id="hostVisionFace"',
            "stepHostVision",
            "startHostVision",
            "stopHostVision",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, html)
        self.assertIn(
            '<select id="hostVisionDebugSubtitle">\n'
            '            <option value="0">关</option>\n'
            '            <option value="1">开</option>',
            html,
        )

    def test_camera_panel_calls_host_camera_proxy_endpoints(self):
        html = (HOST / "web_console.html").read_text(encoding="utf-8")
        for endpoint in [
            "/api/robot/camera/status",
            "/api/robot/camera/set",
            "/api/robot/camera/catchphrase",
            "/api/robot/camera/face-report",
            "/api/robot/camera/wake",
            "/api/robot/camera/clear",
            "/api/host/vision/status",
            "/api/host/vision/step",
            "/api/host/vision/start",
            "/api/host/vision/stop",
        ]:
            with self.subTest(endpoint=endpoint):
                self.assertIn(endpoint, html)

    def test_realtime_action_router_panel_is_removed(self):
        html = (HOST / "web_console.html").read_text(encoding="utf-8")
        for snippet in [
            "动作路由",
            'id="realtimeAction"',
            'id="realtimeIntensity"',
            'id="realtimeDuration"',
            'id="realtimeLog"',
            "runRealtimeAction",
            "/api/robot/realtime/action",
        ]:
            with self.subTest(snippet=snippet):
                self.assertNotIn(snippet, html)
        self.assertNotIn('value="tilt_confused"', html)

    def test_yes_no_asset_test_ports_are_removed(self):
        html = (HOST / "web_console.html").read_text(encoding="utf-8")
        for snippet in [
            "点头/摇头测试口",
            "固件点头资产",
            "固件摇头资产",
            "路由点头",
            "路由摇头",
            "runNodAssetTest",
            "runShakeAssetTest",
            "runRouteNodTest",
            "runRouteShakeTest",
            "/api/robot/test/nod-yes",
            "/api/robot/test/shake-no",
            'id="yesNoTestLog"',
        ]:
            with self.subTest(snippet=snippet):
                self.assertNotIn(snippet, html)

    def test_text_model_action_panel_is_removed(self):
        html = (HOST / "web_console.html").read_text(encoding="utf-8")
        for snippet in [
            "备用普通文本模型动作测试（非实时）",
            "本地是/否测试",
            'id="simpleYesNoQuestion"',
            'id="modelProvider"',
            'id="modelBaseUrl"',
            'id="modelApiKey"',
            'id="modelName"',
            'id="modelUserText"',
            'id="modelSystemPrompt"',
            'id="modelConfigState"',
            'id="modelLog"',
            "runModelAction",
            "runSimpleYesNoAction",
            "/api/robot/simple-yes-no/action",
            "/api/robot/model/action",
            '"asset_id":"behavior.robot_action.nod_yes|behavior.robot_action.shake_no|behavior.emotion.thinking|behavior.emotion.confused',
        ]:
            with self.subTest(snippet=snippet):
                self.assertNotIn(snippet, html)
        self.assertIn("/api/host/model-config/status", html)
        self.assertIn("/api/host/model-config/save", html)
        self.assertIn("保存 API 配置", html)
        self.assertIn("读取 API 配置", html)
        self.assertNotIn("doubao-seed-1-6-flash", html)
        self.assertNotIn('"action_id":"nod_yes|shake_no|look_thinking|tilt_confused', html)

    def test_volcengine_realtime_panel_keeps_api_settings_and_voice_test(self):
        html = (HOST / "web_console.html").read_text(encoding="utf-8")
        for snippet in [
            "豆包 Realtime API 配置",
            "Access Token",
            "X-Api-App-ID",
            "X-Api-Access-Key",
            "Secret Key 当前不参与这条连接",
            'id="realtimeAppId"',
            'id="realtimeAccessToken"',
            'id="realtimeEndpoint"',
            'id="realtimeModelVersion"',
            'id="realtimeSpeaker"',
            'id="realtimeKnownUser"',
            'id="realtimeRecvTimeout"',
            'id="realtimeInsecureTls"',
            'id="realtimeFirmwareNotes"',
            'id="realtimeConfigState"',
            'id="volcRealtimeLog"',
            'id="realtimeVoiceText"',
            'id="realtimeVoiceAutoplay"',
            'id="realtimeVoiceState"',
            'id="realtimeVoiceReply"',
            'id="realtimeVoiceAudio"',
            'id="realtimeRobotAudio"',
            'id="realtimeVoiceAgentDispatch"',
            'id="hostRealtimeVoiceState"',
            'id="hostRealtimeVoiceFrames"',
            'id="hostRealtimeVoiceReply"',
            'id="hostRealtimeVoiceRobotAudio"',
            'id="hostRealtimeVoiceAgentDispatch"',
            'id="hostRealtimeVoiceLatency"',
            '<option value="0" selected>关</option>',
            "发送到机器人播放 API 语音",
            "开始网页实时语音",
            "停止网页实时语音",
            "runRealtimeVoiceText",
            "playRealtimePcm16",
            "startHostRealtimeVoice",
            "stopHostRealtimeVoice",
            "sendHostRealtimeVoiceFrame",
            "droppedSilenceFrameCount",
            "lastAudioGate",
            "lastSpeechToFirstApiEventMs",
            "lastAgentDispatch",
            "agentDispatch",
            "Agent 调度",
            'id="agentDispatchCommand"',
            'id="agentEnabled"',
            'id="agentBaseUrl"',
            'id="agentApiKey"',
            'id="agentModel"',
            'id="agentSystemPrompt"',
            'id="agentAllowedAssets"',
            'id="agentDispatchState"',
            'id="agentMonitorState"',
            'id="agentMonitorSource"',
            'id="agentMonitorMode"',
            'id="agentMonitorUserText"',
            'id="agentMonitorAction"',
            'id="agentMonitorError"',
            'id="agentMonitorHistory"',
            'id="agentDispatchLog"',
            "调度提示词",
            "资产白名单",
            "调度状态监控",
            "saveAgentConfig",
            "loadAgentConfig",
            "refreshAgentMonitor",
            "dispatchAgentAction('nod_yes')",
            "dispatchAgentAction('shake_no')",
            "/api/agent/dispatch-action",
            "/api/agent/dispatch-monitor",
            "mediaDevices.getUserMedia",
            "/api/host/realtime-voice/text",
            "/api/host/realtime-voice/session/status",
            "/api/host/realtime-voice/session/start",
            "/api/host/realtime-voice/session/frame",
            "/api/host/realtime-voice/session/stop",
            "实时语音系统提示词",
            "跳过 TLS 证书校验",
            "wss://openspeech.bytedance.com/api/v3/realtime/dialogue",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, html)
        for removed in [
            "Host 实时资产白名单",
            'id="realtimeAccessKey"',
            'id="realtimeAssetRows"',
            'id="realtimeAssetState"',
            "添加资产",
            "导入固件可运行资产",
            "复制资产=用途",
            "refreshRealtimeAssets",
            "saveRealtimeAssets",
            "addRealtimeAssetRow",
            "removeRealtimeAssetRow",
            "importPlayableRealtimeAssets",
            "formatRealtimeAssetHints",
            "copyRealtimeAssetHints",
            "/api/host/realtime-assets/status",
            "/api/host/realtime-assets/save",
            "/api/robot/catalog/assets",
            "动作 JSON",
            "asset_id 关键词",
            "机器人不能说话",
            'id="liveVoiceState"',
            'id="liveVoiceFrames"',
            'id="liveVoiceReply"',
            'id="liveVoiceRobotAudio"',
            "开始实时收音",
            "停止实时收音",
            "startLiveVoice",
            "stopLiveVoice",
            "sendLiveVoiceChunk",
            "navigator.mediaDevices.getUserMedia",
            "/api/host/realtime-voice/live/status",
            "/api/host/realtime-voice/live/start",
            "/api/host/realtime-voice/live/audio",
            "/api/host/realtime-voice/live/stop",
        ]:
            with self.subTest(removed=removed):
                self.assertNotIn(removed, html)
        self.assertNotIn("tilt_confused=疑惑", html)
        for removed in [
            "开始电脑收音",
            "停止并发送",
            'id="realtimeMicState"',
            'id="realtimeMicStart"',
            'id="realtimeMicStop"',
            "startRealtimeVoiceMic",
            "stopRealtimeVoiceMic",
            "/api/host/realtime-voice/audio",
        ]:
            with self.subTest(removed=removed):
                self.assertNotIn(removed, html)
        for removed in [
            "持续实时对话",
            "recordVolcRealtimeAudioAction",
            "startContinuousRealtime",
            "stopContinuousRealtime",
            "refreshContinuousRealtimeStatus",
            "continuousRealtime",
            "/api/host/realtime-stream/",
        ]:
            with self.subTest(removed=removed):
                self.assertNotIn(removed, html)

    def test_old_voice_wake_panel_is_removed_from_host_console(self):
        html = (HOST / "web_console.html").read_text(encoding="utf-8")
        for removed in [
            "语音唤醒测试",
            'id="voiceWakeWords"',
            'id="voiceWakeWindowMs"',
            'id="voiceWakeState"',
            'id="voiceWakeTranscript"',
            'id="voiceWakeCommand"',
            'id="voiceWakeLog"',
            'id="voiceWakeSimulatedText"',
            'id="debugSubtitleListening"',
            'id="debugSubtitleApiResult"',
            'id="debugSubtitleText"',
            'id="listeningVoiceWakeEnabled"',
            'id="listeningHostCallbackUrl"',
            'id="listeningTimeoutMs"',
            'id="listeningState"',
            'id="listeningWakeEvent"',
            "startVoiceWakeListening",
            "stopVoiceWakeListening",
            "simulateVoiceWake",
            "executeVoiceWakeCommand",
            "handleVoiceWakeTranscript",
            "notifyRobotListening",
            "refreshDebugSubtitle",
            "saveDebugSubtitle",
            "refreshListening",
            "saveListeningConfig",
            "startFirmwareListening",
            "stopFirmwareListening",
            "wakeTestFirmwareListening",
            "refreshWakeEvent",
            "/api/robot/debug/subtitle/status",
            "/api/robot/debug/subtitle/set",
            "/api/robot/debug/subtitle/show",
            "/api/robot/listening/status",
            "/api/robot/listening/config",
            "/api/robot/listening/start",
            "/api/robot/listening/stop",
            "/api/robot/listening/wake-test",
            "/api/host/wake-event/status",
            "SpeechRecognition",
            "webkitSpeechRecognition",
            "等待实时语音接入",
            "realtime_voice_pending",
        ]:
            with self.subTest(removed=removed):
                self.assertNotIn(removed, html)
        self.assertNotIn("runVolcRealtimeTextAction", html)
        self.assertNotIn("/api/robot/model/action", html)

    def test_voice_settings_console_only_keeps_realtime_api_settings(self):
        html = (HOST / "voice_console.html").read_text(encoding="utf-8")
        for snippet in [
            "语音 API 配置台",
            'id="voiceRealtimeAppId"',
            'id="voiceRealtimeAccessToken"',
            'id="voiceRealtimeEndpoint"',
            'id="voiceRealtimeModelVersion"',
            'id="voiceRealtimeSpeaker"',
            'id="voiceRealtimeRecvTimeout"',
            'id="voiceRealtimeInsecureTls"',
            'id="voiceRealtimeFirmwareNotes"',
            "/api/host/model-config/status",
            "/api/host/model-config/save",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, html)
        for removed in [
            "旧：浏览器麦克风",
            "固件聆听窗口",
            "唤醒事件",
            "listeningVoiceWakeEnabled",
            "listeningHostCallbackUrl",
            "listeningTimeoutMs",
            "refreshListening",
            "saveListening",
            "startFirmwareListening",
            "stopFirmwareListening",
            "wakeTestFirmwareListening",
            "refreshWakeEvent",
            "/api/robot/listening/status",
            "/api/robot/listening/config",
            "/api/robot/listening/start",
            "/api/robot/listening/stop",
            "/api/robot/listening/wake-test",
            "/api/robot/wake-event",
            "startVoiceSession",
            "startBrowserMicPlaybackTest",
            "/api/voice-session/start",
            "/api/voice-session/audio",
            "/api/voice-session/volume",
            "/api/voice-stream/",
            "voiceGateway",
            "VoiceGateway",
            "startRobotMicGateway",
            "/api/voice-gateway/",
            "/api/robot/voice-gateway/",
            "voiceStreamVolume",
            "Host 实时资产白名单",
            "realtimeAssetRows",
            "recordVolcRealtimeAudioAction",
            "/api/host/realtime-stream/",
            "/api/host/realtime-assets/save",
        ]:
            with self.subTest(removed=removed):
                self.assertNotIn(removed, html)

    def test_host_console_uses_compact_density(self):
        html = (HOST / "web_console.html").read_text(encoding="utf-8")
        for snippet in [
            "font-size: 13px;",
            "line-height: 1.35;",
            "padding: 10px 14px;",
            "h1 { font-size: 16px; }",
            "h2 { font-size: 14px; }",
            "main { display: grid; gap: 10px; padding: 10px; max-width: 1180px; margin: 0 auto; }",
            "padding: 10px;",
            "grid-template-columns: repeat(auto-fit, minmax(160px, 1fr));",
            "min-height: 32px;",
            "padding: 5px 8px;",
            ".button-row { display: grid; grid-template-columns: repeat(auto-fit, minmax(112px, 1fr)); gap: 6px; margin-top: 10px; }",
            "min-height: 46px;",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, html)

    def test_host_console_camera_log_is_collapsed_by_default(self):
        html = (HOST / "web_console.html").read_text(encoding="utf-8")
        for snippet in [
            ".log-details { margin-top: 8px; font-size: 12px; color: var(--muted); }",
            ".log-details summary { cursor: pointer; display: inline-flex; align-items: center; gap: 6px; border: 1px solid var(--line); background: var(--field); border-radius: 999px; padding: 4px 8px; color: var(--text); }",
            '<details class="log-details"><summary>调试输出</summary><pre id="cameraLog"></pre></details>',
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, html)


if __name__ == "__main__":
    unittest.main()
