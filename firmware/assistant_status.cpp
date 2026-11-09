#include "assistant_status.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>

namespace {

String quoteJson(const String& value) {
  String out = "\"";
  for (uint16_t i = 0; i < value.length(); ++i) {
    char c = value.charAt(i);
    if (c == '"' || c == '\\') out += "\\";
    out += c;
  }
  out += "\"";
  return out;
}

String boolText(bool value) {
  return value ? "true" : "false";
}

String colorHex(uint16_t color) {
  uint8_t r = ((color >> 11) & 0x1F) << 3;
  uint8_t g = ((color >> 5) & 0x3F) << 2;
  uint8_t b = (color & 0x1F) << 3;
  char buf[8];
  snprintf(buf, sizeof(buf), "#%02x%02x%02x", r, g, b);
  return String(buf);
}

uint8_t clampPercent(int value, uint8_t fallback) {
  if (value < 1 || value > 100) return fallback;
  return (uint8_t)value;
}

bool eventWantsDead(const String& message) {
  return message.indexOf("FAIL") >= 0 || message.indexOf("BLOCKED") >= 0 || message.indexOf("LOW CODEX") >= 0 || message.indexOf("ERROR") >= 0;
}

bool eventWantsNeutral(const String& message) {
  return message.indexOf("DONE") >= 0 || message.indexOf(" OK") >= 0;
}

String compactDateLine(const AssistantStatusSnapshot& snapshot) {
  String line = snapshot.date;
  if (snapshot.weekday.length()) line += " " + snapshot.weekday;
  if (snapshot.time.length()) line += " " + snapshot.time;
  return line.length() ? line : "STATUS READY";
}

}  // namespace

void AssistantStatusDisplay::begin(bool enabled, const String& statusUrl, uint32_t pollIntervalMs, uint8_t lowCodexPercent) {
  enabled_ = enabled;
  statusUrl_ = statusUrl;
  pollIntervalMs_ = pollIntervalMs > 1000 ? pollIntervalMs : 1000;
  lowCodexPercent_ = clampPercent(lowCodexPercent, lowCodexPercent_);
}

void AssistantStatusDisplay::configure(bool enabled, const String& statusUrl, uint32_t pollIntervalMs) {
  configure(enabled, statusUrl, pollIntervalMs, displayEnabled_, lineColor_, batteryEnabled_);
}

void AssistantStatusDisplay::configure(bool enabled, const String& statusUrl, uint32_t pollIntervalMs, bool displayEnabled, uint16_t lineColor, bool batteryEnabled) {
  enabled_ = enabled;
  statusUrl_ = statusUrl;
  if (pollIntervalMs > 1000) pollIntervalMs_ = pollIntervalMs;
  displayEnabled_ = displayEnabled;
  lineColor_ = lineColor;
  batteryEnabled_ = batteryEnabled;
  lastPollMs_ = 0;
}

bool AssistantStatusDisplay::enabled() const {
  return enabled_;
}

bool AssistantStatusDisplay::displayEnabled() const {
  return displayEnabled_;
}

bool AssistantStatusDisplay::batteryEnabled() const {
  return batteryEnabled_;
}

uint8_t AssistantStatusDisplay::lowCodexPercent() const {
  return lowCodexPercent_;
}

uint8_t AssistantStatusDisplay::lowBatteryPercent() const {
  return lowBatteryPercent_;
}

uint16_t AssistantStatusDisplay::lineColor() const {
  return lineColor_;
}

String AssistantStatusDisplay::statusUrl() const {
  return statusUrl_;
}

uint32_t AssistantStatusDisplay::pollIntervalMs() const {
  return pollIntervalMs_;
}

void AssistantStatusDisplay::setAlertThresholds(uint8_t lowCodexPercent, uint8_t lowBatteryPercent) {
  lowCodexPercent_ = clampPercent(lowCodexPercent, lowCodexPercent_);
  lowBatteryPercent_ = clampPercent(lowBatteryPercent, lowBatteryPercent_);
}

bool AssistantStatusDisplay::shouldPoll(uint32_t nowMs) const {
  if (!enabled_) return false;
  if (!statusUrl_.length()) return false;
  if (WiFi.status() != WL_CONNECTED) return false;
  return lastPollMs_ == 0 || nowMs - lastPollMs_ >= pollIntervalMs_;
}

void AssistantStatusDisplay::fetchStatus(uint32_t nowMs) {
  lastPollMs_ = nowMs;
  HTTPClient http;
  http.begin(statusUrl_);
  http.setTimeout(2500);
  int code = http.GET();
  if (code < 200 || code >= 300) {
    snapshot_.sourceOnline = false;
    snapshot_.ok = false;
    snapshot_.lastError = "http " + String(code);
    http.end();
    return;
  }
  String body = http.getString();
  http.end();

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    snapshot_.sourceOnline = false;
    snapshot_.ok = false;
    snapshot_.lastError = String("json ") + err.c_str();
    return;
  }

  snapshot_.sourceOnline = true;
  snapshot_.ok = doc["ok"] | false;
  snapshot_.date = String(doc["date"] | "");
  snapshot_.time = String(doc["time"] | "");
  snapshot_.weekday = String(doc["weekday"] | "");
  snapshot_.codingActive = doc["coding"]["active"] | false;
  snapshot_.durationSeconds = doc["coding"]["durationSeconds"] | 0;
  snapshot_.codingDisplay = String(doc["coding"]["display"] | "");
  snapshot_.codexEnabled = doc["codex"]["enabled"] | true;
  snapshot_.codexAvailable = doc["codex"]["available"] | false;
  snapshot_.codexRemainingPercent = snapshot_.codexAvailable ? (int)(doc["codex"]["remainingPercent"] | -1) : -1;
  if (doc["settings"]["eventTtlSeconds"].is<int>()) {
    snapshot_.eventTtlSeconds = (uint16_t)max(1, min(120, doc["settings"]["eventTtlSeconds"].as<int>()));
  }
  if (doc["settings"]["lowCodexPercent"].is<int>()) {
    lowCodexPercent_ = clampPercent(doc["settings"]["lowCodexPercent"].as<int>(), lowCodexPercent_);
  }
  snapshot_.codexLow = snapshot_.codexAvailable && snapshot_.codexRemainingPercent >= 0 && snapshot_.codexRemainingPercent <= lowCodexPercent_;
  snapshot_.lastCompletedToday = snapshot_.completedToday;
  snapshot_.completedToday = doc["tasks"]["completedToday"] | 0;
  snapshot_.eventActive = doc["event"]["active"] | false;
  snapshot_.eventType = String(doc["event"]["type"] | "");
  snapshot_.eventMessage = String(doc["event"]["message"] | "");
  snapshot_.eventTtlSeconds = doc["event"]["ttlSeconds"] | snapshot_.eventTtlSeconds;
  snapshot_.eventRemainingSeconds = doc["event"]["remainingSeconds"] | 0;
  if (snapshot_.eventMessage.length() > 40) snapshot_.eventMessage = snapshot_.eventMessage.substring(0, 40);
  snapshot_.lastError = "";
}

bool AssistantStatusDisplay::batteryLow(RobotHardware& hardware) const {
  if (!hardware.isReady()) return false;
  int percent = hardware.batteryPercent();
  return percent >= 0 && percent <= lowBatteryPercent_;
}

String AssistantStatusDisplay::batteryLine(RobotHardware& hardware) const {
  if (!hardware.isReady()) return "BAT --";
  float voltage = hardware.batteryVoltage();
  int percent = hardware.batteryPercent();
  if (percent >= 0) return "BAT " + String(percent) + "% " + String(voltage, 2) + "V";
  return "BAT " + String(voltage, 2) + "V";
}

String AssistantStatusDisplay::currentLine(RobotHardware& hardware, uint32_t nowMs) {
  if (!snapshot_.sourceOnline && lastPollMs_ != 0) return "STATUS OFFLINE";
  if (snapshot_.eventActive && snapshot_.eventMessage.length()) return snapshot_.eventMessage;
  if (batteryLow(hardware)) return "BAT LOW " + String(hardware.batteryPercent()) + "%";
  uint8_t lineCount = batteryEnabled_ ? 5 : 4;
  if (nowMs - lastLineMs_ >= 2500) {
    lastLineMs_ = nowMs;
    lineIndex_ = (lineIndex_ + 1) % lineCount;
  }
  if (lineIndex_ == 0) return compactDateLine(snapshot_);
  if (lineIndex_ == 1) return String("CODING ") + (snapshot_.codingDisplay.length() ? snapshot_.codingDisplay : "0m");
  if (lineIndex_ == 2) {
    if (!snapshot_.codexEnabled || !snapshot_.codexAvailable) return "CODEX --%";
    String line = "CODEX " + String(snapshot_.codexRemainingPercent) + "%";
    if (snapshot_.codexLow) line += " LOW";
    return line;
  }
  if (lineIndex_ == 3) return "DONE " + String(snapshot_.completedToday) + " TASKS";
  return batteryLine(hardware);
}

void AssistantStatusDisplay::applyFaceState(RobotFaceRenderer& face, RobotHardware& hardware, uint32_t nowMs, bool canControlMood) {
  String line = currentLine(hardware, nowMs);
  lastLine_ = line;
  snapshot_.batteryPercent = hardware.isReady() ? hardware.batteryPercent() : -1;
  snapshot_.batteryLow = batteryLow(hardware);
  if (canControlMood) {
    if (!snapshot_.sourceOnline && lastPollMs_ != 0) {
      face.setMood("confused");
    } else if ((snapshot_.eventActive && eventWantsDead(snapshot_.eventMessage)) || snapshot_.batteryLow || snapshot_.codexLow) {
      face.setMood("dead");
    } else if (snapshot_.eventActive && snapshot_.eventMessage.indexOf("WAIT") >= 0) {
      face.setMood("curious");
    } else if (snapshot_.eventActive && eventWantsNeutral(snapshot_.eventMessage)) {
      face.setMood("neutral");
    } else if (snapshot_.completedToday > snapshot_.lastCompletedToday) {
      face.setMood("neutral");
    } else if (snapshot_.codingActive) {
      face.setMood("thinking");
    }
  }
  face.setAssistantStatusLine(line, displayEnabled_, lineColor_);
}

void AssistantStatusDisplay::update(RobotFaceRenderer& face, RobotHardware& hardware, uint32_t nowMs, bool canControlMood) {
  if (!enabled_) {
    face.clearAssistantStatusLine();
    return;
  }
  if (shouldPoll(nowMs)) fetchStatus(nowMs);
  applyFaceState(face, hardware, nowMs, canControlMood);
}

String AssistantStatusDisplay::statusJson(uint32_t nowMs) const {
  String json = "{\"ok\":true";
  json += ",\"enabled\":" + boolText(enabled_);
  json += ",\"displayEnabled\":" + boolText(displayEnabled_);
  json += ",\"batteryEnabled\":" + boolText(batteryEnabled_);
  json += ",\"lineColor\":" + quoteJson(colorHex(lineColor_));
  json += ",\"statusUrl\":" + quoteJson(statusUrl_);
  json += ",\"pollIntervalMs\":" + String(pollIntervalMs_);
  json += ",\"lastPollAgeMs\":" + String(lastPollMs_ == 0 ? 0 : nowMs - lastPollMs_);
  json += ",\"sourceOnline\":" + boolText(snapshot_.sourceOnline);
  json += ",\"lastLine\":" + quoteJson(lastLine_);
  json += ",\"eventActive\":" + boolText(snapshot_.eventActive);
  json += ",\"eventType\":" + quoteJson(snapshot_.eventType);
  json += ",\"eventMessage\":" + quoteJson(snapshot_.eventMessage);
  json += ",\"eventTtlSeconds\":" + String(snapshot_.eventTtlSeconds);
  json += ",\"eventRemainingSeconds\":" + String(snapshot_.eventRemainingSeconds);
  json += ",\"lowCodexPercent\":" + String(lowCodexPercent_);
  json += ",\"lowBatteryPercent\":" + String(lowBatteryPercent_);
  json += ",\"batteryPercent\":" + String(snapshot_.batteryPercent);
  json += ",\"batteryLow\":" + boolText(snapshot_.batteryLow);
  json += ",\"codexLow\":" + boolText(snapshot_.codexLow);
  json += ",\"completedToday\":" + String(snapshot_.completedToday);
  json += ",\"lastError\":" + quoteJson(snapshot_.lastError);
  json += "}";
  return json;
}
