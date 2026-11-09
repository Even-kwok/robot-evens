#include "asset_logic_catalog.h"

namespace {

void appendRawEntries(String& json, const char* const entries[], uint8_t count) {
  for (uint8_t i = 0; i < count; ++i) {
    if (i > 0) json += ",";
    json += entries[i];
  }
}

String boolJson(bool value) {
  return value ? "true" : "false";
}

const char* const SOUND_ASSETS[] = {
  "{\"id\":\"sound.happy\",\"category\":\"soundAsset\",\"type\":\"sound\",\"title\":\"开心音效\",\"owner\":\"MumblePlayer\",\"playable\":true,\"effects\":[\"sound\"],\"apiNames\":[\"/api/emotion/sound-slot/play\"],\"sourceFiles\":[\"firmware/mumble_player.cpp\",\"assets/audio/emotion-sound-slots/manifest.json\"]}",
  "{\"id\":\"sound.sad\",\"category\":\"soundAsset\",\"type\":\"sound\",\"title\":\"伤心音效\",\"owner\":\"MumblePlayer\",\"playable\":true,\"effects\":[\"sound\"],\"apiNames\":[\"/api/emotion/sound-slot/play\"],\"sourceFiles\":[\"firmware/mumble_player.cpp\",\"assets/audio/emotion-sound-slots/manifest.json\"]}",
  "{\"id\":\"sound.angry\",\"category\":\"soundAsset\",\"type\":\"sound\",\"title\":\"生气音效\",\"owner\":\"MumblePlayer\",\"playable\":true,\"effects\":[\"sound\"],\"apiNames\":[\"/api/emotion/sound-slot/play\"],\"sourceFiles\":[\"firmware/mumble_player.cpp\",\"assets/audio/emotion-sound-slots/manifest.json\"]}",
  "{\"id\":\"sound.surprised\",\"category\":\"soundAsset\",\"type\":\"sound\",\"title\":\"惊讶音效\",\"owner\":\"MumblePlayer\",\"playable\":true,\"effects\":[\"sound\"],\"apiNames\":[\"/api/emotion/sound-slot/play\"],\"sourceFiles\":[\"firmware/mumble_player.cpp\",\"assets/audio/emotion-sound-slots/manifest.json\"]}",
  "{\"id\":\"sound.thinking\",\"category\":\"soundAsset\",\"type\":\"sound\",\"title\":\"思考音效\",\"owner\":\"MumblePlayer\",\"playable\":true,\"effects\":[\"sound\"],\"apiNames\":[\"/api/emotion/sound-slot/play\"],\"sourceFiles\":[\"firmware/mumble_player.cpp\",\"assets/audio/emotion-sound-slots/manifest.json\"]}",
  "{\"id\":\"sound.comfortable_nod\",\"category\":\"soundAsset\",\"type\":\"sound\",\"title\":\"舒服点点头音效槽\",\"owner\":\"MumblePlayer\",\"playable\":false,\"available\":false,\"effects\":[\"sound\"],\"apiNames\":[\"/api/emotion/sound-slot/play\"],\"sourceFiles\":[\"firmware/mumble_player.cpp\",\"assets/audio/emotion-sound-slots/manifest.json\"]}",
  "{\"id\":\"sound.curious\",\"category\":\"soundAsset\",\"type\":\"sound\",\"title\":\"好奇音效槽\",\"owner\":\"MumblePlayer\",\"playable\":false,\"available\":false,\"effects\":[\"sound\"],\"apiNames\":[\"/api/emotion/sound-slot/play\"],\"sourceFiles\":[\"firmware/mumble_player.cpp\",\"assets/audio/emotion-sound-slots/manifest.json\"]}",
  "{\"id\":\"sound.idle_thought\",\"category\":\"soundAsset\",\"type\":\"sound\",\"title\":\"待机嘀咕音效\",\"owner\":\"MumblePlayer\",\"playable\":true,\"effects\":[\"sound\"],\"apiNames\":[\"/api/emotion/sound-slot/play\",\"/api/mumble/play\"],\"sourceFiles\":[\"firmware/mumble_player.cpp\",\"assets/audio/emotion-sound-slots/manifest.json\"]}",
  "{\"id\":\"sound.head_touch_combo_spin\",\"category\":\"soundAsset\",\"type\":\"sound\",\"title\":\"摸头疯狂点头音效\",\"owner\":\"MumblePlayer\",\"playable\":true,\"effects\":[\"sound\"],\"apiNames\":[\"/api/emotion/sound-slot/play\"],\"sourceFiles\":[\"firmware/mumble_player.cpp\",\"assets/audio/emotion-sound-slots/manifest.json\"]}",
  "{\"id\":\"sound.shy\",\"category\":\"soundAsset\",\"type\":\"sound\",\"title\":\"害羞音效\",\"owner\":\"MumblePlayer\",\"playable\":true,\"effects\":[\"sound\"],\"apiNames\":[\"/api/emotion/sound-slot/play\"],\"sourceFiles\":[\"firmware/mumble_player.cpp\",\"assets/audio/emotion-sound-slots/manifest.json\"]}",
  "{\"id\":\"sound.cute\",\"category\":\"soundAsset\",\"type\":\"sound\",\"title\":\"撒娇音效\",\"owner\":\"MumblePlayer\",\"playable\":true,\"effects\":[\"sound\"],\"apiNames\":[\"/api/emotion/sound-slot/play\"],\"sourceFiles\":[\"firmware/mumble_player.cpp\",\"assets/audio/emotion-sound-slots/manifest.json\"]}",
  "{\"id\":\"sound.proud\",\"category\":\"soundAsset\",\"type\":\"sound\",\"title\":\"得意音效\",\"owner\":\"MumblePlayer\",\"playable\":true,\"effects\":[\"sound\"],\"apiNames\":[\"/api/emotion/sound-slot/play\"],\"sourceFiles\":[\"firmware/mumble_player.cpp\",\"assets/audio/emotion-sound-slots/manifest.json\"]}",
  "{\"id\":\"sound.confused\",\"category\":\"soundAsset\",\"type\":\"sound\",\"title\":\"困惑音效\",\"owner\":\"MumblePlayer\",\"playable\":true,\"effects\":[\"sound\"],\"apiNames\":[\"/api/emotion/sound-slot/play\"],\"sourceFiles\":[\"firmware/mumble_player.cpp\",\"assets/audio/emotion-sound-slots/manifest.json\"]}",
  "{\"id\":\"sound.scared\",\"category\":\"soundAsset\",\"type\":\"sound\",\"title\":\"紧张音效\",\"owner\":\"MumblePlayer\",\"playable\":true,\"effects\":[\"sound\"],\"apiNames\":[\"/api/emotion/sound-slot/play\"],\"sourceFiles\":[\"firmware/mumble_player.cpp\",\"assets/audio/emotion-sound-slots/manifest.json\"]}",
  "{\"id\":\"sound.celebrate\",\"category\":\"soundAsset\",\"type\":\"sound\",\"title\":\"庆祝音效\",\"owner\":\"MumblePlayer\",\"playable\":true,\"effects\":[\"sound\"],\"apiNames\":[\"/api/emotion/sound-slot/play\"],\"sourceFiles\":[\"firmware/mumble_player.cpp\",\"assets/audio/emotion-sound-slots/manifest.json\"]}"
};

const char* const EXPRESSION_ASSETS[] = {
  "{\"id\":\"expression.neutral\",\"category\":\"expressionAsset\",\"type\":\"expression\",\"title\":\"平静表情\",\"owner\":\"RobotFaceRenderer\",\"playable\":true,\"effects\":[\"face\"],\"apiNames\":[\"/api/face/set\"],\"sourceFiles\":[\"firmware/face_renderer.cpp\"]}",
  "{\"id\":\"expression.happy\",\"category\":\"expressionAsset\",\"type\":\"expression\",\"title\":\"开心表情\",\"owner\":\"RobotFaceRenderer\",\"playable\":true,\"effects\":[\"face\"],\"apiNames\":[\"/api/face/set\"],\"sourceFiles\":[\"firmware/face_renderer.cpp\"]}",
  "{\"id\":\"expression.angry\",\"category\":\"expressionAsset\",\"type\":\"expression\",\"title\":\"生气表情\",\"owner\":\"RobotFaceRenderer\",\"playable\":true,\"effects\":[\"face\"],\"apiNames\":[\"/api/face/set\"],\"sourceFiles\":[\"firmware/face_renderer.cpp\"]}",
  "{\"id\":\"expression.sad\",\"category\":\"expressionAsset\",\"type\":\"expression\",\"title\":\"难过表情\",\"owner\":\"RobotFaceRenderer\",\"playable\":true,\"effects\":[\"face\"],\"apiNames\":[\"/api/face/set\"],\"sourceFiles\":[\"firmware/face_renderer.cpp\"]}",
  "{\"id\":\"expression.sleeping\",\"category\":\"expressionAsset\",\"type\":\"expression\",\"title\":\"困倦表情\",\"owner\":\"RobotFaceRenderer\",\"playable\":true,\"effects\":[\"face\"],\"apiNames\":[\"/api/face/set\"],\"sourceFiles\":[\"firmware/face_renderer.cpp\"]}",
  "{\"id\":\"expression.tired\",\"category\":\"expressionAsset\",\"type\":\"expression\",\"title\":\"疲惫表情\",\"owner\":\"RobotFaceRenderer\",\"playable\":true,\"effects\":[\"face\"],\"apiNames\":[\"/api/face/set\"],\"sourceFiles\":[\"firmware/face_renderer.cpp\"]}",
  "{\"id\":\"expression.surprised\",\"category\":\"expressionAsset\",\"type\":\"expression\",\"title\":\"惊讶表情\",\"owner\":\"RobotFaceRenderer\",\"playable\":true,\"effects\":[\"face\"],\"apiNames\":[\"/api/face/set\"],\"sourceFiles\":[\"firmware/face_renderer.cpp\"]}",
  "{\"id\":\"expression.love\",\"category\":\"expressionAsset\",\"type\":\"expression\",\"title\":\"喜爱表情\",\"owner\":\"RobotFaceRenderer\",\"playable\":true,\"effects\":[\"face\"],\"apiNames\":[\"/api/face/set\"],\"sourceFiles\":[\"firmware/face_renderer.cpp\"]}",
  "{\"id\":\"expression.dizzy\",\"category\":\"expressionAsset\",\"type\":\"expression\",\"title\":\"眩晕表情\",\"owner\":\"RobotFaceRenderer\",\"playable\":true,\"effects\":[\"face\"],\"apiNames\":[\"/api/face/set\"],\"sourceFiles\":[\"firmware/face_renderer.cpp\"]}",
  "{\"id\":\"expression.wink\",\"category\":\"expressionAsset\",\"type\":\"expression\",\"title\":\"眨眼表情\",\"owner\":\"RobotFaceRenderer\",\"playable\":true,\"effects\":[\"face\"],\"apiNames\":[\"/api/face/set\"],\"sourceFiles\":[\"firmware/face_renderer.cpp\"]}",
  "{\"id\":\"expression.dead\",\"category\":\"expressionAsset\",\"type\":\"expression\",\"title\":\"无语表情\",\"owner\":\"RobotFaceRenderer\",\"playable\":true,\"effects\":[\"face\"],\"apiNames\":[\"/api/face/set\"],\"sourceFiles\":[\"firmware/face_renderer.cpp\"]}",
  "{\"id\":\"expression.thinking\",\"category\":\"expressionAsset\",\"type\":\"expression\",\"title\":\"思考表情\",\"owner\":\"RobotFaceRenderer\",\"playable\":true,\"effects\":[\"face\"],\"apiNames\":[\"/api/face/set\"],\"sourceFiles\":[\"firmware/face_renderer.cpp\"]}",
  "{\"id\":\"expression.curious\",\"category\":\"expressionAsset\",\"type\":\"expression\",\"title\":\"好奇表情\",\"owner\":\"RobotFaceRenderer\",\"playable\":true,\"effects\":[\"face\"],\"apiNames\":[\"/api/face/set\"],\"sourceFiles\":[\"firmware/face_renderer.cpp\"]}",
  "{\"id\":\"expression.sunglasses\",\"category\":\"expressionAsset\",\"type\":\"expression\",\"title\":\"墨镜表情\",\"owner\":\"RobotFaceRenderer\",\"playable\":true,\"effects\":[\"face\"],\"apiNames\":[\"/api/face/set\"],\"sourceFiles\":[\"firmware/face_renderer.cpp\"]}",
  "{\"id\":\"expression.loading\",\"category\":\"expressionAsset\",\"type\":\"expression\",\"title\":\"加载表情\",\"owner\":\"RobotFaceRenderer\",\"playable\":true,\"effects\":[\"face\"],\"apiNames\":[\"/api/face/set\"],\"sourceFiles\":[\"firmware/face_renderer.cpp\"]}"
};

const char* const SCREEN_ASSETS[] = {
  "{\"id\":\"screen.matrix_rain\",\"category\":\"screenAsset\",\"type\":\"screen\",\"name\":\"matrixRain\",\"title\":\"Matrix 数字雨\",\"owner\":\"RobotFaceRenderer\",\"playable\":true,\"effects\":[\"screen\",\"fullscreen\",\"animation\"],\"apiNames\":[\"/api/screen/set\",\"/api/screen/trigger\"],\"sourceFiles\":[\"firmware/face_renderer.cpp\",\"firmware/web_console.cpp\"]}"
};

const char* const MOTION_ASSETS[] = {
  "{\"id\":\"motion.pose.sleeping_low\",\"category\":\"motionAsset\",\"type\":\"motion\",\"title\":\"睡觉最低姿态\",\"owner\":\"RobotHardware\",\"playable\":true,\"effects\":[\"servo\"],\"apiNames\":[\"/api/motion/pose\"],\"sourceFiles\":[\"firmware/robot_hardware.cpp\",\"firmware/web_console.cpp\"]}",
  "{\"id\":\"motion.pose.tired_low\",\"category\":\"motionAsset\",\"type\":\"motion\",\"title\":\"疲惫略高姿态\",\"owner\":\"RobotHardware\",\"playable\":true,\"effects\":[\"servo\"],\"apiNames\":[\"/api/motion/pose\"],\"sourceFiles\":[\"firmware/robot_hardware.cpp\",\"firmware/web_console.cpp\"]}",
  "{\"id\":\"motion.robot_action.nod_yes\",\"category\":\"motionAsset\",\"type\":\"motion\",\"title\":\"是 / 点头确认动作\",\"owner\":\"EmotionActionController\",\"playable\":true,\"effects\":[\"servo\",\"face\",\"rgb\"],\"apiNames\":[\"/api/emotion/play\"],\"sourceFiles\":[\"firmware/emotion_actions.cpp\"]}",
  "{\"id\":\"motion.robot_action.shake_no\",\"category\":\"motionAsset\",\"type\":\"motion\",\"title\":\"否 / 摇头拒绝动作\",\"owner\":\"EmotionActionController\",\"playable\":true,\"effects\":[\"servo\",\"face\",\"rgb\"],\"apiNames\":[\"/api/emotion/play\"],\"sourceFiles\":[\"firmware/emotion_actions.cpp\"]}",
  "{\"id\":\"motion.emotion.sleep_startle\",\"category\":\"motionAsset\",\"type\":\"motion\",\"title\":\"被惊醒快速动作\",\"owner\":\"EmotionActionController\",\"playable\":true,\"effects\":[\"servo\",\"face\",\"rgb\"],\"apiNames\":[\"/api/emotion/play\"],\"sourceFiles\":[\"firmware/emotion_actions.cpp\"]}"
};

const char* const LIGHT_ASSETS[] = {
  "{\"id\":\"light.rainbow_marquee\",\"category\":\"lightAsset\",\"type\":\"light\",\"title\":\"彩虹跑马灯\",\"owner\":\"EmotionActionController\",\"playable\":false,\"effects\":[\"rgb\"],\"apiNames\":[\"/api/emotion/play\"],\"sourceFiles\":[\"firmware/emotion_actions.cpp\"]}"
};

const char* const BEHAVIOR_ASSETS[] = {
  "{\"id\":\"behavior.robot_action.nod_yes\",\"category\":\"behaviorAsset\",\"type\":\"robotAction\",\"title\":\"是 / 点头确认\",\"owner\":\"EmotionActionController\",\"playable\":true,\"effects\":[\"emotion\",\"servo\",\"face\",\"rgb\"],\"uses\":[\"motion.robot_action.nod_yes\",\"expression.happy\"],\"apiNames\":[\"/api/emotion/play\"],\"sourceFiles\":[\"firmware/emotion_actions.cpp\"]}",
  "{\"id\":\"behavior.robot_action.shake_no\",\"category\":\"behaviorAsset\",\"type\":\"robotAction\",\"title\":\"否 / 摇头拒绝\",\"owner\":\"EmotionActionController\",\"playable\":true,\"effects\":[\"emotion\",\"servo\",\"face\",\"rgb\"],\"uses\":[\"motion.robot_action.shake_no\",\"expression.curious\"],\"apiNames\":[\"/api/emotion/play\"],\"sourceFiles\":[\"firmware/emotion_actions.cpp\"]}",
  "{\"id\":\"behavior.emotion.comfortable_nod\",\"category\":\"behaviorAsset\",\"type\":\"emotion\",\"title\":\"舒服点点头\",\"owner\":\"EmotionActionController\",\"playable\":true,\"effects\":[\"emotion\",\"servo\",\"face\",\"rgb\"],\"uses\":[\"expression.love\"],\"apiNames\":[\"/api/emotion/play\"],\"sourceFiles\":[\"firmware/emotion_actions.cpp\"]}",
  "{\"id\":\"behavior.emotion.happy\",\"category\":\"behaviorAsset\",\"type\":\"emotion\",\"title\":\"开心点头\",\"owner\":\"EmotionActionController\",\"playable\":true,\"effects\":[\"emotion\",\"servo\",\"face\",\"sound\",\"rgb\"],\"uses\":[\"sound.happy\",\"expression.happy\",\"light.rainbow_marquee\"],\"apiNames\":[\"/api/emotion/play\"],\"sourceFiles\":[\"firmware/emotion_actions.cpp\"]}",
  "{\"id\":\"behavior.emotion.happy.nod\",\"category\":\"behaviorAsset\",\"type\":\"emotionVariant\",\"title\":\"开心点头 / 疯狂点头\",\"owner\":\"EmotionActionController\",\"playable\":true,\"effects\":[\"servo\",\"face\",\"sound\",\"rgb\"],\"uses\":[\"behavior.emotion.happy\",\"sound.happy\",\"expression.happy\",\"light.rainbow_marquee\"],\"apiNames\":[\"/api/emotion/play\"],\"sourceFiles\":[\"firmware/emotion_actions.cpp\"]}",
  "{\"id\":\"behavior.emotion.happy.sway\",\"category\":\"behaviorAsset\",\"type\":\"emotionVariant\",\"title\":\"开心点头 / 左右开心晃\",\"owner\":\"EmotionActionController\",\"playable\":true,\"effects\":[\"servo\",\"face\",\"sound\",\"rgb\"],\"uses\":[\"behavior.emotion.happy\",\"sound.happy\",\"expression.happy\",\"light.rainbow_marquee\"],\"apiNames\":[\"/api/emotion/play\"],\"sourceFiles\":[\"firmware/emotion_actions.cpp\"]}",
  "{\"id\":\"behavior.emotion.happy.bounce\",\"category\":\"behaviorAsset\",\"type\":\"emotionVariant\",\"title\":\"开心点头 / 轻快雀跃\",\"owner\":\"EmotionActionController\",\"playable\":true,\"effects\":[\"servo\",\"face\",\"sound\",\"rgb\"],\"uses\":[\"behavior.emotion.happy\",\"sound.happy\",\"expression.happy\",\"light.rainbow_marquee\"],\"apiNames\":[\"/api/emotion/play\"],\"sourceFiles\":[\"firmware/emotion_actions.cpp\"]}",
  "{\"id\":\"behavior.emotion.sad\",\"category\":\"behaviorAsset\",\"type\":\"emotion\",\"title\":\"伤心低头\",\"owner\":\"EmotionActionController\",\"playable\":true,\"effects\":[\"emotion\",\"servo\",\"face\",\"sound\",\"rgb\"],\"uses\":[\"sound.sad\",\"expression.sad\"],\"apiNames\":[\"/api/emotion/play\"],\"sourceFiles\":[\"firmware/emotion_actions.cpp\"]}",
  "{\"id\":\"behavior.emotion.angry\",\"category\":\"behaviorAsset\",\"type\":\"emotion\",\"title\":\"生气摆头\",\"owner\":\"EmotionActionController\",\"playable\":true,\"effects\":[\"emotion\",\"servo\",\"face\",\"sound\",\"rgb\"],\"uses\":[\"sound.angry\",\"expression.angry\"],\"apiNames\":[\"/api/emotion/play\"],\"sourceFiles\":[\"firmware/emotion_actions.cpp\"]}",
  "{\"id\":\"behavior.emotion.surprised\",\"category\":\"behaviorAsset\",\"type\":\"emotion\",\"title\":\"惊讶后仰\",\"owner\":\"EmotionActionController\",\"playable\":true,\"effects\":[\"emotion\",\"servo\",\"face\",\"sound\",\"rgb\"],\"uses\":[\"sound.surprised\",\"expression.surprised\"],\"apiNames\":[\"/api/emotion/play\"],\"sourceFiles\":[\"firmware/emotion_actions.cpp\"]}",
  "{\"id\":\"behavior.emotion.sleep_startle\",\"category\":\"behaviorAsset\",\"type\":\"emotion\",\"title\":\"被惊醒\",\"owner\":\"EmotionActionController\",\"playable\":true,\"effects\":[\"emotion\",\"servo\",\"face\",\"rgb\"],\"uses\":[\"motion.emotion.sleep_startle\",\"expression.surprised\"],\"apiNames\":[\"/api/emotion/play\"],\"sourceFiles\":[\"firmware/emotion_actions.cpp\"]}",
  "{\"id\":\"behavior.emotion.thinking\",\"category\":\"behaviorAsset\",\"type\":\"emotion\",\"title\":\"思考低头\",\"owner\":\"EmotionActionController\",\"playable\":true,\"effects\":[\"emotion\",\"servo\",\"face\",\"sound\",\"rgb\"],\"uses\":[\"sound.thinking\",\"expression.thinking\"],\"apiNames\":[\"/api/emotion/play\"],\"sourceFiles\":[\"firmware/emotion_actions.cpp\"]}",
  "{\"id\":\"behavior.emotion.shy\",\"category\":\"behaviorAsset\",\"type\":\"emotion\",\"title\":\"害羞躲闪\",\"owner\":\"EmotionActionController\",\"playable\":true,\"effects\":[\"emotion\",\"servo\",\"face\",\"sound\",\"rgb\"],\"uses\":[\"sound.shy\",\"expression.happy\"],\"apiNames\":[\"/api/emotion/play\"],\"sourceFiles\":[\"firmware/emotion_actions.cpp\"]}",
  "{\"id\":\"behavior.emotion.cute\",\"category\":\"behaviorAsset\",\"type\":\"emotion\",\"title\":\"撒娇求关注\",\"owner\":\"EmotionActionController\",\"playable\":true,\"effects\":[\"emotion\",\"servo\",\"face\",\"sound\",\"rgb\"],\"uses\":[\"sound.cute\",\"expression.happy\"],\"apiNames\":[\"/api/emotion/play\"],\"sourceFiles\":[\"firmware/emotion_actions.cpp\"]}",
  "{\"id\":\"behavior.emotion.proud\",\"category\":\"behaviorAsset\",\"type\":\"emotion\",\"title\":\"得意抬头\",\"owner\":\"EmotionActionController\",\"playable\":true,\"effects\":[\"emotion\",\"servo\",\"face\",\"sound\",\"rgb\"],\"uses\":[\"sound.proud\",\"expression.happy\"],\"apiNames\":[\"/api/emotion/play\"],\"sourceFiles\":[\"firmware/emotion_actions.cpp\"]}",
  "{\"id\":\"behavior.emotion.confused\",\"category\":\"behaviorAsset\",\"type\":\"emotion\",\"title\":\"困惑歪头\",\"owner\":\"EmotionActionController\",\"playable\":true,\"effects\":[\"emotion\",\"servo\",\"face\",\"sound\",\"rgb\"],\"uses\":[\"sound.confused\",\"expression.thinking\"],\"apiNames\":[\"/api/emotion/play\"],\"sourceFiles\":[\"firmware/emotion_actions.cpp\"]}",
  "{\"id\":\"behavior.emotion.scared\",\"category\":\"behaviorAsset\",\"type\":\"emotion\",\"title\":\"紧张后缩\",\"owner\":\"EmotionActionController\",\"playable\":true,\"effects\":[\"emotion\",\"servo\",\"face\",\"sound\",\"rgb\"],\"uses\":[\"sound.scared\",\"expression.surprised\"],\"apiNames\":[\"/api/emotion/play\"],\"sourceFiles\":[\"firmware/emotion_actions.cpp\"]}",
  "{\"id\":\"behavior.emotion.celebrate\",\"category\":\"behaviorAsset\",\"type\":\"emotion\",\"title\":\"庆祝欢呼\",\"owner\":\"EmotionActionController\",\"playable\":true,\"effects\":[\"emotion\",\"servo\",\"face\",\"sound\",\"rgb\"],\"uses\":[\"sound.celebrate\",\"expression.happy\"],\"apiNames\":[\"/api/emotion/play\"],\"sourceFiles\":[\"firmware/emotion_actions.cpp\"]}",
  "{\"id\":\"behavior.emotion.head_touch_combo_spin\",\"category\":\"behaviorAsset\",\"type\":\"emotion\",\"title\":\"摸头疯狂点头\",\"owner\":\"EmotionActionController\",\"playable\":true,\"effects\":[\"emotion\",\"servo\",\"face\",\"sound\",\"rgb\"],\"uses\":[\"sound.head_touch_combo_spin\",\"expression.happy\"],\"apiNames\":[\"/api/emotion/play\"],\"sourceFiles\":[\"firmware/emotion_actions.cpp\"]}",
  "{\"id\":\"behavior.emotion.idle_thought\",\"category\":\"behaviorAsset\",\"type\":\"emotion\",\"title\":\"待机思绪\",\"owner\":\"EmotionActionController\",\"playable\":true,\"effects\":[\"emotion\",\"servo\",\"face\",\"sound\",\"rgb\"],\"uses\":[\"sound.idle_thought\",\"expression.thinking\"],\"apiNames\":[\"/api/emotion/play\"],\"sourceFiles\":[\"firmware/emotion_actions.cpp\"]}"
};

const char* const DEPRECATED_ASSETS[] = {
  nullptr
};

constexpr uint8_t DEPRECATED_ASSET_COUNT = 0;

String logicEntry(const char* id, const char* type, const char* title, const char* owner, bool enabled, const char* effects, const char* conditions, const char* uses, const char* blockedBy, const char* mayConflictWith, const char* apiNames, const char* sourceFiles) {
  String json = "{\"id\":\"";
  json += id;
  json += "\",\"category\":\"logicAsset\",\"type\":\"";
  json += type;
  json += "\",\"title\":\"";
  json += title;
  json += "\",\"owner\":\"";
  json += owner;
  json += "\",\"enabled\":";
  json += boolJson(enabled);
  json += ",\"playable\":false,\"effects\":";
  json += effects;
  json += ",\"conditions\":";
  json += conditions;
  json += ",\"uses\":";
  json += uses;
  json += ",\"blockedBy\":";
  json += blockedBy;
  json += ",\"mayConflictWith\":";
  json += mayConflictWith;
  json += ",\"apiNames\":";
  json += apiNames;
  json += ",\"sourceFiles\":";
  json += sourceFiles;
  json += "}";
  return json;
}

String logicEntriesJson(const EmotionTriggerController& triggers, const IdleBehaviorController& idle, const RobotClock& clock, const CameraAttentionController& camera) {
  String json = "[";
  json += logicEntry("logic.trigger.head_touch_happy", "trigger", "摸头触发开心", "EmotionTriggerController", triggers.headTouchHappyEnabled(), "[\"emotion\",\"servo\",\"face\",\"sound\",\"rgb\"]", "[\"head touch over threshold\",\"cooldown ready\"]", "[\"behavior.emotion.comfortable_nod\"]", "[\"active emotion action\"]", "[\"logic.idle.normal\",\"logic.camera.scan\"]", "[\"/api/triggers/status\",\"/api/triggers/set\"]", "[\"firmware/emotion_triggers.cpp\",\"firmware/emotion_triggers.h\"]");
  json += ",";
  json += logicEntry("logic.trigger.head_touch_combo", "trigger", "摸头五连/续摸触发疯狂点头", "EmotionTriggerController", triggers.headTouchComboEnabled(), "[\"emotion\",\"servo\",\"face\",\"sound\",\"rgb\"]", "[\"five head touches inside combo window\",\"repeat touch inside 5 second follow-up window\"]", "[\"behavior.emotion.head_touch_combo_spin\"]", "[\"active emotion action\"]", "[\"logic.idle.normal\",\"logic.camera.scan\"]", "[\"/api/triggers/status\",\"/api/triggers/set\"]", "[\"firmware/emotion_triggers.cpp\",\"firmware/emotion_triggers.h\"]");
  json += ",";
  json += logicEntry("logic.trigger.screen_touch_angry", "trigger", "摸屏幕触发生气", "EmotionTriggerController", triggers.screenTouchAngryEnabled(), "[\"emotion\",\"servo\",\"face\",\"sound\",\"rgb\"]", "[\"screen touch pressed\",\"cooldown ready\"]", "[\"behavior.emotion.angry\"]", "[\"active emotion action\"]", "[\"logic.idle.normal\",\"logic.camera.scan\"]", "[\"/api/triggers/status\",\"/api/triggers/set\"]", "[\"firmware/emotion_triggers.cpp\",\"firmware/emotion_triggers.h\"]");
  json += ",";
  json += logicEntry("logic.trigger.imu_motion", "trigger", "IMU 摇晃触发生气", "EmotionTriggerController", triggers.imuMotionEnabled(), "[\"emotion\",\"servo\",\"face\",\"sound\",\"rgb\"]", "[\"IMU enabled\",\"acceleration or gyro threshold matched\"]", "[\"behavior.emotion.angry\"]", "[\"active emotion action\"]", "[\"logic.idle.normal\",\"logic.camera.scan\"]", "[\"/api/triggers/status\",\"/api/triggers/set\"]", "[\"firmware/emotion_triggers.cpp\",\"firmware/emotion_triggers.h\"]");
  json += ",";
  json += logicEntry("logic.trigger.random_emotion", "trigger", "随机情绪动作", "EmotionTriggerController", triggers.randomEmotionEnabled(), "[\"emotion\",\"servo\",\"face\",\"sound\",\"rgb\"]", "[\"random interval elapsed\",\"robot not blocked by higher priority action\"]", "[\"behavior.emotion.*\"]", "[\"active emotion action\",\"manual control\"]", "[\"logic.idle.normal\"]", "[\"/api/triggers/status\",\"/api/triggers/set\"]", "[\"firmware/emotion_triggers.cpp\",\"firmware/emotion_triggers.h\"]");
  json += ",";
  json += logicEntry("logic.idle.normal", "idle", "正常待机行为", "IdleBehaviorController", idle.enabled(), "[\"servo\",\"face\"]", "[\"idle enabled\",\"not paused by manual control\",\"no higher priority owner active\"]", "[\"expression.curious\",\"motion.idle.*\"]", "[\"active emotion action\",\"manual control\",\"active camera attention\"]", "[\"behavior.emotion.*\",\"logic.camera.scan\"]", "[\"/api/idle/status\",\"/api/idle/set\",\"/api/idle/next\"]", "[\"firmware/idle_behavior.cpp\",\"firmware/idle_behavior.h\"]");
  json += ",";
  json += logicEntry("logic.camera.scan", "attention", "本地人脸取证", "CameraAttentionController", camera.enabled(), "[\"servo\",\"face\",\"clock\",\"sound\"]", "[\"camera enabled\",\"current idle pose capture\",\"return to snapshot\",\"confirm face\"]", "[\"expression.curious\",\"local catchphrase\"]", "[\"active emotion action\",\"voice busy\"]", "[\"logic.idle.normal\",\"behavior.emotion.*\"]", "[\"/api/camera/status\",\"/api/camera/set\",\"/api/camera/catchphrase\",\"/api/camera/face-report\"]", "[\"firmware/camera_attention.cpp\",\"firmware/camera_attention.h\",\"firmware/local_face_detector.cpp\",\"firmware/local_face_detector.h\"]");
  json += ",";
  json += logicEntry("logic.camera.voice_wake", "attention", "语音唤醒转向最近人脸", "CameraAttentionController", camera.enabled(), "[\"servo\",\"face\",\"clock\"]", "[\"camera enabled\",\"recent face memory exists\",\"voice wake endpoint called\"]", "[\"expression.curious\"]", "[\"missing recent face memory\"]", "[\"logic.idle.normal\",\"behavior.emotion.*\"]", "[\"/api/camera/wake\",\"/api/camera/clear\"]", "[\"firmware/camera_attention.cpp\",\"firmware/camera_attention.h\"]");
  json += ",";
  json += logicEntry("logic.clock.night_sleep", "clock", "夜间睡眠逻辑", "RobotClock", clock.nightEnabled(), "[\"clock\",\"subtitle\",\"sleep\"]", "[\"night window active\",\"no interaction longer than sleepAfterMs\",\"night awake period ended\"]", "[\"expression.sleeping\",\"logic.idle.normal\"]", "[\"recent interaction\",\"logic.clock.night_touch_awake\"]", "[\"logic.idle.normal\"]", "[\"/api/clock/status\",\"/api/clock/set\"]", "[\"firmware/robot_clock.cpp\",\"firmware/robot_clock.h\"]");
  json += ",";
  json += logicEntry("logic.clock.night_touch_awake", "clock", "夜间触摸清醒期", "RobotClock", clock.nightEnabled(), "[\"clock\",\"servo\",\"face\",\"sleep\"]", "[\"night window active\",\"head touch happy trigger or screen touch angry trigger or sleepStartle touch wake\",\"random 10-20 minute awake period\"]", "[\"logic.idle.normal\",\"logic.trigger.head_touch_happy\",\"logic.trigger.screen_touch_angry\",\"behavior.emotion.sleep_startle\"]", "[\"daytime\",\"night trigger disabled\"]", "[\"logic.clock.night_sleep\",\"logic.idle.normal\"]", "[\"/api/clock/status\",\"/api/triggers/status\",\"/api/catalog/logic\"]", "[\"firmware/robot_clock.cpp\",\"firmware/robot_clock.h\",\"firmware/emotion_triggers.cpp\",\"firmware/idle_behavior.cpp\"]");
  json += "]";
  return json;
}

void appendAssetGroups(String& json) {
  json += "\"soundAssets\":[";
  appendRawEntries(json, SOUND_ASSETS, (uint8_t)(sizeof(SOUND_ASSETS) / sizeof(SOUND_ASSETS[0])));
  json += "],\"expressionAssets\":[";
  appendRawEntries(json, EXPRESSION_ASSETS, (uint8_t)(sizeof(EXPRESSION_ASSETS) / sizeof(EXPRESSION_ASSETS[0])));
  json += "],\"screenAssets\":[";
  appendRawEntries(json, SCREEN_ASSETS, (uint8_t)(sizeof(SCREEN_ASSETS) / sizeof(SCREEN_ASSETS[0])));
  json += "],\"motionAssets\":[";
  appendRawEntries(json, MOTION_ASSETS, (uint8_t)(sizeof(MOTION_ASSETS) / sizeof(MOTION_ASSETS[0])));
  json += "],\"lightAssets\":[";
  appendRawEntries(json, LIGHT_ASSETS, (uint8_t)(sizeof(LIGHT_ASSETS) / sizeof(LIGHT_ASSETS[0])));
  json += "],\"behaviorAssets\":[";
  appendRawEntries(json, BEHAVIOR_ASSETS, (uint8_t)(sizeof(BEHAVIOR_ASSETS) / sizeof(BEHAVIOR_ASSETS[0])));
  json += "],\"deprecatedAssets\":[";
  appendRawEntries(json, DEPRECATED_ASSETS, DEPRECATED_ASSET_COUNT);
  json += "]";
}

}  // namespace

String catalogAssetsJson(const MumblePlayer& mumble) {
  (void)mumble;
  String json = R"JSON({"ok":true,"version":3,"invokeApi":"/api/asset/invoke",)JSON";
  appendAssetGroups(json);
  json += "}";
  return json;
}

String catalogLogicJson(const EmotionTriggerController& triggers, const IdleBehaviorController& idle, const RobotClock& clock, const CameraAttentionController& camera) {
  String json = "{\"ok\":true,\"version\":3,\"logicAssets\":";
  json += logicEntriesJson(triggers, idle, clock, camera);
  json += "}";
  return json;
}

String catalogConflictsJson() {
  String json = "{\"ok\":true,\"version\":3,\"conflicts\":[";
  json += "{\"source\":\"logic.idle.normal\",\"target\":\"behavior.emotion.*\",\"reason\":\"both can affect servo and face; emotion action has priority in the current firmware loop\"}";
  json += ",{\"source\":\"logic.clock.night_sleep\",\"target\":\"logic.idle.normal\",\"reason\":\"night sleep changes idle posture and subtitle clock behavior during night windows\"}";
  json += ",{\"source\":\"logic.clock.night_touch_awake\",\"target\":\"logic.clock.night_sleep\",\"reason\":\"night touch reactions keep normal standby active for a random 10-20 minute awake period before tired night idle resumes\"}";
  json += "]}";
  return json;
}

String catalogJson(const EmotionTriggerController& triggers, const IdleBehaviorController& idle, const MumblePlayer& mumble, const RobotClock& clock, const CameraAttentionController& camera) {
  (void)mumble;
  String json = R"JSON({"ok":true,"version":3,"invokeApi":"/api/asset/invoke","categories":{)JSON";
  appendAssetGroups(json);
  json += ",\"logicAssets\":";
  json += logicEntriesJson(triggers, idle, clock, camera);
  json += "}}";
  return json;
}
