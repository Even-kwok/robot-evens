#include "robot_clock.h"

#if __has_include("config.local.h")
#include "config.local.h"
#endif
#include "config.example.h"

namespace {

constexpr uint32_t TIME_SYNC_CHECK_INTERVAL_MS = 15000;
constexpr uint32_t MIN_SLEEP_AFTER_MS = 60000;
constexpr uint32_t MAX_SLEEP_AFTER_MS = 14400000UL;

String jsonBool(bool value) {
  return value ? "true" : "false";
}

}  // namespace

void RobotClock::begin() {
  lastInteractionMs_ = millis();
  nightAwakeUntilMs_ = 0;
  lastNightAwakeDurationMs_ = 0;
  configTime(ROBOT_TIME_GMT_OFFSET_SEC, ROBOT_TIME_DAYLIGHT_OFFSET_SEC, ROBOT_NTP_SERVER_1, ROBOT_NTP_SERVER_2);
  refreshTimeSync();
}

void RobotClock::update(uint32_t nowMs) {
  if (nowMs - lastSyncCheckMs_ < TIME_SYNC_CHECK_INTERVAL_MS) return;
  lastSyncCheckMs_ = nowMs;
  refreshTimeSync();
}

void RobotClock::setShowDateTimeSubtitle(bool enabled) {
  showDateTimeSubtitle_ = enabled;
}

bool RobotClock::showDateTimeSubtitle() const {
  return showDateTimeSubtitle_;
}

void RobotClock::setNightSettings(bool enabled, uint8_t startHour, uint8_t endHour, uint32_t sleepAfterMs) {
  nightEnabled_ = enabled;
  nightStartHour_ = constrain(startHour, 0, 23);
  nightEndHour_ = constrain(endHour, 0, 23);
  sleepAfterMs_ = constrain(sleepAfterMs, MIN_SLEEP_AFTER_MS, MAX_SLEEP_AFTER_MS);
  if (!nightEnabled_) nightAwakeUntilMs_ = 0;
}

bool RobotClock::nightEnabled() const {
  return nightEnabled_;
}

uint8_t RobotClock::nightStartHour() const {
  return nightStartHour_;
}

uint8_t RobotClock::nightEndHour() const {
  return nightEndHour_;
}

uint32_t RobotClock::sleepAfterMs() const {
  return sleepAfterMs_;
}

void RobotClock::markInteraction(uint32_t nowMs) {
  lastInteractionMs_ = nowMs;
  wakeFromSleep(nowMs);
}

void RobotClock::wakeFromSleep(uint32_t nowMs) {
  sleeping_ = false;
  sleepingSinceMs_ = 0;
  lastInteractionMs_ = nowMs;
}

void RobotClock::startNightAwakePeriod(uint32_t nowMs) {
  if (!isNightNow()) return;
  lastNightAwakeDurationMs_ = chooseNightAwakeDurationMs(nowMs);
  nightAwakeUntilMs_ = nowMs + lastNightAwakeDurationMs_;
  wakeFromSleep(nowMs);
}

void RobotClock::setSleeping(bool sleeping, uint32_t nowMs) {
  if (sleeping_ == sleeping) return;
  sleeping_ = sleeping;
  sleepingSinceMs_ = sleeping ? nowMs : 0;
}

bool RobotClock::sleeping() const {
  return sleeping_;
}

bool RobotClock::timeSynced() const {
  return timeSynced_;
}

bool RobotClock::isNightNow() const {
  if (!nightEnabled_) return false;
  struct tm timeInfo;
  if (!readLocalTime(timeInfo)) return false;
  return isHourInNightWindow((uint8_t)timeInfo.tm_hour, nightStartHour_, nightEndHour_);
}

bool RobotClock::nightAwakeActive(uint32_t nowMs) const {
  return nightAwakeUntilMs_ != 0 && isNightNow() && nowMs < nightAwakeUntilMs_;
}

uint32_t RobotClock::nightAwakeRemainingMs(uint32_t nowMs) const {
  return nightAwakeActive(nowMs) ? nightAwakeUntilMs_ - nowMs : 0;
}

bool RobotClock::shouldSleep(uint32_t nowMs) const {
  if (!isNightNow() || nightAwakeActive(nowMs)) return false;
  uint32_t sleepBaseMs = nightSleepReferenceMs();
  return nowMs >= sleepBaseMs && nowMs - sleepBaseMs >= sleepAfterMs_;
}

uint32_t RobotClock::interactionAgeMs(uint32_t nowMs) const {
  return nowMs >= lastInteractionMs_ ? nowMs - lastInteractionMs_ : 0;
}

String RobotClock::localDateTimeText() const {
  struct tm timeInfo;
  if (!readLocalTime(timeInfo)) return "-- -- --:--";
  char buf[24];
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", &timeInfo);
  return String(buf);
}

String RobotClock::statusJson() const {
  uint32_t nowMs = millis();
  bool nightNow = isNightNow();
  bool nightAwake = nightAwakeActive(nowMs);
  uint32_t sleepingAgeMs = sleeping_ && nowMs >= sleepingSinceMs_ ? nowMs - sleepingSinceMs_ : 0;
  String json = "{\"ok\":true";
  json += ",\"synced\":" + jsonBool(timeSynced_);
  json += ",\"dateTimeText\":\"" + localDateTimeText() + "\"";
  json += ",\"showDateTimeSubtitle\":" + jsonBool(showDateTimeSubtitle_);
  json += ",\"nightEnabled\":" + jsonBool(nightEnabled_);
  json += ",\"nightStartHour\":" + String(nightStartHour_);
  json += ",\"nightEndHour\":" + String(nightEndHour_);
  json += ",\"sleepAfterMs\":" + String(sleepAfterMs_);
  json += ",\"isNight\":" + jsonBool(nightNow);
  json += ",\"interactionAgeMs\":" + String(interactionAgeMs(nowMs));
  json += ",\"sleeping\":" + jsonBool(sleeping_);
  json += ",\"sleepingSinceMs\":" + String(sleepingSinceMs_);
  json += ",\"sleepingAgeMs\":" + String(sleepingAgeMs);
  json += ",\"nightAwakeActive\":" + jsonBool(nightAwake);
  json += ",\"nightAwakeUntilMs\":" + String(nightAwakeUntilMs_);
  json += ",\"nightAwakeRemainingMs\":" + String(nightAwakeRemainingMs(nowMs));
  json += ",\"lastNightAwakeDurationMs\":" + String(lastNightAwakeDurationMs_);
  json += "}";
  return json;
}

bool RobotClock::isHourInNightWindow(uint8_t hour, uint8_t startHour, uint8_t endHour) {
  hour = constrain(hour, 0, 23);
  startHour = constrain(startHour, 0, 23);
  endHour = constrain(endHour, 0, 23);
  if (startHour == endHour) return true;
  if (startHour > endHour) return hour >= startHour || hour < endHour;
  return hour >= startHour && hour < endHour;
}

void RobotClock::refreshTimeSync() {
  struct tm timeInfo;
  timeSynced_ = getLocalTime(&timeInfo);
}

bool RobotClock::readLocalTime(struct tm& timeInfo) const {
  return getLocalTime(&timeInfo);
}

uint32_t RobotClock::chooseNightAwakeDurationMs(uint32_t nowMs) const {
  uint32_t span = NIGHT_AWAKE_MAX_MS - NIGHT_AWAKE_MIN_MS + 1;
  uint32_t noise = nowMs ^ (lastInteractionMs_ * 1103515245UL) ^ (sleepingSinceMs_ * 2654435761UL);
  noise ^= noise >> 16;
  noise *= 2246822519UL;
  noise ^= noise >> 13;
  return NIGHT_AWAKE_MIN_MS + (noise % span);
}

uint32_t RobotClock::nightSleepReferenceMs() const {
  return nightAwakeUntilMs_ > lastInteractionMs_ ? nightAwakeUntilMs_ : lastInteractionMs_;
}
