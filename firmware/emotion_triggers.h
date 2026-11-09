#pragma once

#include <Arduino.h>
#include "emotion_actions.h"
#include "idle_behavior.h"
#include "robot_hardware.h"
#include "robot_clock.h"

class MumblePlayer;

class EmotionTriggerController {
public:
  void begin();
  void setHeadTouchHappyEnabled(bool enabled);
  void setScreenTouchAngryEnabled(bool enabled);
  void setHeadTouchComboEnabled(bool enabled);
  void setHeadTouchThreshold(uint16_t threshold);
  void setImuMotionEnabled(bool enabled);
  void setImuMotionThresholds(float angryShakeAccelDeltaG, float angryShakeGyroDps, float upsideDownGravityDotThreshold);
  void setRandomEmotionEnabled(bool enabled);
  void setRandomEmotionIntervalRangeMs(uint32_t minIntervalMs, uint32_t maxIntervalMs);
  bool headTouchHappyEnabled() const;
  bool screenTouchAngryEnabled() const;
  bool headTouchComboEnabled() const;
  bool imuMotionEnabled() const;
  float angryShakeAccelDeltaG() const;
  float angryShakeGyroDps() const;
  float upsideDownGravityDotThreshold() const;
  bool randomEmotionEnabled() const;
  uint32_t randomEmotionMinIntervalMs() const;
  uint32_t randomEmotionMaxIntervalMs() const;
  void update(RobotHardware& hardware, EmotionActionController& emotion, IdleBehaviorController& idle, MumblePlayer& mumble, RobotClock& clock, uint32_t nowMs);
  String statusJson() const;

private:
  bool headTouchHappyEnabled_ = true;
  bool headTouchWasPressed_ = false;
  bool headTouchPressed_ = false;
  uint8_t headTouchStableFrames_ = 0;
  uint16_t headTouchThreshold_ = 2;
  int headTouchPeak_ = 0;
  uint32_t nextHeadTouchHappyAtMs_ = 0;
  bool headTouchComboEnabled_ = true;
  bool headTouchComboWasRawPressed_ = false;
  uint8_t headTouchComboCount_ = 0;
  uint32_t headTouchComboWindowStartedAtMs_ = 0;
  uint32_t nextHeadTouchComboAtMs_ = 0;
  uint32_t headTouchComboRepeatUntilMs_ = 0;
  bool screenTouchAngryEnabled_ = true;
  bool screenTouchWasPressed_ = false;
  bool screenTouchPressed_ = false;
  int screenTouchX_ = -1;
  int screenTouchY_ = -1;
  uint8_t screenTouchCount_ = 0;
  uint32_t nextScreenTouchAngryAtMs_ = 0;
  bool imuMotionEnabled_ = true;
  bool imuAvailable_ = false;
  bool imuUpsideDown_ = false;
  bool hasPreviousImuSample_ = false;
  bool imuGravityReferenceCaptured_ = false;
  float imuAccelX_ = 0.0f;
  float imuAccelY_ = 0.0f;
  float imuAccelZ_ = 0.0f;
  float imuGravityRefX_ = 0.0f;
  float imuGravityRefY_ = 0.0f;
  float imuGravityRefZ_ = 1.0f;
  float imuPreviousAccelX_ = 0.0f;
  float imuPreviousAccelY_ = 0.0f;
  float imuPreviousAccelZ_ = 0.0f;
  float imuGyroX_ = 0.0f;
  float imuGyroY_ = 0.0f;
  float imuGyroZ_ = 0.0f;
  float imuAccelerationMagnitudeG_ = 0.0f;
  float imuVectorDeltaG_ = 0.0f;
  float imuShakeAccelerationDeltaG_ = 0.0f;
  float imuGyroMagnitudeDps_ = 0.0f;
  float imuGravityDot_ = 1.0f;
  float angryShakeAccelDeltaG_ = 0.55f;
  float angryShakeGyroDps_ = 220.0f;
  float upsideDownGravityDotThreshold_ = -0.65f;
  bool randomEmotionEnabled_ = false;
  uint32_t randomEmotionMinIntervalMs_ = 15000;
  uint32_t randomEmotionMaxIntervalMs_ = 45000;
  uint32_t nextRandomEmotionAtMs_ = 0;
  uint32_t lastRandomEmotionDelayMs_ = 0;
  uint32_t lastTriggeredAtMs_ = 0;
  const char* lastTrigger_ = "none";
  const char* lastRandomEmotion_ = "none";

  const char* chooseRandomEmotionAction(uint32_t nowMs) const;
  uint32_t chooseNextRandomEmotionDelayMs(uint32_t nowMs) const;
  void scheduleNextRandomEmotion(uint32_t nowMs);
  void resetHeadTouchCombo();
  bool headTouchComboRepeatWindowActive(uint32_t nowMs) const;
  void extendHeadTouchComboRepeatWindow(uint32_t nowMs);
  bool trackHeadTouchCombo(uint32_t nowMs);
  void calibrateImuGravityReference(const ImuSample& imu);
  bool canTriggerContinuousImuAngry(const EmotionActionController& emotion) const;
};
