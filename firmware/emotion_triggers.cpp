#include "emotion_triggers.h"

#include "mumble_player.h"

#include <math.h>

namespace {

constexpr uint32_t HEAD_TOUCH_HAPPY_COOLDOWN_MS = 0;
constexpr uint32_t HEAD_TOUCH_COMBO_WINDOW_MS = 10000;
constexpr uint8_t HEAD_TOUCH_COMBO_TARGET_COUNT = 5;
constexpr uint32_t HEAD_TOUCH_COMBO_REPEAT_WINDOW_MS = 5000;
constexpr uint32_t HEAD_TOUCH_COMBO_IDLE_PAUSE_MS = 8000;
constexpr uint32_t HEAD_TOUCH_PROXIMITY_SEARCH_IDLE_PAUSE_MS = 6200;
constexpr uint32_t SCREEN_TOUCH_ANGRY_COOLDOWN_MS = 0;
constexpr float ANGRY_SHAKE_ACCEL_DELTA_G = 0.55f;
constexpr float ANGRY_SHAKE_GYRO_DPS = 220.0f;
constexpr float UPSIDE_DOWN_GRAVITY_DOT_THRESHOLD = -0.65f;
constexpr float MIN_SHAKE_ACCEL_DELTA_G = 0.02f;
constexpr float MAX_SHAKE_ACCEL_DELTA_G = 3.00f;
constexpr float MIN_SHAKE_GYRO_DPS = 5.0f;
constexpr float MAX_SHAKE_GYRO_DPS = 1200.0f;
constexpr float MIN_UPSIDE_DOWN_GRAVITY_DOT_THRESHOLD = -1.00f;
constexpr float MAX_UPSIDE_DOWN_GRAVITY_DOT_THRESHOLD = -0.05f;
constexpr float MIN_GRAVITY_REFERENCE_ACCEL_G = 0.50f;
constexpr float MAX_GRAVITY_REFERENCE_ACCEL_G = 1.50f;
constexpr uint32_t IMU_ANGRY_SHAKE_COOLDOWN_MS = 0;
constexpr uint16_t DEFAULT_HEAD_TOUCH_THRESHOLD = 2;
constexpr uint16_t MIN_HEAD_TOUCH_THRESHOLD = 1;
constexpr uint16_t MAX_HEAD_TOUCH_THRESHOLD = 30;
constexpr uint8_t HEAD_TOUCH_STABLE_FRAMES = 2;
constexpr uint32_t RANDOM_EMOTION_MIN_INTERVAL_MS = 10000;
constexpr uint32_t RANDOM_EMOTION_MAX_INTERVAL_MS = 120000;
constexpr uint32_t DEFAULT_RANDOM_EMOTION_MIN_INTERVAL_MS = 15000;
constexpr uint32_t DEFAULT_RANDOM_EMOTION_MAX_INTERVAL_MS = 45000;
constexpr uint32_t RANDOM_EMOTION_IDLE_PAUSE_MS = 6000;

const char* const RANDOM_EMOTION_ACTIONS[] = {
  "happy",
  "sad",
  "angry",
  "surprised",
  "thinking",
  "shy",
  "cute",
  "proud",
  "confused",
  "scared",
  "celebrate",
};

constexpr uint8_t RANDOM_EMOTION_ACTION_COUNT = sizeof(RANDOM_EMOTION_ACTIONS) / sizeof(RANDOM_EMOTION_ACTIONS[0]);

}  // namespace

void EmotionTriggerController::begin() {
  headTouchHappyEnabled_ = true;
  headTouchWasPressed_ = false;
  headTouchPressed_ = false;
  headTouchStableFrames_ = 0;
  headTouchThreshold_ = DEFAULT_HEAD_TOUCH_THRESHOLD;
  headTouchPeak_ = 0;
  nextHeadTouchHappyAtMs_ = 0;
  headTouchComboEnabled_ = true;
  headTouchComboWasRawPressed_ = false;
  headTouchComboCount_ = 0;
  headTouchComboWindowStartedAtMs_ = 0;
  nextHeadTouchComboAtMs_ = 0;
  headTouchComboRepeatUntilMs_ = 0;
  screenTouchAngryEnabled_ = true;
  screenTouchWasPressed_ = false;
  screenTouchPressed_ = false;
  screenTouchX_ = -1;
  screenTouchY_ = -1;
  screenTouchCount_ = 0;
  nextScreenTouchAngryAtMs_ = 0;
  imuMotionEnabled_ = true;
  imuAvailable_ = false;
  imuUpsideDown_ = false;
  hasPreviousImuSample_ = false;
  imuGravityReferenceCaptured_ = false;
  imuAccelX_ = 0.0f;
  imuAccelY_ = 0.0f;
  imuAccelZ_ = 0.0f;
  imuGravityRefX_ = 0.0f;
  imuGravityRefY_ = 0.0f;
  imuGravityRefZ_ = 1.0f;
  imuPreviousAccelX_ = 0.0f;
  imuPreviousAccelY_ = 0.0f;
  imuPreviousAccelZ_ = 0.0f;
  imuGyroX_ = 0.0f;
  imuGyroY_ = 0.0f;
  imuGyroZ_ = 0.0f;
  imuAccelerationMagnitudeG_ = 0.0f;
  imuVectorDeltaG_ = 0.0f;
  imuShakeAccelerationDeltaG_ = 0.0f;
  imuGyroMagnitudeDps_ = 0.0f;
  imuGravityDot_ = 1.0f;
  angryShakeAccelDeltaG_ = ANGRY_SHAKE_ACCEL_DELTA_G;
  angryShakeGyroDps_ = ANGRY_SHAKE_GYRO_DPS;
  upsideDownGravityDotThreshold_ = UPSIDE_DOWN_GRAVITY_DOT_THRESHOLD;
  randomEmotionEnabled_ = false;
  randomEmotionMinIntervalMs_ = DEFAULT_RANDOM_EMOTION_MIN_INTERVAL_MS;
  randomEmotionMaxIntervalMs_ = DEFAULT_RANDOM_EMOTION_MAX_INTERVAL_MS;
  nextRandomEmotionAtMs_ = 0;
  lastRandomEmotionDelayMs_ = 0;
  lastTriggeredAtMs_ = 0;
  lastTrigger_ = "none";
  lastRandomEmotion_ = "none";
  if (randomEmotionEnabled_) scheduleNextRandomEmotion(millis());
}

void EmotionTriggerController::setHeadTouchHappyEnabled(bool enabled) {
  headTouchHappyEnabled_ = enabled;
  if (!enabled) {
    headTouchWasPressed_ = false;
    headTouchPressed_ = false;
    headTouchStableFrames_ = 0;
  }
}

void EmotionTriggerController::setHeadTouchComboEnabled(bool enabled) {
  headTouchComboEnabled_ = enabled;
  if (!enabled) {
    headTouchComboWasRawPressed_ = false;
    headTouchComboRepeatUntilMs_ = 0;
    resetHeadTouchCombo();
  }
}

void EmotionTriggerController::setScreenTouchAngryEnabled(bool enabled) {
  screenTouchAngryEnabled_ = enabled;
  if (!enabled) {
    screenTouchWasPressed_ = false;
    screenTouchPressed_ = false;
    screenTouchX_ = -1;
    screenTouchY_ = -1;
    screenTouchCount_ = 0;
  }
}

void EmotionTriggerController::setHeadTouchThreshold(uint16_t threshold) {
  headTouchThreshold_ = constrain(threshold, MIN_HEAD_TOUCH_THRESHOLD, MAX_HEAD_TOUCH_THRESHOLD);
  headTouchStableFrames_ = 0;
  headTouchWasPressed_ = false;
  headTouchPressed_ = false;
  headTouchComboWasRawPressed_ = false;
  headTouchComboRepeatUntilMs_ = 0;
  resetHeadTouchCombo();
}

void EmotionTriggerController::setImuMotionEnabled(bool enabled) {
  imuMotionEnabled_ = enabled;
}

void EmotionTriggerController::setImuMotionThresholds(float angryShakeAccelDeltaG, float angryShakeGyroDps, float upsideDownGravityDotThreshold) {
  angryShakeAccelDeltaG_ = constrain(angryShakeAccelDeltaG, MIN_SHAKE_ACCEL_DELTA_G, MAX_SHAKE_ACCEL_DELTA_G);
  angryShakeGyroDps_ = constrain(angryShakeGyroDps, MIN_SHAKE_GYRO_DPS, MAX_SHAKE_GYRO_DPS);
  upsideDownGravityDotThreshold_ = constrain(upsideDownGravityDotThreshold, MIN_UPSIDE_DOWN_GRAVITY_DOT_THRESHOLD, MAX_UPSIDE_DOWN_GRAVITY_DOT_THRESHOLD);
}

void EmotionTriggerController::setRandomEmotionEnabled(bool enabled) {
  randomEmotionEnabled_ = enabled;
  if (enabled) {
    scheduleNextRandomEmotion(millis());
  } else {
    nextRandomEmotionAtMs_ = 0;
    lastRandomEmotionDelayMs_ = 0;
    lastRandomEmotion_ = "none";
  }
}

void EmotionTriggerController::setRandomEmotionIntervalRangeMs(uint32_t minIntervalMs, uint32_t maxIntervalMs) {
  minIntervalMs = constrain(minIntervalMs, RANDOM_EMOTION_MIN_INTERVAL_MS, RANDOM_EMOTION_MAX_INTERVAL_MS);
  maxIntervalMs = constrain(maxIntervalMs, RANDOM_EMOTION_MIN_INTERVAL_MS, RANDOM_EMOTION_MAX_INTERVAL_MS);
  if (maxIntervalMs < minIntervalMs) {
    uint32_t swapped = minIntervalMs;
    minIntervalMs = maxIntervalMs;
    maxIntervalMs = swapped;
  }
  randomEmotionMinIntervalMs_ = minIntervalMs;
  randomEmotionMaxIntervalMs_ = maxIntervalMs;
  if (randomEmotionEnabled_) scheduleNextRandomEmotion(millis());
}

bool EmotionTriggerController::headTouchHappyEnabled() const {
  return headTouchHappyEnabled_;
}

bool EmotionTriggerController::screenTouchAngryEnabled() const {
  return screenTouchAngryEnabled_;
}

bool EmotionTriggerController::headTouchComboEnabled() const {
  return headTouchComboEnabled_;
}

bool EmotionTriggerController::imuMotionEnabled() const {
  return imuMotionEnabled_;
}

float EmotionTriggerController::angryShakeAccelDeltaG() const {
  return angryShakeAccelDeltaG_;
}

float EmotionTriggerController::angryShakeGyroDps() const {
  return angryShakeGyroDps_;
}

float EmotionTriggerController::upsideDownGravityDotThreshold() const {
  return upsideDownGravityDotThreshold_;
}

bool EmotionTriggerController::randomEmotionEnabled() const {
  return randomEmotionEnabled_;
}

uint32_t EmotionTriggerController::randomEmotionMinIntervalMs() const {
  return randomEmotionMinIntervalMs_;
}

uint32_t EmotionTriggerController::randomEmotionMaxIntervalMs() const {
  return randomEmotionMaxIntervalMs_;
}

void EmotionTriggerController::calibrateImuGravityReference(const ImuSample& imu) {
  if (imuGravityReferenceCaptured_) return;
  if (!imu.available) return;
  if (imu.accelMagnitudeG < MIN_GRAVITY_REFERENCE_ACCEL_G || imu.accelMagnitudeG > MAX_GRAVITY_REFERENCE_ACCEL_G) return;
  float invMagnitude = 1.0f / imu.accelMagnitudeG;
  imuGravityRefX_ = imu.accelX * invMagnitude;
  imuGravityRefY_ = imu.accelY * invMagnitude;
  imuGravityRefZ_ = imu.accelZ * invMagnitude;
  imuGravityReferenceCaptured_ = true;
}

void EmotionTriggerController::update(RobotHardware& hardware, EmotionActionController& emotion, IdleBehaviorController& idle, MumblePlayer& mumble, RobotClock& clock, uint32_t nowMs) {
  if (!hardware.isReady()) return;

  TouchState touch = hardware.readTouchState();
  headTouchPeak_ = max(max(touch.front, touch.middle), touch.back);
  bool headTouchRawPressed = headTouchPeak_ >= headTouchThreshold_;
  if (headTouchRawPressed) {
    if (headTouchStableFrames_ < HEAD_TOUCH_STABLE_FRAMES) headTouchStableFrames_++;
  } else {
    headTouchStableFrames_ = 0;
  }
  headTouchPressed_ = headTouchStableFrames_ >= HEAD_TOUCH_STABLE_FRAMES;
  bool risingEdge = headTouchPressed_ && !headTouchWasPressed_;
  headTouchWasPressed_ = headTouchPressed_;

  ScreenTouchState screenTouch = hardware.readScreenTouchState();
  screenTouchPressed_ = screenTouch.pressed;
  screenTouchX_ = screenTouch.x;
  screenTouchY_ = screenTouch.y;
  screenTouchCount_ = screenTouch.count;
  bool screenRisingEdge = screenTouchPressed_ && !screenTouchWasPressed_;
  screenTouchWasPressed_ = screenTouchPressed_;

  ImuSample imu = hardware.readImuSample();
  imuAvailable_ = imu.available;
  imuAccelX_ = imu.accelX;
  imuAccelY_ = imu.accelY;
  imuAccelZ_ = imu.accelZ;
  imuGyroX_ = imu.gyroX;
  imuGyroY_ = imu.gyroY;
  imuGyroZ_ = imu.gyroZ;
  imuAccelerationMagnitudeG_ = imu.accelMagnitudeG;
  imuGyroMagnitudeDps_ = imu.gyroMagnitudeDps;
  if (imuAvailable_ && imuAccelerationMagnitudeG_ > 0.0f) {
    calibrateImuGravityReference(imu);
    float invMagnitude = 1.0f / imuAccelerationMagnitudeG_;
    float gravityX = imuAccelX_ * invMagnitude;
    float gravityY = imuAccelY_ * invMagnitude;
    float gravityZ = imuAccelZ_ * invMagnitude;
    imuGravityDot_ = gravityX * imuGravityRefX_ + gravityY * imuGravityRefY_ + gravityZ * imuGravityRefZ_;
    if (hasPreviousImuSample_) {
      float dx = imuAccelX_ - imuPreviousAccelX_;
      float dy = imuAccelY_ - imuPreviousAccelY_;
      float dz = imuAccelZ_ - imuPreviousAccelZ_;
      imuVectorDeltaG_ = sqrtf(dx * dx + dy * dy + dz * dz);
    } else {
      imuVectorDeltaG_ = 0.0f;
    }
    imuShakeAccelerationDeltaG_ = imuVectorDeltaG_;
    imuPreviousAccelX_ = imuAccelX_;
    imuPreviousAccelY_ = imuAccelY_;
    imuPreviousAccelZ_ = imuAccelZ_;
    hasPreviousImuSample_ = true;
  } else {
    hasPreviousImuSample_ = false;
    imuVectorDeltaG_ = 0.0f;
    imuShakeAccelerationDeltaG_ = 0.0f;
    imuGravityDot_ = 1.0f;
  }
  imuUpsideDown_ = imuAvailable_ && imuGravityReferenceCaptured_ && imuGravityDot_ <= upsideDownGravityDotThreshold_;
  bool imuHardShakeActive = imuShakeAccelerationDeltaG_ >= angryShakeAccelDeltaG_ || imuGyroMagnitudeDps_ >= angryShakeGyroDps_;

  if (risingEdge || screenRisingEdge || (imuUpsideDown_ && imuHardShakeActive)) {
    clock.markInteraction(nowMs);
  }

  const bool angryEmotionActive = emotion.isActiveAction("angry");
  if (angryEmotionActive) {
    headTouchComboRepeatUntilMs_ = 0;
    resetHeadTouchCombo();
  }

  if ((risingEdge || screenRisingEdge) && idle.currentStepWakeableByTouch()) {
    if (emotion.active()) emotion.stop(hardware, nowMs);
    idle.pauseForManualControl(nowMs, 4000);
    emotion.play("sleepStartle", nowMs, true);
    clock.startNightAwakePeriod(nowMs);
    lastTrigger_ = "sleepStartle";
    lastTriggeredAtMs_ = nowMs;
    headTouchComboRepeatUntilMs_ = 0;
    resetHeadTouchCombo();
    if (randomEmotionEnabled_) scheduleNextRandomEmotion(nowMs);
    return;
  }

  if (screenTouchAngryEnabled_ && screenRisingEdge) {
    if (emotion.active()) emotion.stop(hardware, nowMs);
    idle.pauseForManualControl(nowMs, SCREEN_TOUCH_ANGRY_COOLDOWN_MS);
    emotion.play("angry", nowMs, true);
    mumble.playEmotionSlotJson("angry", nowMs, emotion.durationMs());
    clock.startNightAwakePeriod(nowMs);
    lastTrigger_ = "screenTouchAngry";
    lastTriggeredAtMs_ = nowMs;
    nextScreenTouchAngryAtMs_ = 0;
    if (randomEmotionEnabled_) scheduleNextRandomEmotion(nowMs);
    return;
  }

  if (imuMotionEnabled_ && imuAvailable_ && imuUpsideDown_ && imuHardShakeActive && canTriggerContinuousImuAngry(emotion)) {
    idle.pauseForManualControl(nowMs, IMU_ANGRY_SHAKE_COOLDOWN_MS);
    emotion.play("angry", nowMs, true);
    mumble.playEmotionSlotJson("angry", nowMs, emotion.durationMs());
    lastTrigger_ = "imuAngryShake";
    lastTriggeredAtMs_ = nowMs;
    if (randomEmotionEnabled_) scheduleNextRandomEmotion(nowMs);
    return;
  }

  if (!angryEmotionActive && headTouchHappyEnabled_ && risingEdge) {
    if (trackHeadTouchCombo(nowMs)) {
      if (emotion.active()) emotion.stop(hardware, nowMs);
      idle.pauseForManualControl(nowMs, HEAD_TOUCH_COMBO_IDLE_PAUSE_MS);
      emotion.play("headTouchComboSpin", nowMs, true);
      mumble.playEmotionSlotJson("headTouchComboSpin", nowMs, emotion.durationMs());
      clock.startNightAwakePeriod(nowMs);
      lastTrigger_ = "headTouchComboSpin";
      lastTriggeredAtMs_ = nowMs;
      nextHeadTouchComboAtMs_ = 0;
      resetHeadTouchCombo();
      if (randomEmotionEnabled_) scheduleNextRandomEmotion(nowMs);
      return;
    }
    if (emotion.active()) emotion.stop(hardware, nowMs);
    idle.pauseForManualControl(nowMs, HEAD_TOUCH_HAPPY_COOLDOWN_MS);
    emotion.play("comfortableNod", nowMs, true);
    mumble.playEmotionSlotJson("comfortableNod", nowMs, emotion.durationMs());
    clock.startNightAwakePeriod(nowMs);
    lastTrigger_ = "headTouchComfortableNod";
    lastTriggeredAtMs_ = nowMs;
    nextHeadTouchHappyAtMs_ = 0;
    if (randomEmotionEnabled_) scheduleNextRandomEmotion(nowMs);
    return;
  }

  if (!randomEmotionEnabled_) return;
  if (emotion.active()) return;
  if (nowMs < nextRandomEmotionAtMs_) return;

  const char* actionName = chooseRandomEmotionAction(nowMs);
  idle.pauseForManualControl(nowMs, RANDOM_EMOTION_IDLE_PAUSE_MS);
  emotion.play(actionName, nowMs, true);
  mumble.playEmotionSlotJson(actionName, nowMs, emotion.durationMs());
  lastTrigger_ = "randomEmotion";
  lastRandomEmotion_ = actionName;
  lastTriggeredAtMs_ = nowMs;
  scheduleNextRandomEmotion(nowMs);
}

String EmotionTriggerController::statusJson() const {
  uint32_t nowMs = millis();
  uint32_t cooldownMs = nowMs < nextHeadTouchHappyAtMs_ ? nextHeadTouchHappyAtMs_ - nowMs : 0;
  uint32_t headTouchComboCooldownMs = nowMs < nextHeadTouchComboAtMs_ ? nextHeadTouchComboAtMs_ - nowMs : 0;
  uint32_t headTouchComboElapsedMs = headTouchComboCount_ > 0 ? nowMs - headTouchComboWindowStartedAtMs_ : 0;
  bool headTouchComboWindowActive = headTouchComboCount_ > 0 && headTouchComboElapsedMs < HEAD_TOUCH_COMBO_WINDOW_MS;
  uint8_t headTouchComboVisibleCount = headTouchComboWindowActive ? headTouchComboCount_ : 0;
  uint32_t headTouchComboWindowRemainingMs = headTouchComboWindowActive ? HEAD_TOUCH_COMBO_WINDOW_MS - headTouchComboElapsedMs : 0;
  bool comboRepeatWindowActive = headTouchComboRepeatWindowActive(nowMs);
  uint32_t headTouchComboRepeatWindowRemainingMs = comboRepeatWindowActive ? headTouchComboRepeatUntilMs_ - nowMs : 0;
  uint32_t screenTouchCooldownMs = nowMs < nextScreenTouchAngryAtMs_ ? nextScreenTouchAngryAtMs_ - nowMs : 0;
  uint32_t nextRandomEmotionInMs = randomEmotionEnabled_ && nowMs < nextRandomEmotionAtMs_ ? nextRandomEmotionAtMs_ - nowMs : 0;
  String json = "{\"ok\":true";
  json += ",\"headTouchHappyEnabled\":" + String(headTouchHappyEnabled_ ? "true" : "false");
  json += ",\"headTouchPressed\":" + String(headTouchPressed_ ? "true" : "false");
  json += ",\"headTouchThreshold\":" + String(headTouchThreshold_);
  json += ",\"headTouchPeak\":" + String(headTouchPeak_);
  json += ",\"headTouchComboEnabled\":" + String(headTouchComboEnabled_ ? "true" : "false");
  json += ",\"headTouchComboCount\":" + String(headTouchComboVisibleCount);
  json += ",\"headTouchComboTargetCount\":" + String(HEAD_TOUCH_COMBO_TARGET_COUNT);
  json += ",\"headTouchComboWindowMs\":" + String(HEAD_TOUCH_COMBO_WINDOW_MS);
  json += ",\"headTouchComboWindowActive\":" + String(headTouchComboWindowActive ? "true" : "false");
  json += ",\"headTouchComboWindowRemainingMs\":" + String(headTouchComboWindowRemainingMs);
  json += ",\"headTouchComboCooldownMs\":" + String(headTouchComboCooldownMs);
  json += ",\"headTouchComboRepeatWindowMs\":" + String(HEAD_TOUCH_COMBO_REPEAT_WINDOW_MS);
  json += ",\"headTouchComboRepeatWindowActive\":" + String(comboRepeatWindowActive ? "true" : "false");
  json += ",\"headTouchComboRepeatWindowRemainingMs\":" + String(headTouchComboRepeatWindowRemainingMs);
  json += ",\"screenTouchAngryEnabled\":" + String(screenTouchAngryEnabled_ ? "true" : "false");
  json += ",\"screenTouchPressed\":" + String(screenTouchPressed_ ? "true" : "false");
  json += ",\"screenTouchX\":" + String(screenTouchX_);
  json += ",\"screenTouchY\":" + String(screenTouchY_);
  json += ",\"screenTouchCount\":" + String(screenTouchCount_);
  json += ",\"imuMotionEnabled\":" + String(imuMotionEnabled_ ? "true" : "false");
  json += ",\"imuAvailable\":" + String(imuAvailable_ ? "true" : "false");
  json += ",\"imuAccelX\":" + String(imuAccelX_, 3);
  json += ",\"imuAccelY\":" + String(imuAccelY_, 3);
  json += ",\"imuAccelZ\":" + String(imuAccelZ_, 3);
  json += ",\"imuGyroX\":" + String(imuGyroX_, 1);
  json += ",\"imuGyroY\":" + String(imuGyroY_, 1);
  json += ",\"imuGyroZ\":" + String(imuGyroZ_, 1);
  json += ",\"imuAccelerationMagnitudeG\":" + String(imuAccelerationMagnitudeG_, 3);
  json += ",\"imuVectorDeltaG\":" + String(imuVectorDeltaG_, 3);
  json += ",\"imuShakeAccelerationDeltaG\":" + String(imuShakeAccelerationDeltaG_, 3);
  json += ",\"imuGyroMagnitudeDps\":" + String(imuGyroMagnitudeDps_, 1);
  json += ",\"imuGravityReferenceCaptured\":" + String(imuGravityReferenceCaptured_ ? "true" : "false");
  json += ",\"imuGravityDot\":" + String(imuGravityDot_, 3);
  json += ",\"imuUpsideDown\":" + String(imuUpsideDown_ ? "true" : "false");
  json += ",\"angryShakeAccelDeltaG\":" + String(angryShakeAccelDeltaG_, 3);
  json += ",\"angryShakeGyroDps\":" + String(angryShakeGyroDps_, 1);
  json += ",\"upsideDownGravityDotThreshold\":" + String(upsideDownGravityDotThreshold_, 3);
  json += ",\"upsideDownAccelZThreshold\":" + String(upsideDownGravityDotThreshold_, 3);
  json += ",\"imuAngryShakeCooldownMs\":" + String(IMU_ANGRY_SHAKE_COOLDOWN_MS);
  json += ",\"randomEmotionEnabled\":" + String(randomEmotionEnabled_ ? "true" : "false");
  json += ",\"randomEmotionMinIntervalMs\":" + String(randomEmotionMinIntervalMs_);
  json += ",\"randomEmotionMaxIntervalMs\":" + String(randomEmotionMaxIntervalMs_);
  json += ",\"lastRandomEmotionDelayMs\":" + String(lastRandomEmotionDelayMs_);
  json += ",\"nextRandomEmotionInMs\":" + String(nextRandomEmotionInMs);
  json += ",\"threshold\":" + String(headTouchThreshold_);
  json += ",\"stableFrames\":" + String(headTouchStableFrames_);
  json += ",\"condition\":\"摸头\"";
  json += ",\"action\":\"舒服点点头\"";
  json += ",\"lastTrigger\":\"" + String(lastTrigger_) + "\"";
  json += ",\"lastRandomEmotion\":\"" + String(lastRandomEmotion_) + "\"";
  json += ",\"lastTriggeredAtMs\":" + String(lastTriggeredAtMs_);
  json += ",\"cooldownMs\":" + String(cooldownMs);
  json += ",\"screenTouchCooldownMs\":" + String(screenTouchCooldownMs);
  json += "}";
  return json;
}

const char* EmotionTriggerController::chooseRandomEmotionAction(uint32_t nowMs) const {
  uint8_t index = (uint8_t)((nowMs / 997 + lastTriggeredAtMs_ / 389 + randomEmotionMinIntervalMs_ / 1000 + randomEmotionMaxIntervalMs_ / 3000) % RANDOM_EMOTION_ACTION_COUNT);
  return RANDOM_EMOTION_ACTIONS[index];
}

uint32_t EmotionTriggerController::chooseNextRandomEmotionDelayMs(uint32_t nowMs) const {
  if (randomEmotionMaxIntervalMs_ <= randomEmotionMinIntervalMs_) return randomEmotionMinIntervalMs_;
  uint32_t span = randomEmotionMaxIntervalMs_ - randomEmotionMinIntervalMs_ + 1;
  uint32_t noise = nowMs ^ (lastTriggeredAtMs_ * 1103515245UL) ^ ((uint32_t)headTouchPeak_ * 2654435761UL);
  noise ^= noise >> 16;
  noise *= 2246822519UL;
  noise ^= noise >> 13;
  return randomEmotionMinIntervalMs_ + (noise % span);
}

void EmotionTriggerController::scheduleNextRandomEmotion(uint32_t nowMs) {
  lastRandomEmotionDelayMs_ = chooseNextRandomEmotionDelayMs(nowMs);
  nextRandomEmotionAtMs_ = nowMs + lastRandomEmotionDelayMs_;
}

void EmotionTriggerController::resetHeadTouchCombo() {
  headTouchComboCount_ = 0;
  headTouchComboWindowStartedAtMs_ = 0;
}

bool EmotionTriggerController::headTouchComboRepeatWindowActive(uint32_t nowMs) const {
  return headTouchComboRepeatUntilMs_ != 0 && nowMs <= headTouchComboRepeatUntilMs_;
}

void EmotionTriggerController::extendHeadTouchComboRepeatWindow(uint32_t nowMs) {
  headTouchComboRepeatUntilMs_ = nowMs + HEAD_TOUCH_COMBO_REPEAT_WINDOW_MS;
}

bool EmotionTriggerController::trackHeadTouchCombo(uint32_t nowMs) {
  if (!headTouchComboEnabled_) return false;
  if (headTouchComboRepeatWindowActive(nowMs)) {
    extendHeadTouchComboRepeatWindow(nowMs);
    resetHeadTouchCombo();
    return true;
  }
  if (headTouchComboRepeatUntilMs_ != 0 && nowMs > headTouchComboRepeatUntilMs_) {
    headTouchComboRepeatUntilMs_ = 0;
    resetHeadTouchCombo();
  }
  if (nowMs < nextHeadTouchComboAtMs_) {
    resetHeadTouchCombo();
    return false;
  }
  if (headTouchComboCount_ == 0 || nowMs - headTouchComboWindowStartedAtMs_ > HEAD_TOUCH_COMBO_WINDOW_MS) {
    headTouchComboWindowStartedAtMs_ = nowMs;
    headTouchComboCount_ = 1;
  } else if (headTouchComboCount_ < HEAD_TOUCH_COMBO_TARGET_COUNT) {
    headTouchComboCount_++;
  }
  if (headTouchComboCount_ >= HEAD_TOUCH_COMBO_TARGET_COUNT) {
    extendHeadTouchComboRepeatWindow(nowMs);
    return true;
  }
  return false;
}

bool EmotionTriggerController::canTriggerContinuousImuAngry(const EmotionActionController& emotion) const {
  return !emotion.active();
}
