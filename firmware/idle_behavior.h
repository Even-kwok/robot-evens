#pragma once

#include <Arduino.h>
#include "face_renderer.h"
#include "robot_clock.h"
#include "robot_hardware.h"

struct IdleStep {
  int16_t yaw;
  int16_t pitch;
  float lookX;
  float lookY;
  float ambientLevel;
  uint16_t durationMs;
  uint16_t speed;
  const char* label;
};

class IdleBehaviorController {
public:
  void begin(bool enabled = true);
  void update(RobotHardware& hardware, RobotFaceRenderer& face, RobotClock& clock, uint32_t nowMs);
  void setEnabled(bool enabled);
  void pauseForManualControl(uint32_t nowMs, uint32_t durationMs = 6000);
  void triggerNext(uint32_t nowMs);
  bool enabled() const;
  const char* currentStepLabel() const;
  bool currentStepWakeableByTouch() const;
  String statusJson() const;

private:
  bool enabled_ = true;
  uint8_t behaviorIndex_ = 0;
  uint8_t stepIndex_ = 0;
  uint32_t nextStepAtMs_ = 0;
  uint32_t pausedUntilMs_ = 0;
  int16_t lastAppliedYaw_ = 0;
  int16_t lastAppliedPitch_ = 120;
  uint32_t nextNightStepAtMs_ = 0;
  uint8_t nightStepIndex_ = 0;
  const char* currentBehaviorName_ = "回中停顿";
  const char* currentStepLabel_ = "待机";
  bool currentStepWakeableByTouch_ = false;

  uint8_t chooseNextBehavior(uint32_t nowMs) const;
  uint16_t naturalPauseJitterMs(uint32_t nowMs) const;
  void applyStep(RobotHardware& hardware, RobotFaceRenderer& face, const IdleStep& step);
  void applyNightStep(RobotHardware& hardware, RobotFaceRenderer& face, RobotClock& clock, uint32_t nowMs);
};
