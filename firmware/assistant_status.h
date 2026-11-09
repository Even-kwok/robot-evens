#pragma once

#include <Arduino.h>
#include "face_renderer.h"
#include "robot_hardware.h"

struct AssistantStatusSnapshot {
  bool ok = false;
  bool sourceOnline = false;
  bool codingActive = false;
  bool codexEnabled = true;
  bool codexAvailable = false;
  bool codexLow = false;
  bool eventActive = false;
  bool batteryLow = false;
  int codexRemainingPercent = -1;
  int batteryPercent = -1;
  int completedToday = 0;
  int lastCompletedToday = 0;
  uint16_t eventTtlSeconds = 12;
  uint16_t eventRemainingSeconds = 0;
  uint32_t durationSeconds = 0;
  String date;
  String time;
  String weekday;
  String codingDisplay;
  String eventType;
  String eventMessage;
  String lastError;
};

class AssistantStatusDisplay {
public:
  void begin(bool enabled, const String& statusUrl, uint32_t pollIntervalMs, uint8_t lowCodexPercent);
  void configure(bool enabled, const String& statusUrl, uint32_t pollIntervalMs);
  void configure(bool enabled, const String& statusUrl, uint32_t pollIntervalMs, bool displayEnabled, uint16_t lineColor, bool batteryEnabled);
  void update(RobotFaceRenderer& face, RobotHardware& hardware, uint32_t nowMs, bool canControlMood);
  String statusJson(uint32_t nowMs) const;
  bool enabled() const;
  bool displayEnabled() const;
  bool batteryEnabled() const;
  uint8_t lowCodexPercent() const;
  uint8_t lowBatteryPercent() const;
  uint16_t lineColor() const;
  String statusUrl() const;
  uint32_t pollIntervalMs() const;
  void setAlertThresholds(uint8_t lowCodexPercent, uint8_t lowBatteryPercent);

private:
  bool enabled_ = false;
  String statusUrl_;
  uint32_t pollIntervalMs_ = 5000;
  uint8_t lowCodexPercent_ = 20;
  uint8_t lowBatteryPercent_ = 20;
  bool displayEnabled_ = true;
  bool batteryEnabled_ = true;
  uint16_t lineColor_ = 0x07EC;
  uint32_t lastPollMs_ = 0;
  uint32_t lastLineMs_ = 0;
  uint8_t lineIndex_ = 0;
  String lastLine_ = "";
  AssistantStatusSnapshot snapshot_;

  bool shouldPoll(uint32_t nowMs) const;
  void fetchStatus(uint32_t nowMs);
  void applyFaceState(RobotFaceRenderer& face, RobotHardware& hardware, uint32_t nowMs, bool canControlMood);
  String currentLine(RobotHardware& hardware, uint32_t nowMs);
  String batteryLine(RobotHardware& hardware) const;
  bool batteryLow(RobotHardware& hardware) const;
};
