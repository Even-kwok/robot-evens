#include "api.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <cstring>
#include "assistant_status.h"
#include "asset_logic_catalog.h"
#include "motion_stream.h"
#include "mumble_player.h"
#include "web_console.h"
#include "wifi_manager.h"

namespace {

bool debugListeningSubtitleEnabled = false;
bool debugApiSubtitleEnabled = false;
bool voiceWakeEnabled = false;
bool powerButtonWakeEnabled = false;
bool listeningActive = false;
bool listeningWakeRequested = false;
String hostCallbackUrl = "";
uint32_t listeningStartedMs = 0;
uint32_t listeningUntilMs = 0;
uint32_t listenTimeoutMs = 15000;

constexpr uint32_t MIN_LISTEN_TIMEOUT_MS = 3000;
constexpr uint32_t MAX_LISTEN_TIMEOUT_MS = 30000;

uint16_t parseHexColor(String value, uint16_t fallback) {
  value.trim();
  if (value.startsWith("#")) value.remove(0, 1);
  if (value.length() != 6) return fallback;
  char* end = nullptr;
  uint32_t rgb = strtoul(value.c_str(), &end, 16);
  if (end == value.c_str() || *end != '\0') return fallback;
  uint8_t r = (rgb >> 16) & 0xFF;
  uint8_t g = (rgb >> 8) & 0xFF;
  uint8_t b = rgb & 0xFF;
  return ((uint16_t)(r & 0xF8) << 8) | ((uint16_t)(g & 0xFC) << 3) | (b >> 3);
}

RgbColor rgbColorFromHex(String value, const RgbColor& fallback) {
  value.trim();
  if (value.startsWith("#")) value.remove(0, 1);
  if (value.length() != 6) return fallback;
  char* end = nullptr;
  uint32_t rgb = strtoul(value.c_str(), &end, 16);
  if (end == value.c_str() || *end != '\0') return fallback;
  return {
    (uint8_t)((rgb >> 16) & 0xFF),
    (uint8_t)((rgb >> 8) & 0xFF),
    (uint8_t)(rgb & 0xFF),
  };
}

String hexFromJson(JsonVariantConst value, const String& fallback) {
  if (value.is<const char*>()) return String(value.as<const char*>());
  return fallback;
}

bool readJson(WebServer& server, JsonDocument& doc) {
  String body = server.arg("plain");
  if (!body.length()) return true;
  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid json\"}");
    return false;
  }
  return true;
}

void sendJson(WebServer& server, const String& body) {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", body);
}

void sendJsonStatus(WebServer& server, int status, const String& body) {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(status, "application/json", body);
}

bool requireHardware(WebServer& server, RobotHardware& hardware) {
  if (hardware.isReady()) return true;
  sendJsonStatus(server, 503, hardware.unavailableJson());
  return false;
}

String motionStateJson(const MotionState& state) {
  String json = "{\"ok\":true";
  json += ",\"yaw\":" + String(state.yaw);
  json += ",\"pitch\":" + String(state.pitch);
  json += ",\"speed\":" + String(state.speed);
  json += ",\"velocity\":" + String(state.velocity);
  json += "}";
  return json;
}

String jsonQuote(const String& value) {
  String out = "\"";
  for (uint16_t i = 0; i < value.length(); ++i) {
    char c = value.charAt(i);
    if (c == '"' || c == '\\') out += "\\";
    out += c;
  }
  out += "\"";
  return out;
}

String debugSubtitleStatusJson() {
  String json = "{\"ok\":true";
  json += ",\"listening\":" + String(debugListeningSubtitleEnabled ? "true" : "false");
  json += ",\"apiResult\":" + String(debugApiSubtitleEnabled ? "true" : "false");
  json += "}";
  return json;
}

bool showDebugSubtitle(RobotFaceRenderer& face, const String& kind, const String& text) {
  bool enabled = false;
  if (kind == "listening") enabled = debugListeningSubtitleEnabled;
  if (kind == "apiResult") enabled = debugApiSubtitleEnabled;
  if (!enabled) return false;
  String displayText = text;
  if (!displayText.length()) displayText = kind == "listening" ? "Listening" : "{}";
  SubtitleState subtitle = face.subtitle();
  face.setSubtitle(displayText, true, subtitle.color);
  return true;
}

uint32_t clampListeningTimeout(uint32_t value) {
  if (value < MIN_LISTEN_TIMEOUT_MS) return MIN_LISTEN_TIMEOUT_MS;
  if (value > MAX_LISTEN_TIMEOUT_MS) return MAX_LISTEN_TIMEOUT_MS;
  return value;
}

void stopListeningMode(RobotFaceRenderer& face, uint32_t nowMs) {
  (void)nowMs;
  listeningActive = false;
  listeningWakeRequested = false;
  listeningUntilMs = 0;
  if (debugListeningSubtitleEnabled) face.clearSubtitle();
}

void updateListeningTimeout(RobotFaceRenderer& face, uint32_t nowMs) {
  if (listeningActive && (int32_t)(nowMs - listeningUntilMs) >= 0) {
    stopListeningMode(face, nowMs);
  }
}

String listeningStatusJson(uint32_t nowMs) {
  bool wifiConnected = WiFi.status() == WL_CONNECTED;
  uint32_t remainingMs = (listeningActive && listeningUntilMs > nowMs) ? listeningUntilMs - nowMs : 0;
  String json = "{\"ok\":true";
  json += ",\"voiceWakeEnabled\":" + String(voiceWakeEnabled ? "true" : "false");
  json += ",\"powerButtonWakeEnabled\":" + String(powerButtonWakeEnabled ? "true" : "false");
  json += ",\"hostCallbackUrl\":" + jsonQuote(hostCallbackUrl);
  json += ",\"active\":" + String(listeningActive ? "true" : "false");
  json += ",\"listeningWakeRequested\":" + String(listeningWakeRequested ? "true" : "false");
  json += ",\"wifiConnected\":" + String(wifiConnected ? "true" : "false");
  json += ",\"listenTimeoutMs\":" + String(listenTimeoutMs);
  json += ",\"remainingMs\":" + String(remainingMs);
  json += ",\"startedMs\":" + String(listeningStartedMs);
  json += "}";
  return json;
}

String listeningStatusWithFields(uint32_t nowMs, const String& fields) {
  String json = listeningStatusJson(nowMs);
  json.remove(json.length() - 1);
  if (fields.length()) json += fields;
  json += "}";
  return json;
}

String listeningStartError() {
  if (WiFi.status() != WL_CONNECTED) return "wifi disconnected";
  if (!voiceWakeEnabled) return "voice wake disabled";
  return "";
}

bool startListeningMode(RobotFaceRenderer& face, IdleBehaviorController& idle, uint32_t nowMs, uint32_t requestedTimeoutMs, bool requireVoiceWakeEnabled = true) {
  listenTimeoutMs = clampListeningTimeout(requestedTimeoutMs ? requestedTimeoutMs : listenTimeoutMs);
  if (WiFi.status() != WL_CONNECTED || (requireVoiceWakeEnabled && !voiceWakeEnabled)) return false;
  listeningActive = true;
  listeningWakeRequested = true;
  listeningStartedMs = nowMs;
  listeningUntilMs = nowMs + listenTimeoutMs;
  face.setMood("curious");
  if (debugListeningSubtitleEnabled) {
    SubtitleState subtitle = face.subtitle();
    face.setSubtitle("Listening", true, subtitle.color);
  }
  idle.pauseForManualControl(nowMs, listenTimeoutMs + 500);
  return true;
}

String startListeningFromCameraJson(RobotFaceRenderer& face, IdleBehaviorController& idle, uint32_t nowMs) {
  bool started = startListeningMode(face, idle, nowMs, listenTimeoutMs);
  String extra = ",\"listeningStarted\":" + String(started ? "true" : "false");
  if (!started) extra += ",\"listeningError\":" + jsonQuote(listeningStartError());
  return listeningStatusWithFields(nowMs, extra);
}

bool notifyListeningHost(const String& source) {
  if (!hostCallbackUrl.length()) return false;
  if (WiFi.status() != WL_CONNECTED) return false;
  HTTPClient http;
  http.begin(hostCallbackUrl);
  http.setTimeout(2500);
  http.addHeader("Content-Type", "application/json");
  String body = "{\"source\":" + jsonQuote(source);
  body += ",\"event\":\"wake\"";
  body += ",\"active\":" + String(listeningActive ? "true" : "false");
  body += ",\"listenTimeoutMs\":" + String(listenTimeoutMs);
  body += ",\"robotIp\":" + jsonQuote(WiFi.localIP().toString());
  body += "}";
  int code = http.POST(body);
  http.end();
  return code >= 200 && code < 300;
}

String startListeningAndNotifyJson(RobotFaceRenderer& face, IdleBehaviorController& idle, uint32_t nowMs, uint32_t timeoutMs, const String& source, bool requireVoiceWakeEnabled = true) {
  bool started = startListeningMode(face, idle, nowMs, timeoutMs, requireVoiceWakeEnabled);
  bool notified = started && notifyListeningHost(source);
  String extra = ",\"started\":" + String(started ? "true" : "false");
  extra += ",\"listeningStarted\":" + String(started ? "true" : "false");
  extra += ",\"notified\":" + String(notified ? "true" : "false");
  extra += ",\"source\":" + jsonQuote(source);
  if (!started) extra += ",\"error\":" + jsonQuote(listeningStartError());
  return listeningStatusWithFields(nowMs, extra);
}

String assetIdFromJson(JsonVariantConst doc) {
  if (doc["id"].is<const char*>()) return String(doc["id"].as<const char*>());
  if (doc["assetId"].is<const char*>()) return String(doc["assetId"].as<const char*>());
  if (doc["asset"].is<const char*>()) return String(doc["asset"].as<const char*>());
  return "";
}

uint32_t uintFromJson(JsonVariantConst doc, const char* key, uint32_t fallback) {
  if (doc[key].is<int>()) return (uint32_t)max(0, doc[key].as<int>());
  return fallback;
}

int intFromJson(JsonVariantConst doc, const char* key, int fallback) {
  if (doc[key].is<int>()) return doc[key].as<int>();
  return fallback;
}

bool boolFromJson(JsonVariantConst doc, const char* key, bool fallback) {
  if (doc[key].is<bool>()) return doc[key].as<bool>();
  return fallback;
}

String assetInvokeErrorJson(const String& id, const char* error) {
  String json = "{\"ok\":false,\"id\":";
  json += jsonQuote(id);
  json += ",\"error\":\"";
  json += error;
  json += "\"}";
  return json;
}

String assetInvokeResultJson(const String& id, const char* type, const char* route, const String& result, const String& actionName = "") {
  bool ok = result.indexOf("\"ok\":false") < 0;
  String json = "{\"ok\":";
  json += ok ? "true" : "false";
  json += ",\"id\":";
  json += jsonQuote(id);
  json += ",\"type\":\"";
  json += type;
  json += "\",\"route\":\"";
  json += route;
  json += "\"";
  if (actionName.length()) {
    json += ",\"actionName\":";
    json += jsonQuote(actionName);
  }
  json += ",\"result\":";
  json += result;
  json += "}";
  return json;
}

String soundSlotFromAssetId(const String& id) {
  if (!id.startsWith("sound.")) return "";
  String slot = id.substring(String("sound.").length());
  if (slot == "comfortable_nod") return "comfortableNod";
  if (slot == "head_touch_combo_spin") return "headTouchComboSpin";
  if (slot == "idle_thought") return "idleThought";
  return slot;
}

String emotionActionFromBehaviorAssetId(const String& id) {
  if (id == "behavior.robot_action.nod_yes" || id == "motion.robot_action.nod_yes") return "nod_yes";
  if (id == "behavior.robot_action.shake_no" || id == "motion.robot_action.shake_no") return "shake_no";
  if (id == "motion.emotion.sleep_startle") return "sleepStartle";
  if (!id.startsWith("behavior.emotion.")) return "";
  String name = id.substring(String("behavior.emotion.").length());
  if (name == "comfortable_nod") return "comfortableNod";
  if (name == "head_touch_combo_spin") return "headTouchComboSpin";
  if (name == "idle_thought") return "idleThought";
  if (name == "sleep_wake") return "sleepWake";
  if (name == "sleep_startle") return "sleepStartle";
  if (name.startsWith("happy.")) return "happy";
  return name;
}

String applyRainbowMarqueeLightAsset(RobotHardware& hardware) {
  const RgbColor colors[6] = {
    {255, 42, 84},
    {255, 176, 40},
    {255, 238, 64},
    {35, 230, 116},
    {42, 196, 255},
    {146, 96, 255},
  };
  RgbColor frame[RGB_LED_COUNT] = {};
  for (uint8_t i = 0; i < RGB_LED_COUNT; ++i) {
    frame[i] = colors[i % 6];
  }
  hardware.setRgbEffectFrame(frame);
  return "{\"ok\":true,\"light\":\"rainbow_marquee\"}";
}

bool applyRgbPreset(RobotHardware& hardware, const String& preset, uint32_t nowMs, bool blockedByEmotionLights) {
  if (preset == "softGreenBreath") {
    hardware.setRgbBreathingEnabled(false, nowMs, blockedByEmotionLights);
    hardware.setRgbMarqueeEnabled(false, nowMs, blockedByEmotionLights);
    hardware.setRgbRainbowEnabled(false, nowMs, blockedByEmotionLights);
    hardware.setRgbBrightness(150);
    hardware.setRgbAll(0, 64, 32);
    hardware.setRgbPresetName("softGreenBreath");
    hardware.setRgbBreathingEnabled(true, nowMs, blockedByEmotionLights);
    return true;
  }
  if (preset == "rainbowMarquee") {
    hardware.setRgbBreathingEnabled(false, nowMs, blockedByEmotionLights);
    hardware.setRgbRainbowEnabled(false, nowMs, blockedByEmotionLights);
    hardware.setRgbBrightness(220);
    hardware.setRgbPresetName("rainbowMarquee");
    hardware.setRgbMarqueeEnabled(true, nowMs, blockedByEmotionLights);
    return true;
  }
  if (preset == "rainbowStatic") {
    hardware.setRgbBreathingEnabled(false, nowMs, blockedByEmotionLights);
    hardware.setRgbMarqueeEnabled(false, nowMs, blockedByEmotionLights);
    hardware.setRgbBrightness(220);
    hardware.setRgbPresetName("rainbowStatic");
    hardware.setRgbRainbowEnabled(true, nowMs, blockedByEmotionLights);
    return true;
  }
  return false;
}

String invokeAssetJson(JsonVariantConst doc, RobotFaceRenderer& face, RobotHardware& hardware, IdleBehaviorController& idle, EmotionActionController& emotion, MumblePlayer& mumble, RobotClock& clock, int& status) {
  String id = assetIdFromJson(doc);
  if (!id.length()) {
    status = 400;
    return assetInvokeErrorJson(id, "missing asset id");
  }
  uint32_t nowMs = millis();

  if (id.startsWith("logic.")) {
    status = 400;
    return assetInvokeErrorJson(id, "logic assets are not directly invokable");
  }

  if (id.startsWith("expression.")) {
    String mood = id.substring(String("expression.").length());
    FaceMood parsedMood;
    if (!RobotFaceRenderer::parseMood(mood, parsedMood)) {
      status = 400;
      return assetInvokeErrorJson(id, "unknown expression asset");
    }
    clock.markInteraction(nowMs);
    idle.pauseForManualControl(nowMs);
    face.setMood(mood);
    FaceStyle style = face.style();
    if (doc["eyeColor"].is<const char*>()) style.eyeColor = parseHexColor(String(doc["eyeColor"].as<const char*>()), style.eyeColor);
    if (doc["color"].is<const char*>()) style.eyeColor = parseHexColor(String(doc["color"].as<const char*>()), style.eyeColor);
    face.setStyle(style);
    return assetInvokeResultJson(id, "expression", "/api/face/set", face.stateJson());
  }

  if (id == "screen.matrix_rain") {
    clock.markInteraction(nowMs);
    idle.pauseForManualControl(nowMs);
    uint32_t durationMs = uintFromJson(doc, "durationMs", 30000);
    if (!face.setScreenAsset("matrixRain", nowMs, durationMs)) {
      status = 400;
      return assetInvokeErrorJson(id, "unknown screen asset");
    }
    return assetInvokeResultJson(id, "screen", "/api/screen/trigger", face.screenAssetJson(nowMs));
  }

  if (!hardware.isReady()) {
    status = 503;
    return hardware.unavailableJson();
  }

  if (id.startsWith("sound.")) {
    String slot = soundSlotFromAssetId(id);
    if (!slot.length()) {
      status = 400;
      return assetInvokeErrorJson(id, "unknown sound asset");
    }
    idle.pauseForManualControl(nowMs, 2500);
    String result = mumble.playEmotionSlotJson(slot, nowMs, uintFromJson(doc, "durationMs", 0));
    return assetInvokeResultJson(id, "sound", "/api/emotion/sound-slot/play", result, slot);
  }

  if (id == "motion.pose.sleeping_low") {
    clock.markInteraction(nowMs);
    idle.pauseForManualControl(nowMs);
    int speed = intFromJson(doc, "speed", 60);
    return assetInvokeResultJson(id, "motion", "/api/motion/pose", motionStateJson(hardware.pose(0, 0, speed)));
  }

  if (id == "motion.pose.tired_low") {
    clock.markInteraction(nowMs);
    idle.pauseForManualControl(nowMs);
    int speed = intFromJson(doc, "speed", 60);
    return assetInvokeResultJson(id, "motion", "/api/motion/pose", motionStateJson(hardware.pose(0, 12, speed)));
  }

  if (id == "light.rainbow_marquee") {
    clock.markInteraction(nowMs);
    uint32_t durationMs = uintFromJson(doc, "durationMs", 1800);
    idle.pauseForManualControl(nowMs, durationMs);
    return assetInvokeResultJson(id, "light", "/api/rgb", applyRainbowMarqueeLightAsset(hardware));
  }

  String actionName = emotionActionFromBehaviorAssetId(id);
  if (actionName.length()) {
    if (!EmotionActionController::isKnownAction(actionName)) {
      status = 400;
      return assetInvokeErrorJson(id, "unknown behavior asset");
    }
    bool lightsEnabled = boolFromJson(doc, "lights", true);
    bool defaultSoundEnabled = !(actionName == "nod_yes" || actionName == "shake_no" || actionName == "comfortableNod" || actionName == "curious" || actionName == "sleepStartle");
    bool soundEnabled = boolFromJson(doc, "sound", defaultSoundEnabled);
    clock.markInteraction(nowMs);
    idle.pauseForManualControl(nowMs, 4000);
    if (emotion.active()) emotion.stop(hardware, nowMs);
    emotion.play(actionName, nowMs, lightsEnabled);
    String soundJson = soundEnabled ? mumble.playEmotionSlotJson(actionName, nowMs, emotion.durationMs()) : "{\"ok\":true,\"enabled\":false,\"skipped\":true}";
    String result = emotion.statusJson();
    result.remove(result.length() - 1);
    result += ",\"sound\":";
    result += soundJson;
    result += "}";
    return assetInvokeResultJson(id, "behavior", "/api/emotion/play", result, actionName);
  }

  status = 400;
  return assetInvokeErrorJson(id, "unknown or non-invokable asset");
}

}  // namespace

bool handlePowerButtonWake(RobotFaceRenderer& face, IdleBehaviorController& idle, RobotClock& clock, uint32_t nowMs) {
  if (!powerButtonWakeEnabled) return false;
  clock.markInteraction(nowMs);
  String result = startListeningAndNotifyJson(face, idle, nowMs, listenTimeoutMs, "power-button", false);
  return result.indexOf("\"listeningStarted\":true") >= 0;
}

void registerApiRoutes(WebServer& server, RobotFaceRenderer& face, RobotHardware& hardware, IdleBehaviorController& idle, EmotionActionController& emotion, EmotionTriggerController& triggers, MumblePlayer& mumble, RobotClock& clock, ProximityTrackerController& proximity, CameraAttentionController& camera, LocalFaceDetector& faceDetector, MotionStreamReceiver& motionStream, AssistantStatusDisplay& assistantStatus, RobotWifiManager& wifiManager) {
  server.on("/", HTTP_GET, [&server, &wifiManager]() {
    server.send(200, "text/html; charset=utf-8", wifiManager.setupPortalActive() ? wifiManager.setupPageHtml() : webConsoleHtml());
  });

  server.on("/setup", HTTP_GET, [&server, &wifiManager]() {
    server.send(200, "text/html; charset=utf-8", wifiManager.setupPageHtml());
  });

  server.on("/api/ping", HTTP_GET, [&server]() {
    sendJson(server, "{\"ok\":true,\"name\":\"Desktop Robot\"}");
  });

  server.on("/api/status", HTTP_GET, [&server, &face, &hardware, &clock]() {
    sendJson(server, hardware.statusJson(face.snapshot(), clock.statusJson()));
  });

  server.on("/api/setup/status", HTTP_GET, [&server, &wifiManager]() {
    sendJson(server, wifiManager.statusJson());
  });

  server.on("/api/setup/scan", HTTP_POST, [&server, &wifiManager]() {
    sendJson(server, wifiManager.scanJson());
  });

  server.on("/api/setup/connect", HTTP_POST, [&server, &wifiManager]() {
    JsonDocument doc;
    if (!readJson(server, doc)) return;
    String ssid = doc["ssid"].is<const char*>() ? String(doc["ssid"].as<const char*>()) : "";
    String password = doc["password"].is<const char*>() ? String(doc["password"].as<const char*>()) : "";
    uint32_t timeoutMs = doc["timeoutMs"].is<int>() ? (uint32_t)max(0, doc["timeoutMs"].as<int>()) : 15000;
    int status = 200;
    String body = wifiManager.connectJson(ssid, password, timeoutMs, status);
    sendJsonStatus(server, status, body);
  });

  server.on("/api/setup/forget", HTTP_POST, [&server, &wifiManager]() {
    sendJson(server, wifiManager.forgetJson());
  });

  server.on("/api/setup/close", HTTP_POST, [&server, &wifiManager]() {
    sendJson(server, wifiManager.closeJson());
  });

  server.on("/api/assistant-display/status", HTTP_GET, [&server, &assistantStatus]() {
    sendJson(server, assistantStatus.statusJson(millis()));
  });

  server.on("/api/assistant-display/set", HTTP_POST, [&server, &assistantStatus]() {
    JsonDocument doc;
    if (!readJson(server, doc)) return;
    bool enabled = boolFromJson(doc.as<JsonVariantConst>(), "enabled", assistantStatus.enabled());
    String assistantStatusUrl = doc["assistantStatusUrl"].is<const char*>()
      ? String(doc["assistantStatusUrl"].as<const char*>())
      : assistantStatus.statusUrl();
    uint32_t pollIntervalMs = uintFromJson(doc.as<JsonVariantConst>(), "pollIntervalMs", assistantStatus.pollIntervalMs());
    bool displayEnabled = boolFromJson(doc.as<JsonVariantConst>(), "displayEnabled", assistantStatus.displayEnabled());
    bool batteryEnabled = boolFromJson(doc.as<JsonVariantConst>(), "batteryEnabled", assistantStatus.batteryEnabled());
    uint8_t lowCodexPercent = (uint8_t)min(100U, max(1U, uintFromJson(doc.as<JsonVariantConst>(), "lowCodexPercent", assistantStatus.lowCodexPercent())));
    uint8_t lowBatteryPercent = (uint8_t)min(100U, max(1U, uintFromJson(doc.as<JsonVariantConst>(), "lowBatteryPercent", assistantStatus.lowBatteryPercent())));
    uint16_t lineColor = assistantStatus.lineColor();
    if (doc["lineColor"].is<const char*>()) lineColor = parseHexColor(String(doc["lineColor"].as<const char*>()), lineColor);
    if (doc["color"].is<const char*>()) lineColor = parseHexColor(String(doc["color"].as<const char*>()), lineColor);
    assistantStatus.configure(enabled, assistantStatusUrl, pollIntervalMs, displayEnabled, lineColor, batteryEnabled);
    assistantStatus.setAlertThresholds(lowCodexPercent, lowBatteryPercent);
    sendJson(server, assistantStatus.statusJson(millis()));
  });

  server.on("/api/catalog", HTTP_GET, [&server, &triggers, &idle, &mumble, &clock, &camera]() {
    sendJson(server, catalogJson(triggers, idle, mumble, clock, camera));
  });

  server.on("/api/catalog/assets", HTTP_GET, [&server, &mumble]() {
    sendJson(server, catalogAssetsJson(mumble));
  });

  server.on("/api/catalog/logic", HTTP_GET, [&server, &triggers, &idle, &clock, &camera]() {
    sendJson(server, catalogLogicJson(triggers, idle, clock, camera));
  });

  server.on("/api/catalog/conflicts", HTTP_GET, [&server]() {
    sendJson(server, catalogConflictsJson());
  });

  server.on("/api/asset/invoke", HTTP_POST, [&server, &face, &hardware, &idle, &emotion, &mumble, &clock]() {
    JsonDocument doc;
    if (!readJson(server, doc)) return;
    int status = 200;
    String body = invokeAssetJson(doc.as<JsonVariantConst>(), face, hardware, idle, emotion, mumble, clock, status);
    sendJsonStatus(server, status, body);
  });

  server.on("/api/face/list", HTTP_GET, [&server]() {
    sendJson(server, "{\"moods\":[\"neutral\",\"happy\",\"angry\",\"sad\",\"sleeping\",\"tired\",\"surprised\",\"love\",\"dizzy\",\"wink\",\"dead\",\"thinking\",\"curious\",\"sunglasses\",\"loading\"],\"fields\":[\"eyeWidthScale\",\"eyeHeightScale\",\"cornerRadius\",\"eyeOffsetX\",\"eyeOffsetY\",\"eyeSpacing\",\"eyeColor\",\"glowEnabled\",\"glowStrength\",\"scanlinesEnabled\",\"scanlineSpacing\",\"scanlineAlpha\",\"audioLevel\",\"lookX\",\"lookY\"]}");
  });

  server.on("/api/screen/status", HTTP_GET, [&server, &face]() {
    sendJson(server, face.screenAssetJson(millis()));
  });

  server.on("/api/screen/set", HTTP_POST, [&server, &face, &idle, &clock]() {
    JsonDocument doc;
    if (!readJson(server, doc)) return;
    uint32_t nowMs = millis();
    clock.markInteraction(nowMs);
    idle.pauseForManualControl(nowMs);
    String asset = "none";
    if (doc["asset"].is<const char*>()) asset = String(doc["asset"].as<const char*>());
    if (doc["mode"].is<const char*>()) asset = String(doc["mode"].as<const char*>());
    uint32_t durationMs = 0;
    if (doc["durationMs"].is<int>()) durationMs = (uint32_t)max(0, doc["durationMs"].as<int>());
    if (asset == "none" || asset == "off" || asset == "clear") face.clearScreenAsset();
    else if (!face.setScreenAsset(asset, nowMs, durationMs)) {
      sendJsonStatus(server, 400, "{\"ok\":false,\"error\":\"unknown screen asset\"}");
      return;
    }
    sendJson(server, face.screenAssetJson(nowMs));
  });

  server.on("/api/screen/trigger", HTTP_POST, [&server, &face, &idle, &clock]() {
    JsonDocument doc;
    if (!readJson(server, doc)) return;
    uint32_t nowMs = millis();
    clock.markInteraction(nowMs);
    idle.pauseForManualControl(nowMs);
    String asset = "matrixRain";
    if (doc["asset"].is<const char*>()) asset = String(doc["asset"].as<const char*>());
    if (doc["mode"].is<const char*>()) asset = String(doc["mode"].as<const char*>());
    uint32_t durationMs = 30000;
    if (doc["durationMs"].is<int>()) durationMs = (uint32_t)max(0, doc["durationMs"].as<int>());
    if (!face.setScreenAsset(asset, nowMs, durationMs)) {
      sendJsonStatus(server, 400, "{\"ok\":false,\"error\":\"unknown screen asset\"}");
      return;
    }
    sendJson(server, face.screenAssetJson(nowMs));
  });

  server.on("/api/face/set", HTTP_POST, [&server, &face, &idle, &clock]() {
    JsonDocument doc;
    if (!readJson(server, doc)) return;
    uint32_t nowMs = millis();
    clock.markInteraction(nowMs);
    idle.pauseForManualControl(nowMs);
    if (doc["mood"].is<const char*>()) face.setMood(String(doc["mood"].as<const char*>()));
    FaceStyle style = face.style();
    if (doc["eyeColor"].is<const char*>()) style.eyeColor = parseHexColor(String(doc["eyeColor"].as<const char*>()), style.eyeColor);
    if (doc["color"].is<const char*>()) style.eyeColor = parseHexColor(String(doc["color"].as<const char*>()), style.eyeColor);
    face.setStyle(style);
    sendJson(server, face.stateJson());
  });

  server.on("/api/face/style", HTTP_POST, [&server, &face, &idle, &clock]() {
    JsonDocument doc;
    if (!readJson(server, doc)) return;
    uint32_t nowMs = millis();
    clock.markInteraction(nowMs);
    idle.pauseForManualControl(nowMs);
    if (doc["mood"].is<const char*>()) face.setMood(String(doc["mood"].as<const char*>()));
    FaceStyle style = face.style();
    if (doc["eyeWidthScale"].is<float>()) style.eyeWidthScale = doc["eyeWidthScale"].as<float>();
    if (doc["eyeHeightScale"].is<float>()) style.eyeHeightScale = doc["eyeHeightScale"].as<float>();
    if (doc["cornerRadius"].is<int>()) style.cornerRadius = doc["cornerRadius"].as<int>();
    if (doc["eyeOffsetX"].is<int>()) style.eyeOffsetX = doc["eyeOffsetX"].as<int>();
    if (doc["eyeOffsetY"].is<int>()) style.eyeOffsetY = doc["eyeOffsetY"].as<int>();
    if (doc["eyeSpacing"].is<int>()) style.eyeSpacing = doc["eyeSpacing"].as<int>();
    if (doc["eyeColor"].is<const char*>()) style.eyeColor = parseHexColor(String(doc["eyeColor"].as<const char*>()), style.eyeColor);
    if (doc["glowEnabled"].is<bool>()) style.glowEnabled = doc["glowEnabled"].as<bool>();
    if (doc["glowStrength"].is<int>()) style.glowStrength = doc["glowStrength"].as<int>();
    if (doc["scanlinesEnabled"].is<bool>()) style.scanlinesEnabled = doc["scanlinesEnabled"].as<bool>();
    if (doc["scanlineSpacing"].is<int>()) style.scanlineSpacing = doc["scanlineSpacing"].as<int>();
    if (doc["scanlineAlpha"].is<int>()) style.scanlineAlpha = doc["scanlineAlpha"].as<int>();
    face.patchStyle(style);
    sendJson(server, face.stateJson());
  });

  server.on("/api/face/react", HTTP_POST, [&server, &face, &hardware, &idle, &emotion, &clock]() {
    JsonDocument doc;
    if (!readJson(server, doc)) return;
    uint32_t nowMs = millis();
    clock.markInteraction(nowMs);
    FaceReactState react = face.react();
    SubtitleState subtitle = face.subtitle();
    float lookX = doc["lookX"].is<float>() ? doc["lookX"].as<float>() : react.lookX;
    float lookY = doc["lookY"].is<float>() ? doc["lookY"].as<float>() : react.lookY;
    float audioLevel = doc["audioLevel"].is<float>() ? doc["audioLevel"].as<float>() : react.audioLevel;
    idle.pauseForManualControl(nowMs);
    face.setReact(lookX, lookY, audioLevel);
    if (doc["subtitle"].is<const char*>()) {
      face.setSubtitle(String(doc["subtitle"].as<const char*>()), true, subtitle.color);
    }
    if (hardware.isReady()) hardware.applySpeechLightLevel(audioLevel, emotion.lightsActive());
    sendJson(server, face.stateJson());
  });

  server.on("/api/subtitle/set", HTTP_POST, [&server, &face, &clock]() {
    JsonDocument doc;
    if (!readJson(server, doc)) return;
    uint32_t nowMs = millis();
    clock.markInteraction(nowMs);
    SubtitleState subtitle = face.subtitle();
    String text = subtitle.text;
    bool enabled = subtitle.enabled;
    uint16_t color = subtitle.color;
    if (doc["text"].is<const char*>()) text = String(doc["text"].as<const char*>());
    if (doc["enabled"].is<bool>()) enabled = doc["enabled"].as<bool>();
    if (doc["color"].is<const char*>()) color = parseHexColor(String(doc["color"].as<const char*>()), color);
    if (doc["showDateTime"].is<bool>()) {
      clock.setShowDateTimeSubtitle(doc["showDateTime"].as<bool>());
      face.setSubtitleDateTimeEnabled(clock.showDateTimeSubtitle());
    }
    if (doc["showDateTimeSubtitle"].is<bool>()) {
      clock.setShowDateTimeSubtitle(doc["showDateTimeSubtitle"].as<bool>());
      face.setSubtitleDateTimeEnabled(clock.showDateTimeSubtitle());
    }
    face.setSubtitle(text, enabled, color);
    sendJson(server, face.stateJson());
  });

  server.on("/api/subtitle/clear", HTTP_POST, [&server, &face, &clock]() {
    uint32_t nowMs = millis();
    clock.markInteraction(nowMs);
    face.clearSubtitle();
    sendJson(server, face.stateJson());
  });

  server.on("/api/debug/subtitle/status", HTTP_GET, [&server]() {
    sendJson(server, debugSubtitleStatusJson());
  });

  server.on("/api/debug/subtitle/set", HTTP_POST, [&server]() {
    JsonDocument doc;
    if (!readJson(server, doc)) return;
    if (doc["listening"].is<bool>()) debugListeningSubtitleEnabled = doc["listening"].as<bool>();
    if (doc["apiResult"].is<bool>()) debugApiSubtitleEnabled = doc["apiResult"].as<bool>();
    sendJson(server, debugSubtitleStatusJson());
  });

  server.on("/api/debug/subtitle/show", HTTP_POST, [&server, &face, &clock]() {
    JsonDocument doc;
    if (!readJson(server, doc)) return;
    uint32_t nowMs = millis();
    clock.markInteraction(nowMs);
    String kind = doc["kind"].is<const char*>() ? String(doc["kind"].as<const char*>()) : "apiResult";
    String text = doc["text"].is<const char*>() ? String(doc["text"].as<const char*>()) : "";
    bool displayed = showDebugSubtitle(face, kind, text);
    String json = debugSubtitleStatusJson();
    json.remove(json.length() - 1);
    json += ",\"kind\":" + jsonQuote(kind);
    json += ",\"displayed\":" + String(displayed ? "true" : "false");
    json += ",\"text\":" + jsonQuote(text);
    json += "}";
    sendJson(server, json);
  });

  server.on("/api/listening/status", HTTP_GET, [&server, &face]() {
    uint32_t nowMs = millis();
    updateListeningTimeout(face, nowMs);
    sendJson(server, listeningStatusJson(nowMs));
  });

  server.on("/api/listening/config", HTTP_POST, [&server, &face]() {
    JsonDocument doc;
    if (!readJson(server, doc)) return;
    if (doc["voiceWakeEnabled"].is<bool>()) voiceWakeEnabled = doc["voiceWakeEnabled"].as<bool>();
    if (doc["powerButtonWakeEnabled"].is<bool>()) powerButtonWakeEnabled = doc["powerButtonWakeEnabled"].as<bool>();
    if (doc["hostCallbackUrl"].is<const char*>()) hostCallbackUrl = String(doc["hostCallbackUrl"].as<const char*>());
    if (doc["listenTimeoutMs"].is<int>()) listenTimeoutMs = clampListeningTimeout((uint32_t)max(0, doc["listenTimeoutMs"].as<int>()));
    uint32_t nowMs = millis();
    if (!voiceWakeEnabled && listeningActive) {
      stopListeningMode(face, nowMs);
    }
    updateListeningTimeout(face, nowMs);
    sendJson(server, listeningStatusJson(nowMs));
  });

  server.on("/api/listening/start", HTTP_POST, [&server, &face, &idle, &clock]() {
    JsonDocument doc;
    if (!readJson(server, doc)) return;
    uint32_t nowMs = millis();
    clock.markInteraction(nowMs);
    uint32_t timeoutMs = listenTimeoutMs;
    if (doc["listenTimeoutMs"].is<int>()) timeoutMs = (uint32_t)max(0, doc["listenTimeoutMs"].as<int>());
    bool started = startListeningMode(face, idle, nowMs, timeoutMs);
    String extra = ",\"started\":" + String(started ? "true" : "false");
    if (!started) extra += ",\"error\":" + jsonQuote(listeningStartError());
    sendJson(server, listeningStatusWithFields(nowMs, extra));
  });

  server.on("/api/listening/stop", HTTP_POST, [&server, &face, &clock]() {
    uint32_t nowMs = millis();
    clock.markInteraction(nowMs);
    stopListeningMode(face, nowMs);
    sendJson(server, listeningStatusWithFields(nowMs, ",\"stopped\":true"));
  });

  server.on("/api/listening/wake-test", HTTP_POST, [&server, &face, &idle, &clock]() {
    JsonDocument doc;
    if (!readJson(server, doc)) return;
    uint32_t nowMs = millis();
    clock.markInteraction(nowMs);
    String source = doc["source"].is<const char*>() ? String(doc["source"].as<const char*>()) : "firmware";
    uint32_t timeoutMs = listenTimeoutMs;
    if (doc["listenTimeoutMs"].is<int>()) timeoutMs = (uint32_t)max(0, doc["listenTimeoutMs"].as<int>());
    sendJson(server, startListeningAndNotifyJson(face, idle, nowMs, timeoutMs, source));
  });

  server.on("/api/listening/wake", HTTP_POST, [&server, &face, &idle, &clock]() {
    JsonDocument doc;
    if (!readJson(server, doc)) return;
    uint32_t nowMs = millis();
    clock.markInteraction(nowMs);
    String source = doc["source"].is<const char*>() ? String(doc["source"].as<const char*>()) : "firmware-wake";
    uint32_t timeoutMs = listenTimeoutMs;
    if (doc["listenTimeoutMs"].is<int>()) timeoutMs = (uint32_t)max(0, doc["listenTimeoutMs"].as<int>());
    sendJson(server, startListeningAndNotifyJson(face, idle, nowMs, timeoutMs, source));
  });

  server.on("/api/clock/status", HTTP_GET, [&server, &clock]() {
    sendJson(server, clock.statusJson());
  });

  server.on("/api/clock/set", HTTP_POST, [&server, &face, &clock]() {
    JsonDocument doc;
    if (!readJson(server, doc)) return;
    uint32_t nowMs = millis();
    clock.markInteraction(nowMs);
    if (doc["showDateTimeSubtitle"].is<bool>()) {
      clock.setShowDateTimeSubtitle(doc["showDateTimeSubtitle"].as<bool>());
      face.setSubtitleDateTimeEnabled(clock.showDateTimeSubtitle());
    }
    bool nightEnabled = clock.nightEnabled();
    uint8_t nightStartHour = clock.nightStartHour();
    uint8_t nightEndHour = clock.nightEndHour();
    uint32_t sleepAfterMs = clock.sleepAfterMs();
    if (doc["nightEnabled"].is<bool>()) nightEnabled = doc["nightEnabled"].as<bool>();
    if (doc["nightStartHour"].is<int>()) nightStartHour = (uint8_t)doc["nightStartHour"].as<int>();
    if (doc["nightEndHour"].is<int>()) nightEndHour = (uint8_t)doc["nightEndHour"].as<int>();
    if (doc["sleepAfterMs"].is<int>()) sleepAfterMs = (uint32_t)doc["sleepAfterMs"].as<int>();
    clock.setNightSettings(nightEnabled, nightStartHour, nightEndHour, sleepAfterMs);
    sendJson(server, clock.statusJson());
  });

  server.on("/api/motion/pose", HTTP_POST, [&server, &hardware, &idle, &clock]() {
    if (!requireHardware(server, hardware)) return;
    JsonDocument doc;
    if (!readJson(server, doc)) return;
    uint32_t nowMs = millis();
    clock.markInteraction(nowMs);
    idle.pauseForManualControl(nowMs);
    hardware.extendTemporaryRgbOverride(nowMs);
    sendJson(server, motionStateJson(hardware.pose(doc["yaw"] | 0, doc["pitch"] | 120, doc["speed"] | 300)));
  });

  server.on("/api/motion/stream/status", HTTP_GET, [&server, &motionStream]() {
    sendJson(server, motionStream.statusJson(millis()));
  });

  server.on("/api/motion/rest-pitch", HTTP_POST, [&server, &hardware, &idle, &clock]() {
    if (!requireHardware(server, hardware)) return;
    JsonDocument doc;
    if (!readJson(server, doc)) return;
    uint32_t nowMs = millis();
    clock.markInteraction(nowMs);
    idle.pauseForManualControl(nowMs);
    if (doc["pitch"].is<int>()) hardware.setRestPitch(doc["pitch"].as<int>());
    bool apply = doc["apply"].is<bool>() ? doc["apply"].as<bool>() : false;
    if (apply) hardware.pose(0, hardware.restPitch(), doc["speed"] | 300);
    String json = "{\"ok\":true,\"motion\":";
    json += hardware.motionJson();
    json += "}";
    sendJson(server, json);
  });

  server.on("/api/motion/rotate", HTTP_POST, [&server, &hardware, &idle, &clock]() {
    if (!requireHardware(server, hardware)) return;
    JsonDocument doc;
    if (!readJson(server, doc)) return;
    uint32_t nowMs = millis();
    clock.markInteraction(nowMs);
    idle.pauseForManualControl(nowMs);
    hardware.extendTemporaryRgbOverride(nowMs);
    sendJson(server, motionStateJson(hardware.rotateX(doc["velocity"] | 0)));
  });

  server.on("/api/motion/home", HTTP_POST, [&server, &hardware, &idle, &clock]() {
    if (!requireHardware(server, hardware)) return;
    JsonDocument doc;
    if (!readJson(server, doc)) return;
    uint32_t nowMs = millis();
    clock.markInteraction(nowMs);
    idle.pauseForManualControl(nowMs);
    hardware.extendTemporaryRgbOverride(nowMs);
    sendJson(server, motionStateJson(hardware.home(doc["speed"] | 300)));
  });

  server.on("/api/rgb", HTTP_POST, [&server, &hardware, &idle, &emotion]() {
    if (!requireHardware(server, hardware)) return;
    JsonDocument doc;
    if (!readJson(server, doc)) return;
    uint32_t nowMs = millis();
    bool hasBrightness = doc["brightness"].is<int>();
    bool hasBreathing = doc["breathing"].is<bool>();
    bool hasMarquee = doc["marquee"].is<bool>();
    bool hasRainbow = doc["rainbow"].is<bool>();
    bool hasPreset = doc["preset"].is<const char*>();
    bool hasColor = doc["color"].is<const char*>();
    bool captureStandbyRgb = doc["standby"].is<bool>()
      ? doc["standby"].as<bool>()
      : (doc["temporary"].is<bool>() && !doc["temporary"].as<bool>());
    if (hasPreset) {
      applyRgbPreset(hardware, String(doc["preset"].as<const char*>()), nowMs, emotion.lightsActive());
    }
    if (hasBrightness) {
      hardware.setRgbBrightness((uint8_t)constrain(doc["brightness"].as<int>(), 0, 255));
      hardware.setRgbPresetName("manual");
    }
    if (hasColor || (!hasBrightness && !hasBreathing && !hasMarquee && !hasRainbow && !hasPreset)) {
      String color = hexFromJson(doc["color"], "#000000");
      uint16_t c = parseHexColor(color, 0);
      uint8_t r = (uint8_t)(((c >> 11) & 0x1F) * 255 / 31);
      uint8_t g = (uint8_t)(((c >> 5) & 0x3F) * 255 / 63);
      uint8_t b = (uint8_t)((c & 0x1F) * 255 / 31);
      String side = hexFromJson(doc["side"], "all");
      if (doc["index"].is<int>()) hardware.setRgbOne(doc["index"].as<int>(), r, g, b);
      else if (side == "left") hardware.setRgbLeft(r, g, b);
      else if (side == "right") hardware.setRgbRight(r, g, b);
      else hardware.setRgbAll(r, g, b);
      hardware.setRgbPresetName("manual");
    }
    if (hasBreathing) {
      hardware.setRgbBreathingEnabled(doc["breathing"].as<bool>(), nowMs, emotion.lightsActive());
      hardware.setRgbPresetName("manual");
    }
    if (hasMarquee) {
      hardware.setRgbMarqueeEnabled(doc["marquee"].as<bool>(), nowMs, emotion.lightsActive());
      hardware.setRgbPresetName("manual");
    }
    if (hasRainbow) {
      hardware.setRgbRainbowEnabled(doc["rainbow"].as<bool>(), nowMs, emotion.lightsActive());
      hardware.setRgbPresetName("manual");
    }
    if (captureStandbyRgb) {
      hardware.captureStandbyRgb();
    } else {
      hardware.markTemporaryRgbOverride(nowMs);
      idle.pauseForManualControl(nowMs, RGB_TEMPORARY_RESTORE_MS);
    }
    String json = "{\"ok\":true";
    json += ",\"rgb\":";
    json += hardware.rgbJson();
    json += "}";
    sendJson(server, json);
  });

  server.on("/api/rgb/restore", HTTP_POST, [&server, &hardware]() {
    if (!requireHardware(server, hardware)) return;
    uint32_t nowMs = millis();
    hardware.restoreStandbyRgb(nowMs);
    String json = "{\"ok\":true";
    json += ",\"rgb\":";
    json += hardware.rgbJson();
    json += "}";
    sendJson(server, json);
  });

  server.on("/api/touch", HTTP_GET, [&server, &hardware]() {
    if (!requireHardware(server, hardware)) return;
    sendJson(server, hardware.touchJson());
  });

  server.on("/api/proximity/status", HTTP_GET, [&server, &proximity]() {
    sendJson(server, proximity.statusJson());
  });

  server.on("/api/camera/status", HTTP_GET, [&server, &camera, &faceDetector]() {
    sendJson(server, camera.statusJson(faceDetector));
  });

  server.on("/api/camera/frame.bmp", HTTP_GET, [&server, &faceDetector]() {
    String error;
    if (!faceDetector.streamPreviewBmp(server, error)) {
      int status = error == "face task running" ? 409 : 503;
      sendJsonStatus(server, status, "{\"ok\":false,\"error\":" + jsonQuote(error) + "}");
    }
  });

  server.on("/api/camera/set", HTTP_POST, [&server, &clock, &camera, &faceDetector]() {
    JsonDocument doc;
    if (!readJson(server, doc)) return;
    uint32_t nowMs = millis();
    clock.markInteraction(nowMs);
    if (doc["enabled"].is<bool>()) camera.setEnabled(doc["enabled"].as<bool>());
    if (doc["cameraEnabled"].is<bool>()) camera.setEnabled(doc["cameraEnabled"].as<bool>());
    if (doc["captureIntervalMs"].is<int>()) camera.setCaptureIntervalMs((uint32_t)doc["captureIntervalMs"].as<int>());
    if (doc["detectTimeoutMs"].is<int>()) camera.setDetectTimeoutMs((uint32_t)doc["detectTimeoutMs"].as<int>());
    if (doc["returnSettleMs"].is<int>()) camera.setReturnSettleMs((uint32_t)doc["returnSettleMs"].as<int>());
    if (doc["confirmSettleMs"].is<int>()) camera.setConfirmSettleMs((uint32_t)doc["confirmSettleMs"].as<int>());
    if (doc["lockHoldMs"].is<int>()) camera.setLockHoldMs((uint32_t)doc["lockHoldMs"].as<int>());
    if (doc["cooldownMs"].is<int>()) camera.setCooldownMs((uint32_t)doc["cooldownMs"].as<int>());
    if (doc["centerDeadZoneX"].is<float>() || doc["centerDeadZoneY"].is<float>()) {
      float x = doc["centerDeadZoneX"].is<float>() ? doc["centerDeadZoneX"].as<float>() : 0.10f;
      float y = doc["centerDeadZoneY"].is<float>() ? doc["centerDeadZoneY"].as<float>() : 0.12f;
      camera.setCenterDeadZone(x, y);
    }
    if (doc["yawStepMax"].is<int>() || doc["pitchStepMax"].is<int>()) {
      int yawStep = doc["yawStepMax"].is<int>() ? doc["yawStepMax"].as<int>() : 48;
      int pitchStep = doc["pitchStepMax"].is<int>() ? doc["pitchStepMax"].as<int>() : 28;
      camera.setServoStepMax((int16_t)yawStep, (int16_t)pitchStep);
    }
    if (doc["catchphraseAssetId"].is<const char*>() || doc["listenAfterCatchphrase"].is<bool>()) {
      String assetId = doc["catchphraseAssetId"].is<const char*>() ? String(doc["catchphraseAssetId"].as<const char*>()) : String(camera.catchphraseAssetId());
      bool listenAfter = doc["listenAfterCatchphrase"].is<bool>() ? doc["listenAfterCatchphrase"].as<bool>() : camera.listenAfterCatchphrase();
      camera.setCatchphrase(assetId, listenAfter);
    }
    if (doc["feedbackSubtitleEnabled"].is<bool>()) camera.setFeedbackSubtitleEnabled(doc["feedbackSubtitleEnabled"].as<bool>());
    sendJson(server, camera.statusJson(faceDetector));
  });

  server.on("/api/camera/catchphrase", HTTP_POST, [&server, &camera, &faceDetector]() {
    JsonDocument doc;
    if (!readJson(server, doc)) return;
    String assetId = doc["assetId"].is<const char*>() ? String(doc["assetId"].as<const char*>()) : String(camera.catchphraseAssetId());
    bool listenAfter = doc["listenAfter"].is<bool>() ? doc["listenAfter"].as<bool>() : camera.listenAfterCatchphrase();
    if (doc["listenAfterCatchphrase"].is<bool>()) listenAfter = doc["listenAfterCatchphrase"].as<bool>();
    camera.setCatchphrase(assetId, listenAfter);
    sendJson(server, camera.statusJson(faceDetector));
  });

  server.on("/api/camera/face-report", HTTP_POST, [&server, &hardware, &face, &idle, &emotion, &clock, &mumble, &camera, &faceDetector]() {
    JsonDocument doc;
    if (!readJson(server, doc)) return;
    CameraFaceReport report = {};
    if (doc["scanId"].is<int>()) report.scanId = (uint32_t)doc["scanId"].as<int>();
    if (doc["faceDetected"].is<bool>()) report.faceDetected = doc["faceDetected"].as<bool>();
    if (doc["confidence"].is<float>()) report.confidence = doc["confidence"].as<float>();
    if (doc["faceCenterX"].is<float>()) report.faceCenterX = doc["faceCenterX"].as<float>();
    if (doc["faceCenterY"].is<float>()) report.faceCenterY = doc["faceCenterY"].as<float>();
    if (doc["faceWidth"].is<float>()) report.faceWidth = doc["faceWidth"].as<float>();
    if (doc["faceHeight"].is<float>()) report.faceHeight = doc["faceHeight"].as<float>();
    if (doc["force"].is<bool>()) report.force = doc["force"].as<bool>();
    bool hasYaw = doc["yaw"].is<int>();
    bool hasPitch = doc["pitch"].is<int>();
    if (hasYaw && hasPitch) {
      report.yaw = (int16_t)doc["yaw"].as<int>();
      report.pitch = (int16_t)doc["pitch"].as<int>();
      report.hasPoseOverride = true;
    }
    if (report.faceDetected && !requireHardware(server, hardware)) return;
    bool accepted = camera.reportFace(report, hardware, face, idle, clock, mumble, faceDetector, emotion.active(), millis());
    String json = camera.statusJson(faceDetector);
    json.remove(json.length() - 1);
    json += ",\"accepted\":" + String(accepted ? "true" : "false") + "}";
    sendJson(server, json);
  });

  server.on("/api/camera/wake", HTTP_POST, [&server, &hardware, &face, &idle, &clock, &camera]() {
    if (!requireHardware(server, hardware)) return;
    JsonDocument doc;
    if (!readJson(server, doc)) return;
    uint32_t holdMs = doc["holdMs"].is<int>() ? (uint32_t)doc["holdMs"].as<int>() : 0;
    uint32_t nowMs = millis();
    String source = doc["source"].is<const char*>() ? String(doc["source"].as<const char*>()) : "camera-wake";
    bool listenAfter = doc["listenAfter"].is<bool>() ? doc["listenAfter"].as<bool>() : camera.listenAfterCatchphrase();
    if (doc["listenAfterCatchphrase"].is<bool>()) listenAfter = doc["listenAfterCatchphrase"].as<bool>();
    uint32_t timeoutMs = listenTimeoutMs;
    if (doc["listenTimeoutMs"].is<int>()) timeoutMs = (uint32_t)max(0, doc["listenTimeoutMs"].as<int>());
    String json = camera.handleVoiceWake(hardware, face, idle, clock, nowMs, holdMs);
    String listeningJson = listenAfter ? startListeningAndNotifyJson(face, idle, nowMs, timeoutMs, source) : "{\"ok\":true,\"skipped\":true}";
    bool displayed = showDebugSubtitle(face, "listening", "Listening");
    json.remove(json.length() - 1);
    json += ",\"listening\":";
    json += listeningJson;
    json += ",\"listenAfterCatchphrase\":" + String(listenAfter ? "true" : "false");
    json += ",\"debugSubtitleDisplayed\":" + String(displayed ? "true" : "false");
    json += "}";
    sendJson(server, json);
  });

  server.on("/api/camera/clear", HTTP_POST, [&server, &clock, &camera, &faceDetector]() {
    uint32_t nowMs = millis();
    clock.markInteraction(nowMs);
    camera.clear(faceDetector, nowMs);
    sendJson(server, camera.statusJson(faceDetector));
  });

  server.on("/api/audio/status", HTTP_GET, [&server, &hardware]() {
    if (!requireHardware(server, hardware)) return;
    sendJson(server, hardware.audioStatusJson());
  });

  server.on("/api/audio/tone", HTTP_POST, [&server, &hardware]() {
    if (!requireHardware(server, hardware)) return;
    JsonDocument doc;
    if (!readJson(server, doc)) return;
    sendJson(server, hardware.playToneJson(doc["frequency"] | 880, doc["durationMs"] | 240, doc["volume"] | 96));
  });

  server.on("/api/audio/stop", HTTP_POST, [&server, &hardware]() {
    if (!requireHardware(server, hardware)) return;
    sendJson(server, hardware.stopAudioJson());
  });

  server.on("/api/mumble/status", HTTP_GET, [&server, &mumble]() {
    sendJson(server, mumble.statusJson());
  });

  server.on("/api/mumble/set", HTTP_POST, [&server, &mumble]() {
    JsonDocument doc;
    if (!readJson(server, doc)) return;
    uint32_t nowMs = millis();
    if (doc["enabled"].is<bool>()) mumble.setEnabled(doc["enabled"].as<bool>());
    if (doc["speedPercent"].is<int>()) mumble.setSpeedPercent((uint16_t)doc["speedPercent"].as<int>());
    if (doc["volume"].is<int>()) mumble.setApiVoiceVolume((uint8_t)constrain(doc["volume"].as<int>(), 0, 255));
    if (doc["segmentDurationMs"].is<int>()) mumble.setSegmentDurationMs((uint16_t)doc["segmentDurationMs"].as<int>());
    if (doc["fullChancePercent"].is<int>()) mumble.setFullChancePercent((uint8_t)doc["fullChancePercent"].as<int>());
    if (doc["autoEnabled"].is<bool>()) mumble.setAutoEnabled(doc["autoEnabled"].as<bool>(), nowMs);
    sendJson(server, mumble.statusJson());
  });

  server.on("/api/mumble/play", HTTP_POST, [&server, &face, &hardware, &idle, &emotion, &mumble]() {
    if (!requireHardware(server, hardware)) return;
    JsonDocument doc;
    if (!readJson(server, doc)) return;
    String mode = "random";
    if (doc["mode"].is<const char*>()) mode = String(doc["mode"].as<const char*>());
    uint32_t nowMs = millis();
    idle.pauseForManualControl(nowMs, 2500);
    if (mode == "subtitle") {
      sendJson(server, mumble.playForSubtitleJson(face.subtitle(), nowMs));
    } else if (mode == "emotion") {
      sendJson(server, mumble.playForEmotionJson(nowMs, emotion.durationMs()));
    } else if (mode == "full") {
      sendJson(server, mumble.playFullJson(nowMs));
    } else {
      uint32_t durationMs = doc["durationMs"].is<int>() ? (uint32_t)doc["durationMs"].as<int>() : 0;
      sendJson(server, mumble.playRandomSegmentJson(nowMs, durationMs));
    }
  });

  server.on("/api/mumble/stop", HTTP_POST, [&server, &hardware, &mumble]() {
    if (!requireHardware(server, hardware)) return;
    sendJson(server, mumble.stopJson());
  });

  server.on("/api/mumble/upload", HTTP_POST, [&server, &mumble]() {
    sendJson(server, mumble.uploadStatusJson());
  }, [&server, &mumble]() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      mumble.uploadBegin(upload.filename, upload.totalSize);
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      mumble.uploadWrite(upload.buf, upload.currentSize);
    } else if (upload.status == UPLOAD_FILE_END) {
      mumble.uploadEnd();
    } else if (upload.status == UPLOAD_FILE_ABORTED) {
      mumble.uploadAbort();
    }
  });

  server.on("/api/emotion/sound-slots", HTTP_GET, [&server, &mumble]() {
    sendJson(server, mumble.emotionSlotsJson());
  });

  server.on("/api/emotion/sound-slot/set", HTTP_POST, [&server, &mumble]() {
    JsonDocument doc;
    if (!readJson(server, doc)) return;
    String name = "happy";
    if (doc["slot"].is<const char*>()) name = String(doc["slot"].as<const char*>());
    if (doc["name"].is<const char*>()) name = String(doc["name"].as<const char*>());
    bool enabled = doc["enabled"].is<bool>() ? doc["enabled"].as<bool>() : true;
    if (!mumble.setEmotionSlotEnabled(name, enabled)) {
      sendJsonStatus(server, 400, "{\"ok\":false,\"error\":\"unknown emotion sound slot\"}");
      return;
    }
    sendJson(server, mumble.emotionSlotsJson());
  });

  server.on("/api/emotion/sound-slot/play", HTTP_POST, [&server, &hardware, &idle, &mumble]() {
    if (!requireHardware(server, hardware)) return;
    JsonDocument doc;
    if (!readJson(server, doc)) return;
    String name = "happy";
    if (doc["slot"].is<const char*>()) name = String(doc["slot"].as<const char*>());
    if (doc["name"].is<const char*>()) name = String(doc["name"].as<const char*>());
    uint32_t nowMs = millis();
    idle.pauseForManualControl(nowMs, 2500);
    sendJson(server, mumble.playEmotionSlotJson(name, nowMs, doc["durationMs"] | 0));
  });

  server.on("/api/emotion/sound-slot/upload", HTTP_POST, [&server, &mumble]() {
    String name = server.arg("slot");
    sendJson(server, mumble.emotionSlotUploadStatusJson(name));
  }, [&server, &mumble]() {
    HTTPUpload& upload = server.upload();
    String name = server.arg("slot");
    if (upload.status == UPLOAD_FILE_START) {
      mumble.uploadEmotionSlotBegin(name, upload.filename, upload.totalSize);
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      mumble.uploadEmotionSlotWrite(upload.buf, upload.currentSize);
    } else if (upload.status == UPLOAD_FILE_END) {
      mumble.uploadEmotionSlotEnd();
    } else if (upload.status == UPLOAD_FILE_ABORTED) {
      mumble.uploadEmotionSlotAbort();
    }
  });

  server.on("/api/mic/sample", HTTP_POST, [&server, &hardware]() {
    if (!requireHardware(server, hardware)) return;
    JsonDocument doc;
    if (!readJson(server, doc)) return;
    sendJson(server, hardware.micSampleJson(doc["samples"] | 256, doc["sampleRate"] | 16000));
  });

  server.on("/api/idle/status", HTTP_GET, [&server, &idle]() {
    sendJson(server, idle.statusJson());
  });

  server.on("/api/idle/set", HTTP_POST, [&server, &idle]() {
    JsonDocument doc;
    if (!readJson(server, doc)) return;
    if (doc["enabled"].is<bool>()) idle.setEnabled(doc["enabled"].as<bool>());
    sendJson(server, idle.statusJson());
  });

  server.on("/api/idle/next", HTTP_POST, [&server, &idle]() {
    idle.triggerNext(millis());
    sendJson(server, idle.statusJson());
  });

  server.on("/api/emotion/status", HTTP_GET, [&server, &emotion]() {
    sendJson(server, emotion.statusJson());
  });

  server.on("/api/emotion/play", HTTP_POST, [&server, &hardware, &idle, &emotion, &mumble, &clock]() {
    if (!requireHardware(server, hardware)) return;
    JsonDocument doc;
    if (!readJson(server, doc)) return;
    String name = "happy";
    if (doc["name"].is<const char*>()) name = String(doc["name"].as<const char*>());
    if (doc["action"].is<const char*>()) name = String(doc["action"].as<const char*>());
    bool lightsEnabled = true;
    if (doc["lights"].is<bool>()) lightsEnabled = doc["lights"].as<bool>();
    bool soundEnabled = name != "sleepStartle";
    if (doc["sound"].is<bool>()) soundEnabled = doc["sound"].as<bool>();
    if (!EmotionActionController::isKnownAction(name)) {
      sendJsonStatus(server, 400, "{\"ok\":false,\"error\":\"unknown emotion action\"}");
      return;
    }
    uint32_t nowMs = millis();
    clock.markInteraction(nowMs);
    idle.pauseForManualControl(nowMs, 4000);
    if (emotion.active()) emotion.stop(hardware, nowMs);
    emotion.play(name, nowMs, lightsEnabled);
    String soundJson = soundEnabled ? mumble.playEmotionSlotJson(name, nowMs, emotion.durationMs()) : "{\"ok\":true,\"enabled\":false,\"skipped\":true}";
    String status = emotion.statusJson();
    status.remove(status.length() - 1);
    status += ",\"sound\":" + soundJson + "}";
    sendJson(server, status);
  });

  server.on("/api/emotion/stop", HTTP_POST, [&server, &hardware, &idle, &emotion, &clock]() {
    uint32_t nowMs = millis();
    clock.markInteraction(nowMs);
    emotion.stop(hardware, nowMs);
    idle.pauseForManualControl(nowMs, 1000);
    sendJson(server, emotion.statusJson());
  });

  server.on("/api/triggers/status", HTTP_GET, [&server, &triggers]() {
    sendJson(server, triggers.statusJson());
  });

  server.on("/api/triggers/set", HTTP_POST, [&server, &triggers, &clock]() {
    JsonDocument doc;
    if (!readJson(server, doc)) return;
    uint32_t nowMs = millis();
    clock.markInteraction(nowMs);
    if (doc["headTouchHappyEnabled"].is<bool>()) {
      triggers.setHeadTouchHappyEnabled(doc["headTouchHappyEnabled"].as<bool>());
    }
    if (doc["headTouchComboEnabled"].is<bool>()) {
      triggers.setHeadTouchComboEnabled(doc["headTouchComboEnabled"].as<bool>());
    }
    if (doc["screenTouchAngryEnabled"].is<bool>()) {
      triggers.setScreenTouchAngryEnabled(doc["screenTouchAngryEnabled"].as<bool>());
    }
    if (doc["headTouchThreshold"].is<int>()) {
      triggers.setHeadTouchThreshold((uint16_t)doc["headTouchThreshold"].as<int>());
    }
    if (doc["imuMotionEnabled"].is<bool>()) {
      triggers.setImuMotionEnabled(doc["imuMotionEnabled"].as<bool>());
    }
    if (doc["angryShakeAccelDeltaG"].is<float>() || doc["angryShakeGyroDps"].is<float>() || doc["upsideDownGravityDotThreshold"].is<float>() || doc["upsideDownAccelZThreshold"].is<float>()) {
      float angryShakeAccelDeltaG = triggers.angryShakeAccelDeltaG();
      float angryShakeGyroDps = triggers.angryShakeGyroDps();
      float upsideDownGravityDotThreshold = triggers.upsideDownGravityDotThreshold();
      if (doc["angryShakeAccelDeltaG"].is<float>()) angryShakeAccelDeltaG = doc["angryShakeAccelDeltaG"].as<float>();
      if (doc["angryShakeGyroDps"].is<float>()) angryShakeGyroDps = doc["angryShakeGyroDps"].as<float>();
      if (doc["upsideDownAccelZThreshold"].is<float>()) upsideDownGravityDotThreshold = doc["upsideDownAccelZThreshold"].as<float>();
      if (doc["upsideDownGravityDotThreshold"].is<float>()) upsideDownGravityDotThreshold = doc["upsideDownGravityDotThreshold"].as<float>();
      triggers.setImuMotionThresholds(angryShakeAccelDeltaG, angryShakeGyroDps, upsideDownGravityDotThreshold);
    }
    if (doc["randomEmotionMinIntervalMs"].is<int>() || doc["randomEmotionMaxIntervalMs"].is<int>()) {
      uint32_t minIntervalMs = triggers.randomEmotionMinIntervalMs();
      uint32_t maxIntervalMs = triggers.randomEmotionMaxIntervalMs();
      if (doc["randomEmotionMinIntervalMs"].is<int>()) minIntervalMs = (uint32_t)doc["randomEmotionMinIntervalMs"].as<int>();
      if (doc["randomEmotionMaxIntervalMs"].is<int>()) maxIntervalMs = (uint32_t)doc["randomEmotionMaxIntervalMs"].as<int>();
      triggers.setRandomEmotionIntervalRangeMs(minIntervalMs, maxIntervalMs);
    }
    if (doc["randomEmotionEnabled"].is<bool>()) {
      triggers.setRandomEmotionEnabled(doc["randomEmotionEnabled"].as<bool>());
    }
    sendJson(server, triggers.statusJson());
  });

  server.on("/api/imu", HTTP_GET, [&server, &hardware]() {
    if (!requireHardware(server, hardware)) return;
    sendJson(server, hardware.imuJson());
  });

  server.on("/api/power", HTTP_GET, [&server, &hardware]() {
    if (!requireHardware(server, hardware)) return;
    sendJson(server, hardware.powerJson());
  });

  server.onNotFound([&server]() {
    server.send(404, "application/json", "{\"ok\":false,\"error\":\"not found\"}");
  });
}
