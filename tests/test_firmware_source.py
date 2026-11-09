import re
import unittest
from collections import Counter
from pathlib import Path

from host.robot_action_router import allowed_action_ids


ROOT = Path(__file__).resolve().parents[1]
FIRMWARE = ROOT / "firmware"


class FirmwareSourceTest(unittest.TestCase):
    def read_all(self):
        if not FIRMWARE.exists():
            return ""
        return "\n".join(
            path.read_text(encoding="utf-8")
            for path in sorted(FIRMWARE.glob("*.*"))
            if path.suffix in {".ino", ".h", ".cpp"} and path.name not in {"config.local.h", "bb_mumble_audio.cpp"}
        )

    def test_required_api_routes_are_registered(self):
        source = self.read_all()
        for route in [
            '"/api/status"',
            '"/api/face/list"',
            '"/api/face/set"',
            '"/api/face/style"',
            '"/api/face/shape"',
            '"/api/face/preview/status"',
            '"/api/face/preview"',
            '"/api/face/preview/stop"',
            '"/api/face/react"',
            '"/api/screen/status"',
            '"/api/screen/set"',
            '"/api/screen/trigger"',
            '"/api/subtitle/set"',
            '"/api/subtitle/clear"',
            '"/api/clock/status"',
            '"/api/clock/set"',
            '"/api/motion/pose"',
            '"/api/motion/rest-pitch"',
            '"/api/motion/stream/status"',
            '"/api/motion/rotate"',
            '"/api/motion/home"',
            '"/api/rgb"',
            '"/api/rgb/restore"',
            '"/api/touch"',
            '"/api/proximity/status"',
            '"/api/audio/status"',
            '"/api/audio/tone"',
            '"/api/audio/stop"',
            '"/api/mumble/status"',
            '"/api/mumble/set"',
            '"/api/mumble/play"',
            '"/api/mumble/stop"',
            '"/api/mumble/upload"',
            '"/api/mic/sample"',
            '"/api/idle/status"',
            '"/api/idle/set"',
            '"/api/idle/next"',
            '"/api/emotion/status"',
            '"/api/emotion/play"',
            '"/api/emotion/stop"',
            '"/api/emotion/sound-slots"',
            '"/api/emotion/sound-slot/set"',
            '"/api/emotion/sound-slot/play"',
            '"/api/emotion/sound-slot/upload"',
            '"/api/triggers/status"',
            '"/api/triggers/set"',
            '"/api/imu"',
            '"/api/power"',
            '"/api/ping"',
            '"/api/listening/status"',
            '"/api/listening/config"',
            '"/api/listening/start"',
            '"/api/listening/stop"',
            '"/api/listening/wake-test"',
        ]:
            with self.subTest(route=route):
                self.assertIn(route, source)

    def test_assistant_status_display_module_exists_and_formats_today_status(self):
        source = self.read_all()
        config = (FIRMWARE / "config.example.h").read_text(encoding="utf-8")

        for snippet in [
            '#include "assistant_status.h"',
            "class AssistantStatusDisplay",
            "AssistantStatusSnapshot",
            "HTTPClient http",
            "deserializeJson(doc",
            "CODING ",
            "CODEX --%",
            "CODEX ",
            " LOW",
            "DONE ",
            " TASKS",
            "STATUS OFFLINE",
            "setMood(\"neutral\")",
            "setMood(\"tired\")",
            "setMood(\"confused\")",
            "setAssistantStatusLine(line",
        ]:
            with self.subTest(snippet=snippet):
                self.assertTrue(snippet in source, snippet)

        for snippet in [
            "ROBOT_ASSISTANT_STATUS_ENABLED",
            "ROBOT_ASSISTANT_STATUS_URL",
            "ROBOT_ASSISTANT_STATUS_POLL_MS",
            "ROBOT_ASSISTANT_STATUS_LOW_CODEX_PERCENT",
        ]:
            with self.subTest(config=snippet):
                self.assertIn(snippet, config)

    def test_assistant_status_firmware_default_is_enabled(self):
        config = (FIRMWARE / "config.example.h").read_text(encoding="utf-8")
        self.assertIn("#define ROBOT_ASSISTANT_STATUS_ENABLED 1", config)

    def test_assistant_status_display_is_configurable_and_updated_in_loop(self):
        source = self.read_all()

        for snippet in [
            "AssistantStatusDisplay assistantStatus",
            "assistantStatus.begin(",
            "ROBOT_ASSISTANT_STATUS_ENABLED",
            "ROBOT_ASSISTANT_STATUS_URL",
            "ROBOT_ASSISTANT_STATUS_POLL_MS",
            "ROBOT_ASSISTANT_STATUS_LOW_CODEX_PERCENT",
            "assistantStatus.update(faceRenderer, hardware, nowMs, assistantStatusCanControlMood)",
            "registerApiRoutes(server, faceRenderer, hardware, idleBehavior, emotionActions, emotionTriggers, mumble, robotClock, proximityTracker, cameraAttention, localFaceDetector, motionStream, assistantStatus, wifiManager)",
            'server.on("/api/assistant-display/status", HTTP_GET',
            'server.on("/api/assistant-display/set", HTTP_POST',
            "assistantStatus.statusJson(millis())",
            "assistantStatus.configure(",
            "assistantStatusUrl",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)

    def test_assistant_status_event_prompts_override_normal_rotation(self):
        source = self.read_all()

        for snippet in [
            "bool eventActive = false;",
            "String eventMessage;",
            "snapshot_.eventActive = doc[\"event\"][\"active\"] | false;",
            "snapshot_.eventMessage = String(doc[\"event\"][\"message\"] | \"\");",
            "if (snapshot_.eventActive && snapshot_.eventMessage.length()) return snapshot_.eventMessage;",
            "eventActive",
            "eventMessage",
            ',\\"eventActive\\":',
            ',\\"eventMessage\\":',
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)

    def test_assistant_status_alert_moods_and_management_panel_are_configurable(self):
        source = self.read_all()
        assistant_cpp = (FIRMWARE / "assistant_status.cpp").read_text(encoding="utf-8")
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")

        for snippet in [
            "lowBatteryPercent_ = 20",
            "lowCodexPercent() const",
            "lowBatteryPercent() const",
            "setAlertThresholds",
            "batteryLow(hardware)",
            "eventWantsDead",
            "eventWantsNeutral",
            "face.setMood(\"dead\")",
            "face.setMood(\"neutral\")",
            "snapshot_.eventType = String(doc[\"event\"][\"type\"] | \"\");",
            "snapshot_.eventTtlSeconds = doc[\"event\"][\"ttlSeconds\"] | snapshot_.eventTtlSeconds;",
            "snapshot_.eventRemainingSeconds = doc[\"event\"][\"remainingSeconds\"] | 0;",
            "doc[\"settings\"][\"eventTtlSeconds\"]",
            "doc[\"settings\"][\"lowCodexPercent\"]",
            "lowBatteryPercent",
            "lowCodexPercent",
            "batteryLow",
            "eventType",
            "eventTtlSeconds",
            "eventRemainingSeconds",
            'id="assistantEventTtlSeconds"',
            'id="assistantLowCodexPercent"',
            'id="assistantLowBatteryPercent"',
            'id="assistantEventMessage"',
            'id="assistantEventRemaining"',
            "setAssistantSourceSettings",
            "clearAssistantSourceEvent",
            "sendAssistantSourceEvent",
            "/api/assistant/settings",
            "/api/assistant/events/clear",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source + console)

        for forbidden in [
            'snapshot_.eventMessage.indexOf("DONE") >= 0 || snapshot_.eventMessage.indexOf(" OK") >= 0)) {\n      face.setMood("happy");',
            'snapshot_.codexLow) {\n      face.setMood("tired");',
            'snapshot_.eventMessage.indexOf("FAIL") >= 0) {\n      face.setMood("sad");',
        ]:
            with self.subTest(forbidden=forbidden):
                self.assertNotIn(forbidden, assistant_cpp)

    def test_assistant_status_mood_updates_are_gated_by_face_priority(self):
        source = self.read_all()
        assistant_cpp = (FIRMWARE / "assistant_status.cpp").read_text(encoding="utf-8")
        firmware = (FIRMWARE / "firmware.ino").read_text(encoding="utf-8")

        for snippet in [
            "bool canControlMood",
            "if (canControlMood)",
            "assistantStatus.update(faceRenderer, hardware, nowMs, assistantStatusCanControlMood)",
            "!emotionActions.active()",
            "!proximityTracker.active()",
            "!cameraAttention.active()",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)

        mood_block = re.search(r"if \(canControlMood\) \{[\s\S]*?face\.setAssistantStatusLine", assistant_cpp)
        self.assertIsNotNone(mood_block)
        self.assertIn('face.setMood("neutral")', mood_block.group(0))
        self.assertIn('face.setMood("thinking")', mood_block.group(0))
        self.assertRegex(
            firmware,
            r"bool assistantStatusCanControlMood = !emotionActions\.active\(\) && !proximityTracker\.active\(\) && !cameraAttention\.active\(\);",
        )

    def test_assistant_status_uses_plain_bottom_info_layer_with_battery_controls(self):
        source = self.read_all()
        face_cpp = (FIRMWARE / "face_renderer.cpp").read_text(encoding="utf-8")
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")

        for snippet in [
            "struct AssistantStatusLineState",
            "setAssistantStatusLine",
            "AssistantStatusLineState assistantStatusLine() const",
            "drawAssistantStatusLine",
            "drawPlainBottomText",
            "drawPlainBottomText(canvas, text, subtitle_.color, slot);",
            "drawPlainBottomText(canvas, assistantStatusLine_.text, assistantStatusLine_.color, 0);",
            "canvas.setTextColor(color);",
            "drawSubtitle(frame_);",
            "drawAssistantStatusLine(frame_);",
            "drawScanlines(frame_, style_);",
            "face.setAssistantStatusLine(line, displayEnabled_, lineColor_);",
            "batteryLine(hardware)",
            "batteryPercent() const",
            "BAT ",
            "displayEnabled",
            "batteryEnabled",
            "lineColor",
            'id="panel-assistant-status"',
            'id="assistantDisplayEnabled"',
            'id="assistantBatteryEnabled"',
            'id="assistantStatusColor"',
            'data-target="assistantStatusColor"',
            'data-apply="assistantStatus"',
            "syncAssistantStatusControls",
            "setAssistantStatus",
            "refreshAssistantStatus",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source + console)

        for forbidden in [
            "canvas.fillRect(0, barY, frameWidth_, SUBTITLE_BAR_HEIGHT, TFT_BLACK)",
            "canvas.drawFastHLine(0, barY, frameWidth_, dim565(subtitle_.color, 36))",
            "face.setSubtitle(line, true, subtitle.color)",
        ]:
            with self.subTest(forbidden=forbidden):
                self.assertNotIn(forbidden, face_cpp + source)

    def test_motion_stream_udp_receiver_is_wired_into_firmware_loop(self):
        source = self.read_all()
        for snippet in [
            '#include "motion_stream.h"',
            "#include <WiFiUdp.h>",
            "MotionStreamReceiver motionStream",
            "motionStream.begin()",
            "motionStream.update(hardware, idleBehavior, robotClock, nowMs)",
            'server.on("/api/motion/stream/status", HTTP_GET',
            "motionStream.statusJson(millis())",
            "udp_.parsePacket()",
            "udp_.read(packet",
            "hardware.pose(latestYaw_, latestPitch_, latestSpeed_)",
            "idle.pauseForManualControl(nowMs, MOTION_STREAM_IDLE_HOLD_MS)",
            "clock.markInteraction(nowMs)",
            '\\"port\\":',
            '\\"packetsReceived\\":',
            '\\"lastPacketAgeMs\\":',
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)

    def test_asset_logic_catalog_routes_and_metadata_are_exposed(self):
        source = self.read_all()
        for snippet in [
            '#include "asset_logic_catalog.h"',
            '"/api/catalog"',
            '"/api/catalog/assets"',
            '"/api/catalog/logic"',
            '"/api/catalog/conflicts"',
            '"/api/asset/invoke"',
            "catalogJson(",
            "catalogAssetsJson(",
            "catalogLogicJson(",
            "catalogConflictsJson(",
            "invokeApi",
            "soundAssets",
            "expressionAssets",
            "motionAssets",
            "lightAssets",
            "behaviorAssets",
            "deprecatedAssets",
            "logicAssets",
            "logic.trigger.head_touch_happy",
            "logic.trigger.random_emotion",
            "logic.idle.normal",
            "logic.camera.scan",
            "logic.clock.night_sleep",
            "behavior.emotion.happy",
            "behavior.emotion.happy.nod",
            "behavior.robot_action.nod_yes",
            "behavior.robot_action.shake_no",
            "sound.happy",
            "sound.comfortable_nod",
            "sound.curious",
            "expression.happy",
            "motion.robot_action.nod_yes",
            "motion.robot_action.shake_no",
            "motion.pose.sleeping_low",
            "light.rainbow_marquee",
            "mayConflictWith",
            "sourceFiles",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)

    def test_catalog_asset_ids_are_unique_and_deprecated_assets_do_not_shadow_active_assets(self):
        catalog = (FIRMWARE / "asset_logic_catalog.cpp").read_text(encoding="utf-8")
        static_ids = re.findall(r'\\"id\\":\\"([^\\"]+)\\"', catalog)
        logic_ids = re.findall(r'logicEntry\("([^"]+)"', catalog)
        all_ids = static_ids + logic_ids
        duplicates = sorted(asset_id for asset_id, count in Counter(all_ids).items() if count > 1)
        self.assertEqual([], duplicates)

        deprecated_match = re.search(
            r"const char\* const DEPRECATED_ASSETS\[\] = \{(?P<body>.*?)\};",
            catalog,
            re.S,
        )
        self.assertIsNotNone(deprecated_match)
        deprecated_ids = re.findall(r'\\"id\\":\\"([^\\"]+)\\"', deprecated_match.group("body"))
        deprecated_duplicates = sorted(asset_id for asset_id, count in Counter(deprecated_ids).items() if count > 1)
        self.assertEqual([], deprecated_duplicates)
        self.assertTrue(set(deprecated_ids).isdisjoint(set(all_ids) - set(deprecated_ids)))
        self.assertIn("DEPRECATED_ASSET_COUNT", catalog)

    def test_catalog_hides_non_direct_motion_implementation_assets(self):
        catalog = (FIRMWARE / "asset_logic_catalog.cpp").read_text(encoding="utf-8")
        for snippet in [
            "motion.emotion.comfortable_nod",
            "motion.emotion.happy.nod",
            "motion.proximity.peek",
            "motion.camera.scan",
            "motion.camera.face_lock",
            "舒服点头动作",
            "开心疯狂点头动作",
            "红外靠近扫视动作",
            "摄像头扫描动作",
            "摄像头人脸锁定动作",
        ]:
            with self.subTest(snippet=snippet):
                self.assertNotIn(snippet, catalog)

        for snippet in [
            "motion.pose.sleeping_low",
            "motion.pose.tired_low",
            "motion.robot_action.nod_yes",
            "motion.robot_action.shake_no",
            "behavior.emotion.comfortable_nod",
            "behavior.emotion.happy.nod",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, catalog)

    def test_host_model_actions_are_backed_by_firmware_catalog_assets(self):
        catalog = (FIRMWARE / "asset_logic_catalog.cpp").read_text(encoding="utf-8")
        static_ids = set(re.findall(r'\\"id\\":\\"([^\\"]+)\\"', catalog))
        logic_ids = set(re.findall(r'logicEntry\("([^"]+)"', catalog))
        catalog_ids = static_ids | logic_ids

        missing = sorted(set(allowed_action_ids()) - catalog_ids)

        self.assertEqual([], missing)

    def test_catalog_is_read_only_and_preserves_existing_runtime_controllers(self):
        source = self.read_all()
        catalog_h = (FIRMWARE / "asset_logic_catalog.h").read_text(encoding="utf-8")
        catalog_cpp = (FIRMWARE / "asset_logic_catalog.cpp").read_text(encoding="utf-8")
        catalog_source = catalog_h + catalog_cpp
        for controller in [
            "EmotionActionController",
            "EmotionTriggerController",
            "IdleBehaviorController",
            "CameraAttentionController",
            "MumblePlayer",
            "RobotClock",
        ]:
            with self.subTest(controller=controller):
                self.assertIn(controller, catalog_source)
        for forbidden in [
            ".play(",
            ".stop(",
            ".setEnabled(",
            ".pauseForManualControl(",
            ".triggerNext(",
            ".startHeadTouchSearch(",
            ".handleVoiceWake(",
        ]:
            with self.subTest(forbidden=forbidden):
                self.assertNotIn(forbidden, catalog_source)
        self.assertIn('server.on("/api/emotion/play"', source)
        self.assertIn('server.on("/api/triggers/set"', source)
        self.assertIn('server.on("/api/idle/status"', source)

    def test_asset_invoke_route_dispatches_playable_assets_to_runtime_controllers(self):
        source = self.read_all()
        for snippet in [
            'server.on("/api/asset/invoke", HTTP_POST',
            "invokeAssetJson(",
            "assetIdFromJson(",
            "assetInvokeErrorJson(",
            "assetInvokeResultJson(",
            "soundSlotFromAssetId(",
            "emotionActionFromBehaviorAssetId(",
            "applyRainbowMarqueeLightAsset(",
            "RobotFaceRenderer::parseMood",
            "mumble.playEmotionSlotJson",
            "hardware.pose(0, 0",
            "hardware.pose(0, 12",
            "hardware.setRgbOne",
            "EmotionActionController::isKnownAction",
            "emotion.play(actionName",
            "logic assets are not directly invokable",
            "clock.markInteraction(nowMs)",
            "idle.pauseForManualControl(nowMs",
            "sendJsonStatus(server, status",
            "\"invokeApi\":\"/api/asset/invoke\"",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)

    def test_catalog_separates_base_asset_types_and_lists_all_face_moods(self):
        catalog = (FIRMWARE / "asset_logic_catalog.cpp").read_text(encoding="utf-8")
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")
        for snippet in [
            "SOUND_ASSETS",
            "EXPRESSION_ASSETS",
            "SCREEN_ASSETS",
            "MOTION_ASSETS",
            "LIGHT_ASSETS",
            "soundAssets",
            "expressionAssets",
            "screenAssets",
            "motionAssets",
            "lightAssets",
            "catalogSoundAssets",
            "catalogExpressionAssets",
            "catalogScreenAssets",
            "catalogMotionAssets",
            "catalogLightAssets",
            "声音资产",
            "表情资产",
            "画面资产",
            "动作资产",
            "灯光资产",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, catalog + console)
        for mood in [
            "neutral",
            "happy",
            "angry",
            "sad",
            "sleeping",
            "tired",
            "surprised",
            "love",
            "dizzy",
            "wink",
            "dead",
            "thinking",
            "curious",
            "sunglasses",
            "loading",
        ]:
            with self.subTest(mood=mood):
                self.assertIn(f"expression.{mood}", catalog)
        for snippet in [
            "screen.matrix_rain",
            "matrixRain",
            "Matrix 数字雨",
            "[\\\"screen\\\",\\\"fullscreen\\\",\\\"animation\\\"]",
            "/api/screen/set",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, catalog)
        self.assertNotIn("媒体资产", console)

    def test_robot_action_assets_include_yes_no_body_language(self):
        source = self.read_all()
        for snippet in [
            "ROBOT_ACTION_NOD_YES",
            "ROBOT_ACTION_SHAKE_NO",
            '{"nod_yes", "是 / 点头确认"',
            '{"shake_no", "否 / 摇头拒绝"',
            "点头确认-低头",
            "摇头拒绝-看左",
            "behavior.robot_action.nod_yes",
            "behavior.robot_action.shake_no",
            "motion.robot_action.nod_yes",
            "motion.robot_action.shake_no",
            "[\\\"/api/emotion/play\\\"]",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)

    def test_robot_action_yes_no_assets_use_reference_test_motion_feel(self):
        emotion_cpp = (FIRMWARE / "emotion_actions.cpp").read_text(encoding="utf-8")
        for snippet in [
            "ROBOT_ACTION_REFERENCE_CENTER_PITCH = DEFAULT_REST_PITCH - 4",
            "ROBOT_ACTION_NOD_UP_PITCH = DEFAULT_REST_PITCH + 220",
            "ROBOT_ACTION_NOD_DOWN_PITCH = PITCH_MIN + 58",
            "ROBOT_ACTION_SHAKE_LEFT_YAW = -320",
            "ROBOT_ACTION_SHAKE_RIGHT_YAW = 320",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, emotion_cpp)

        nod_block = re.search(
            r"const EmotionStep ROBOT_ACTION_NOD_YES\[\] = \{(?P<body>[\s\S]*?)\};",
            emotion_cpp,
        )
        self.assertIsNotNone(nod_block)
        nod_body = nod_block.group("body")
        self.assertIn("ROBOT_ACTION_NOD_DOWN_PITCH", nod_body)
        self.assertIn("ROBOT_ACTION_NOD_UP_PITCH", nod_body)
        self.assertIn("ROBOT_ACTION_REFERENCE_CENTER_PITCH", nod_body)
        self.assertNotIn("rotateVelocity", nod_body)
        self.assertNotIn(" 78,", nod_body)

        shake_block = re.search(
            r"const EmotionStep ROBOT_ACTION_SHAKE_NO\[\] = \{(?P<body>[\s\S]*?)\};",
            emotion_cpp,
        )
        self.assertIsNotNone(shake_block)
        shake_body = shake_block.group("body")
        self.assertIn("ROBOT_ACTION_SHAKE_LEFT_YAW", shake_body)
        self.assertIn("ROBOT_ACTION_SHAKE_RIGHT_YAW", shake_body)
        self.assertIn("ROBOT_ACTION_REFERENCE_CENTER_PITCH", shake_body)
        self.assertNotIn("rotateVelocity", shake_body)

    def test_web_console_exposes_asset_and_rule_catalog_panel(self):
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")
        for snippet in [
            'href="#panel-catalog"',
            'id="panel-catalog"',
            "资产与规则",
            "catalogSoundAssets",
            "catalogExpressionAssets",
            "catalogScreenAssets",
            "catalogMotionAssets",
            "catalogLightAssets",
            "catalogBehaviorAssets",
            "catalogDeprecatedAssets",
            "catalogLogicAssets",
            "catalogConflicts",
            "refreshCatalog",
            "renderCatalog",
            "api('/api/catalog'",
            "api('/api/catalog/conflicts'",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, console)

    def test_web_console_asset_catalog_panel_is_collapsed_by_default(self):
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")
        for snippet in [
            '<section id="panel-catalog" class="catalog-section">',
            '<details class="catalog-collapsible">',
            '<summary class="catalog-summary">',
            '<div class="catalog-panel-body">',
            "catalog-collapsible>summary",
            "catalog-collapsible[open]>summary",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, console)

        self.assertNotRegex(console, r'<details class="catalog-collapsible"[^>]*\bopen\b')

    def test_matrix_screen_asset_renderer_contract(self):
        header = (FIRMWARE / "face_renderer.h").read_text(encoding="utf-8")
        source = (FIRMWARE / "face_renderer.cpp").read_text(encoding="utf-8")
        combined = header + source
        for snippet in [
            "enum class ScreenAssetMode",
            "MatrixRain",
            "setScreenAsset",
            "clearScreenAsset",
            "screenAssetJson",
            "screenAssetActive",
            "screenAssetUntilMs_",
            "MATRIX_RAIN_GLYPHS",
            "drawMatrixRain",
            "drawMatrixRain(frame_, nowMs);",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, combined)
        update_match = re.search(r"void RobotFaceRenderer::update[\s\S]*?\n\}", source)
        self.assertIsNotNone(update_match)
        update_block = update_match.group(0)
        self.assertLess(update_block.find("screenAssetActive(nowMs)"), update_block.find("drawEyes(frame_, nowMs);"))
        self.assertLess(update_block.find("drawMatrixRain(frame_, nowMs);"), update_block.find("drawEyes(frame_, nowMs);"))

    def test_screen_asset_api_routes_are_registered(self):
        source = (FIRMWARE / "api.cpp").read_text(encoding="utf-8")
        for snippet in [
            'server.on("/api/screen/status"',
            'server.on("/api/screen/set"',
            'server.on("/api/screen/trigger"',
            "face.screenAssetJson(millis())",
            "face.setScreenAsset",
            "face.clearScreenAsset",
            "\"matrixRain\"",
            "\"durationMs\"",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)

    def test_web_console_exposes_screen_asset_controls(self):
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")
        for snippet in [
            'href="#panel-screen"',
            'id="panel-screen"',
            "画面资产",
            'id="screenAsset"',
            'id="screenDuration"',
            'id="screenAssetMode"',
            "triggerMatrix",
            "clearScreenAsset",
            "refreshScreenAsset",
            "api('/api/screen/status'",
            "api('/api/screen/set'",
            "api('/api/screen/trigger'",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, console)

    def test_wifi_secret_is_local_only(self):
        self.assertIn(
            "firmware/config.local.h",
            (ROOT / ".gitignore").read_text(encoding="utf-8"),
        )
        example = FIRMWARE / "config.example.h"
        self.assertTrue(example.exists(), "public config example should exist")
        self.assertIn("YourPassword", example.read_text(encoding="utf-8"))

    def test_motion_limits_are_documented_in_firmware(self):
        source = self.read_all()
        self.assertRegex(source, r"YAW_MIN\s*=\s*-1280")
        self.assertRegex(source, r"YAW_MAX\s*=\s*1280")
        self.assertRegex(source, r"PITCH_MIN\s*=\s*0")
        self.assertRegex(source, r"PITCH_MAX\s*=\s*900")
        self.assertRegex(source, r"ROTATE_MIN\s*=\s*-1000")
        self.assertRegex(source, r"ROTATE_MAX\s*=\s*1000")

    def test_face_renderer_is_eye_only_and_robotface_style(self):
        source = self.read_all()
        for mood in [
            "neutral",
            "happy",
            "angry",
            "sad",
            "sleeping",
            "surprised",
            "love",
            "dizzy",
            "wink",
            "dead",
            "thinking",
            "sunglasses",
            "loading",
        ]:
            with self.subTest(mood=mood):
                self.assertIn(f'"{mood}"', source)
        for field in [
            "eyeWidthScale",
            "eyeHeightScale",
            "cornerRadius",
            "eyeOffsetX",
            "eyeOffsetY",
            "eyeSpacing",
            "eyeColor",
            "glowEnabled",
            "glowStrength",
            "scanlinesEnabled",
            "scanlineSpacing",
            "scanlineAlpha",
            "audioLevel",
            "lookX",
            "lookY",
        ]:
            with self.subTest(field=field):
                self.assertIn(field, source)
        self.assertNotIn("drawMouth", source)
        self.assertNotIn("drawBrow", source)

    def test_face_renderer_uses_offscreen_sprite_to_avoid_flicker(self):
        source = self.read_all()
        face_cpp = (FIRMWARE / "face_renderer.cpp").read_text(encoding="utf-8")
        self.assertIn("LGFX_Sprite", source)
        self.assertIn("pushSprite", source)
        self.assertIn("setColorDepth", source)
        self.assertNotIn("display.fillScreen(TFT_BLACK);", face_cpp)

    def test_face_renderer_draws_dos_style_bottom_subtitles(self):
        face_h = (FIRMWARE / "face_renderer.h").read_text(encoding="utf-8")
        face_cpp = (FIRMWARE / "face_renderer.cpp").read_text(encoding="utf-8")
        source = face_h + face_cpp
        for snippet in [
            "struct SubtitleState",
            "setSubtitle",
            "clearSubtitle",
            "SubtitleState subtitle() const",
            "drawSubtitle",
            "SUBTITLE_BAR_HEIGHT",
            "frameHeight_ - SUBTITLE_BAR_HEIGHT",
            "canvas.setFont(&fonts::Font0)",
            "canvas.setTextSize(2)",
            "drawEyes(frame_, nowMs);",
            "drawSubtitle(frame_);",
            '\\"subtitle\\":{',
            "jsonEscape(subtitle_.text)",
            'bool enabled = true',
            'String text = ""',
            "uint16_t color = 0x07EC",
            "bool showDateTime = false",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)

    def test_subtitle_api_and_console_controls_are_exposed(self):
        api = (FIRMWARE / "api.cpp").read_text(encoding="utf-8")
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")
        for snippet in [
            'server.on("/api/subtitle/set"',
            'server.on("/api/subtitle/clear"',
            'doc["text"].is<const char*>()',
            'doc["subtitle"].is<const char*>()',
            "face.setSubtitle",
            "face.clearSubtitle",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, api)
        for snippet in [
            'href="#panel-subtitle"',
            'id="panel-subtitle"',
            'id="subtitleText"',
            'value="" placeholder="balabala..."',
            'id="subtitleEnabled"',
            'id="subtitleColor"',
            'id="subtitleColor" type="color" value="#00ff62"',
            "setValue('subtitleColor',sub.color||'#00ff62')",
            "syncSubtitleControls",
            "setSubtitle",
            "clearSubtitle",
            "api('/api/subtitle/set'",
            "api('/api/subtitle/clear'",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, console)

    def test_subtitle_color_has_palette_swatches(self):
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")
        for snippet in [
            'data-target="subtitleColor"',
            'data-apply="subtitle"',
            'data-color="#ffffff"',
            'data-color="#00ffff"',
            'data-color="#00ff66"',
            'data-color="#ffcc33"',
            "btn.dataset.apply==='subtitle'",
            "setSubtitle()",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, console)

    def test_angry_and_sad_eyes_match_reference_shapes(self):
        face_cpp = (FIRMWARE / "face_renderer.cpp").read_text(encoding="utf-8")
        face_h = (FIRMWARE / "face_renderer.h").read_text(encoding="utf-8")
        self.assertIn("drawAngryEye", face_h + face_cpp)
        self.assertIn("drawSadEye", face_h + face_cpp)
        self.assertIn("drawThickLine", face_h + face_cpp)
        self.assertRegex(face_cpp, r"case FaceMood::Angry:[\s\S]*drawAngryEye")
        self.assertRegex(face_cpp, r"case FaceMood::Sad:[\s\S]*drawSadEye")

    def test_tired_eyes_reuse_sad_arc_with_sleepy_lid(self):
        face_cpp = (FIRMWARE / "face_renderer.cpp").read_text(encoding="utf-8")
        tired_match = re.search(r"void RobotFaceRenderer::drawTiredEye[\s\S]*?\n\}", face_cpp)
        self.assertIsNotNone(tired_match)
        tired_block = tired_match.group(0)
        for snippet in [
            "tiredArcHeight",
            "tiredArcWidth",
            "tiredDowncastY",
            "drawSadEye(canvas, cx, tiredDowncastY, tiredArcWidth, tiredArcHeight",
            "drawThickLine(canvas",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, tired_block)
        self.assertNotIn("fillRoundRect", tired_block)

    def test_happy_eyes_are_regenerated_as_thick_closed_arcs(self):
        face_cpp = (FIRMWARE / "face_renderer.cpp").read_text(encoding="utf-8")
        face_h = (FIRMWARE / "face_renderer.h").read_text(encoding="utf-8")
        source = face_h + face_cpp
        for snippet in [
            "drawHappyEye",
            "drawThickArc",
            "drawRoundCap",
            "happyArcThickness",
            "happyArcHeight",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)
        self.assertRegex(face_cpp, r"case FaceMood::Happy:[\s\S]*drawHappyEye")
        happy_match = re.search(r"case FaceMood::Happy:[\s\S]*?break;", face_cpp)
        self.assertIsNotNone(happy_match)
        self.assertNotIn("drawArcEye", happy_match.group(0))

    def test_curious_eyes_are_pupil_free_robot_shapes(self):
        face_cpp = (FIRMWARE / "face_renderer.cpp").read_text(encoding="utf-8")
        curious_match = re.search(r"void RobotFaceRenderer::drawCuriousEye[\s\S]*?\n\}", face_cpp)
        self.assertIsNotNone(curious_match)
        curious_block = curious_match.group(0)
        for snippet in [
            "curiousOpenBias",
            "curiousSquintBias",
            "curiousVerticalBias",
            "drawRoundedEye(canvas, cx, curiousCy",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, curious_block)
        for snippet in [
            "drawThickLine",
            "cutout",
            "sparkle",
            "fillCircle",
            "TFT_BLACK",
        ]:
            with self.subTest(snippet=snippet):
                self.assertNotIn(snippet, curious_block)

    def test_surprised_eyes_keep_original_rounded_glow_path(self):
        face_cpp = (FIRMWARE / "face_renderer.cpp").read_text(encoding="utf-8")
        glow_match = re.search(r"const bool roundedEyeGlow =[\s\S]*?;", face_cpp)
        self.assertIsNotNone(glow_match)
        glow_block = glow_match.group(0)
        self.assertIn("FaceMood::Surprised", glow_block)
        surprised_match = re.search(r"case FaceMood::Surprised:[\s\S]*?break;", face_cpp)
        self.assertIsNotNone(surprised_match)
        surprised_block = surprised_match.group(0)
        self.assertIn("drawRoundedEye(canvas, cx, cy", surprised_block)
        self.assertNotIn("drawThickLine", surprised_block)

    def test_dizzy_eyes_use_thicker_spiral_lines(self):
        face_cpp = (FIRMWARE / "face_renderer.cpp").read_text(encoding="utf-8")
        spiral_match = re.search(r"void RobotFaceRenderer::drawSpiralEye[\s\S]*?\n\}", face_cpp)
        self.assertIsNotNone(spiral_match)
        spiral_block = spiral_match.group(0)
        for snippet in [
            "spiralThickness",
            "drawThickLine(canvas, lastX, lastY, x, y, spiralThickness",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, spiral_block)
        self.assertNotIn("canvas.drawLine(lastX, lastY, x, y, color)", spiral_block)

    def test_angry_eyes_stay_separated_at_default_spacing(self):
        face_cpp = (FIRMWARE / "face_renderer.cpp").read_text(encoding="utf-8")
        match = re.search(r"case FaceMood::Angry:[\s\S]*?case FaceMood::Sad:", face_cpp)
        self.assertIsNotNone(match)
        angry_block = match.group(0)
        self.assertIn("angryOutwardOffset", angry_block)
        self.assertIn("angryEyeWidth", angry_block)
        self.assertIn("angryGlowWidth", angry_block)
        self.assertNotIn("w + 26", angry_block)
        self.assertNotIn("w + 34", angry_block)

    def test_hardware_service_uses_official_robot_apis(self):
        source = self.read_all()
        for snippet in [
            "M5StackChan.Motion.move",
            "M5StackChan.Motion.rotateX",
            "M5StackChan.Motion.goHome",
            "M5StackChan.setRgbColor",
            "M5StackChan.refreshRgb",
            "M5StackChan.TouchSensor.getIntensities",
            "M5StackChan.TouchSensor.wasSwipedForward",
            "M5StackChan.TouchSensor.wasSwipedBackward",
            "M5StackChan.getBatteryVoltage",
            "M5StackChan.getBatteryCurrent",
            "M5.Speaker.tone",
            "M5.Speaker.setVolume",
            "M5.Speaker.stop",
            "M5.Mic.record",
            "M5.Mic.isEnabled",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)

    def test_robot_mic_sampling_is_throttled_and_reuses_mic_session(self):
        header = (FIRMWARE / "robot_hardware.h").read_text(encoding="utf-8")
        source = (FIRMWARE / "robot_hardware.cpp").read_text(encoding="utf-8")
        mic_sample = re.search(r"String RobotHardware::micSampleJson[\s\S]*?ImuSample RobotHardware::readImuSample", source)
        self.assertIsNotNone(mic_sample)
        block = mic_sample.group(0)
        for snippet in [
            "lastMicSampleAtMs_",
            "lastMicSampleJson_",
            "cached",
            "if (!M5.Mic.isEnabled())",
            "M5.Mic.begin()",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, block + header)
        self.assertNotRegex(block, r"M5\\.Speaker\\.end\\(\\)[\\s\\S]*M5\\.Mic\\.begin\\(\\)")

    def test_rgb_lights_can_be_controlled_by_left_and_right_side(self):
        source = self.read_all()
        for snippet in [
            "RGB_LEFT_START",
            "RGB_LEFT_END",
            "RGB_RIGHT_START",
            "RGB_RIGHT_END",
            "setRgbLeft",
            "setRgbRight",
            "setRgbRange(RGB_LEFT_START, RGB_RIGHT_END",
            'side == "left"',
            'side == "right"',
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)

    def test_robot_hardware_tracks_configured_rgb_and_temporary_effect_frames(self):
        robot_h = (FIRMWARE / "robot_hardware.h").read_text(encoding="utf-8")
        robot_cpp = (FIRMWARE / "robot_hardware.cpp").read_text(encoding="utf-8")
        source = robot_h + robot_cpp
        for snippet in [
            "constexpr uint8_t RGB_LED_COUNT = 12",
            "struct RgbColor",
            "struct RgbSnapshot",
            "configuredRgb_",
            "rgbConfigRevision_",
            "snapshotRgb",
            "restoreRgbSnapshot",
            "restoreConfiguredRgb",
            "setRgbEffectFrame",
            "applySpeechLightLevel",
            "scaleRgb",
            "rgbEffectActive_",
            "speechLightActive_",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)
        self.assertRegex(robot_cpp, r"void RobotHardware::setRgbOne[\s\S]*configuredRgb_\[index\]")
        self.assertRegex(robot_cpp, r"void RobotHardware::setRgbRange[\s\S]*configuredRgb_\[index\]")
        self.assertRegex(robot_cpp, r"void RobotHardware::setRgbEffectFrame[\s\S]*M5StackChan\.setRgbColor")
        self.assertRegex(robot_cpp, r"void RobotHardware::restoreRgbSnapshot[\s\S]*snapshot\.revision == rgbConfigRevision_")

    def test_rgb_snapshot_restores_manual_light_mode_after_temporary_expression_lights(self):
        robot_h = (FIRMWARE / "robot_hardware.h").read_text(encoding="utf-8")
        robot_cpp = (FIRMWARE / "robot_hardware.cpp").read_text(encoding="utf-8")
        source = robot_h + robot_cpp

        for snippet in [
            "uint8_t brightness",
            "bool breathingEnabled",
            "bool marqueeEnabled",
            "bool rainbowEnabled",
            "const char* presetName",
            "snapshot.brightness = rgbBrightness_",
            "snapshot.breathingEnabled = rgbBreathingEnabled_",
            "snapshot.marqueeEnabled = rgbMarqueeEnabled_",
            "snapshot.rainbowEnabled = rgbRainbowEnabled_",
            "snapshot.presetName = rgbPresetName_",
            "rgbBrightness_ = snapshot.brightness",
            "rgbBreathingEnabled_ = snapshot.breathingEnabled",
            "rgbMarqueeEnabled_ = snapshot.marqueeEnabled",
            "rgbRainbowEnabled_ = snapshot.rainbowEnabled",
            "rgbPresetName_ = snapshot.presetName == nullptr ? \"manual\" : snapshot.presetName",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)

    def test_standby_rgb_is_restored_when_idle_resumes_after_feature_lights(self):
        robot_h = (FIRMWARE / "robot_hardware.h").read_text(encoding="utf-8")
        robot_cpp = (FIRMWARE / "robot_hardware.cpp").read_text(encoding="utf-8")
        idle_cpp = (FIRMWARE / "idle_behavior.cpp").read_text(encoding="utf-8")
        api = (FIRMWARE / "api.cpp").read_text(encoding="utf-8")
        firmware_ino = (FIRMWARE / "firmware.ino").read_text(encoding="utf-8")
        motion_stream_cpp = (FIRMWARE / "motion_stream.cpp").read_text(encoding="utf-8")
        source = robot_h + robot_cpp + idle_cpp + api + firmware_ino + motion_stream_cpp

        for snippet in [
            "standbyRgb_",
            "standbyRgbBrightness_",
            "standbyRgbBreathingEnabled_",
            "standbyRgbPresetName_",
            "rgbTemporaryEffectActive_",
            "captureStandbyRgb",
            "restoreStandbyRgb",
            "markTemporaryRgbOverride",
            "extendTemporaryRgbOverride",
            "updateTemporaryRgbOverride",
            "rgbTemporaryRestorePending_",
            "rgbTemporaryRestoreAtMs_",
            "hardware.captureStandbyRgb()",
            "hardware.restoreStandbyRgb(nowMs)",
            'doc["standby"].is<bool>()',
            'doc["temporary"].is<bool>()',
            "hardware.markTemporaryRgbOverride(nowMs);",
            "idle.pauseForManualControl(nowMs, RGB_TEMPORARY_RESTORE_MS);",
            "hardware.updateTemporaryRgbOverride(nowMs)",
            "hardware.extendTemporaryRgbOverride(nowMs)",
            'server.on("/api/rgb/restore"',
            '\\"standby\\":{',
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)

        self.assertRegex(api, r"bool captureStandbyRgb[\s\S]*doc\[\"standby\"\][\s\S]*doc\[\"temporary\"\]")
        self.assertRegex(api, r'server\.on\("/api/rgb"[\s\S]*&idle[\s\S]*idle\.pauseForManualControl\(nowMs, RGB_TEMPORARY_RESTORE_MS\)')
        self.assertRegex(api, r'server\.on\("/api/motion/pose"[\s\S]*hardware\.extendTemporaryRgbOverride\(nowMs\)')
        self.assertRegex(api, r'if \(captureStandbyRgb\) \{[\s\S]*hardware\.captureStandbyRgb\(\);[\s\S]*\} else \{[\s\S]*hardware\.markTemporaryRgbOverride\(nowMs\);[\s\S]*idle\.pauseForManualControl\(nowMs, RGB_TEMPORARY_RESTORE_MS\);')
        self.assertRegex(source, r'MotionStreamReceiver::update[\s\S]*hardware\.extendTemporaryRgbOverride\(nowMs\)')
        self.assertRegex(api, r'server\.on\("/api/rgb/restore"[\s\S]*hardware\.restoreStandbyRgb\(nowMs\)')
        self.assertRegex(robot_cpp, r"void RobotHardware::restoreStandbyRgb\(uint32_t nowMs\)[\s\S]*standbyRgb_\[index\]")
        self.assertRegex(robot_cpp, r"void RobotHardware::updateTemporaryRgbOverride\(uint32_t nowMs\)[\s\S]*restoreStandbyRgb\(nowMs\)")
        self.assertRegex(robot_cpp, r"void RobotHardware::setRgbEffectFrame[\s\S]*rgbTemporaryEffectActive_ = temporary")
        self.assertRegex(robot_cpp, r"updateRgbManualEffects[\s\S]*if \(rgbTemporaryEffectActive_\) return;")
        self.assertRegex(robot_cpp, r"updateRgbBreathing[\s\S]*setRgbEffectFrame\(frame, false\)")
        self.assertRegex(robot_cpp, r"updateRgbMarquee[\s\S]*setRgbEffectFrame\(frame, false\)")
        light_asset = re.search(r"String applyRainbowMarqueeLightAsset[\s\S]*?\n\}", api)
        self.assertIsNotNone(light_asset)
        light_asset_body = light_asset.group(0)
        self.assertIn("hardware.setRgbEffectFrame(frame)", light_asset_body)
        self.assertNotIn("hardware.setRgbOne", light_asset_body)
        self.assertRegex(api, r'id == "light\.rainbow_marquee"[\s\S]*idle\.pauseForManualControl\(nowMs, durationMs\)')

    def test_rgb_config_revision_tracks_light_mode_settings(self):
        robot_cpp = (FIRMWARE / "robot_hardware.cpp").read_text(encoding="utf-8")

        for method in [
            "setRgbBrightness",
            "setRgbBreathingEnabled",
            "setRgbMarqueeEnabled",
            "setRgbRainbowEnabled",
            "setRgbPresetName",
        ]:
            with self.subTest(method=method):
                match = re.search(rf"void RobotHardware::{method}[\s\S]*?\n\}}", robot_cpp)
                self.assertIsNotNone(match)
                self.assertIn("rgbConfigRevision_++", match.group(0))

    def test_rgb_brightness_and_manual_breathing_are_exposed(self):
        robot_h = (FIRMWARE / "robot_hardware.h").read_text(encoding="utf-8")
        robot_cpp = (FIRMWARE / "robot_hardware.cpp").read_text(encoding="utf-8")
        api = (FIRMWARE / "api.cpp").read_text(encoding="utf-8")
        ino = (FIRMWARE / "firmware.ino").read_text(encoding="utf-8")
        source = robot_h + robot_cpp + api + ino
        for snippet in [
            "setRgbBrightness",
            "rgbBrightness() const",
            "rgbBrightness_ = DEFAULT_RGB_BRIGHTNESS",
            "DEFAULT_RGB_BRIGHTNESS = 60",
            "setRgbBreathingEnabled",
            "rgbBreathingEnabled() const",
            "updateRgbBreathing",
            "updateRgbManualEffects",
            "rgbBreathingEnabled_",
            "RGB_BREATH_CYCLE_MS",
            "RGB_BREATH_FRAME_MS",
            "rgbJson() const",
            '\\"brightness\\":',
            '\\"breathing\\":',
            'doc["brightness"].is<int>()',
            'doc["breathing"].is<bool>()',
            "hardware.setRgbBrightness",
            "hardware.setRgbBreathingEnabled",
            "hardware.rgbJson()",
            "hardware.updateRgbManualEffects(nowMs, emotionActions.lightsActive())",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)
        self.assertRegex(robot_cpp, r"void RobotHardware::writeRgb[\s\S]*scaleRgb\(color, rgbBrightness_\)")
        self.assertRegex(robot_cpp, r"updateRgbBreathing[\s\S]*if \(blockedByTemporaryLights \|\| speechLightActive_\) return;")
        self.assertRegex(robot_cpp, r"updateRgbBreathing[\s\S]*scaleRgb\(configuredRgb_\[index\], breathBrightness")

    def test_factory_rgb_defaults_match_current_device_lighting(self):
        robot_h = (FIRMWARE / "robot_hardware.h").read_text(encoding="utf-8")
        robot_cpp = (FIRMWARE / "robot_hardware.cpp").read_text(encoding="utf-8")
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")
        source = robot_h + robot_cpp + console
        for snippet in [
            "DEFAULT_RGB_COLOR = {0, 255, 98}",
            "DEFAULT_RGB_BRIGHTNESS = 60",
            "rgbBrightness_ = DEFAULT_RGB_BRIGHTNESS",
            "configuredRgb_[index] = DEFAULT_RGB_COLOR",
            "displayedRgb_[index] = DEFAULT_RGB_COLOR",
            "rgbBreathingEnabled_ = true",
            "rgbMarqueeEnabled_ = false",
            "rgbRainbowEnabled_ = false",
            "updateRgbBreathing(0, false)",
            "rgbColorToHex(configuredRgb_[RGB_LEFT_START])",
            "\\\"leftColor\\\":",
            "\\\"rightColor\\\":",
            "setValue('rgbAll',rgb.color||'#00ff62')",
            "setValue('rgbLeft',rgb.leftColor||rgb.color||'#00ff62')",
            "setValue('rgbRight',rgb.rightColor||rgb.color||'#00ff62')",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)

    def test_manual_marquee_rainbow_and_light_presets_are_exposed(self):
        robot_h = (FIRMWARE / "robot_hardware.h").read_text(encoding="utf-8")
        robot_cpp = (FIRMWARE / "robot_hardware.cpp").read_text(encoding="utf-8")
        api = (FIRMWARE / "api.cpp").read_text(encoding="utf-8")
        source = robot_h + robot_cpp + api
        for snippet in [
            "setRgbMarqueeEnabled",
            "rgbMarqueeEnabled() const",
            "setRgbRainbowEnabled",
            "rgbRainbowEnabled() const",
            "updateRgbMarquee",
            "showRgbRainbow",
            "rgbMarqueeEnabled_",
            "rgbRainbowEnabled_",
            "RGB_MARQUEE_FRAME_MS",
            "rainbowColor",
            '\\"marquee\\":',
            '\\"rainbow\\":',
            '\\"preset\\":',
            'doc["marquee"].is<bool>()',
            'doc["rainbow"].is<bool>()',
            'doc["preset"].is<const char*>()',
            "applyRgbPreset",
            "hardware.setRgbMarqueeEnabled",
            "hardware.setRgbRainbowEnabled",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)
        self.assertRegex(robot_cpp, r"updateRgbManualEffects[\s\S]*updateRgbMarquee")
        self.assertRegex(robot_cpp, r"updateRgbMarquee[\s\S]*rainbowColor\(index \+ rgbMarqueeOffset_")
        self.assertRegex(api, r"applyRgbPreset[\s\S]*softGreenBreath[\s\S]*rainbowMarquee[\s\S]*rainbowStatic")

    def test_emotion_actions_define_light_effects_and_restore_rgb(self):
        emotion_h = (FIRMWARE / "emotion_actions.h").read_text(encoding="utf-8")
        emotion_cpp = (FIRMWARE / "emotion_actions.cpp").read_text(encoding="utf-8")
        source = emotion_h + emotion_cpp
        for snippet in [
            "enum class EmotionLightEffect",
            "RainbowMarquee",
            "Breathing",
            "RedPulse",
            "SurpriseFlash",
            "ThinkingTravel",
            "EmotionLightEffect lightEffect",
            "play(const String& name, uint32_t nowMs, bool lightsEnabled",
            "stop(RobotHardware& hardware, uint32_t nowMs)",
            "lightsActive() const",
            "rgbSnapshot_",
            "lightSnapshotCaptured_",
            "restoreEmotionLights",
            "applyEmotionLight",
            "hardware.snapshotRgb()",
            "hardware.restoreRgbSnapshot(rgbSnapshot_)",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)
        self.assertRegex(emotion_cpp, r"EMOTION_HAPPY_NOD\[\][\s\S]*RainbowMarquee")
        self.assertRegex(emotion_cpp, r"EMOTION_SAD_DIAGONAL_DOWN\[\][\s\S]*Breathing")
        self.assertRegex(emotion_cpp, r"EMOTION_ANGRY_SHAKE\[\][\s\S]*RedPulse")
        self.assertRegex(emotion_cpp, r"EMOTION_SURPRISED_RECOIL\[\][\s\S]*SurpriseFlash")
        self.assertRegex(emotion_cpp, r"EMOTION_THINKING_DIP\[\][\s\S]*ThinkingTravel")

    def test_comfortable_nod_happy_steps_use_marquee_lights(self):
        emotion_cpp = (FIRMWARE / "emotion_actions.cpp").read_text(encoding="utf-8")
        comfortable_block = re.search(
            r"const EmotionStep EMOTION_COMFORTABLE_NOD\[\] = \{(?P<body>[\s\S]*?)\};",
            emotion_cpp,
        )
        self.assertIsNotNone(comfortable_block)
        happy_rows = re.findall(r'\{"happy"[^{}]*\}', comfortable_block.group("body"))
        self.assertGreaterEqual(len(happy_rows), 1)
        for row in happy_rows:
            with self.subTest(row=row):
                self.assertIn("EmotionLightEffect::BlinkingMarquee", row)

    def test_head_touch_comfortable_nod_has_visible_pitch_swing(self):
        emotion_cpp = (FIRMWARE / "emotion_actions.cpp").read_text(encoding="utf-8")
        comfortable_block = re.search(
            r"const EmotionStep EMOTION_COMFORTABLE_NOD\[\] = \{(?P<body>[\s\S]*?)\};",
            emotion_cpp,
        )
        self.assertIsNotNone(comfortable_block)
        source = emotion_cpp
        body = comfortable_block.group("body")

        for snippet in [
            "COMFORTABLE_NOD_UP_PITCH = DEFAULT_REST_PITCH + 100",
            "COMFORTABLE_NOD_DOWN_PITCH = PITCH_MIN + 72",
            "COMFORTABLE_NOD_SECOND_UP_PITCH = DEFAULT_REST_PITCH + 82",
            "COMFORTABLE_NOD_SECOND_DOWN_PITCH = PITCH_MIN + 82",
        ]:
            with self.subTest(constant=snippet):
                self.assertIn(snippet, source)

        for snippet in [
            '{"happy", 0, COMFORTABLE_NOD_UP_PITCH',
            '{"love", 0, COMFORTABLE_NOD_DOWN_PITCH',
            '{"happy", 0, COMFORTABLE_NOD_SECOND_UP_PITCH',
            '{"love", 0, COMFORTABLE_NOD_SECOND_DOWN_PITCH',
            '0.16f, 90, 440',
            '0.34f, 125, 640',
            '0.34f, 125, 660',
            '0.28f, 115, 640',
            '0.26f, 115, 620',
            '0.08f, 220, 380',
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, body)

    def test_emotion_action_library_is_data_driven_and_expanded(self):
        emotion_h = (FIRMWARE / "emotion_actions.h").read_text(encoding="utf-8")
        emotion_cpp = (FIRMWARE / "emotion_actions.cpp").read_text(encoding="utf-8")
        source = emotion_h + emotion_cpp
        for snippet in [
            "EMOTION_COMFORTABLE_NOD",
            "EMOTION_SHY_HIDE",
            "EMOTION_CUTE_ATTENTION",
            "EMOTION_PROUD_SWAY",
            "EMOTION_CONFUSED_TILT",
            "EMOTION_SCARED_RECOIL",
            "EMOTION_CELEBRATE",
            "SoftPinkPulse",
            "WarmBounce",
            "GoldPulse",
            "BluePurpleBlink",
            "ColdFlash",
            "CelebrationRainbow",
            "emotionLibraryJson",
            "for (uint8_t i = 0; i < EMOTION_ACTION_COUNT; ++i)",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)
        for name, title in [
            ("comfortableNod", "舒服点点头"),
            ("shy", "害羞躲闪"),
            ("cute", "撒娇求关注"),
            ("proud", "得意抬头"),
            ("confused", "困惑歪头"),
            ("scared", "紧张后缩"),
            ("celebrate", "庆祝欢呼"),
        ]:
            with self.subTest(action=name):
                self.assertRegex(emotion_cpp, rf'\{{"{name}", "{title}", EMOTION_')
        self.assertNotIn(
            '\\"library\\":[\\"开心点头\\",\\"伤心低头\\",\\"生气摆头\\",\\"惊讶后仰\\",\\"思考低头\\"]',
            emotion_cpp,
        )

    def test_emotion_actions_support_multiple_variants_per_emotion(self):
        emotion_h = (FIRMWARE / "emotion_actions.h").read_text(encoding="utf-8")
        emotion_cpp = (FIRMWARE / "emotion_actions.cpp").read_text(encoding="utf-8")
        source = emotion_h + emotion_cpp
        for snippet in [
            "struct EmotionVariant",
            "const EmotionVariant* variants",
            "variantCount",
            "variantIndex_",
            "variantName_",
            "variantTitle_",
            "playCount_",
            "chooseVariantIndex",
            "const EmotionVariant& selectedVariant",
            "steps_ = selectedVariant.steps",
            '\\"variant\\":',
            '\\"variantTitle\\":',
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)
        for array_name in [
            "EMOTION_HAPPY_VARIANTS",
            "EMOTION_SAD_VARIANTS",
            "EMOTION_ANGRY_VARIANTS",
            "EMOTION_SURPRISED_VARIANTS",
            "EMOTION_THINKING_VARIANTS",
            "EMOTION_SHY_VARIANTS",
            "EMOTION_CUTE_VARIANTS",
            "EMOTION_PROUD_VARIANTS",
            "EMOTION_CONFUSED_VARIANTS",
            "EMOTION_SCARED_VARIANTS",
            "EMOTION_CELEBRATE_VARIANTS",
        ]:
            with self.subTest(array_name=array_name):
                match = re.search(rf"const EmotionVariant {array_name}\[\] = \{{([\s\S]*?)\n\}};", emotion_cpp)
                self.assertIsNotNone(match)
                self.assertGreaterEqual(match.group(1).count('{"'), 3)

    def test_console_displays_current_emotion_variant(self):
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")
        for snippet in [
            'id="emotionVariant"',
            "变体",
            "$('emotionVariant').textContent=s.variantTitle||s.variant||'-'",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, console)

    def test_speech_react_pulses_current_rgb_when_emotion_lights_are_inactive(self):
        api = (FIRMWARE / "api.cpp").read_text(encoding="utf-8")
        robot_cpp = (FIRMWARE / "robot_hardware.cpp").read_text(encoding="utf-8")
        self.assertRegex(api, r'"/api/face/react"[\s\S]*hardware\.applySpeechLightLevel\(audioLevel, emotion\.lightsActive\(\)\)')
        self.assertRegex(robot_cpp, r"applySpeechLightLevel[\s\S]*if \(blockedByEmotionLights\) return;")
        self.assertRegex(robot_cpp, r"applySpeechLightLevel[\s\S]*configuredRgb_\[index\]")
        self.assertRegex(robot_cpp, r"applySpeechLightLevel[\s\S]*restoreConfiguredRgb\(\)")

    def test_emotion_light_api_and_console_toggle_are_exposed(self):
        api = (FIRMWARE / "api.cpp").read_text(encoding="utf-8")
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")
        emotion_cpp = (FIRMWARE / "emotion_actions.cpp").read_text(encoding="utf-8")
        for snippet in [
            'doc["lights"].is<bool>()',
            "emotion.play(name, nowMs, lightsEnabled)",
            "emotion.stop(hardware, nowMs)",
            '\\"lightsEnabled\\":',
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, api + emotion_cpp)
        for snippet in [
            'id="emotionLights"',
            "动作灯效",
            "灯效",
            "emotionLightsState",
            "lights:bool('emotionLights')",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, console)

    def test_console_exposes_expanded_emotion_action_controls(self):
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")
        for snippet in [
            "害羞躲闪",
            "撒娇求关注",
            "得意抬头",
            "困惑歪头",
            "紧张后缩",
            "庆祝欢呼",
            "playEmotion('shy')",
            "playEmotion('cute')",
            "playEmotion('proud')",
            "playEmotion('confused')",
            "playEmotion('scared')",
            "playEmotion('celebrate')",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, console)

    def test_sleep_startle_action_asset_pauses_confused_then_small_left_right(self):
        api = (FIRMWARE / "api.cpp").read_text(encoding="utf-8")
        emotion_cpp = (FIRMWARE / "emotion_actions.cpp").read_text(encoding="utf-8")
        catalog = (FIRMWARE / "asset_logic_catalog.cpp").read_text(encoding="utf-8")
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")
        block_match = re.search(r"const EmotionStep EMOTION_SLEEP_STARTLE\[\] = \{(?P<body>[\s\S]*?)\};", emotion_cpp)
        self.assertIsNotNone(block_match)
        block = block_match.group("body")
        for snippet in [
            '"sleeping"',
            '"surprised"',
            "惊醒快速抬头",
            "惊醒懵一下停顿",
            "惊醒懵逼想一下",
            "惊醒小幅左右确认-左",
            "惊醒小幅左右确认-右",
            "惊醒回中停顿",
            "惊醒再次小幅左右-左",
            "惊醒再次小幅左右-右",
            "EmotionLightEffect::SurpriseFlash",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, block)
        self.assertNotIn("惊醒上下摇头", block)
        self.assertRegex(emotion_cpp, r'\{"sleepStartle", "被惊醒", EMOTION_SLEEP_STARTLE_VARIANTS')
        self.assertIn("behavior.emotion.sleep_startle", catalog)
        self.assertIn("motion.emotion.sleep_startle", catalog)
        self.assertIn('id == "motion.emotion.sleep_startle"', api)
        self.assertIn('return "sleepStartle"', api)
        self.assertIn("playEmotion('sleepStartle')", console)

    def test_head_touch_happy_trigger_controller_is_wired(self):
        source = self.read_all()
        for snippet in [
            "EmotionTriggerController",
            "headTouchHappyEnabled",
            "readTouchState",
            "nextHeadTouchHappyAtMs_",
            "headTouchWasPressed_",
            'emotion.play("comfortableNod", nowMs, true)',
            'mumble.playEmotionSlotJson("comfortableNod", nowMs, emotion.durationMs())',
            'lastTrigger_ = "headTouchComfortableNod"',
            "emotionTriggers.update(hardware, emotionActions, idleBehavior, mumble, robotClock, nowMs)",
            "registerApiRoutes(server, faceRenderer, hardware, idleBehavior, emotionActions, emotionTriggers, mumble, robotClock, proximityTracker, cameraAttention, localFaceDetector, motionStream, assistantStatus, wifiManager)",
            'doc["headTouchHappyEnabled"].is<bool>()',
            '\\"lastTrigger\\":',
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)

    def test_touch_wakes_idle_sleep_before_happy_or_angry_triggers(self):
        trigger_cpp = (FIRMWARE / "emotion_triggers.cpp").read_text(encoding="utf-8")
        wake_pos = trigger_cpp.find("idle.currentStepWakeableByTouch()")
        angry_pos = trigger_cpp.find("screenTouchAngryEnabled_ && screenRisingEdge")
        happy_pos = trigger_cpp.find("headTouchHappyEnabled_ && risingEdge")
        self.assertNotEqual(-1, wake_pos)
        self.assertNotEqual(-1, angry_pos)
        self.assertNotEqual(-1, happy_pos)
        self.assertLess(wake_pos, angry_pos)
        self.assertLess(wake_pos, happy_pos)
        wake_block = re.search(r"if \(\(risingEdge \|\| screenRisingEdge\) && idle\.currentStepWakeableByTouch\(\)\) \{(?P<body>[\s\S]*?)\n  \}", trigger_cpp)
        self.assertIsNotNone(wake_block)
        for snippet in [
            'emotion.play("sleepStartle", nowMs, true)',
            'lastTrigger_ = "sleepStartle"',
            "resetHeadTouchCombo()",
            "return;",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, wake_block.group("body"))

    def test_night_touch_triggers_random_awake_period_before_tired_idle(self):
        clock_h = (FIRMWARE / "robot_clock.h").read_text(encoding="utf-8")
        clock_cpp = (FIRMWARE / "robot_clock.cpp").read_text(encoding="utf-8")
        idle_cpp = (FIRMWARE / "idle_behavior.cpp").read_text(encoding="utf-8")
        trigger_cpp = (FIRMWARE / "emotion_triggers.cpp").read_text(encoding="utf-8")
        catalog = (FIRMWARE / "asset_logic_catalog.cpp").read_text(encoding="utf-8")
        source = clock_h + clock_cpp + idle_cpp + trigger_cpp + catalog
        for snippet in [
            "NIGHT_AWAKE_MIN_MS = 600000UL",
            "NIGHT_AWAKE_MAX_MS = 1200000UL",
            "startNightAwakePeriod",
            "nightAwakeActive(uint32_t nowMs) const",
            "nightAwakeRemainingMs(uint32_t nowMs) const",
            "chooseNightAwakeDurationMs",
            "nightAwakeUntilMs_",
            "lastNightAwakeDurationMs_",
            '\\"nightAwakeActive\\":',
            '\\"nightAwakeRemainingMs\\":',
            'clock.nightAwakeActive(nowMs)',
            'clock.startNightAwakePeriod(nowMs)',
            "logic.clock.night_touch_awake",
            "夜间触摸清醒期",
            "random 10-20 minute awake period",
            "logic.clock.night_sleep",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)

        self.assertRegex(idle_cpp, r"if \(clock\.isNightNow\(\) && !clock\.nightAwakeActive\(nowMs\)\) \{[\s\S]*?applyNightStep")

        for trigger in [
            'lastTrigger_ = "sleepStartle"',
            'lastTrigger_ = "screenTouchAngry"',
            'lastTrigger_ = "headTouchComfortableNod"',
        ]:
            with self.subTest(trigger=trigger):
                trigger_pos = trigger_cpp.find(trigger)
                awake_pos = trigger_cpp.rfind("clock.startNightAwakePeriod(nowMs)", 0, trigger_pos)
                self.assertNotEqual(-1, trigger_pos)
                self.assertNotEqual(-1, awake_pos)

    def test_head_touch_happy_trigger_is_calibratable_and_default_on(self):
        trigger_h = (FIRMWARE / "emotion_triggers.h").read_text(encoding="utf-8")
        trigger_cpp = (FIRMWARE / "emotion_triggers.cpp").read_text(encoding="utf-8")
        api = (FIRMWARE / "api.cpp").read_text(encoding="utf-8")
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")
        source = trigger_h + trigger_cpp
        for snippet in [
            "DEFAULT_HEAD_TOUCH_THRESHOLD = 2",
            "HEAD_TOUCH_STABLE_FRAMES = 2",
            "headTouchHappyEnabled_ = true",
            "headTouchThreshold_ = DEFAULT_HEAD_TOUCH_THRESHOLD",
            "headTouchStableFrames_",
            "headTouchPeak_",
            "setHeadTouchThreshold",
            "headTouchStableFrames_ >= HEAD_TOUCH_STABLE_FRAMES",
            "max(max(touch.front, touch.middle), touch.back)",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)
        for snippet in [
            'doc["headTouchThreshold"].is<int>()',
            '\\"headTouchThreshold\\":',
            '\\"headTouchPeak\\":',
            'id="headTouchThreshold"',
            "headTouchThreshold:+$('headTouchThreshold').value",
            "touchPeak",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, api + trigger_cpp + console)

    def test_head_touch_auto_trigger_keeps_love_nod_and_restores_five_touch_combo(self):
        trigger_cpp = (FIRMWARE / "emotion_triggers.cpp").read_text(encoding="utf-8")
        emotion_cpp = (FIRMWARE / "emotion_actions.cpp").read_text(encoding="utf-8")
        mumble_cpp = (FIRMWARE / "mumble_player.cpp").read_text(encoding="utf-8")
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")
        source = trigger_cpp + emotion_cpp + mumble_cpp + console
        for snippet in [
            "HEAD_TOUCH_COMBO_WINDOW_MS = 10000",
            "HEAD_TOUCH_COMBO_TARGET_COUNT = 5",
            'if (!angryEmotionActive && headTouchHappyEnabled_ && risingEdge)',
            "if (trackHeadTouchCombo(nowMs))",
            'emotion.play("headTouchComboSpin", nowMs, true)',
            'mumble.playEmotionSlotJson("headTouchComboSpin", nowMs, emotion.durationMs())',
            'lastTrigger_ = "headTouchComboSpin"',
            "extendHeadTouchComboRepeatWindow(nowMs)",
            "resetHeadTouchCombo();",
            'emotion.play("comfortableNod", nowMs, true)',
            'mumble.playEmotionSlotJson("comfortableNod", nowMs, emotion.durationMs())',
            'lastTrigger_ = "headTouchComfortableNod"',
            '\\"condition\\":\\"摸头\\"',
            '\\"action\\":\\"舒服点点头\\"',
            "EMOTION_COMFORTABLE_NOD",
            '{"comfortableNod", "舒服点点头", EMOTION_COMFORTABLE_NOD_VARIANTS',
            '{"comfortableNod", "舒服点点头"}',
            "摸头触发舒服点点头",
            "10秒内摸5次疯狂点头，触发后5秒内再摸继续疯狂点头",
            "playEmotion('comfortableNod')",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)
        comfortable_nod = re.search(
            r"const EmotionStep EMOTION_COMFORTABLE_NOD\[\] = \{(?P<body>[\s\S]*?)\};",
            emotion_cpp,
        )
        self.assertIsNotNone(comfortable_nod)
        self.assertIn('"love"', comfortable_nod.group("body"))
        for snippet in [
            'proximity.startHeadTouchSearch(nowMs)',
            "comboRisingEdge",
            'if (!angryEmotionActive && headTouchProximitySearchEnabled_ && comboRisingEdge)',
            'if (!angryEmotionActive && headTouchHappyEnabled_ && comboRisingEdge)',
        ]:
            with self.subTest(snippet=snippet):
                self.assertNotIn(snippet, trigger_cpp)

    def test_head_touch_combo_repeats_on_each_touch_inside_five_second_followup_window(self):
        trigger_h = (FIRMWARE / "emotion_triggers.h").read_text(encoding="utf-8")
        trigger_cpp = (FIRMWARE / "emotion_triggers.cpp").read_text(encoding="utf-8")
        catalog = (FIRMWARE / "asset_logic_catalog.cpp").read_text(encoding="utf-8")
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")
        source = trigger_h + trigger_cpp + catalog + console
        for snippet in [
            "HEAD_TOUCH_COMBO_REPEAT_WINDOW_MS = 5000",
            "headTouchComboRepeatUntilMs_",
            "headTouchComboRepeatWindowActive(nowMs)",
            "extendHeadTouchComboRepeatWindow(nowMs)",
            "headTouchComboRepeatUntilMs_ = nowMs + HEAD_TOUCH_COMBO_REPEAT_WINDOW_MS",
            "if (headTouchComboRepeatWindowActive(nowMs))",
            "resetHeadTouchCombo();",
            "return true;",
            '\\"headTouchComboRepeatWindowMs\\":',
            '\\"headTouchComboRepeatWindowActive\\":',
            '\\"headTouchComboRepeatWindowRemainingMs\\":',
            "五连摸续演窗口",
            "触发后5秒内再摸继续疯狂点头",
            "repeat touch inside 5 second follow-up window",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)
        self.assertNotIn("nextHeadTouchComboAtMs_ = nowMs + HEAD_TOUCH_COMBO_COOLDOWN_MS", trigger_cpp)

    def test_screen_touch_state_is_exposed_by_touch_api(self):
        robot_h = (FIRMWARE / "robot_hardware.h").read_text(encoding="utf-8")
        robot_cpp = (FIRMWARE / "robot_hardware.cpp").read_text(encoding="utf-8")
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")
        source = robot_h + robot_cpp + console
        for snippet in [
            "struct ScreenTouchState",
            "readScreenTouchState",
            "M5.Touch.getCount()",
            "M5.Touch.getDetail",
            '\\"screenPressed\\":',
            '\\"screenX\\":',
            '\\"screenY\\":',
            '\\"screenCount\\":',
            'id="screenTouchPressed"',
            'id="screenTouchPoint"',
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)

    def test_screen_touch_angry_trigger_is_wired(self):
        trigger_h = (FIRMWARE / "emotion_triggers.h").read_text(encoding="utf-8")
        trigger_cpp = (FIRMWARE / "emotion_triggers.cpp").read_text(encoding="utf-8")
        api = (FIRMWARE / "api.cpp").read_text(encoding="utf-8")
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")
        source = trigger_h + trigger_cpp + api + console
        for snippet in [
            "screenTouchAngryEnabled",
            "setScreenTouchAngryEnabled",
            "readScreenTouchState",
            "SCREEN_TOUCH_ANGRY_COOLDOWN_MS = 0",
            "nextScreenTouchAngryAtMs_",
            "screenTouchWasPressed_",
            "screenRisingEdge",
            "if (emotion.active()) emotion.stop(hardware, nowMs);",
            'emotion.play("angry", nowMs, true)',
            'doc["screenTouchAngryEnabled"].is<bool>()',
            '\\"screenTouchAngryEnabled\\":',
            '\\"screenTouchPressed\\":',
            '\\"screenTouchX\\":',
            '\\"screenTouchY\\":',
            "摸屏幕触发生气",
            "屏幕生气冷却",
            'id="triggerScreenTouchAngry"',
            "screenTouchAngryEnabled:bool('triggerScreenTouchAngry')",
            "triggerScreenTouchAngry",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)

    def test_proximity_triggered_behaviors_are_removed(self):
        source = self.read_all()
        for snippet in [
            "startHeadTouchSearch",
            "headTouchProximitySearchEnabled",
            "proximity.startHeadTouchSearch",
            "HEAD_TOUCH_SEARCH_SWEEP_YAWS",
            "bestSearchYaw_",
            "applyHeadTouchLockPose",
            'server.on("/api/proximity/search"',
            "startProximitySearch",
            "摸头红外找人测试",
            "setHandWaveEnabled",
            "setHandWaveDeltaThreshold",
            "updateHandWaveDetection",
            "applyHandWavePose",
            "handWaveScore_",
            "proximityHandWaveEnabled",
            "proximityWaveDelta",
            "手部快速摇晃",
            "setFollowTestEnabled",
            "followTestYaw_",
            "applyFollowTestPose",
            "proximityFollowTestEnabled",
            "跟随测试",
            "triggerSleepWake",
            "clock.wakeFromSleep(nowMs)",
            'emotion.play("sleepWake", nowMs, true)',
            'mode_ = "sleepWake"',
        ]:
            with self.subTest(snippet=snippet):
                self.assertNotIn(snippet, source)

    def test_proximity_debug_reader_has_no_servo_or_face_side_effects(self):
        tracker_h = (FIRMWARE / "proximity_tracker.h").read_text(encoding="utf-8")
        tracker_cpp = (FIRMWARE / "proximity_tracker.cpp").read_text(encoding="utf-8")
        source = tracker_h + tracker_cpp
        for snippet in [
            "class ProximityTrackerController",
            "void update(RobotHardware& hardware, uint32_t nowMs)",
            "lastState_ = hardware.readProximityState()",
            "bool active() const",
            "return false",
            '\\"proximityRaw\\":',
            '\\"ambientRaw\\":',
            '\\"mode\\":\\"debug\\"',
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)
        for snippet in [
            "RobotFaceRenderer",
            "IdleBehaviorController",
            "EmotionActionController",
            "poseWithRestPitchOffset",
            "face.setMood",
            "face.setReact",
            "pauseForManualControl",
            "markInteraction",
        ]:
            with self.subTest(snippet=snippet):
                self.assertNotIn(snippet, source)

    def test_robot_hardware_exposes_bmi270_accel_and_gyro_sample(self):
        robot_h = (FIRMWARE / "robot_hardware.h").read_text(encoding="utf-8")
        robot_cpp = (FIRMWARE / "robot_hardware.cpp").read_text(encoding="utf-8")
        source = robot_h + robot_cpp
        for snippet in [
            "struct ImuSample",
            "readImuSample",
            "float accelMagnitudeG",
            "float gyroMagnitudeDps",
            "M5.Imu.getAccel(&sample.accelX, &sample.accelY, &sample.accelZ)",
            "M5.Imu.getGyro(&sample.gyroX, &sample.gyroY, &sample.gyroZ)",
            '\\"gyroX\\":',
            '\\"gyroY\\":',
            '\\"gyroZ\\":',
            '\\"accelMagnitudeG\\":',
            '\\"gyroMagnitudeDps\\":',
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)

    def test_imu_motion_emotion_trigger_is_wired_without_angry_cooldown(self):
        trigger_h = (FIRMWARE / "emotion_triggers.h").read_text(encoding="utf-8")
        trigger_cpp = (FIRMWARE / "emotion_triggers.cpp").read_text(encoding="utf-8")
        api = (FIRMWARE / "api.cpp").read_text(encoding="utf-8")
        source = trigger_h + trigger_cpp + api
        for snippet in [
            "ANGRY_SHAKE_ACCEL_DELTA_G",
            "ANGRY_SHAKE_GYRO_DPS",
            "UPSIDE_DOWN_GRAVITY_DOT_THRESHOLD",
            "IMU_ANGRY_SHAKE_COOLDOWN_MS = 0",
            "imuMotionEnabled",
            "setImuMotionEnabled",
            "setImuMotionThresholds",
            "readImuSample",
            "imuShakeAccelerationDeltaG_",
            "imuGyroMagnitudeDps_",
            "imuUpsideDown_",
            "canTriggerContinuousImuAngry",
            "imuMotionEnabled_ && imuAvailable_ && imuUpsideDown_ && imuHardShakeActive",
            'emotion.play("angry", nowMs, true)',
            'lastTrigger_ = "imuAngryShake"',
            'doc["imuMotionEnabled"].is<bool>()',
            'doc["angryShakeGyroDps"].is<float>()',
            '\\"imuMotionEnabled\\":',
            '\\"imuShakeAccelerationDeltaG\\":',
            '\\"imuGyroMagnitudeDps\\":',
            '\\"imuUpsideDown\\":',
            '\\"imuAngryShakeCooldownMs\\":',
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)

    def test_light_shake_trigger_is_removed_to_avoid_false_positives(self):
        trigger_h = (FIRMWARE / "emotion_triggers.h").read_text(encoding="utf-8")
        trigger_cpp = (FIRMWARE / "emotion_triggers.cpp").read_text(encoding="utf-8")
        api = (FIRMWARE / "api.cpp").read_text(encoding="utf-8")
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")
        source = trigger_h + trigger_cpp + api + console
        for snippet in [
            "LIGHT_SHAKE_ACCEL_DELTA_G",
            "LIGHT_SHAKE_GYRO_DPS",
            "IMU_LIGHT_SHAKE_HAPPY_COOLDOWN_MS",
            "imuLightShakeWasActive_",
            "nextImuLightShakeHappyAtMs_",
            "imuLightShakeActive",
            "imuLightShakeRisingEdge",
            'lastTrigger_ = "imuLightShake',
            'emotion.play("curious", nowMs, true)',
            'doc["lightShakeAccelDeltaG"]',
            'doc["lightShakeGyroDps"]',
            "lightShakeAccelDeltaG",
            "lightShakeGyroDps",
            "轻摇",
        ]:
            with self.subTest(snippet=snippet):
                self.assertNotIn(snippet, source)

    def test_curious_lookaround_emotion_and_face_are_available_without_auto_trigger(self):
        face_h = (FIRMWARE / "face_renderer.h").read_text(encoding="utf-8")
        face_cpp = (FIRMWARE / "face_renderer.cpp").read_text(encoding="utf-8")
        api = (FIRMWARE / "api.cpp").read_text(encoding="utf-8")
        emotion_cpp = (FIRMWARE / "emotion_actions.cpp").read_text(encoding="utf-8")
        player = (FIRMWARE / "mumble_player.cpp").read_text(encoding="utf-8")
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")
        source = face_h + face_cpp + api + emotion_cpp + player + console
        for snippet in [
            "FaceMood::Curious",
            '"curious"',
            "drawCuriousEye",
            "EMOTION_CURIOUS_LOOK_AROUND",
            "EMOTION_CURIOUS_VARIANTS",
            '{"curious", "好奇观察", EMOTION_CURIOUS_VARIANTS',
            "playEmotion('curious')",
            'value="curious">好奇',
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)
        self.assertRegex(face_cpp, r"case FaceMood::Curious:[\s\S]*drawCuriousEye")

    def test_imu_inversion_uses_gravity_reference_not_fixed_z_axis(self):
        trigger_h = (FIRMWARE / "emotion_triggers.h").read_text(encoding="utf-8")
        trigger_cpp = (FIRMWARE / "emotion_triggers.cpp").read_text(encoding="utf-8")
        api = (FIRMWARE / "api.cpp").read_text(encoding="utf-8")
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")
        source = trigger_h + trigger_cpp + api + console
        for snippet in [
            "calibrateImuGravityReference",
            "imuGravityReferenceCaptured_",
            "imuGravityRefX_",
            "imuGravityDot_",
            "upsideDownGravityDotThreshold_",
            "imuVectorDeltaG_",
            "sqrtf(dx * dx + dy * dy + dz * dz)",
            "imuGravityDot_ <= upsideDownGravityDotThreshold_",
            "doc[\"upsideDownGravityDotThreshold\"].is<float>()",
            "\\\"imuGravityDot\\\":",
            "\\\"upsideDownGravityDotThreshold\\\":",
            "倒置匹配阈值",
            "s.upsideDownGravityDotThreshold",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)
        self.assertNotIn("imuAccelZ_ <= upsideDownAccelZThreshold_", trigger_cpp)

    def test_console_exposes_imu_motion_trigger_controls(self):
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")
        for snippet in [
            "陀螺仪/加速度触发",
            "倒置大摇生气",
            'id="triggerImuMotion"',
            'id="angryShakeAccelDeltaG"',
            'id="angryShakeGyroDps"',
            'id="upsideDownGravityDotThreshold"',
            'id="imuTriggerState"',
            'id="imuShakeStrength"',
            'id="imuGyroStrength"',
            'id="imuUpsideDownState"',
            'id="imuThresholdState"',
            "setBoolValue('triggerImuMotion',!!s.imuMotionEnabled)",
            "imuMotionEnabled:bool('triggerImuMotion')",
            "angryShakeGyroDps:+$('angryShakeGyroDps').value",
            "triggerImuMotion",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, console)

    def test_head_touch_combo_nod_is_automatic_five_touch_trigger(self):
        trigger_h = (FIRMWARE / "emotion_triggers.h").read_text(encoding="utf-8")
        trigger_cpp = (FIRMWARE / "emotion_triggers.cpp").read_text(encoding="utf-8")
        api = (FIRMWARE / "api.cpp").read_text(encoding="utf-8")
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")
        source = trigger_h + trigger_cpp + api + console
        for snippet in [
            "HEAD_TOUCH_COMBO_WINDOW_MS = 10000",
            "HEAD_TOUCH_COMBO_TARGET_COUNT = 5",
            "headTouchComboEnabled",
            "setHeadTouchComboEnabled",
            "headTouchComboCount_",
            "headTouchComboWindowStartedAtMs_",
            "headTouchComboWasRawPressed_",
            "nextHeadTouchComboAtMs_",
            "headTouchComboRepeatUntilMs_",
            "trackHeadTouchCombo",
            "headTouchRawPressed",
            'if (!angryEmotionActive && headTouchHappyEnabled_ && risingEdge)',
            "if (trackHeadTouchCombo(nowMs))",
            'emotion.play("headTouchComboSpin", nowMs, true)',
            'lastTrigger_ = "headTouchComboSpin"',
            'doc["headTouchComboEnabled"].is<bool>()',
            '\\"headTouchComboEnabled\\":',
            '\\"headTouchComboCount\\":',
            '\\"headTouchComboWindowActive\\":',
            '\\"headTouchComboWindowRemainingMs\\":',
            '\\"headTouchComboCooldownMs\\":',
            '\\"headTouchComboRepeatWindowRemainingMs\\":',
            'id="triggerHeadTouchCombo"',
            "setBoolValue('triggerHeadTouchCombo',!!s.headTouchComboEnabled)",
            "headTouchComboEnabled:bool('triggerHeadTouchCombo')",
            "10秒内摸5次疯狂点头，触发后5秒内再摸继续疯狂点头",
            "五连摸续演窗口",
            "s.lastTrigger==='headTouchComboSpin'?'摸头五连触发疯狂点头'",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)
        for snippet in [
            "comboRisingEdge",
            "30秒内摸够5次点头",
        ]:
            with self.subTest(snippet=snippet):
                self.assertNotIn(snippet, trigger_cpp + console)

    def test_head_touch_combo_emotion_nods_with_blinking_marquee(self):
        emotion_h = (FIRMWARE / "emotion_actions.h").read_text(encoding="utf-8")
        emotion_cpp = (FIRMWARE / "emotion_actions.cpp").read_text(encoding="utf-8")
        trigger_cpp = (FIRMWARE / "emotion_triggers.cpp").read_text(encoding="utf-8")
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")
        source = emotion_h + emotion_cpp + trigger_cpp + console
        for snippet in [
            "BlinkingMarquee",
            "int16_t rotateVelocity = 0",
            "EMOTION_HEAD_TOUCH_COMBO_NOD",
            '{"headTouchComboSpin", "摸头疯狂点头"',
            "hardware.rotateX(step.rotateVelocity)",
            "hardware.rotateX(0)",
            "摸头疯狂点头",
            "playEmotion('headTouchComboSpin')",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)

    def test_head_touch_emotions_are_suppressed_while_angry_is_running(self):
        emotion_h = (FIRMWARE / "emotion_actions.h").read_text(encoding="utf-8")
        emotion_cpp = (FIRMWARE / "emotion_actions.cpp").read_text(encoding="utf-8")
        trigger_cpp = (FIRMWARE / "emotion_triggers.cpp").read_text(encoding="utf-8")
        source = emotion_h + emotion_cpp + trigger_cpp
        for snippet in [
            "isActiveAction(const char* name) const",
            "return active_ && strcmp(actionName_, name) == 0",
            'const bool angryEmotionActive = emotion.isActiveAction("angry")',
            "if (angryEmotionActive) {",
            "resetHeadTouchCombo();",
            "if (!angryEmotionActive && headTouchHappyEnabled_ && risingEdge)",
            "if (trackHeadTouchCombo(nowMs))",
            'emotion.play("headTouchComboSpin", nowMs, true)',
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)
        for snippet in [
            "comboRisingEdge",
            "if (!angryEmotionActive && headTouchProximitySearchEnabled_ && comboRisingEdge)",
            "if (!angryEmotionActive && headTouchHappyEnabled_ && comboRisingEdge)",
        ]:
            with self.subTest(snippet=snippet):
                self.assertNotIn(snippet, trigger_cpp)

    def test_head_touch_combo_nod_uses_longer_marquee_nod_sequence(self):
        emotion_cpp = (FIRMWARE / "emotion_actions.cpp").read_text(encoding="utf-8")
        for snippet in [
            "HEAD_TOUCH_NOD_CENTER_YAW = 0",
            "HEAD_TOUCH_NOD_UP_PITCH = DEFAULT_REST_PITCH + 250",
            "HEAD_TOUCH_NOD_DOWN_PITCH = PITCH_MIN + 58",
            "全程跑马灯",
            "疯狂点头-抬头",
            "疯狂点头-低头",
            "HEAD_TOUCH_NOD_CENTER_YAW, HEAD_TOUCH_NOD_UP_PITCH",
            "HEAD_TOUCH_NOD_CENTER_YAW, HEAD_TOUCH_NOD_DOWN_PITCH",
            "HEAD_TOUCH_NOD_CENTER_YAW, DEFAULT_REST_PITCH",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, emotion_cpp)
        nod_block = re.search(
            r"const EmotionStep EMOTION_HEAD_TOUCH_COMBO_NOD\[\] = \{(?P<body>[\s\S]*?)\};",
            emotion_cpp,
        )
        self.assertIsNotNone(nod_block)
        body = nod_block.group("body")
        self.assertGreaterEqual(body.count("EmotionLightEffect::BlinkingMarquee"), 7)
        self.assertNotIn("EmotionLightEffect::RainbowMarquee", body)
        self.assertNotIn("HEAD_TOUCH_SPIN", body)
        self.assertNotIn("YAW_MIN", body)
        self.assertNotIn("YAW_MAX", body)
        self.assertNotIn("左转", body)
        self.assertNotIn("右转", body)

        happy_block = re.search(
            r"const EmotionStep EMOTION_HAPPY_NOD\[\] = \{(?P<body>[\s\S]*?)\};",
            emotion_cpp,
        )
        self.assertIsNotNone(happy_block)

        def total_duration(block):
            rows = re.findall(r"\{([^{}]+)\}", block)
            durations = []
            for row in rows:
                fields = [field.strip() for field in row.split(",")]
                durations.append(int(fields[6]))
            return 500 + sum(durations)

        self.assertGreater(total_duration(body), total_duration(happy_block.group("body")))
        self.assertGreaterEqual(total_duration(body), 2200)
        self.assertLess(total_duration(body), 3000)

    def test_combo_nod_emotion_is_available_as_direct_test_action(self):
        emotion_cpp = (FIRMWARE / "emotion_actions.cpp").read_text(encoding="utf-8")
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")
        for snippet in [
            '{"spin", "点头测试", EMOTION_HEAD_TOUCH_COMBO_VARIANTS',
            "全程跑马灯",
            "疯狂点头-抬头",
            "疯狂点头-低头",
            "playEmotion('spin')",
            "点头测试",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, emotion_cpp + console)
        self.assertRegex(emotion_cpp, r"EMOTION_HEAD_TOUCH_COMBO_NOD[\s\S]*疯狂点头-抬头[\s\S]*疯狂点头-低头")

    def test_combo_nod_emotion_avoids_spin_pose_extremes_and_rotation_velocity(self):
        hardware_h = (FIRMWARE / "robot_hardware.h").read_text(encoding="utf-8")
        hardware_cpp = (FIRMWARE / "robot_hardware.cpp").read_text(encoding="utf-8")
        emotion_cpp = (FIRMWARE / "emotion_actions.cpp").read_text(encoding="utf-8")
        source = hardware_h + hardware_cpp + emotion_cpp
        for snippet in [
            "prepareYawRotation",
            "pitchWithRestPitchOffset",
            "M5StackChan.Motion.stop()",
            "M5StackChan.Motion.movePitch",
            "if (step.rotateVelocity != 0)",
            "hardware.prepareYawRotation()",
            "hardware.pitchWithRestPitchOffset(step.pitch, step.speed)",
            "hardware.rotateX(step.rotateVelocity)",
            "hardware.poseWithRestPitchOffset(step.yaw, step.pitch, step.speed)",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)
        rotating_branch = re.search(
            r"if \(step\.rotateVelocity != 0\) \{(?P<body>[\s\S]*?)\} else \{",
            emotion_cpp,
        )
        self.assertIsNotNone(rotating_branch)
        self.assertNotIn("poseWithRestPitchOffset", rotating_branch.group("body"))
        nod_block = re.search(
            r"const EmotionStep EMOTION_HEAD_TOUCH_COMBO_NOD\[\] = \{(?P<body>[\s\S]*?)\};",
            emotion_cpp,
        )
        self.assertIsNotNone(nod_block)
        body = nod_block.group("body")
        self.assertNotIn("rotateVelocity", body)
        self.assertNotIn("YAW_MIN", body)
        self.assertNotIn("YAW_MAX", body)

    def test_console_exposes_trigger_condition_panel(self):
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")
        for snippet in [
            "触发条件",
            "摸头触发舒服点点头",
            'id="triggerHeadTouchHappy"',
            'id="triggerHeadTouchCombo"',
            "triggerCondition",
            "triggerCooldown",
            "triggerLast",
            "setTriggers",
            "refreshTriggers",
            "/api/triggers/status",
            "/api/triggers/set",
            'id="triggerLog"',
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, console)

    def test_all_face_moods_share_blink_animation(self):
        face_h = (FIRMWARE / "face_renderer.h").read_text(encoding="utf-8")
        face_cpp = (FIRMWARE / "face_renderer.cpp").read_text(encoding="utf-8")
        source = face_h + face_cpp
        for snippet in [
            "blinkHeightScale",
            "drawBlinkEye",
            "BLINK_INTERVAL_MS",
            "BLINK_CLOSED_SCALE",
            "const float blinkScale = blinkHeightScale(nowMs)",
            "int blinkH = max(3, (int)(baseH * blinkScale))",
            "drawOneEye(canvas, true, leftX, y, baseW, blinkH, nowMs, blinkScale)",
            "drawOneEye(canvas, false, rightX, y, baseW, blinkH, nowMs, blinkScale)",
            "if (blinkScale <= BLINK_CLOSED_SCALE)",
            "drawBlinkEye(canvas, cx, cy, w + 18, color)",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)
        self.assertRegex(face_cpp, r"drawOneEye[\s\S]*if \(blinkScale <= BLINK_CLOSED_SCALE\)[\s\S]*switch \(mood_\)")

    def test_idle_behavior_defaults_to_neutral_face_with_thoughtful_overrides(self):
        idle_cpp = (FIRMWARE / "idle_behavior.cpp").read_text(encoding="utf-8")
        self.assertIn('return "neutral"', idle_cpp)
        self.assertIn('return "thinking"', idle_cpp)
        self.assertRegex(idle_cpp, r"idleMoodForStep[\s\S]*return \"neutral\"")
        self.assertRegex(idle_cpp, r"IdleBehaviorController::applyStep[\s\S]*face\.setMood\(idleMoodForStep\(step\)\)[\s\S]*face\.setReact")

    def test_idle_behavior_keeps_richer_library_without_random_face_toggle(self):
        idle_h = (FIRMWARE / "idle_behavior.h").read_text(encoding="utf-8")
        idle_cpp = (FIRMWARE / "idle_behavior.cpp").read_text(encoding="utf-8")
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")
        source = idle_h + idle_cpp + console
        for snippet in [
            "BEHAVIOR_MICRO_NOD",
            "BEHAVIOR_SHY_PEEK",
            "BEHAVIOR_SLEEPY_SETTLE",
            "轻点头",
            "偷看回中",
            "困意回落",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)
        for removed in [
            "setRandomMoodEnabled",
            "randomMoodEnabled_",
            "chooseIdleMood",
            "IDLE_RANDOM_MOODS",
            'id="idleRandomMood"',
            "randomMoodEnabled:bool('idleRandomMood')",
        ]:
            with self.subTest(removed=removed):
                self.assertNotIn(removed, source)

    def test_idle_behavior_adds_emo_tired_sleep_and_touch_wakeability(self):
        idle_h = (FIRMWARE / "idle_behavior.h").read_text(encoding="utf-8")
        idle_cpp = (FIRMWARE / "idle_behavior.cpp").read_text(encoding="utf-8")
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")
        source = idle_h + idle_cpp + console
        for snippet in [
            "BEHAVIOR_EMO_SAD",
            "BEHAVIOR_TIRED_DRIFT",
            "BEHAVIOR_DOZE_SLEEP",
            "emo低头",
            "疲惫回落",
            "打瞌睡",
            "currentStepWakeableByTouch() const",
            "currentStepWakeableByTouch_",
            "idleWakeableForStep(step)",
            '\\"wakeableByTouch\\":',
            'return "sad"',
            'return "tired"',
            'return "sleeping"',
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)

    def test_random_emotion_action_trigger_is_wired(self):
        trigger_h = (FIRMWARE / "emotion_triggers.h").read_text(encoding="utf-8")
        trigger_cpp = (FIRMWARE / "emotion_triggers.cpp").read_text(encoding="utf-8")
        api = (FIRMWARE / "api.cpp").read_text(encoding="utf-8")
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")
        source = trigger_h + trigger_cpp + api + console
        for snippet in [
            "setRandomEmotionEnabled",
            "setRandomEmotionIntervalRangeMs",
            "bool randomEmotionEnabled_ = false;",
            "randomEmotionEnabled_ = false",
            "RANDOM_EMOTION_ACTIONS",
            "chooseRandomEmotionAction",
            "chooseNextRandomEmotionDelayMs",
            "nextRandomEmotionAtMs_",
            "nextRandomEmotionAtMs_ = 0",
            "randomEmotionMinIntervalMs_",
            "randomEmotionMaxIntervalMs_",
            "emotion.play(actionName, nowMs, true)",
            'doc["randomEmotionEnabled"].is<bool>()',
            'doc["randomEmotionMinIntervalMs"].is<int>()',
            'doc["randomEmotionMaxIntervalMs"].is<int>()',
            '\\"randomEmotionEnabled\\":',
            '\\"randomEmotionMinIntervalMs\\":',
            '\\"randomEmotionMaxIntervalMs\\":',
            '\\"lastRandomEmotionDelayMs\\":',
            '\\"nextRandomEmotionInMs\\":',
            'id="randomEmotionAction"',
            'id="randomEmotionMinInterval"',
            'id="randomEmotionMaxInterval"',
            "随机情绪动作",
            "randomEmotionEnabled:bool('randomEmotionAction')",
            "randomEmotionMinIntervalMs:+$('randomEmotionMinInterval').value",
            "randomEmotionMaxIntervalMs:+$('randomEmotionMaxInterval').value",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)

    def test_proximity_sensor_hardware_interface_is_wired(self):
        hardware_h = (FIRMWARE / "robot_hardware.h").read_text(encoding="utf-8")
        hardware_cpp = (FIRMWARE / "robot_hardware.cpp").read_text(encoding="utf-8")
        source = hardware_h + hardware_cpp
        for snippet in [
            "struct ProximityState",
            "beginProximitySensor",
            "readProximityState",
            "proximityJson",
            "LTR553_I2C_ADDRESS = 0x23",
            "LTR553_REG_PS_DATA_0 = 0x8D",
            "LTR553_REG_ALS_DATA_CH1_0 = 0x88",
            "M5.In_I2C.scanID(LTR553_I2C_ADDRESS",
            "M5.In_I2C.writeRegister8",
            "M5.In_I2C.readRegister",
            "proximitySensorAvailable_",
            "proximitySensorEnabled_",
            "proximityRaw",
            "ambientRaw",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)

    def test_proximity_tracker_is_status_only_debug_reader(self):
        self.assertTrue((FIRMWARE / "proximity_tracker.h").exists())
        self.assertTrue((FIRMWARE / "proximity_tracker.cpp").exists())
        tracker_h = (FIRMWARE / "proximity_tracker.h").read_text(encoding="utf-8")
        tracker_cpp = (FIRMWARE / "proximity_tracker.cpp").read_text(encoding="utf-8")
        firmware = (FIRMWARE / "firmware.ino").read_text(encoding="utf-8")
        source = tracker_h + tracker_cpp + firmware
        for snippet in [
            "class ProximityTrackerController",
            "void update(RobotHardware& hardware, uint32_t nowMs)",
            "lastState_ = hardware.readProximityState()",
            "proximityTracker.update(hardware, nowMs)",
            "if (!voiceQuietMode && !emotionActions.active() && !proximityTracker.active() && !cameraAttention.active())",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)
        for snippet in [
            "setEnabled",
            "choosePeekYaw",
            "applyPeekPose",
            "active_ = true",
            "poseWithRestPitchOffset",
        ]:
            with self.subTest(snippet=snippet):
                self.assertNotIn(snippet, tracker_h + tracker_cpp)

    def test_local_face_detector_contract_is_wired(self):
        self.assertTrue((FIRMWARE / "local_face_detector.h").exists())
        self.assertTrue((FIRMWARE / "local_face_detector.cpp").exists())
        detector_h = (FIRMWARE / "local_face_detector.h").read_text(encoding="utf-8")
        detector_cpp = (FIRMWARE / "local_face_detector.cpp").read_text(encoding="utf-8")
        source = detector_h + detector_cpp
        for snippet in [
            "struct LocalFaceRequest",
            "struct LocalFaceResult",
            "class LocalFaceDetector",
            "begin()",
            "submitCapture(",
            "takeResult(",
            "injectResult(",
            "cancel(",
            "streamPreviewBmp(",
            "uint8_t r = pixel & 0xf8",
            "uint8_t g = ((pixel & 0x0007) << 5) | ((pixel & 0xe000) >> 11)",
            "uint8_t b = (pixel & 0x1f00) >> 5",
            "statusJson() const",
            "snapshotId",
            "faceCenterX",
            "faceCenterY",
            "cameraAvailable",
            "detecting",
            "lastDetectDurationMs",
            "ROBOT_LOCAL_FACE_DETECTION_ENABLED",
            "__has_include",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)

    def test_local_face_detector_reports_uninitialized_camera_as_unavailable(self):
        detector_cpp = (FIRMWARE / "local_face_detector.cpp").read_text(encoding="utf-8")
        config = (FIRMWARE / "config.example.h").read_text(encoding="utf-8")
        for snippet in [
            'error = "camera auto init disabled"',
            'cameraAvailable_ = false',
            'lastError_ = "camera driver unavailable"',
            "config.grab_mode = CAMERA_GRAB_WHEN_EMPTY",
            "config.fb_count = 1",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, detector_cpp)
        for snippet in [
            "#define ROBOT_LOCAL_FACE_DETECTION_TASK_STACK 32768",
            "#define ROBOT_LOCAL_FACE_DETECTION_TASK_CORE 1",
            "#define ROBOT_LOCAL_FACE_CAMERA_AUTO_INIT 1",
            "#define ROBOT_LOCAL_FACE_CAMERA_PIN_D0 39",
            "#define ROBOT_LOCAL_FACE_CAMERA_PIN_D1 40",
            "#define ROBOT_LOCAL_FACE_CAMERA_PIN_D2 41",
            "#define ROBOT_LOCAL_FACE_CAMERA_PIN_D3 42",
            "#define ROBOT_LOCAL_FACE_CAMERA_PIN_D4 15",
            "#define ROBOT_LOCAL_FACE_CAMERA_PIN_D5 16",
            "#define ROBOT_LOCAL_FACE_CAMERA_PIN_D6 48",
            "#define ROBOT_LOCAL_FACE_CAMERA_PIN_D7 47",
            "#define ROBOT_LOCAL_FACE_CAMERA_PIN_XCLK -1",
            "#define ROBOT_LOCAL_FACE_CAMERA_PIN_PCLK 45",
            "#define ROBOT_LOCAL_FACE_CAMERA_PIN_VSYNC 46",
            "#define ROBOT_LOCAL_FACE_CAMERA_PIN_HREF 38",
            "#define ROBOT_LOCAL_FACE_CAMERA_PIN_SIOD 12",
            "#define ROBOT_LOCAL_FACE_CAMERA_PIN_SIOC 11",
            "#define ROBOT_LOCAL_FACE_CAMERA_PIN_PWDN -1",
            "#define ROBOT_LOCAL_FACE_CAMERA_PIN_RESET -1",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, config)

    def test_local_face_detector_starts_after_stackchan_hardware_ready(self):
        ino = (FIRMWARE / "firmware.ino").read_text(encoding="utf-8")
        detector_h = (FIRMWARE / "local_face_detector.h").read_text(encoding="utf-8")
        detector_cpp = (FIRMWARE / "local_face_detector.cpp").read_text(encoding="utf-8")
        for snippet in [
            "bool initialized() const",
            "initialized_ = true",
            "taskRunning_ = true",
            "xTaskCreatePinnedToCore",
            "ROBOT_LOCAL_FACE_DETECTION_TASK_STACK",
            '\\"taskRunning\\"',
            '\\"initialized\\"',
            "if (hardware.isReady() && !localFaceDetector.initialized())",
            "localFaceDetector.begin()",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, detector_h + detector_cpp + ino)
        setup_match = re.search(r"void setup\(\) \{(?P<body>.*?)\n\}", ino, re.S)
        self.assertIsNotNone(setup_match)
        self.assertNotIn("localFaceDetector.begin()", setup_match.group("body"))

    def test_camera_attention_uses_local_snapshot_confirmation_not_scripted_scans(self):
        camera_h = (FIRMWARE / "camera_attention.h").read_text(encoding="utf-8")
        camera_cpp = (FIRMWARE / "camera_attention.cpp").read_text(encoding="utf-8")
        firmware = (FIRMWARE / "firmware.ino").read_text(encoding="utf-8")
        hardware_h = (FIRMWARE / "robot_hardware.h").read_text(encoding="utf-8")
        source = camera_h + camera_cpp + firmware + hardware_h
        for snippet in [
            '#include "local_face_detector.h"',
            "struct FaceSnapshot",
            "enum class CameraAttentionMode",
            "captureCurrentPose",
            "returnToSnapshot",
            "startConfirmCapture",
            "enterFaceLock",
            "applyFaceCentering",
            "showFaceFeedbackSubtitle",
            "clearFaceFeedbackSubtitle",
            "showCameraFeedbackSubtitle",
            "SubtitleState subtitle = face.subtitle();",
            'face.setSubtitle("Halo?", true, subtitle.color)',
            "FACE_FEEDBACK_SUBTITLE_MS = 2500",
            "CAMERA_CAPTURE_FEEDBACK_SUBTITLE_MS = 900",
            "CAMERA_NO_FACE_FEEDBACK_SUBTITLE_MS = 1100",
            'showCameraFeedbackSubtitle(face, nowMs, "Looking..."',
            'showCameraFeedbackSubtitle(face, nowMs, "No face"',
            "playCatchphrase",
            "startListeningAfterCatchphrase",
            "if (mode_ == CameraAttentionMode::Catchphrase) {",
            "active_ = true;",
            "catchphraseAssetId_",
            "listenAfterCatchphrase_",
            "setFeedbackSubtitleEnabled",
            "feedbackSubtitleEnabled_ = false",
            "faceFeedbackSubtitleActive_",
            "cameraFeedbackSubtitleText_",
            "snapshotId_",
            "currentMotion() const",
            "LocalFaceDetector localFaceDetector",
            "localFaceDetector.begin()",
            "cameraAttention.update(hardware, faceRenderer, idleBehavior, robotClock, mumble, localFaceDetector",
            "bool voiceBusy = false",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)
        for forbidden in [
            "CAMERA_SCAN_POINTS",
            "chooseNextScanPoint",
            "struct CameraScanPoint",
            "左上近",
            "右上远",
            "左下近",
            "右下远",
            "FACE_FEEDBACK_SUBTITLE_COLOR",
            '"看一下..."',
            '"没看到脸"',
        ]:
            with self.subTest(forbidden=forbidden):
                self.assertNotIn(forbidden, camera_h + camera_cpp)

    def test_local_face_presence_api_and_console_are_exposed(self):
        api_h = (FIRMWARE / "api.h").read_text(encoding="utf-8")
        api = (FIRMWARE / "api.cpp").read_text(encoding="utf-8")
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")
        camera_cpp = (FIRMWARE / "camera_attention.cpp").read_text(encoding="utf-8")
        detector_cpp = (FIRMWARE / "local_face_detector.cpp").read_text(encoding="utf-8")
        source = api_h + api + console + camera_cpp + detector_cpp
        for snippet in [
            '#include "local_face_detector.h"',
            "LocalFaceDetector& faceDetector",
            'server.on("/api/camera/status"',
            'server.on("/api/camera/set"',
            'server.on("/api/camera/catchphrase"',
            'server.on("/api/camera/face-report"',
            'server.on("/api/camera/wake"',
            'server.on("/api/camera/clear"',
            'server.on("/api/camera/frame.bmp"',
            "faceDetector.streamPreviewBmp",
            "server.sendHeader(\"Content-Type\", \"image/bmp\")",
            'doc["captureIntervalMs"].is<int>()',
            'doc["detectTimeoutMs"].is<int>()',
            'doc["returnSettleMs"].is<int>()',
            'doc["lockHoldMs"].is<int>()',
            'doc["catchphraseAssetId"].is<const char*>()',
            'doc["listenAfterCatchphrase"].is<bool>()',
            'doc["feedbackSubtitleEnabled"].is<bool>()',
            'doc["force"].is<bool>()',
            "report.force",
            '\\"snapshotId\\":',
            '\\"detecting\\":',
            '\\"confirming\\":',
            '\\"reason\\":',
            '\\"faceCenterX\\":',
            '\\"targetYaw\\":',
            '\\"catchphraseAssetId\\":',
            '\\"listenAfterCatchphrase\\":',
            '\\"feedbackSubtitleEnabled\\":',
            '\\"lastFrameWidth\\":',
            '\\"lastFrameHeight\\":',
            '\\"lastConfidence\\":',
            'id="cameraCaptureInterval"',
            'id="cameraDetectTimeout"',
            'id="cameraReturnSettle"',
            'id="cameraCatchphraseAsset"',
            'id="cameraListenAfterCatchphrase"',
            'id="cameraFeedbackSubtitleEnabled"',
            'id="cameraPreview"',
            'id="cameraPreviewStatus"',
            'id="cameraReason"',
            'id="cameraDetectorError"',
            'id="cameraDetectorTask"',
            'id="cameraFrameSize"',
            "setCameraCatchphrase",
            "simulateCameraFace",
            "simulateCameraNoFace",
            "force:true",
            "syntheticFaceForced",
            "showFaceFeedbackSubtitle(face, nowMs)",
            "const detector=s.detector||{}",
            "const detectMs=s.lastDetectDurationMs||detector.lastDetectDurationMs",
            "setBoolValue('cameraFeedbackSubtitleEnabled',!!s.feedbackSubtitleEnabled)",
            "feedbackSubtitleEnabled:bool('cameraFeedbackSubtitleEnabled')",
            "refreshCameraPreview",
            "'/api/camera/frame.bmp?ts='",
            "detector.taskRunning?'运行中':'待机'",
            "detector.lastFrameWidth&&detector.lastFrameHeight",
            "s.reason||'-'",
            "detector.lastError||'-'",
            "api('/api/camera/catchphrase'",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)

    def test_forced_host_face_reports_refresh_existing_lock_without_restarting_halo(self):
        camera_cpp = (FIRMWARE / "camera_attention.cpp").read_text(encoding="utf-8")
        for snippet in [
            "bool refreshingForcedFace = report.force && report.faceDetected && (mode_ == CameraAttentionMode::FaceLock || mode_ == CameraAttentionMode::Catchphrase)",
            "if (refreshingForcedFace) {",
            "lastReason_ = \"syntheticFaceRefreshed\"",
            "applyFaceCentering(result, hardware, face, idle, nowMs)",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, camera_cpp)
        refresh_match = re.search(r"if \(refreshingForcedFace\) \{(?P<body>[\s\S]*?)\n    return true;\n  \}", camera_cpp)
        self.assertIsNotNone(refresh_match)
        refresh_block = refresh_match.group("body")
        self.assertNotIn("showFaceFeedbackSubtitle", refresh_block)
        self.assertNotIn("catchphrasePlayed_ = false", refresh_block)
        report_face_match = re.search(r"bool CameraAttentionController::reportFace[\s\S]*?\n\}", camera_cpp)
        self.assertIsNotNone(report_face_match)
        report_face_block = report_face_match.group(0)
        self.assertLess(
            report_face_block.index("if (refreshingForcedFace) {"),
            report_face_block.index("enterFaceLock(result, hardware, face, idle, nowMs)"),
        )

    def test_debug_subtitle_switches_show_listening_and_api_results(self):
        api = (FIRMWARE / "api.cpp").read_text(encoding="utf-8")
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")
        source = api + console
        for snippet in [
            "debugListeningSubtitleEnabled",
            "debugApiSubtitleEnabled",
            'server.on("/api/debug/subtitle/status"',
            'server.on("/api/debug/subtitle/set"',
            'server.on("/api/debug/subtitle/show"',
            'doc["listening"].is<bool>()',
            'doc["apiResult"].is<bool>()',
            "SubtitleState subtitle = face.subtitle();",
            "face.setSubtitle(displayText, true, subtitle.color)",
            'face.setSubtitle("Listening", true, subtitle.color)',
            'showDebugSubtitle(face, "listening", "Listening"',
            'showDebugSubtitle(face, kind, text',
            'id="panel-debug"',
            'id="debugSubtitleListening"',
            'id="debugSubtitleApiResult"',
            "refreshDebugSubtitle",
            "setDebugSubtitle",
            "showDebugListening",
            "showDebugApiSample",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)
        for forbidden in [
            "debugSubtitleColor",
            "debugSubtitleColorHex",
            'id="debugSubtitleColor"',
            "color:$('debugSubtitleColor').value",
            "data-apply==='debugSubtitle'",
        ]:
            with self.subTest(forbidden=forbidden):
                self.assertNotIn(forbidden, source)

    def test_listening_mode_requires_wifi_and_voice_wake_switch(self):
        api = (FIRMWARE / "api.cpp").read_text(encoding="utf-8")
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")
        source = api + console
        for snippet in [
            "voiceWakeEnabled",
            "hostCallbackUrl",
            "listeningActive",
            "listeningWakeRequested",
            "listeningStatusJson",
            "startListeningMode",
            "stopListeningMode",
            "notifyListeningHost",
            "WiFi.status() == WL_CONNECTED",
            "HTTPClient http",
            'server.on("/api/listening/status"',
            'server.on("/api/listening/config"',
            'server.on("/api/listening/start"',
            'server.on("/api/listening/stop"',
            'server.on("/api/listening/wake-test"',
            'doc["voiceWakeEnabled"].is<bool>()',
            'doc["hostCallbackUrl"].is<const char*>()',
            "face.setSubtitle(\"Listening\"",
            "idle.pauseForManualControl(nowMs",
            'id="panel-listening"',
            'id="listeningVoiceWakeEnabled"',
            'id="listeningHostCallbackUrl"',
            "refreshListening",
            "saveListening",
            "startListening",
            "stopListening",
            "wakeTestListening",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)
        for removed in [
            "realtimeVoiceEnabled",
            'doc["realtimeVoiceEnabled"].is<bool>()',
            'id="listeningRealtimeVoiceEnabled"',
        ]:
            with self.subTest(removed=removed):
                self.assertNotIn(removed, source)

    def test_robot_wake_api_notifies_host_realtime_voice_without_console_polling(self):
        api = (FIRMWARE / "api.cpp").read_text(encoding="utf-8")
        for snippet in [
            "startListeningAndNotifyJson",
            'server.on("/api/listening/wake"',
            'server.on("/api/listening/wake-test"',
            'notifyListeningHost(source)',
            'doc["source"].is<const char*>()',
            'camera.listenAfterCatchphrase()',
            'source = doc["source"].is<const char*>() ? String(doc["source"].as<const char*>()) : "camera-wake"',
            '\\"listeningStarted\\"',
            '\\"notified\\"',
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, api)

    def test_power_button_wake_switch_is_wired_to_listening_mode(self):
        api = (FIRMWARE / "api.cpp").read_text(encoding="utf-8")
        api_h = (FIRMWARE / "api.h").read_text(encoding="utf-8")
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")
        ino = (FIRMWARE / "firmware.ino").read_text(encoding="utf-8")
        source = api + api_h + console + ino
        for snippet in [
            "powerButtonWakeEnabled",
            '\\"powerButtonWakeEnabled\\":',
            'doc["powerButtonWakeEnabled"].is<bool>()',
            'id="listeningPowerButtonWakeEnabled"',
            "setBoolValue('listeningPowerButtonWakeEnabled',!!s.powerButtonWakeEnabled)",
            "powerButtonWakeEnabled:bool('listeningPowerButtonWakeEnabled')",
            "'listeningPowerButtonWakeEnabled'",
            "handlePowerButtonWake",
            "M5.BtnPWR.wasClicked()",
            'startListeningAndNotifyJson(face, idle, nowMs, listenTimeoutMs, "power-button", false)',
            '"power-button"',
        ]:
            with self.subTest(snippet=snippet):
                self.assertTrue(snippet in source, snippet)

    def test_power_button_triple_click_opens_wifi_setup_portal(self):
        ino = (FIRMWARE / "firmware.ino").read_text(encoding="utf-8")
        for snippet in [
            "WIFI_SETUP_CLICK_TARGET = 3",
            "WIFI_SETUP_CLICK_WINDOW_MS",
            "POWER_BUTTON_WAKE_DEFER_MS",
            "powerButtonClickCount",
            "handlePowerButtonClick",
            "updatePowerButtonClickWindow",
            'wifiManager.openSetupPortal("power-triple")',
            "wifiManager.closeSetupPortal()",
            "faceRenderer.begin(M5.Display)",
            "drawSetupPortalScreen(true)",
            "handlePowerButtonWake(faceRenderer, idleBehavior, robotClock, nowMs)",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, ino)

    def test_power_button_triple_click_closes_setup_and_immediately_restores_face(self):
        ino = (FIRMWARE / "firmware.ino").read_text(encoding="utf-8")
        self.assertIn("void restoreRobotFaceScreen(uint32_t nowMs)", ino)
        self.assertIn("faceRenderer.update(M5.Display, nowMs + 34)", ino)
        close_block = re.search(
            r"if \(wifiManager\.setupPortalActive\(\)\) \{([\s\S]*?)return;",
            ino,
        )
        self.assertIsNotNone(close_block)
        self.assertIn("wifiManager.closeSetupPortal()", close_block.group(1))
        self.assertIn("restoreRobotFaceScreen(nowMs)", close_block.group(1))

    def test_failed_boot_keeps_the_robot_on_its_normal_face_until_manual_setup(self):
        manager = (FIRMWARE / "wifi_manager.cpp").read_text(encoding="utf-8")
        ino = (FIRMWARE / "firmware.ino").read_text(encoding="utf-8")
        boot_block = re.search(
            r"bool RobotWifiManager::connectBoot[\s\S]*?String RobotWifiManager::statusJsonWithExtra",
            manager,
        )
        self.assertIsNotNone(boot_block)
        self.assertNotIn("openSetupPortal", boot_block.group(0))
        self.assertIn("3 presses: open Wi-Fi setup", ino)
        self.assertIn("Setup hotspot ready", ino)

    def test_network_overlay_stays_hidden_when_station_connected(self):
        ino = (FIRMWARE / "firmware.ino").read_text(encoding="utf-8")
        overlay_match = re.search(r"void drawNetworkOverlay\(uint32_t nowMs\) \{(?P<body>.*?)\n\}", ino, re.S)
        self.assertIsNotNone(overlay_match)
        self.assertIn("if (WiFi.status() == WL_CONNECTED) return;", overlay_match.group("body"))

    def test_wifi_setup_manager_owns_portal_credentials_and_page(self):
        wifi_h = FIRMWARE / "wifi_manager.h"
        wifi_cpp = FIRMWARE / "wifi_manager.cpp"
        self.assertTrue(wifi_h.exists(), "wifi_manager.h should declare the setup Wi-Fi manager")
        self.assertTrue(wifi_cpp.exists(), "wifi_manager.cpp should implement the setup Wi-Fi manager")

        manager = wifi_h.read_text(encoding="utf-8") + wifi_cpp.read_text(encoding="utf-8")
        config = (FIRMWARE / "config.example.h").read_text(encoding="utf-8")
        for snippet in [
            "#include <Preferences.h>",
            'prefs_.begin("robot_wifi", false)',
            "class RobotWifiManager",
            "openSetupPortal",
            "closeSetupPortal",
            "setupPortalActive",
            "setupPageHtml",
            "WiFi.mode(WIFI_AP_STA)",
            "WiFi.softAP(ROBOT_AP_SSID, ROBOT_AP_PASSWORD)",
            "WiFi.softAPdisconnect(true)",
            "Desktop Robot Wi-Fi",
            "SSID entered manually",
            "PASS: 12345678",
            "http://192.168.4.1",
            "savedCredentialsAvailable_",
            "connectJson",
            "forgetJson",
            "closeJson",
            "closeSetupPortal()",
            "Return to standby",
            "closeSetup()",
            "scanJson",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, manager + config)

        self.assertIn('#define ROBOT_AP_SSID "Desktop Robot Setup"', config)
        self.assertIn('#define ROBOT_AP_PASSWORD "12345678"', config)
        self.assertNotIn('\\"password\\":', manager)
        self.assertNotIn('\\"pass\\":', manager)
        self.assertNotIn("scanWifi()", manager)
        self.assertNotIn('id="networkList"', manager)

    def test_wifi_setup_keeps_the_success_response_available(self):
        manager = (FIRMWARE / "wifi_manager.cpp").read_text(encoding="utf-8")
        ino = (FIRMWARE / "firmware.ino").read_text(encoding="utf-8")
        success_block = re.search(
            r"String RobotWifiManager::connectJson[\s\S]*?String RobotWifiManager::forgetJson",
            manager,
        )
        self.assertIsNotNone(success_block)
        self.assertIn("setupPortalRetained", success_block.group(0))
        self.assertNotIn("closeSetupPortal();", success_block.group(0))
        self.assertIn("requestSetupPortalClose", manager)
        self.assertIn("wifiManager.update(millis())", ino)
        self.assertIn("connectionNotice", manager)
        self.assertIn("Connected successfully.", manager)
        self.assertIn("Connection failed:", manager)
        self.assertIn("M5.Display.setTextSize(3)", ino)
        self.assertIn("M5.Display.setTextSize(2)", ino)
        self.assertIn("Version: ", ino)
        self.assertIn("State: ", ino)
        self.assertIn("configuredSsid", ino)
        self.assertIn("NOT CONNECTED", manager)
        self.assertIn("esp_wifi_set_country_code(\"CN\", true)", manager)
        self.assertIn("usingSavedCredentials", manager)

    def test_setup_wifi_routes_and_page_are_registered(self):
        api = (FIRMWARE / "api.cpp").read_text(encoding="utf-8")
        api_h = (FIRMWARE / "api.h").read_text(encoding="utf-8")
        ino = (FIRMWARE / "firmware.ino").read_text(encoding="utf-8")
        source = api + api_h + ino
        for snippet in [
            '#include "wifi_manager.h"',
            "RobotWifiManager wifiManager",
            "RobotWifiManager& wifiManager",
            "wifiManager.begin(ROBOT_WIFI_SSID, ROBOT_WIFI_PASSWORD, ROBOT_MDNS_NAME)",
            "wifiManager.connectBoot()",
            "wifiManager.setupPortalActive() ? wifiManager.setupPageHtml() : webConsoleHtml()",
            'server.on("/setup", HTTP_GET',
            'server.on("/api/setup/status", HTTP_GET',
            'server.on("/api/setup/scan", HTTP_POST',
            'server.on("/api/setup/connect", HTTP_POST',
            'server.on("/api/setup/forget", HTTP_POST',
            'server.on("/api/setup/close", HTTP_POST',
            "wifiManager.statusJson()",
            "wifiManager.scanJson()",
            "wifiManager.connectJson(",
            "wifiManager.forgetJson()",
            "wifiManager.closeJson()",
            "wifiManager.displayRevision()",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)

    def test_old_voice_stream_player_and_routes_are_removed(self):
        source = self.read_all()
        self.assertFalse((FIRMWARE / "voice_stream_player.h").exists())
        self.assertFalse((FIRMWARE / "voice_stream_player.cpp").exists())
        for snippet in [
            "voice_stream_player.h",
            "VoiceStreamPlayer",
            "voiceStream",
            "/api/voice-stream/",
            "X-Stream-Id",
            "X-Sequence",
            "X-Final",
        ]:
            with self.subTest(snippet=snippet):
                self.assertNotIn(snippet, source)

    def test_realtime_voice_nod_controller_is_removed(self):
        source = self.read_all()
        self.assertFalse((FIRMWARE / "voice_nod_controller.h").exists())
        self.assertFalse((FIRMWARE / "voice_nod_controller.cpp").exists())
        for snippet in [
            "voice_nod_controller.h",
            "voice_nod_controller.cpp",
            "VoiceNodController",
            "voiceNod",
            "/api/voice-nod/",
            "VOICE_NOD_",
            "panel-voice-nod",
            "refreshVoiceNod",
            "saveVoiceNod",
            "voiceNodLight",
        ]:
            with self.subTest(snippet=snippet):
                self.assertNotIn(snippet, source)

    def test_robot_mic_voice_gateway_client_is_removed(self):
        self.assertFalse((FIRMWARE / "voice_gateway_client.cpp").exists())
        self.assertFalse((FIRMWARE / "voice_gateway_client.h").exists())
        source = self.read_all()
        for snippet in [
            "voice_gateway_client.h",
            "VoiceGatewayClient",
            "voiceGateway",
            "voiceQuietModeActive",
            "updateVoiceQuietMode",
            "/api/voice-gateway/",
            "VOICE_GATEWAY_",
            "FRAME_MIC_PCM16",
            "FRAME_TTS_PCM16",
        ]:
            with self.subTest(snippet=snippet):
                self.assertNotIn(snippet, source)

    def test_idle_thought_drift_module_is_removed(self):
        self.assertFalse((FIRMWARE / "idle_thought_drift.h").exists())
        self.assertFalse((FIRMWARE / "idle_thought_drift.cpp").exists())
        source = self.read_all()
        for snippet in [
            "IdleThoughtDriftController",
            "idleThoughtDrift",
            '#include "idle_thought_drift.h"',
            'server.on("/api/thought/status"',
            'server.on("/api/thought/set"',
            'server.on("/api/thought/trigger"',
            'id="panel-thought"',
            'href="#panel-thought"',
            "logic.thought_drift.idle",
            "behavior.thought.idle",
        ]:
            with self.subTest(snippet=snippet):
                self.assertNotIn(snippet, source)

    def test_idle_thought_emotion_asset_remains_manual_only(self):
        emotion_cpp = (FIRMWARE / "emotion_actions.cpp").read_text(encoding="utf-8")
        player_h = (FIRMWARE / "mumble_player.h").read_text(encoding="utf-8")
        player_cpp = (FIRMWARE / "mumble_player.cpp").read_text(encoding="utf-8")
        sound_assets = (FIRMWARE / "emotion_sound_assets.cpp").read_text(encoding="utf-8")
        source = emotion_cpp + player_h + player_cpp + sound_assets
        for snippet in [
            "EMOTION_IDLE_THOUGHT",
            "EMOTION_IDLE_THOUGHT_VARIANTS",
            '{"idleThought", "待机思绪", EMOTION_IDLE_THOUGHT_VARIANTS',
            '{"idleThought", "待机嘀咕"}',
            '{"idleThought", "待机嘀咕", "idleThought.raw"',
            "MUMBLE_EMOTION_SLOT_COUNT = 15",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)

    def test_proximity_api_and_console_are_exposed(self):
        api_h = (FIRMWARE / "api.h").read_text(encoding="utf-8")
        api = (FIRMWARE / "api.cpp").read_text(encoding="utf-8")
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")
        firmware = (FIRMWARE / "firmware.ino").read_text(encoding="utf-8")
        tracker_h = (FIRMWARE / "proximity_tracker.h").read_text(encoding="utf-8")
        tracker_cpp = (FIRMWARE / "proximity_tracker.cpp").read_text(encoding="utf-8")
        source = api_h + api + console + firmware + tracker_h + tracker_cpp
        for snippet in [
            '#include "proximity_tracker.h"',
            "ProximityTrackerController proximityTracker",
            "ProximityTrackerController& proximity",
            'server.on("/api/proximity/status"',
            '\\"proximityRaw\\":',
            '\\"ambientRaw\\":',
            'id="panel-proximity"',
            'href="#panel-proximity"',
            'id="proximityAvailable"',
            'id="proximitySensorEnabled"',
            'id="proximityNear"',
            "renderProximity",
            "refreshProximity",
            "api('/api/proximity/status'",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)
        for snippet in [
            'server.on("/api/proximity/set"',
            'server.on("/api/proximity/search"',
            'doc["proximityEnabled"].is<bool>()',
            'id="proximityEnabled"',
            "setProximity",
            "startProximitySearch",
            "应用红外设置",
            "摸头红外找人测试",
            "轻微微动",
            "手部快速摇晃",
            "跟随测试",
        ]:
            with self.subTest(snippet=snippet):
                self.assertNotIn(snippet, source)

    def test_clock_module_subtitle_datetime_and_night_trigger_are_wired(self):
        self.assertTrue((FIRMWARE / "robot_clock.h").exists())
        self.assertTrue((FIRMWARE / "robot_clock.cpp").exists())
        clock_h = (FIRMWARE / "robot_clock.h").read_text(encoding="utf-8")
        clock_cpp = (FIRMWARE / "robot_clock.cpp").read_text(encoding="utf-8")
        config = (FIRMWARE / "config.example.h").read_text(encoding="utf-8")
        api_h = (FIRMWARE / "api.h").read_text(encoding="utf-8")
        api = (FIRMWARE / "api.cpp").read_text(encoding="utf-8")
        firmware = (FIRMWARE / "firmware.ino").read_text(encoding="utf-8")
        hardware_h = (FIRMWARE / "robot_hardware.h").read_text(encoding="utf-8")
        hardware_cpp = (FIRMWARE / "robot_hardware.cpp").read_text(encoding="utf-8")
        source = clock_h + clock_cpp + config + api_h + api + firmware + hardware_h + hardware_cpp
        for snippet in [
            "class RobotClock",
            "RobotClock robotClock",
            "robotClock.begin()",
            "robotClock.update(nowMs)",
            "robot_clock.h",
            "configTime(ROBOT_TIME_GMT_OFFSET_SEC, ROBOT_TIME_DAYLIGHT_OFFSET_SEC, ROBOT_NTP_SERVER_1, ROBOT_NTP_SERVER_2)",
            "getLocalTime(&timeInfo)",
            "ROBOT_TIME_GMT_OFFSET_SEC 28800",
            "ROBOT_NTP_SERVER_1",
            "DEFAULT_NIGHT_START_HOUR = 22",
            "DEFAULT_NIGHT_END_HOUR = 8",
            "DEFAULT_NIGHT_SLEEP_AFTER_MS = 1800000UL",
            "isHourInNightWindow",
            "startHour > endHour",
            "setShowDateTimeSubtitle",
            "showDateTimeSubtitle",
            "bool showDateTimeSubtitle_ = false",
            "localDateTimeText",
            "setNightSettings",
            "markInteraction",
            "wakeFromSleep",
            "shouldSleep",
            "sleeping_",
            '\\"dateTimeText\\":',
            '\\"showDateTimeSubtitle\\":',
            '\\"nightEnabled\\":',
            '\\"nightStartHour\\":',
            '\\"nightEndHour\\":',
            '\\"sleepAfterMs\\":',
            '\\"interactionAgeMs\\":',
            '\\"sleeping\\":',
            'server.on("/api/clock/status"',
            'server.on("/api/clock/set"',
            'doc["showDateTimeSubtitle"].is<bool>()',
            'doc["nightEnabled"].is<bool>()',
            'doc["nightStartHour"].is<int>()',
            'doc["nightEndHour"].is<int>()',
            'doc["sleepAfterMs"].is<int>()',
            "clock.statusJson()",
            "registerApiRoutes(server, faceRenderer, hardware, idleBehavior, emotionActions, emotionTriggers, mumble, robotClock, proximityTracker, cameraAttention, localFaceDetector, motionStream, assistantStatus, wifiManager)",
            "hardware.statusJson(face.snapshot(), clock.statusJson())",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)

    def test_tired_face_mood_and_night_idle_behavior_are_exposed(self):
        face_h = (FIRMWARE / "face_renderer.h").read_text(encoding="utf-8")
        face_cpp = (FIRMWARE / "face_renderer.cpp").read_text(encoding="utf-8")
        idle_h = (FIRMWARE / "idle_behavior.h").read_text(encoding="utf-8")
        idle_cpp = (FIRMWARE / "idle_behavior.cpp").read_text(encoding="utf-8")
        trigger_h = (FIRMWARE / "emotion_triggers.h").read_text(encoding="utf-8")
        trigger_cpp = (FIRMWARE / "emotion_triggers.cpp").read_text(encoding="utf-8")
        api = (FIRMWARE / "api.cpp").read_text(encoding="utf-8")
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")
        firmware = (FIRMWARE / "firmware.ino").read_text(encoding="utf-8")
        source = face_h + face_cpp + idle_h + idle_cpp + trigger_h + trigger_cpp + api + console + firmware
        for snippet in [
            "Tired",
            '"tired"',
            "MOOD_COUNT = 15",
            "moodStyles_[(uint8_t)FaceMood::Tired] = makeDefaultStyle",
            "case FaceMood::Tired",
            "drawTiredEye",
            '<option value="tired">疲惫</option>',
            "tired:'疲惫'",
            '\\"moods\\":[\\"neutral\\",\\"happy\\",\\"angry\\",\\"sad\\",\\"sleeping\\",\\"tired\\"',
            "bool showDateTime = false",
            "setSubtitleDateTimeEnabled",
            "setDateTimeText",
            "dateTimeText_",
            "subtitle_.showDateTime",
            '\\"showDateTime\\":',
            'id="subtitleDateTime"',
            "subtitleDateTime:bool('subtitleDateTime')",
            "setBoolValue('subtitleDateTime'",
            "update(RobotHardware& hardware, RobotFaceRenderer& face, RobotClock& clock, uint32_t nowMs)",
            "applyNightStep",
            "NIGHT_DOZE_INTERVAL_MS",
            "NIGHT_SLEEP_PITCH",
            "clock.isNightNow()",
            "clock.shouldSleep(nowMs)",
            "clock.setSleeping(true",
            "clock.setSleeping(false",
            'face.setMood("tired")',
            'face.setMood("sleeping")',
            "clock.markInteraction(nowMs)",
            "emotionTriggers.update(hardware, emotionActions, idleBehavior, mumble, robotClock, nowMs)",
            "idleBehavior.update(hardware, faceRenderer, robotClock, nowMs)",
            'id="nightTriggerEnabled"',
            'id="nightStartHour"',
            'id="nightEndHour"',
            'id="nightSleepAfterMinutes"',
            "renderClock",
            "setClock",
            "refreshClock",
            "夜间疲惫",
            "睡眠状态",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)

    def test_sunglasses_face_mood_is_exposed_and_draws_shades(self):
        face_h = (FIRMWARE / "face_renderer.h").read_text(encoding="utf-8")
        face_cpp = (FIRMWARE / "face_renderer.cpp").read_text(encoding="utf-8")
        api = (FIRMWARE / "api.cpp").read_text(encoding="utf-8")
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")
        catalog = (FIRMWARE / "asset_logic_catalog.cpp").read_text(encoding="utf-8")
        source = face_h + face_cpp + api + console + catalog
        for snippet in [
            "Sunglasses",
            '"sunglasses"',
            "MOOD_COUNT = 15",
            "moodStyles_[(uint8_t)FaceMood::Sunglasses] = makeDefaultStyle",
            "case FaceMood::Sunglasses",
            "drawSunglassesEye",
            '<option value="sunglasses">墨镜</option>',
            "sunglasses:'墨镜'",
            '\\"moods\\":[\\"neutral\\",\\"happy\\",\\"angry\\",\\"sad\\",\\"sleeping\\",\\"tired\\",\\"surprised\\",\\"love\\",\\"dizzy\\",\\"wink\\",\\"dead\\",\\"thinking\\",\\"curious\\",\\"sunglasses\\",\\"loading\\"',
            "expression.sunglasses",
            "墨镜表情",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)
        sunglasses_match = re.search(r"void RobotFaceRenderer::drawSunglassesEye[\s\S]*?\n\}", face_cpp)
        self.assertIsNotNone(sunglasses_match)
        sunglasses_block = sunglasses_match.group(0)
        for snippet in [
            "const int lensW = max(2, w)",
            "const int lensH = max(2, h)",
            "const int browH = max(6, lensH / 5)",
            "const int outerWing = max(8, lensW / 6)",
            "const int noseDrop = max(8, lensH / 3)",
            "const int topOuterX = left ? lensX - outerWing : lensX + lensW + outerWing",
            "const int lowerOuterX = left ? lensX + max(4, lensW / 9) : lensX + lensW - max(4, lensW / 9)",
            "const uint16_t lensFillColor = dim565(color, 72)",
            "const uint16_t lensShadowColor = dim565(color, 46)",
            "const uint16_t highlightColor = TFT_WHITE",
            "canvas.fillTriangle(topOuterX, lensY + browH, topInnerX, lensY, noseInnerX, lensY + noseDrop, lensFillColor)",
            "canvas.fillTriangle(topOuterX, lensY + browH, noseInnerX, lensY + noseDrop, lowerInnerX, bottomY - browH / 2, lensFillColor)",
            "canvas.fillTriangle(topOuterX, lensY + browH, lowerInnerX, bottomY - browH / 2, lowerOuterX, bottomY, lensFillColor)",
            "canvas.fillTriangle(topOuterX, lensY, topInnerX, lensY, topOuterX, lensY + browH, browColor)",
            "canvas.fillTriangle(topInnerX, lensY, noseInnerX, lensY + noseDrop, topOuterX, lensY + browH, browColor)",
            "canvas.fillCircle(lowerOuterX, bottomY - browH / 2, max(2, browH / 2), lensFillColor)",
            "canvas.fillTriangle(shadowOuterX, shadowY, shadowInnerX, shadowY, lowerInnerX, bottomY - browH / 2, lensShadowColor)",
            "canvas.drawLine(lensX + max(5, lensW / 5)",
            "canvas.fillCircle(topInnerX + (left ? -max(4, lensW / 10) : max(4, lensW / 10))",
            "canvas.fillRoundRect(bridgeX, bridgeY, bridgeW, bridgeH",
        ]:
            with self.subTest(shape_snippet=snippet):
                self.assertIn(snippet, sunglasses_block)
        for forbidden in [
            "const int lensW = constrain(w + 38",
            "const int lensH = constrain(h + 18",
            "const uint16_t lensColor = dim565(color, 7)",
            "canvas.fillRoundRect(lensX, lensY, lensW, lensH, radius, lensFillColor)",
            "templeEndX",
            "drawThickLine(canvas, outerX",
        ]:
            with self.subTest(forbidden=forbidden):
                self.assertNotIn(forbidden, sunglasses_block)

    def test_night_tired_idle_uses_wider_multi_angle_sequence(self):
        idle_cpp = (FIRMWARE / "idle_behavior.cpp").read_text(encoding="utf-8")
        for snippet in [
            "NIGHT_TIRED_IDLE_SEQUENCE",
            "NIGHT_TIRED_SEQUENCE_COUNT",
            "nightStepIndex_ % NIGHT_TIRED_SEQUENCE_COUNT",
            "疲惫左栽头",
            "疲惫右栽头",
            "疲惫低头打盹",
            "疲惫惊跳一下",
            "疲惫惊跳回落",
            "疲惫左右大角度打盹",
            "{-248, NIGHT_TIRED_PITCH + 16",
            "{264, NIGHT_TIRED_PITCH + 18",
            "{-322, NIGHT_TIRED_PITCH + 28",
            "{326, NIGHT_TIRED_PITCH + 30",
            "{18, NIGHT_TIRED_STARTLE_PITCH",
            "nightStepIndex_ % (NIGHT_TIRED_SEQUENCE_COUNT + 3)",
            "face.setMood(\"tired\")",
            "face.setReact(step.lookX, step.lookY, step.ambientLevel)",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, idle_cpp)
        self.assertNotIn("NIGHT_GUARD_PEEK_PITCH", idle_cpp)
        self.assertNotIn("提防", idle_cpp)
        self.assertNotIn("nightStepIndex_ % 7) == 4", idle_cpp)
        self.assertNotIn("bool guardedPeek = guardPhase == 5 || guardPhase == 6", idle_cpp)

    def test_night_sleep_and_tired_head_pitch_are_lowest(self):
        idle_cpp = (FIRMWARE / "idle_behavior.cpp").read_text(encoding="utf-8")
        for snippet in [
            "constexpr int16_t NIGHT_SLEEP_PITCH = PITCH_MIN;",
            "constexpr int16_t NIGHT_TIRED_PITCH = PITCH_MIN + 12;",
            "hardware.pose(0, NIGHT_SLEEP_PITCH, NIGHT_SLEEP_SPEED)",
            "hardware.pose(0, NIGHT_SLEEP_PITCH + 8, NIGHT_SLEEP_SPEED)",
            "const IdleStep& step = NIGHT_TIRED_IDLE_SEQUENCE[sequenceIndex]",
            "hardware.poseWithRestPitchOffset(step.yaw, step.pitch, step.speed)",
            "face.setReact(0.0f, 0.58f, 0.01f)",
            "face.setReact(0.0f, 0.52f, 0.02f)",
            "face.setReact(step.lookX, step.lookY, step.ambientLevel)",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, idle_cpp)

    def test_console_emotion_panel_has_servo_pose_editor(self):
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")
        for snippet in [
            "情绪舵机编辑",
            'id="emotionServoYaw"',
            'id="emotionServoPitch"',
            'id="emotionServoSpeed"',
            "emotionSleepPitch=0",
            "emotionTiredPitch=12",
            "syncEmotionServoControls",
            "applyEmotionServoPose",
            "applyEmotionServoPreset('sleeping'",
            "applyEmotionServoPreset('tired'",
            "api('/api/motion/pose'",
            "api('/api/face/set'",
            "api('/api/face/react'",
            "'emotionServoYaw','emotionServoPitch','emotionServoSpeed'",
            "bindNumberControl('emotionServoYaw')",
            "isEmotionServoControl",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, console)

    def test_console_switches_auto_apply_before_polling_refresh(self):
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")
        for snippet in [
            "isIdleControl",
            "el.id==='idleEnabled'",
            "if(isIdleControl)toggleIdle()",
            "isTriggerControl",
            "['triggerHeadTouchHappy','triggerHeadTouchCombo','triggerScreenTouchAngry','headTouchThreshold','triggerImuMotion','angryShakeAccelDeltaG','angryShakeGyroDps','upsideDownGravityDotThreshold','randomEmotionAction','randomEmotionMinInterval','randomEmotionMaxInterval'].includes(el.id)",
            "if(isTriggerControl)setTriggers()",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, console)

    def test_console_ui_has_grouped_panels_and_trigger_diagnostics(self):
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")
        for snippet in [
            "top-nav",
            "panel-heading",
            "section-kicker",
            "panel-grid",
            'id="panel-triggers"',
            'href="#panel-triggers"',
            "触摸峰值",
            "触发阈值",
            "随机间隔",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, console)

    def test_trigger_condition_panel_uses_compact_layout(self):
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")
        for snippet in [
            '.trigger-panel{padding:10px}',
            '.trigger-layout{display:grid;grid-template-columns:minmax(0,.9fr) minmax(360px,1.1fr);gap:8px 10px;align-items:start}',
            '.trigger-settings{display:grid;grid-template-columns:repeat(auto-fit,minmax(150px,1fr));gap:6px 8px;align-items:end}',
            '.trigger-status{grid-template-columns:repeat(auto-fit,minmax(96px,1fr));gap:5px;margin:0}',
            '.trigger-status .metric{padding:5px 7px;min-height:46px}',
            '<section id="panel-triggers" class="trigger-panel">',
            '<div class="trigger-layout">',
            '<div class="trigger-settings">',
            '<div class="metric-grid trigger-status">',
            '<div class="row actions trigger-actions">',
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, console)

    def test_console_ui_uses_compact_density(self):
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")
        for snippet in [
            "font-size:12px;line-height:1.28",
            "main{max-width:1240px;margin:auto;padding:8px;display:grid;gap:7px}",
            "section{border:1px solid #1e3a47;background:#101820;padding:8px;border-radius:6px",
            ".top-nav{position:sticky;top:0;z-index:4;display:flex;gap:4px;flex-wrap:wrap",
            ".top-nav a{color:#bfefff;text-decoration:none;border:1px solid #2a5364;background:#071016;border-radius:999px;padding:3px 6px;font-size:11px}",
            ".panel-heading{display:flex;align-items:end;justify-content:space-between;gap:8px;margin-bottom:6px}",
            ".grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(160px,1fr));gap:6px 8px;align-items:end}",
            "label{display:grid;gap:3px;font-size:11px",
            "input,select,button{width:100%;min-width:0;background:#071016;color:#e6fbff;border:1px solid #2a5364;border-radius:5px;padding:4px 6px;font:inherit}",
            "input[type=number]{width:58px;text-align:left}",
            ".control-line{display:grid;grid-template-columns:minmax(0,1fr) 58px;gap:5px;align-items:center}",
            ".metric-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(96px,1fr));gap:5px;margin:5px 0}",
            ".metric{border:1px solid #1e3a47;background:#071016;border-radius:5px;padding:5px 6px}",
            ".metric span{font-size:13px;color:#e6fbff}",
            ".emotion-actions button{width:100%;min-height:28px;text-align:left}",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, console)

    def test_console_debug_json_logs_are_collapsed_by_default(self):
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")
        for snippet in [
            ".log-details{margin-top:6px;font-size:11px;color:#a8c7d3}",
            ".log-details summary{cursor:pointer;display:inline-flex;align-items:center;gap:4px;border:1px solid #2a5364;background:#071016;border-radius:999px;padding:3px 6px;color:#bfefff}",
            '<details class="log-details"><summary>调试输出</summary><pre class="mono panel-log" id="cameraLog"></pre></details>',
            '<details class="log-details"><summary>调试输出</summary><pre class="mono panel-log" id="catalogLog"></pre></details>',
            '<details class="log-details"><summary>调试输出</summary><pre class="mono panel-log" id="triggerLog"></pre></details>',
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, console)

    def test_console_exposes_eye_controls(self):
        source = self.read_all()
        for control in [
            "cornerRadius",
            "eyeWidthScale",
            "eyeHeightScale",
            "eyeOffsetX",
            "eyeOffsetY",
            "eyeSpacing",
            "eyeColor",
            "glowEnabled",
            "glowStrength",
            "scanlinesEnabled",
            "scanlineSpacing",
            "scanlineAlpha",
        ]:
            with self.subTest(control=control):
                self.assertIn(control, source)

    def test_face_style_edit_preserves_selected_mood(self):
        api = (FIRMWARE / "api.cpp").read_text(encoding="utf-8")
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")
        face_style_start = api.find('"/api/face/style"')
        self.assertNotEqual(face_style_start, -1)
        next_route = api.find('server.on("', face_style_start + 1)
        handler = api[face_style_start: next_route if next_route != -1 else len(api)]
        self.assertIn('doc["mood"].is<const char*>()', handler)
        self.assertRegex(handler, r'doc\["mood"\]\.is<const char\*>\(\)[\s\S]*FaceStyle style = face\.style\(\);')
        self.assertIn("mood:$('mood').value", console)
        self.assertIn("api('/api/face/library/save',payload)", console)

    def test_face_mood_switch_does_not_leak_override_color_to_other_moods(self):
        api = (FIRMWARE / "api.cpp").read_text(encoding="utf-8")
        face_set_start = api.find('server.on("/api/face/set"')
        self.assertNotEqual(face_set_start, -1)
        next_route = api.find('server.on("', face_set_start + 1)
        handler = api[face_set_start: next_route if next_route != -1 else len(api)]

        self.assertIn("const bool hasEyeColor", handler)
        self.assertIn("const bool hasColor", handler)
        self.assertRegex(handler, r"if \(hasEyeColor \|\| hasColor\) \{[\s\S]*face\.setStyle\(style\);")

    def test_eye_library_replaces_one_fixed_mood_slot_and_is_persistent(self):
        api = (FIRMWARE / "api.cpp").read_text(encoding="utf-8")
        renderer = (FIRMWARE / "face_renderer.cpp").read_text(encoding="utf-8")
        header = (FIRMWARE / "face_renderer.h").read_text(encoding="utf-8")

        for route in [
            "/api/face/library",
            "/api/face/library/save",
            "/api/face/library/delete",
            "/api/face/library/activate",
        ]:
            self.assertIn(route, api)
        self.assertIn("expectedRevision", api)
        self.assertIn("return 409", renderer)
        self.assertIn("MAX_CUSTOM_EYE_LIBRARY_ENTRIES = 15", header)
        self.assertIn('entry.id = "override." + safeMood', renderer)
        self.assertIn("findCustomEyeLibraryEntryForMood(currentMoodName)", renderer)
        self.assertRegex(renderer, r"setCustomEyeShape\([\s\S]*entry\.points, entry\.pointCount")
        self.assertIn("Preferences preferences", renderer)
        self.assertIn('EYE_LIBRARY_NAMESPACE = "robot_face"', renderer)
        self.assertIn("persistEyeLibrary()", renderer)
        self.assertIn("this system expression has no custom eye replacement", renderer)

    def test_custom_eye_sweep_is_optional_previewable_and_override_only(self):
        api = (FIRMWARE / "api.cpp").read_text(encoding="utf-8")
        renderer = (FIRMWARE / "face_renderer.cpp").read_text(encoding="utf-8")
        header = (FIRMWARE / "face_renderer.h").read_text(encoding="utf-8")
        editor = (ROOT / "ios/RobotEvensControl/RobotEvensControl/ExpressionStudioView.swift").read_text(encoding="utf-8")

        self.assertIn('Toggle("扫光特效", isOn: $model.design.sweepEnabled)', editor)
        self.assertGreaterEqual(api.count('doc["sweepEnabled"]'), 6)
        self.assertIn('"sweepEnabled"', api)
        self.assertIn("EYE_SWEEP_STORAGE_PREFIX", renderer)
        self.assertIn("storedEyeEffectName", renderer)
        self.assertIn("decodeStoredEyeEffectName", renderer)
        self.assertIn("drawEyeGlyphSweep", renderer)
        self.assertIn("applyCustomEyeSweep", renderer)
        self.assertIn("customEyePreviewSweepEnabled_", header)
        self.assertIn("customEyeSweepEnabled_", header)
        self.assertNotIn("sweepEnabled", header.split("struct FaceStyle", 1)[1].split("};", 1)[0])

    def test_ios_expression_editor_uses_robot_slots_and_only_approved_eye_catalogs(self):
        editor = (ROOT / "ios/RobotEvensControl/RobotEvensControl/ExpressionStudioView.swift").read_text(encoding="utf-8")
        models = (ROOT / "ios/RobotEvensCore/Sources/RobotEvensCore/CreatorModels.swift").read_text(encoding="utf-8")

        self.assertNotIn('GroupBox("眼形模板")', editor)
        self.assertIn('GroupBox("机器人表情眼形")', editor)
        self.assertIn('GroupBox("新实心眼型库")', editor)
        self.assertIn('GroupBox("符号与图案眼睛库")', editor)
        self.assertIn('GroupBox("眼睛大小与位置")', editor)
        self.assertIn('GroupBox("高级：路径与参考图（可选）")', editor)
        self.assertIn('@State private var showAdvancedPathEditor = false', editor)
        self.assertIn("model.applySolidPreset(preset)", editor)
        self.assertIn("model.applyGlyphPreset(preset)", editor)
        self.assertIn("public enum SolidEyePresetCollection", models)
        self.assertIn("public enum SolidEyePreset", models)
        self.assertNotIn("public enum EyeShapePreset", models)
        self.assertIn("public enum EyeGlyphPreset", models)
        self.assertEqual(models.count('case batch1_01 = "approved.b1.01"'), 1)
        self.assertEqual(models.count('case batch3_16 = "approved.b3.16"'), 1)
        self.assertIn("ApprovedEyePairBitmapCatalog", models)
        self.assertIn("ApprovedEyePairSymbol", editor)
        self.assertIn('Label("删除替换并恢复固件原眼形"', editor)
        self.assertIn("syncEyeLibrary", editor)

    def test_approved_eye_pairs_use_one_cross_platform_left_right_bitmap_catalog(self):
        renderer = (FIRMWARE / "face_renderer.cpp").read_text(encoding="utf-8")
        generated = (FIRMWARE / "approved_eye_pair_bitmaps.h").read_text(encoding="utf-8")
        swift = (
            ROOT
            / "ios/RobotEvensCore/Sources/RobotEvensCore/ApprovedEyePairBitmaps.swift"
        ).read_text(encoding="utf-8")

        self.assertIn('#include "approved_eye_pair_bitmaps.h"', renderer)
        self.assertIn("robot_approved_eye_assets::findIndex", renderer)
        self.assertIn("bitmapAt((uint8_t)approvedIndex, !leftEye)", renderer)
        self.assertIn("kCount = 48", generated)
        self.assertIn("[kCount][2][kBytesPerEye]", generated)
        self.assertIn("approved.b1.01", generated)
        self.assertIn("approved.b3.16", generated)
        self.assertIn("coverageAt", generated)
        self.assertIn("robot_approved_eye_assets::coverageAt", renderer)
        self.assertIn("sprite->width() != assetWidth", renderer)
        self.assertIn("ApprovedEyePairBitmapCatalog", swift)
        self.assertIn("enum Side", swift)
        self.assertIn("public static func coverage", swift)

    def test_symbol_eye_catalog_uses_native_firmware_glyphs_and_persistent_mood_overrides(self):
        api = (FIRMWARE / "api.cpp").read_text(encoding="utf-8")
        renderer = (FIRMWARE / "face_renderer.cpp").read_text(encoding="utf-8")
        header = (FIRMWARE / "face_renderer.h").read_text(encoding="utf-8")
        glyph_assets = (FIRMWARE / "eye_glyph_bitmaps.h").read_text(encoding="utf-8")
        models = (ROOT / "ios/RobotEvensCore/Sources/RobotEvensCore/CreatorModels.swift").read_text(encoding="utf-8")

        glyph_ids = [
            "dollar", "coin", "diamond", "crown",
            "soccer", "basketball", "baseball", "tennis", "football", "dumbbell", "trophy",
            "musicNote", "headphones", "microphone",
            "gamepad", "target", "dice", "lightning",
            "check", "cross", "question", "exclamation", "power", "wifi",
        ]
        for glyph_id in glyph_ids:
            with self.subTest(glyph_id=glyph_id):
                self.assertIn(f'"{glyph_id}"', glyph_assets)
                self.assertIn(f"case {glyph_id}" if glyph_id != "musicNote" else "case musicNote", models)

        for snippet in [
            'kind == "glyph"',
            'doc["glyphId"]',
            "saveEyeGlyphLibraryEntry",
            "setCustomEyeGlyph",
            "startCustomEyeGlyphPreview",
            "drawEyeGlyph",
            "customEyeGlyphId_",
            "customEyePreviewGlyphId_",
            'EYE_GLYPH_STORAGE_PREFIX = "@g:"',
            "storedEyeGlyphName",
            "decodeStoredEyeGlyphName",
            "persisted.pointCount == 0",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, api + renderer + header)

        self.assertIn('(customEyePreviewPointCount_ >= 3 || isValidEyeGlyph(customEyePreviewGlyphId_))', renderer)
        self.assertIn('json += ",\\\"glyphId\\\":\\\""', renderer)
        self.assertIn("entry.pointCount = 0", renderer)
        self.assertIn('#include "eye_glyph_bitmaps.h"', renderer)
        self.assertIn("pushRotateZoomWithAA", renderer)
        self.assertIn("robot_eye_glyph_assets::findIndex", renderer)
        self.assertNotIn('glyphId == "dollar"', renderer)

    def test_custom_eye_shape_is_bounded_and_rendered(self):
        source = self.read_all()
        api = (FIRMWARE / "api.cpp").read_text(encoding="utf-8")
        renderer = (FIRMWARE / "face_renderer.cpp").read_text(encoding="utf-8")

        self.assertIn('server.on("/api/face/shape", HTTP_POST', api)
        self.assertIn("MAX_CUSTOM_EYE_POINTS = 24", source)
        self.assertIn("custom eye shape requires 3-24 points", api)
        self.assertIn("setCustomEyeShape", api)
        self.assertIn("drawCustomEye", renderer)
        self.assertIn("canvas.fillTriangle", renderer)
        self.assertIn('json += ",\\\"customEyeShape\\\":{";', renderer)

    def test_custom_eye_preview_is_temporary_and_preserves_base_face(self):
        source = self.read_all()
        api = (FIRMWARE / "api.cpp").read_text(encoding="utf-8")
        renderer = (FIRMWARE / "face_renderer.cpp").read_text(encoding="utf-8")

        self.assertIn('server.on("/api/face/preview/status", HTTP_GET', api)
        self.assertIn('server.on("/api/face/preview", HTTP_POST', api)
        self.assertIn('server.on("/api/face/preview/stop", HTTP_POST', api)
        self.assertIn("customEyePreviewPoints_", source)
        self.assertIn("drawCustomEyePreview", renderer)
        self.assertIn("startCustomEyePreview", api)
        self.assertIn("stopCustomEyePreview", renderer)
        self.assertIn("1000UL, 15000UL", source)
        self.assertIn('json += ",\\\"preservesBaseFace\\\":true";', renderer)
        self.assertIn("customEyePreviewEnabled_ && !customEyePreviewActive(nowMs)", renderer)

        preview_start = api.find('server.on("/api/face/preview", HTTP_POST')
        preview_end = api.find('server.on("/api/face/preview/stop"', preview_start)
        self.assertNotEqual(preview_start, -1)
        self.assertNotEqual(preview_end, -1)
        handler = api[preview_start:preview_end]
        self.assertNotIn("idle.pauseForManualControl", handler)
        self.assertNotIn("clock.markInteraction", handler)

    def test_custom_eye_curve_supports_per_point_sharp_corners(self):
        source = self.read_all()
        api = (FIRMWARE / "api.cpp").read_text(encoding="utf-8")
        renderer = (FIRMWARE / "face_renderer.cpp").read_text(encoding="utf-8")

        self.assertIn("bool smooth = false;", source)
        self.assertIn("CUSTOM_EYE_CURVE_STEPS = 8", renderer)
        self.assertIn("cubicBezier", renderer)
        self.assertIn("!points[i].smooth && !points[next].smooth", renderer)
        self.assertIn("points[i].smooth", renderer)
        self.assertIn("points[next].smooth", renderer)
        self.assertGreaterEqual(api.count('doc["smoothingEnabled"].is<bool>()'), 2)
        self.assertGreaterEqual(api.count('point["smooth"].is<bool>()'), 2)
        self.assertIn('json += ",\\\"smooth\\\":"', renderer)

    def test_custom_eye_curve_supports_linked_bezier_handles_and_mirroring(self):
        source = self.read_all()
        api = (FIRMWARE / "api.cpp").read_text(encoding="utf-8")
        renderer = (FIRMWARE / "face_renderer.cpp").read_text(encoding="utf-8")

        self.assertIn("bool hasHandles = false;", source)
        for field in ["handleInX", "handleInY", "handleOutX", "handleOutY"]:
            self.assertIn(field, source)
            self.assertIn(f'point["{field}"].is<float>()', api)
        self.assertGreaterEqual(api.count("parseCustomEyeHandles(point, parsed[pointCount])"), 2)
        self.assertIn("points[i].hasHandles", renderer)
        self.assertIn("points[next].hasHandles", renderer)
        self.assertIn("mirrorDirection * points[i].handleOutX", renderer)
        self.assertIn("mirrorDirection * points[next].handleInX", renderer)
        self.assertIn('json += ",\\\"handleOutX\\\":"', renderer)

    def test_custom_eye_preview_uses_supersampled_edges_and_real_blink_path(self):
        source = self.read_all()
        api = (FIRMWARE / "api.cpp").read_text(encoding="utf-8")
        renderer = (FIRMWARE / "face_renderer.cpp").read_text(encoding="utf-8")

        self.assertIn("CUSTOM_EYE_AA_SCALE = 2.0f", renderer)
        self.assertIn("ensureCustomEyeAASprite", renderer)
        self.assertIn("pushRotateZoomWithAA", renderer)
        self.assertIn("drawCustomEyeAntialiased", renderer)
        self.assertIn('doc["blinkEnabled"].is<bool>()', api)
        self.assertIn('doc["blinkNow"].is<bool>()', api)
        self.assertIn("customEyePreviewBlinkHeightScale", renderer)
        self.assertIn('json += ",\\\"blinkEnabled\\\":"', renderer)
        self.assertIn('json += ",\\\"blinkActive\\\":"', renderer)

    def test_face_shape_style_is_saved_per_mood_with_global_visuals_shared(self):
        source = self.read_all()
        self.assertIn("MOOD_COUNT", source)
        self.assertIn("moodStyles_", source)
        self.assertIn("copyGlobalEffects", source)
        self.assertIn("moodStyles_[(uint8_t)mood_] = next", source)
        self.assertIn("style_ = moodStyles_[(uint8_t)mood_]", source)
        for global_field in [
            "eyeColor",
            "glowEnabled",
            "glowStrength",
            "scanlinesEnabled",
            "scanlineSpacing",
            "scanlineAlpha",
        ]:
            with self.subTest(global_field=global_field):
                self.assertRegex(source, rf"moodStyles_\[i\]\.{global_field}\s*=\s*style\.{global_field}")

    def test_all_mood_defaults_match_current_tuned_eye_shapes(self):
        source = self.read_all()
        expected = {
            "Neutral": ("1.28f", "1.19f", 36, 13, -35, 112),
            "Happy": ("0.35f", "1.33f", 53, 0, -51, 105),
            "Angry": ("0.87f", "1.16f", 15, 0, -35, 100),
            "Sad": ("1.0f", "1.0f", 24, 0, -35, 112),
            "Sleeping": ("1.0f", "1.0f", 24, 0, -28, 112),
            "Tired": ("1.19f", "1.56f", 18, 0, -57, 112),
            "Surprised": ("0.58f", "0.35f", 24, 0, -38, 101),
            "Love": ("1.93f", "1.55f", 24, 0, -37, 108),
            "Dizzy": ("1.88f", "1.94f", 24, 0, -25, 112),
            "Wink": ("1.0f", "1.0f", 24, 0, -38, 112),
            "Dead": ("1.08f", "1.0f", 24, 0, -36, 112),
            "Thinking": ("1.0f", "1.0f", 24, 0, -36, 112),
            "Curious": ("0.83f", "0.42f", 30, 0, -46, 90),
            "Loading": ("1.0f", "1.0f", 24, 0, -33, 112),
        }
        for mood, values in expected.items():
            width, height, radius, offset_x, offset_y, spacing = values
            with self.subTest(mood=mood):
                self.assertIn(
                    f"moodStyles_[(uint8_t)FaceMood::{mood}] = makeDefaultStyle({width}, {height}, {radius}, {offset_x}, {offset_y}, {spacing})",
                    source,
                )
        for snippet in [
            "constexpr uint16_t DEFAULT_EYE_COLOR = 0x07EC",
            "constexpr bool DEFAULT_GLOW_ENABLED = false",
            "constexpr uint8_t DEFAULT_GLOW_STRENGTH = 3",
            "constexpr bool DEFAULT_SCANLINES_ENABLED = true",
            "constexpr uint8_t DEFAULT_SCANLINE_SPACING = 5",
            "constexpr uint8_t DEFAULT_SCANLINE_ALPHA = 20",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)

    def test_status_json_includes_full_face_style_for_console_defaults(self):
        hardware_cpp = (FIRMWARE / "robot_hardware.cpp").read_text(encoding="utf-8")
        for snippet in [
            "color565ToHex(face.style.eyeColor)",
            '\\"eyeColor\\"',
            '\\"glowStrength\\"',
            '\\"scanlineSpacing\\"',
            '\\"scanlineAlpha\\"',
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, hardware_cpp)

    def test_default_initialization_sets_every_mood_independently(self):
        face_cpp = (FIRMWARE / "face_renderer.cpp").read_text(encoding="utf-8")
        for mood in [
            "Neutral",
            "Happy",
            "Angry",
            "Sad",
            "Sleeping",
            "Tired",
            "Surprised",
            "Love",
            "Dizzy",
            "Wink",
            "Dead",
            "Thinking",
            "Curious",
            "Loading",
        ]:
            with self.subTest(mood=mood):
                self.assertIn(f"moodStyles_[(uint8_t)FaceMood::{mood}] = makeDefaultStyle", face_cpp)
        self.assertIn("style_ = moodStyles_[(uint8_t)mood_]", face_cpp)

    def test_factory_defaults_match_current_device_settings(self):
        face_h = (FIRMWARE / "face_renderer.h").read_text(encoding="utf-8")
        face_cpp = (FIRMWARE / "face_renderer.cpp").read_text(encoding="utf-8")
        trigger_cpp = (FIRMWARE / "emotion_triggers.cpp").read_text(encoding="utf-8")
        source = face_h + face_cpp + trigger_cpp
        for snippet in [
            "uint16_t eyeColor = 0x07EC",
            "bool glowEnabled = false",
            "bool scanlinesEnabled = true",
            "uint8_t scanlineSpacing = 5",
            "uint8_t scanlineAlpha = 20",
            "FaceMood mood_ = FaceMood::Neutral",
            "moodStyles_[(uint8_t)FaceMood::Neutral] = makeDefaultStyle(1.28f, 1.19f, 36, 13, -35, 112)",
            "moodStyles_[(uint8_t)FaceMood::Angry] = makeDefaultStyle(0.87f, 1.16f, 15, 0, -35, 100)",
            "moodStyles_[(uint8_t)FaceMood::Thinking] = makeDefaultStyle(1.0f, 1.0f, 24, 0, -36, 112)",
            "moodStyles_[(uint8_t)FaceMood::Happy] = makeDefaultStyle(0.35f, 1.33f, 53, 0, -51, 105)",
            "moodStyles_[(uint8_t)FaceMood::Tired] = makeDefaultStyle(1.19f, 1.56f, 18, 0, -57, 112)",
            "moodStyles_[(uint8_t)FaceMood::Curious] = makeDefaultStyle(0.83f, 0.42f, 30, 0, -46, 90)",
            "bool enabled = true",
            'String text = ""',
            "uint16_t color = 0x07EC",
            "bool showDateTime = false",
            "randomEmotionEnabled_ = false",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)

    def test_console_uses_chinese_labels_with_stable_api_values(self):
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")
        for label in [
            "桌面机器人固件控制台",
            "眼睛控制台",
            "动作控制",
            "待机灯光与传感器",
            "应用表情",
            "应用反应",
            "设置待机灯光",
            "刷新状态",
            "平静",
            "开心",
            "生气",
            "难过",
        ]:
            with self.subTest(label=label):
                self.assertIn(label, console)
        self.assertIn('<option value="angry">生气</option>', console)
        self.assertIn('<option value="sad">难过</option>', console)

    def test_console_declares_utf8_for_chinese_text(self):
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")
        api = (FIRMWARE / "api.cpp").read_text(encoding="utf-8")
        self.assertIn('<meta charset="utf-8">', console)
        self.assertIn("text/html; charset=utf-8", api)

    def test_console_keeps_status_compact_by_default(self):
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")
        self.assertIn('id="statusSummary"', console)
        self.assertIn('id="statusDetail"', console)
        self.assertIn("renderStatusSummary", console)
        self.assertIn("<summary>详细状态</summary>", console)
        self.assertNotIn("$('status').textContent=JSON.stringify(s,null,2)", console)

    def test_status_json_exposes_firmware_space_and_system_metrics(self):
        hardware = (FIRMWARE / "robot_hardware.cpp").read_text(encoding="utf-8")
        for snippet in [
            '\\"firmware\\":{',
            '\\"sketchSize\\":',
            "ESP.getSketchSize()",
            '\\"freeSketchSpace\\":',
            "ESP.getFreeSketchSpace()",
            '\\"flashChipSize\\":',
            "ESP.getFlashChipSize()",
            '\\"system\\":{',
            '\\"chipModel\\":',
            "ESP.getChipModel()",
            '\\"sdkVersion\\":',
            "ESP.getSdkVersion()",
            '\\"freeHeap\\":',
            "ESP.getFreeHeap()",
            '\\"heapSize\\":',
            "ESP.getHeapSize()",
            '\\"psramSize\\":',
            "ESP.getPsramSize()",
            '\\"freePsram\\":',
            "ESP.getFreePsram()",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, hardware)

    def test_console_status_summary_shows_firmware_space(self):
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")
        for snippet in [
            "formatBytes",
            "firmwareSpaceText",
            "s.firmware?.sketchSize",
            "s.firmware?.freeSketchSpace",
            "固件",
            "可更新",
            "${firmwareSpaceText(s)}",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, console)

    def test_console_has_swatches_and_safe_mood_switching(self):
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")
        self.assertIn('class="swatch"', console)
        self.assertIn('data-target="eyeColor"', console)
        self.assertIn('data-target="rgbAll"', console)
        self.assertIn("applySwatchColor", console)
        self.assertIn("syncFaceControls", console)
        self.assertIn("selectMood", console)
        self.assertIn("el.id==='mood'?selectMood()", console)
        self.assertIn("sendReact():sendFace()", console)
        self.assertIn("#00ff66", console)
        self.assertIn("#ff405c", console)

    def test_console_exposes_left_and_right_rgb_controls(self):
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")
        for snippet in [
            "待机全部灯光",
            "待机左侧灯光",
            "待机右侧灯光",
            'id="rgbAll"',
            'id="rgbLeft"',
            'id="rgbRight"',
            "setRgb('left')",
            "setRgb('right')",
            "setRgb('all')",
            'data-side="left"',
            'data-side="right"',
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, console)

    def test_console_exposes_rgb_brightness_and_breathing_controls(self):
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")
        for snippet in [
            "亮度",
            "呼吸灯",
            "跑马灯",
            "七彩灯",
            "灯光组合",
            'id="rgbBrightness"',
            'id="rgbBrightnessNum"',
            'id="rgbBreathing"',
            'id="rgbMarquee"',
            'id="rgbRainbow"',
            'id="rgbPreset"',
            "syncRgbControls",
            "setRgbBreathing",
            "setRgbEffects",
            "applyRgbPreset",
            "standby:true",
            "brightness:+$('rgbBrightness').value",
            "breathing:bool('rgbBreathing')",
            "marquee:bool('rgbMarquee')",
            "rainbow:bool('rgbRainbow')",
            "preset:$('rgbPreset').value",
            "if(s.rgb)syncRgbControls(s.rgb)",
            "bindNumberControl('rgbBrightness')",
            "'rgbBrightness','rgbBrightnessNum','rgbBreathing','rgbMarquee','rgbRainbow','rgbPreset'",
            'value="softGreenBreath"',
            'value="rainbowMarquee"',
            'value="rainbowStatic"',
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, console)

    def test_console_realtime_voice_nod_light_panel_is_removed(self):
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")
        for snippet in [
            'href="#panel-voice-nod"',
            '<section id="panel-voice-nod">',
            "语音随行",
            "实时语音点头",
            "说话点头",
            "随行灯光",
            'id="voiceNodEnabled"',
            'id="voiceNodLightEnabled"',
            "syncVoiceNodControls",
            "refreshVoiceNod",
            "saveVoiceNod",
            "api('/api/voice-nod/status'",
            "api('/api/voice-nod/set'",
            "setInterval(()=>refreshVoiceNod()",
            "refreshVoiceNod();",
        ]:
            with self.subTest(snippet=snippet):
                self.assertNotIn(snippet, console)

    def test_console_exposes_touch_audio_mic_and_idle_controls(self):
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")
        for snippet in [
            "传感器测试",
            "读取触摸板",
            "读取 IMU",
            "读取电源",
            "声音与麦克风",
            'id="toneFrequency"',
            'id="toneDuration"',
            'id="speakerVolume"',
            "播放测试音",
            "停止声音",
            "采样麦克风",
            'id="micLevelFill"',
            "待机小行为库",
            "左右扫视",
            "斜上方看",
            "低头思考",
            "向上看",
            "回中停顿",
            "toggleIdle",
            "nextIdle",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, console)

    def test_console_exposes_chat_motion_module_under_idle_controls(self):
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")
        idle_index = console.index("待机小行为库")
        chat_index = console.index("聊天状态")
        idle_log_index = console.index('id="idleLog"')

        self.assertLess(idle_index, chat_index)
        self.assertLess(chat_index, idle_log_index)
        for snippet in [
            'id="chatMotionEnabled"',
            'id="chatMotionStyle"',
            'id="chatMotionState"',
            "播放聊天动作",
            "停止聊天动作",
            "CHAT_MOTION_POSES",
            "CHAT_MOTION_STYLES",
            "playChatMotionOnce",
            "runChatMotionLoop",
            "setChatMotion",
            "api('/api/motion/pose'",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, console)

    def test_chat_motion_console_defaults_to_enabled(self):
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")

        for snippet in [
            '<select id="chatMotionEnabled"><option value="1">持续随机</option><option value="0">关</option></select>',
            '<span id="chatMotionState">等待随机</span>',
            "refreshCatalog(); setChatMotion();",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, console)

    def test_console_separates_test_tone_and_api_voice_volume(self):
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")
        for snippet in [
            "扬声器音量",
            'id="speakerVolume"',
            "API 语音音量",
            'id="apiVoiceVolume"',
            "async function setApiVoiceVolume()",
            "api('/api/mumble/set',{volume:+$('apiVoiceVolume').value})",
            "const isApiVoiceVolumeControl=['apiVoiceVolume','apiVoiceVolumeNum'].includes(el.id);",
            "else if(isApiVoiceVolumeControl)setApiVoiceVolume();",
            "api('/api/audio/tone',{frequency:+$('toneFrequency').value,durationMs:+$('toneDuration').value,volume:+$('speakerVolume').value})",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, console)

    def test_api_voice_volume_defaults_to_current_standard_setting(self):
        mumble_h = (FIRMWARE / "mumble_player.h").read_text(encoding="utf-8")
        mumble_cpp = (FIRMWARE / "mumble_player.cpp").read_text(encoding="utf-8")
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")
        for snippet in [
            "uint8_t apiVoiceVolume_ = 180",
            "apiVoiceVolume_ = 180",
            'id="apiVoiceVolume" type="range" min="0" max="255" step="1" value="180"',
            'id="apiVoiceVolumeNum" type="number" min="0" max="255" step="1" value="180"',
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, mumble_h + mumble_cpp + console)

    def test_api_voice_volume_does_not_control_emotion_sound_slots(self):
        mumble_h = (FIRMWARE / "mumble_player.h").read_text(encoding="utf-8")
        mumble_cpp = (FIRMWARE / "mumble_player.cpp").read_text(encoding="utf-8")
        api = (FIRMWARE / "api.cpp").read_text(encoding="utf-8")
        source = mumble_h + mumble_cpp + api
        for snippet in [
            "void setApiVoiceVolume(uint8_t value)",
            "uint8_t apiVoiceVolume_ = 180",
            "uint8_t emotionSoundVolume_ = 96",
            "apiVoiceVolume_ = 180",
            "emotionSoundVolume_ = 96",
            "void MumblePlayer::setApiVoiceVolume(uint8_t value)",
            "apiVoiceVolume_ = value",
            "json += \",\\\"volume\\\":\" + String(apiVoiceVolume_);",
            "json += \",\\\"apiVoiceVolume\\\":\" + String(apiVoiceVolume_);",
            "json += \",\\\"emotionSoundVolume\\\":\" + String(emotionSoundVolume_);",
            "mumble.setApiVoiceVolume((uint8_t)constrain(doc[\"volume\"].as<int>(), 0, 255))",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)

        segment_block = re.search(r"String MumblePlayer::playSegmentJson[\s\S]*?\n\}", mumble_cpp)
        full_block = re.search(r"String MumblePlayer::playFullFromStartJson[\s\S]*?\n\}", mumble_cpp)
        emotion_block = re.search(r"String MumblePlayer::playBufferFromStartJson[\s\S]*?\n\}", mumble_cpp)
        self.assertIsNotNone(segment_block)
        self.assertIsNotNone(full_block)
        self.assertIsNotNone(emotion_block)
        self.assertIn("M5.Speaker.setVolume(apiVoiceVolume_)", segment_block.group(0))
        self.assertIn("M5.Speaker.setVolume(apiVoiceVolume_)", full_block.group(0))
        self.assertIn("M5.Speaker.setVolume(emotionSoundVolume_)", emotion_block.group(0))
        self.assertNotIn("M5.Speaker.setVolume(apiVoiceVolume_)", emotion_block.group(0))
        self.assertNotIn("uint8_t volume_", source)
        self.assertNotIn("M5.Speaker.setVolume(volume_)", source)

    def test_idle_behavior_library_coordinates_servos_and_eyes(self):
        source = self.read_all()
        for snippet in [
            "IdleBehaviorController",
            "IdleStep",
            "BEHAVIOR_SCAN_LEFT_RIGHT",
            "BEHAVIOR_DIAGONAL_UP",
            "BEHAVIOR_THINK_DOWN",
            "BEHAVIOR_LOOK_UP",
            "BEHAVIOR_CENTER_PAUSE",
            "hardware.poseWithRestPitchOffset(step.yaw",
            "face.setReact(step.lookX",
            "pauseForManualControl",
            "idleBehavior.update(hardware, faceRenderer, robotClock, nowMs)",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)

    def test_idle_behavior_feels_more_lifelike(self):
        idle_cpp = (FIRMWARE / "idle_behavior.cpp").read_text(encoding="utf-8")
        for snippet in [
            "眼睛先看",
            "头部跟随",
            "微回弹",
            "自然停顿",
            "chooseNextBehavior",
            "naturalPauseJitterMs",
            "lastAppliedYaw_",
            "lastAppliedPitch_",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, idle_cpp + (FIRMWARE / "idle_behavior.h").read_text(encoding="utf-8"))
        self.assertNotIn("{-420, 170", idle_cpp)
        self.assertNotIn("{420, 170", idle_cpp)
        self.assertNotIn("{0, 650", idle_cpp)

    def test_idle_behavior_has_more_natural_micro_sequences(self):
        idle_cpp = (FIRMWARE / "idle_behavior.cpp").read_text(encoding="utf-8")
        idle_h = (FIRMWARE / "idle_behavior.h").read_text(encoding="utf-8")
        source = idle_h + idle_cpp
        for snippet in [
            "BEHAVIOR_SOFT_ATTENTION",
            "BEHAVIOR_CURIOUS_SETTLE",
            "视线微扫",
            "头部慢跟",
            "轻微回弹",
            "呼吸停顿",
            "二次确认",
            "ambientLevel",
            "face.setReact(step.lookX, step.lookY, step.ambientLevel)",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)

    def test_idle_behavior_adds_thoughtful_multi_angle_gaze(self):
        idle_cpp = (FIRMWARE / "idle_behavior.cpp").read_text(encoding="utf-8")
        idle_h = (FIRMWARE / "idle_behavior.h").read_text(encoding="utf-8")
        source = idle_h + idle_cpp
        for snippet in [
            "BEHAVIOR_WIDE_ARC_SCAN",
            "BEHAVIOR_THOUGHTFUL_PONDER",
            "BEHAVIOR_LAYERED_ATTENTION",
            "多角度环视",
            "远近切换",
            "眼神先犹豫",
            "斜下观察",
            "抬眼想一下",
            "思考停顿",
            "idleMoodForStep",
            'face.setMood(idleMoodForStep(step))',
            '\\"多角度环视\\",\\"远近切换\\",\\"思考停顿\\"',
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)
        self.assertRegex(idle_cpp, r"BEHAVIOR_THOUGHTFUL_PONDER[\s\S]*0\.46f[\s\S]*0\.38f")
        self.assertRegex(idle_cpp, r"idleMoodForStep[\s\S]*return \"thinking\"")

    def test_idle_behavior_adds_wider_proximity_sweep_for_ir_detection(self):
        idle_cpp = (FIRMWARE / "idle_behavior.cpp").read_text(encoding="utf-8")
        for snippet in [
            "BEHAVIOR_PROXIMITY_SWEEP",
            "红外巡扫",
            "红外巡扫左远",
            "红外巡扫左下",
            "红外巡扫右远",
            "红外巡扫右下",
            "红外巡扫收住",
            "{-360, 272",
            "{-320, 82",
            "{360, 272",
            "{320, 82",
            "\"红外巡扫\", BEHAVIOR_PROXIMITY_SWEEP",
            '\\"红外巡扫\\"',
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, idle_cpp)
        self.assertRegex(idle_cpp, r"BEHAVIOR_SCAN_LEFT_RIGHT[\s\S]*\{-240, 150[\s\S]*\{240, 150")
        self.assertRegex(idle_cpp, r"BEHAVIOR_WIDE_ARC_SCAN[\s\S]*\{-300, 286[\s\S]*\{300, 92")

    def test_emotion_action_library_coordinates_face_eyes_and_servos(self):
        source = self.read_all()
        for snippet in [
            "EmotionActionController",
            "EmotionStep",
            "EMOTION_HAPPY_NOD",
            "EMOTION_SAD_DIAGONAL_DOWN",
            "EMOTION_ANGRY_SHAKE",
            "EMOTION_SURPRISED_RECOIL",
            "EMOTION_THINKING_DIP",
            "face.setMood(String(step.mood))",
            "hardware.poseWithRestPitchOffset(step.yaw",
            "face.setReact(step.lookX",
            "idle.pauseForManualControl",
            "emotionActions.update(hardware, faceRenderer, idleBehavior, nowMs)",
            "if (!voiceQuietMode && !emotionActions.active() && !proximityTracker.active() && !cameraAttention.active())",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)
        self.assertIn("疯狂上下点头", source)
        self.assertIn("斜下方低头", source)
        sad_match = re.search(r"EMOTION_SAD_DIAGONAL_DOWN\[\][\s\S]*?};", source)
        self.assertIsNotNone(sad_match)
        self.assertNotIn("左右摇头", sad_match.group(0))

    def test_console_exposes_emotion_action_controls(self):
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")
        for snippet in [
            "情绪动作",
            "开心点头",
            "伤心低头",
            "斜下方低头",
            "生气摆头",
            "惊讶后仰",
            "思考低头",
            "playEmotion('happy')",
            "playEmotion('sad')",
            "stopEmotion",
            "refreshEmotion",
            'id="emotionLog"',
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, console)

    def test_manual_controls_pause_idle_behavior(self):
        api = (FIRMWARE / "api.cpp").read_text(encoding="utf-8")
        for route in [
            "/api/face/set",
            "/api/face/style",
            "/api/face/react",
            "/api/motion/pose",
            "/api/motion/rest-pitch",
            "/api/motion/rotate",
            "/api/motion/home",
        ]:
            with self.subTest(route=route):
                start = api.find(f'"{route}"')
                self.assertNotEqual(start, -1)
                next_route = api.find('server.on("', start + 1)
                handler = api[start: next_route if next_route != -1 else len(api)]
                self.assertIn("idle.pauseForManualControl", handler)

    def test_rest_pitch_baseline_is_configurable_for_idle_and_emotions(self):
        hardware_h = (FIRMWARE / "robot_hardware.h").read_text(encoding="utf-8")
        hardware_cpp = (FIRMWARE / "robot_hardware.cpp").read_text(encoding="utf-8")
        idle_cpp = (FIRMWARE / "idle_behavior.cpp").read_text(encoding="utf-8")
        emotion_cpp = (FIRMWARE / "emotion_actions.cpp").read_text(encoding="utf-8")
        api = (FIRMWARE / "api.cpp").read_text(encoding="utf-8")
        source = hardware_h + hardware_cpp + api
        for snippet in [
            "DEFAULT_REST_PITCH = 120",
            "restPitch_ = DEFAULT_REST_PITCH",
            "setRestPitch",
            "restPitch() const",
            "restPitchOffset",
            "poseWithRestPitchOffset",
            '\\"restPitch\\":',
            '\\"restPitchDefault\\":',
            'server.on("/api/motion/rest-pitch"',
            'doc["pitch"].is<int>()',
            "hardware.motionJson()",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)
        self.assertIn("hardware.poseWithRestPitchOffset(step.yaw, step.pitch, step.speed)", idle_cpp)
        self.assertIn("hardware.poseWithRestPitchOffset(step.yaw, step.pitch, step.speed)", emotion_cpp)

    def test_console_exposes_rest_pitch_baseline_control(self):
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")
        for snippet in [
            "待机仰角基准",
            'id="restPitch"',
            'id="restPitchNum"',
            "bindNumberControl('restPitch')",
            "setRestPitch",
            "syncMotionControls",
            "api('/api/motion/rest-pitch'",
            "motion.restPitch",
            "'restPitch'",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, console)

    def test_console_has_numeric_inputs_for_precise_motion_controls(self):
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")
        for control in [
            "yaw",
            "pitch",
            "speed",
            "velocity",
        ]:
            with self.subTest(control=control):
                self.assertIn(f'id="{control}Num"', console)
                self.assertIn(f"bindNumberControl('{control}'", console)
                self.assertIn(f"'{control}'", console)

    def test_console_has_numeric_inputs_for_precise_eye_controls(self):
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")
        for control in [
            "eyeWidthScale",
            "eyeHeightScale",
            "cornerRadius",
            "eyeOffsetX",
            "eyeOffsetY",
            "eyeSpacing",
            "lookX",
            "lookY",
        ]:
            with self.subTest(control=control):
                self.assertIn(f'id="{control}Num"', console)
                self.assertIn(f"bindNumberControl('{control}'", console)
        self.assertIn("syncNumberControls", console)

    def test_mumble_has_no_embedded_legacy_audio_asset(self):
        self.assertFalse((FIRMWARE / "bb_mumble_audio.h").exists())
        self.assertFalse((FIRMWARE / "bb_mumble_audio.cpp").exists())
        self.assertFalse((ROOT / "tools" / "generate_bb_mumble_audio.py").exists())
        source = self.read_all()
        self.assertNotIn("BB_MUMBLE_AUDIO", source)
        self.assertNotIn('#include "bb_mumble_audio.h"', source)

    def test_mumble_player_supports_uploaded_raw_pcm_assets(self):
        header = (FIRMWARE / "mumble_player.h").read_text(encoding="utf-8")
        source = (FIRMWARE / "mumble_player.cpp").read_text(encoding="utf-8")
        combined = header + source
        for snippet in [
            "class MumblePlayer",
            "uploadBegin",
            "uploadWrite",
            "uploadEnd",
            "uploadAbort",
            "uploadStatusJson",
            "uploadedAudio_",
            "uploadedAudioLength_",
            "MUMBLE_AUDIO_SAMPLE_RATE = 16000",
            "MUMBLE_MAX_UPLOAD_BYTES = 320000",
            "raw-u8-16k-mono",
            "heap_caps_malloc",
            "hasUploadedAudio",
            "missingAudioJson",
            "mumble audio not uploaded",
            "M5.Speaker.playRaw(uploadedAudio_ + startSample",
            "M5.Speaker.playRaw(uploadedAudio_, uploadedAudioLength_",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, combined)

    def test_mumble_upload_accepts_unknown_start_size_and_caps_capacity(self):
        source = (FIRMWARE / "mumble_player.cpp").read_text(encoding="utf-8")
        for snippet in [
            "size_t capacity = expectedSize == 0 ? MUMBLE_MAX_UPLOAD_BYTES : expectedSize",
            "if (expectedSize > MUMBLE_MAX_UPLOAD_BYTES)",
            "uploadCapacity_ = capacity",
            "emotionUploadCapacity_ = capacity",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)

    def test_mumble_player_supports_random_segments_speed_and_auto(self):
        header = (FIRMWARE / "mumble_player.h").read_text(encoding="utf-8")
        source = (FIRMWARE / "mumble_player.cpp").read_text(encoding="utf-8")
        combined = header + source
        for snippet in [
            "class MumblePlayer",
            "playRandomSegmentJson",
            "playForSubtitleJson",
            "playForEmotionJson",
            "playFullJson",
            "setSpeedPercent",
            "setEnabled",
            "setAutoEnabled",
            "setFullChancePercent",
            "enabled_ = false",
            "segmentDurationMs_",
            "nextAutoAtMs_",
            "disabledJson",
            "countUtf8Characters",
            "M5.Speaker.playRaw",
            "if (!enabled_) return disabledJson(mode)",
            "MUMBLE_AUDIO_SAMPLE_RATE * speedPercent_ / 100",
            '\\"enabled\\":',
            '\\"autoEnabled\\":',
            '\\"hasAudio\\":',
            '\\"speedPercent\\":',
            '\\"segmentDurationMs\\":',
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, combined)

    def test_mumble_api_upload_and_playback_routes_are_wired(self):
        api = (FIRMWARE / "api.cpp").read_text(encoding="utf-8")
        firmware = (FIRMWARE / "firmware.ino").read_text(encoding="utf-8")
        emotion_h = (FIRMWARE / "emotion_actions.h").read_text(encoding="utf-8")
        emotion_cpp = (FIRMWARE / "emotion_actions.cpp").read_text(encoding="utf-8")
        source = api + firmware + emotion_h + emotion_cpp
        for snippet in [
            "#include \"mumble_player.h\"",
            "MumblePlayer mumble",
            "mumble.begin()",
            "mumble.update(nowMs",
            "registerApiRoutes(server, faceRenderer, hardware, idleBehavior, emotionActions, emotionTriggers, mumble, robotClock, proximityTracker, cameraAttention, localFaceDetector, motionStream, assistantStatus, wifiManager)",
            'server.on("/api/mumble/status"',
            'server.on("/api/mumble/set"',
            'server.on("/api/mumble/play"',
            'server.on("/api/mumble/stop"',
            'server.on("/api/mumble/upload", HTTP_POST',
            "HTTPUpload& upload = server.upload()",
            "UPLOAD_FILE_START",
            "UPLOAD_FILE_WRITE",
            "UPLOAD_FILE_END",
            "mumble.uploadBegin(upload.filename, upload.totalSize)",
            "mumble.uploadWrite(upload.buf, upload.currentSize)",
            "mumble.uploadEnd()",
            'doc["enabled"].is<bool>()',
            'doc["speedPercent"].is<int>()',
            'doc["autoEnabled"].is<bool>()',
            'mode == "subtitle"',
            "mumble.playForSubtitleJson(face.subtitle(), nowMs)",
            "mumble.playForEmotionJson(nowMs, emotion.durationMs())",
            "uint32_t durationMs() const",
            '\\"durationMs\\":',
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)

    def test_emotion_sound_slots_are_per_action_and_auto_play_with_emotions(self):
        header = (FIRMWARE / "mumble_player.h").read_text(encoding="utf-8")
        player = (FIRMWARE / "mumble_player.cpp").read_text(encoding="utf-8")
        api = (FIRMWARE / "api.cpp").read_text(encoding="utf-8")
        source = header + player + api
        for snippet in [
            "struct EmotionSoundSlot",
            "MUMBLE_EMOTION_SLOT_COUNT",
            "emotionSlots_",
            "findEmotionSoundSlot",
            "emotionSlotsJson",
            "setEmotionSlotEnabled",
            "uploadEmotionSlotBegin",
            "uploadEmotionSlotWrite",
            "uploadEmotionSlotEnd",
            "playEmotionSlotJson",
            "emotion sound slot not uploaded",
            'server.on("/api/emotion/sound-slots"',
            'server.on("/api/emotion/sound-slot/set"',
            'server.on("/api/emotion/sound-slot/play"',
            'server.on("/api/emotion/sound-slot/upload", HTTP_POST',
            "mumble.playEmotionSlotJson(name, nowMs, emotion.durationMs())",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, source)
        for action in [
            "comfortableNod",
            "happy",
            "headTouchComboSpin",
            "sad",
            "angry",
            "surprised",
            "thinking",
            "idleThought",
            "curious",
            "shy",
            "cute",
            "proud",
            "confused",
            "scared",
            "celebrate",
        ]:
            with self.subTest(action=action):
                self.assertIn(f'{{"{action}",', player)

    def test_emotion_sound_slots_have_built_in_firmware_assets(self):
        header = (FIRMWARE / "emotion_sound_assets.h").read_text(encoding="utf-8")
        assets = (FIRMWARE / "emotion_sound_assets.cpp").read_text(encoding="utf-8")
        player_h = (FIRMWARE / "mumble_player.h").read_text(encoding="utf-8")
        player_cpp = (FIRMWARE / "mumble_player.cpp").read_text(encoding="utf-8")
        combined = header + assets + player_h + player_cpp
        for snippet in [
            "#include \"emotion_sound_assets.h\"",
            "struct EmotionSoundAsset",
            "EMOTION_SOUND_ASSETS",
            "EMOTION_SOUND_ASSET_COUNT",
            "const uint8_t* data",
            "bool ownsAudio",
            "builtinAudio",
            "builtinAudioLength",
            "applyBuiltInEmotionSlotAudio",
            "releaseEmotionSlotAudio",
            "if (slot.ownsAudio && slot.audio != nullptr)",
            "uploadedName = \"builtin:",
            "PROGMEM",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, combined)
        for action in [
            "happy",
            "headTouchComboSpin",
            "sad",
            "angry",
            "surprised",
            "thinking",
            "idleThought",
            "shy",
            "cute",
            "proud",
            "confused",
            "scared",
            "celebrate",
        ]:
            with self.subTest(action=action):
                self.assertIn(f'{{"{action}",', assets)
                self.assertIn(f"const uint8_t EMOTION_SOUND_{action.upper()}[] PROGMEM = {{", assets)

    def test_console_exposes_emotion_sound_slots_upload_toggles_and_preview(self):
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")
        for snippet in [
            "情绪音效槽位",
            'id="emotionSoundSlots"',
            'id="emotionSoundUploadFile"',
            "convertAudioFileToRawU8",
            "renderEmotionSounds",
            "refreshEmotionSounds",
            "setEmotionSound",
            "uploadEmotionSound",
            "previewEmotionSound",
            "api('/api/emotion/sound-slots'",
            "api('/api/emotion/sound-slot/set'",
            "api('/api/emotion/sound-slot/play'",
            "fetch(`/api/emotion/sound-slot/upload?slot=${encodeURIComponent(slot)}`",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, console)

    def test_console_keeps_mumble_controls_out_of_audio_panel(self):
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")
        audio_panel = re.search(r'<section id="panel-audio">(?P<body>.*?)</section>', console, re.S).group("body")
        for snippet in [
            "播放测试音",
            "停止声音",
            "采样麦克风",
            'id="toneFrequency"',
            'id="speakerVolume"',
            'id="micSamples"',
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, audio_panel)
        for snippet in [
            "碎碎念",
            "RAW PCM",
            "上传素材",
            'id="mumble',
            "playMumble",
            "setMumble",
            "uploadMumble",
            "stopMumble",
        ]:
            with self.subTest(snippet=snippet):
                self.assertNotIn(snippet, audio_panel)

    def test_console_layout_prevents_control_overlap(self):
        console = (FIRMWARE / "web_console.cpp").read_text(encoding="utf-8")
        for snippet in [
            "box-sizing:border-box",
            "max-width:1240px",
            "repeat(auto-fit,minmax(160px,1fr))",
            "grid-template-columns:minmax(0,1fr) 58px",
            "input[type=range]{padding:0;width:100%;min-width:0",
            ".field-wide",
            ".actions",
            "@media(max-width:700px)",
        ]:
            with self.subTest(snippet=snippet):
                self.assertIn(snippet, console)

    def test_station_wifi_is_default_control_path(self):
        source = self.read_all()
        example = (FIRMWARE / "config.example.h").read_text(encoding="utf-8")
        self.assertIn("ROBOT_AP_FALLBACK_ENABLED", source)
        self.assertRegex(example, r"ROBOT_AP_FALLBACK_ENABLED\s+0")
        self.assertIn("WiFi.mode(WIFI_STA)", source)
        self.assertNotIn("ROBOT_AP_ALWAYS_ON", source)

    def test_network_diagnostics_are_station_focused(self):
        source = self.read_all()
        self.assertIn("drawNetworkOverlay", source)
        self.assertIn('\\"networkMode\\":', source)
        self.assertIn('\\"apIp\\":', source)
        self.assertIn("setupPortalActive", source)
        self.assertNotIn('\\"password\\":', source)

    def test_wifi_connection_reports_scan_diagnostics(self):
        source = self.read_all()
        self.assertIn("WiFi.scanNetworks", source)
        self.assertIn("lastScanSummary_", source)
        self.assertIn("SSID not found", source)
        self.assertIn("statusName", source)

    def test_open_wifi_uses_passwordless_begin(self):
        source = self.read_all()
        self.assertIn("lastAuth_", source)
        self.assertIn("WIFI_AUTH_OPEN", source)
        self.assertIn("WiFi.begin(ssid.c_str())", source)


if __name__ == "__main__":
    unittest.main()
