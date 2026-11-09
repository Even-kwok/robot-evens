#pragma once

#include <Arduino.h>
#include <time.h>

constexpr uint8_t DEFAULT_NIGHT_START_HOUR = 22;
constexpr uint8_t DEFAULT_NIGHT_END_HOUR = 8;
constexpr uint32_t DEFAULT_NIGHT_SLEEP_AFTER_MS = 1800000UL;
constexpr uint32_t NIGHT_AWAKE_MIN_MS = 600000UL;
constexpr uint32_t NIGHT_AWAKE_MAX_MS = 1200000UL;

class RobotClock {
public:
  void begin();
  void update(uint32_t nowMs);
  void setShowDateTimeSubtitle(bool enabled);
  bool showDateTimeSubtitle() const;
  void setNightSettings(bool enabled, uint8_t startHour, uint8_t endHour, uint32_t sleepAfterMs);
  bool nightEnabled() const;
  uint8_t nightStartHour() const;
  uint8_t nightEndHour() const;
  uint32_t sleepAfterMs() const;
  void markInteraction(uint32_t nowMs);
  void wakeFromSleep(uint32_t nowMs);
  void startNightAwakePeriod(uint32_t nowMs);
  void setSleeping(bool sleeping, uint32_t nowMs);
  bool sleeping() const;
  bool timeSynced() const;
  bool isNightNow() const;
  bool nightAwakeActive(uint32_t nowMs) const;
  uint32_t nightAwakeRemainingMs(uint32_t nowMs) const;
  bool shouldSleep(uint32_t nowMs) const;
  uint32_t interactionAgeMs(uint32_t nowMs) const;
  String localDateTimeText() const;
  String statusJson() const;
  static bool isHourInNightWindow(uint8_t hour, uint8_t startHour, uint8_t endHour);

private:
  bool showDateTimeSubtitle_ = false;
  bool nightEnabled_ = true;
  uint8_t nightStartHour_ = DEFAULT_NIGHT_START_HOUR;
  uint8_t nightEndHour_ = DEFAULT_NIGHT_END_HOUR;
  uint32_t sleepAfterMs_ = DEFAULT_NIGHT_SLEEP_AFTER_MS;
  uint32_t lastInteractionMs_ = 0;
  uint32_t lastSyncCheckMs_ = 0;
  bool timeSynced_ = false;
  bool sleeping_ = false;
  uint32_t sleepingSinceMs_ = 0;
  uint32_t nightAwakeUntilMs_ = 0;
  uint32_t lastNightAwakeDurationMs_ = 0;

  void refreshTimeSync();
  bool readLocalTime(struct tm& timeInfo) const;
  uint32_t chooseNightAwakeDurationMs(uint32_t nowMs) const;
  uint32_t nightSleepReferenceMs() const;
};
