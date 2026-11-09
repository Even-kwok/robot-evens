#include "robot_hardware.h"
#include "firmware_version.h"

#include <M5Unified.hpp>
#include <M5StackChan.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <WiFi.h>

namespace {

constexpr uint8_t LTR553_I2C_ADDRESS = 0x23;
constexpr uint32_t LTR553_I2C_FREQ = 100000;
constexpr uint8_t LTR553_REG_ALS_CONTR = 0x80;
constexpr uint8_t LTR553_REG_PS_CONTR = 0x81;
constexpr uint8_t LTR553_REG_PS_LED = 0x82;
constexpr uint8_t LTR553_REG_PS_N_PULSES = 0x83;
constexpr uint8_t LTR553_REG_PS_MEAS_RATE = 0x84;
constexpr uint8_t LTR553_REG_ALS_MEAS_RATE = 0x85;
constexpr uint8_t LTR553_REG_ALS_DATA_CH1_0 = 0x88;
constexpr uint8_t LTR553_REG_PS_DATA_0 = 0x8D;
constexpr uint16_t LTR553_DEFAULT_NEAR_RAW = 160;
constexpr uint32_t RGB_BREATH_CYCLE_MS = 2400;
constexpr uint32_t RGB_BREATH_FRAME_MS = 40;
constexpr uint8_t RGB_BREATH_MIN_BRIGHTNESS = 24;
constexpr uint8_t RGB_BREATH_MAX_BRIGHTNESS = 255;
constexpr uint32_t RGB_MARQUEE_FRAME_MS = 110;
constexpr uint32_t MIC_SAMPLE_MIN_INTERVAL_MS = 180;

String color565ToHex(uint16_t color) {
  uint8_t r = (uint8_t)(((color >> 11) & 0x1F) * 255 / 31);
  uint8_t g = (uint8_t)(((color >> 5) & 0x3F) * 255 / 63);
  uint8_t b = (uint8_t)((color & 0x1F) * 255 / 31);
  char buf[8];
  snprintf(buf, sizeof(buf), "#%02x%02x%02x", r, g, b);
  return String(buf);
}

String rgbColorToHex(const RgbColor& color) {
  char buf[8];
  snprintf(buf, sizeof(buf), "#%02x%02x%02x", color.r, color.g, color.b);
  return String(buf);
}

RgbColor rainbowColor(uint8_t position) {
  position = position % RGB_LED_COUNT;
  const RgbColor colors[RGB_LED_COUNT] = {
    {255, 0, 0},
    {255, 96, 0},
    {255, 220, 0},
    {96, 255, 0},
    {0, 255, 80},
    {0, 255, 220},
    {0, 180, 255},
    {0, 72, 255},
    {80, 0, 255},
    {180, 0, 255},
    {255, 0, 180},
    {255, 0, 72},
  };
  return colors[position];
}

String jsonEscape(const String& value) {
  String out;
  out.reserve(value.length() + 8);
  for (size_t i = 0; i < value.length(); ++i) {
    uint8_t c = (uint8_t)value[i];
    switch (c) {
      case '\\': out += "\\\\"; break;
      case '"': out += "\\\""; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default:
        if (c < 0x20) {
          char buf[7];
          snprintf(buf, sizeof(buf), "\\u%04x", c);
          out += buf;
        } else {
          out += (char)c;
        }
        break;
    }
  }
  return out;
}

}  // namespace

void RobotHardware::begin() {
  restPitch_ = DEFAULT_REST_PITCH;
  lastMotion_ = {0, DEFAULT_REST_PITCH, 0, 0};
  rgbConfigRevision_ = 0;
  rgbBrightness_ = DEFAULT_RGB_BRIGHTNESS;
  standbyRgbBrightness_ = DEFAULT_RGB_BRIGHTNESS;
  rgbEffectActive_ = false;
  rgbTemporaryEffectActive_ = false;
  rgbTemporaryRestorePending_ = false;
  speechLightActive_ = false;
  rgbBreathingEnabled_ = true;
  rgbMarqueeEnabled_ = false;
  rgbRainbowEnabled_ = false;
  standbyRgbBreathingEnabled_ = true;
  standbyRgbMarqueeEnabled_ = false;
  standbyRgbRainbowEnabled_ = false;
  rgbBreathingStartedMs_ = 0;
  lastRgbBreathingFrameMs_ = 0;
  lastRgbMarqueeFrameMs_ = 0;
  rgbTemporaryRestoreAtMs_ = 0;
  rgbMarqueeOffset_ = 0;
  rgbPresetName_ = "manual";
  standbyRgbPresetName_ = "manual";
  for (uint8_t index = 0; index < RGB_LED_COUNT; ++index) {
    configuredRgb_[index] = DEFAULT_RGB_COLOR;
    displayedRgb_[index] = DEFAULT_RGB_COLOR;
    standbyRgb_[index] = DEFAULT_RGB_COLOR;
  }
  proximitySensorAvailable_ = false;
  proximitySensorEnabled_ = false;
  lastProximityState_ = {};
  beginProximitySensor();
  updateRgbBreathing(0, false);
}

void RobotHardware::beginProximitySensor() {
  proximitySensorAvailable_ = M5.In_I2C.scanID(LTR553_I2C_ADDRESS, LTR553_I2C_FREQ);
  proximitySensorEnabled_ = false;
  if (!proximitySensorAvailable_) return;

  bool ok = true;
  ok &= M5.In_I2C.writeRegister8(LTR553_I2C_ADDRESS, LTR553_REG_PS_LED, 0x7F, LTR553_I2C_FREQ);
  ok &= M5.In_I2C.writeRegister8(LTR553_I2C_ADDRESS, LTR553_REG_PS_N_PULSES, 0x08, LTR553_I2C_FREQ);
  ok &= M5.In_I2C.writeRegister8(LTR553_I2C_ADDRESS, LTR553_REG_PS_MEAS_RATE, 0x03, LTR553_I2C_FREQ);
  ok &= M5.In_I2C.writeRegister8(LTR553_I2C_ADDRESS, LTR553_REG_ALS_MEAS_RATE, 0x03, LTR553_I2C_FREQ);
  ok &= M5.In_I2C.writeRegister8(LTR553_I2C_ADDRESS, LTR553_REG_ALS_CONTR, 0x01, LTR553_I2C_FREQ);
  ok &= M5.In_I2C.writeRegister8(LTR553_I2C_ADDRESS, LTR553_REG_PS_CONTR, 0x03, LTR553_I2C_FREQ);
  proximitySensorEnabled_ = ok;
}

void RobotHardware::setInitStatus(const char* status) {
  initStatus_ = status;
}

void RobotHardware::markReady() {
  ready_ = true;
  initStatus_ = "ready";
}

bool RobotHardware::isReady() const {
  return ready_;
}

String RobotHardware::unavailableJson() const {
  String json = "{\"ok\":false";
  json += ",\"error\":\"robot hardware not ready\"";
  json += ",\"hardware\":{\"ready\":false";
  json += ",\"initStatus\":\"" + String(initStatus_) + "\"";
  json += "}}";
  return json;
}

MotionState RobotHardware::pose(int yaw, int pitch, int speed) {
  yaw = constrain(yaw, YAW_MIN, YAW_MAX);
  pitch = constrain(pitch, PITCH_MIN, PITCH_MAX);
  speed = constrain(speed, SPEED_MIN, SPEED_MAX);
  M5StackChan.Motion.move(yaw, pitch, speed);
  lastMotion_ = {yaw, pitch, speed, 0};
  return lastMotion_;
}

MotionState RobotHardware::poseWithRestPitchOffset(int yaw, int pitch, int speed) {
  return pose(yaw, restPitchOffset(pitch), speed);
}

MotionState RobotHardware::pitchWithRestPitchOffset(int pitch, int speed) {
  pitch = restPitchOffset(pitch);
  speed = constrain(speed, SPEED_MIN, SPEED_MAX);
  M5StackChan.Motion.movePitch(pitch, speed);
  lastMotion_.pitch = pitch;
  lastMotion_.speed = speed;
  return lastMotion_;
}

MotionState RobotHardware::prepareYawRotation() {
  M5StackChan.Motion.stop();
  lastMotion_.velocity = 0;
  return lastMotion_;
}

MotionState RobotHardware::rotateX(int velocity) {
  velocity = constrain(velocity, ROTATE_MIN, ROTATE_MAX);
  M5StackChan.Motion.rotateX(velocity);
  lastMotion_.velocity = velocity;
  return lastMotion_;
}

MotionState RobotHardware::home(int speed) {
  speed = constrain(speed, SPEED_MIN, SPEED_MAX);
  M5StackChan.Motion.goHome(speed);
  lastMotion_ = {0, 0, speed, 0};
  return lastMotion_;
}

void RobotHardware::setRestPitch(int pitch) {
  restPitch_ = constrain(pitch, PITCH_MIN, PITCH_MAX);
}

int RobotHardware::restPitch() const {
  return restPitch_;
}

int RobotHardware::restPitchOffset(int pitch) const {
  return constrain(restPitch_ + (pitch - DEFAULT_REST_PITCH), PITCH_MIN, PITCH_MAX);
}

void RobotHardware::writeRgb(uint8_t index, const RgbColor& color) {
  if (index >= RGB_LED_COUNT) return;
  displayedRgb_[index] = color;
  RgbColor scaled = scaleRgb(color, rgbBrightness_);
  M5StackChan.setRgbColor(index, scaled.r, scaled.g, scaled.b);
}

void RobotHardware::refreshRgb() {
  M5StackChan.refreshRgb();
}

RgbColor RobotHardware::scaleRgb(const RgbColor& color, uint8_t brightness) {
  return {
    (uint8_t)((uint16_t)color.r * brightness / 255),
    (uint8_t)((uint16_t)color.g * brightness / 255),
    (uint8_t)((uint16_t)color.b * brightness / 255),
  };
}

void RobotHardware::setRgbBrightness(uint8_t brightness) {
  if (rgbBrightness_ != brightness) {
    rgbConfigRevision_++;
  }
  rgbBrightness_ = brightness;
  if (rgbEffectActive_) {
    for (uint8_t index = 0; index < RGB_LED_COUNT; ++index) {
      RgbColor scaled = scaleRgb(displayedRgb_[index], rgbBrightness_);
      M5StackChan.setRgbColor(index, scaled.r, scaled.g, scaled.b);
    }
    refreshRgb();
    return;
  }
  restoreConfiguredRgb();
}

uint8_t RobotHardware::rgbBrightness() const {
  return rgbBrightness_;
}

void RobotHardware::setRgbBreathingEnabled(bool enabled, uint32_t nowMs, bool blockedByTemporaryLights) {
  bool changed = rgbBreathingEnabled_ != enabled;
  if (enabled && (rgbMarqueeEnabled_ || rgbRainbowEnabled_)) changed = true;
  if (changed) rgbConfigRevision_++;
  rgbBreathingEnabled_ = enabled;
  if (enabled) {
    rgbMarqueeEnabled_ = false;
    rgbRainbowEnabled_ = false;
    rgbBreathingStartedMs_ = nowMs;
    lastRgbBreathingFrameMs_ = 0;
    updateRgbBreathing(nowMs, blockedByTemporaryLights);
    return;
  }
  lastRgbBreathingFrameMs_ = 0;
  if (!blockedByTemporaryLights && !speechLightActive_) {
    restoreConfiguredRgb();
  }
}

bool RobotHardware::rgbBreathingEnabled() const {
  return rgbBreathingEnabled_;
}

void RobotHardware::setRgbMarqueeEnabled(bool enabled, uint32_t nowMs, bool blockedByTemporaryLights) {
  bool changed = rgbMarqueeEnabled_ != enabled;
  if (enabled && (rgbBreathingEnabled_ || rgbRainbowEnabled_)) changed = true;
  if (changed) rgbConfigRevision_++;
  rgbMarqueeEnabled_ = enabled;
  if (enabled) {
    rgbBreathingEnabled_ = false;
    rgbRainbowEnabled_ = false;
    lastRgbBreathingFrameMs_ = 0;
    lastRgbMarqueeFrameMs_ = 0;
    rgbMarqueeOffset_ = 0;
    updateRgbMarquee(nowMs, blockedByTemporaryLights);
    return;
  }
  lastRgbMarqueeFrameMs_ = 0;
  if (!rgbBreathingEnabled_ && !rgbRainbowEnabled_ && !blockedByTemporaryLights && !speechLightActive_) {
    restoreConfiguredRgb();
  }
}

bool RobotHardware::rgbMarqueeEnabled() const {
  return rgbMarqueeEnabled_;
}

void RobotHardware::setRgbRainbowEnabled(bool enabled, uint32_t nowMs, bool blockedByTemporaryLights) {
  bool changed = rgbRainbowEnabled_ != enabled;
  if (enabled && (rgbBreathingEnabled_ || rgbMarqueeEnabled_)) changed = true;
  if (changed) rgbConfigRevision_++;
  rgbRainbowEnabled_ = enabled;
  if (enabled) {
    rgbBreathingEnabled_ = false;
    rgbMarqueeEnabled_ = false;
    lastRgbBreathingFrameMs_ = 0;
    lastRgbMarqueeFrameMs_ = 0;
    if (!blockedByTemporaryLights && !speechLightActive_) showRgbRainbow();
    return;
  }
  if (!rgbBreathingEnabled_ && !rgbMarqueeEnabled_ && !blockedByTemporaryLights && !speechLightActive_) {
    restoreConfiguredRgb();
  }
}

bool RobotHardware::rgbRainbowEnabled() const {
  return rgbRainbowEnabled_;
}

void RobotHardware::updateRgbBreathing(uint32_t nowMs, bool blockedByTemporaryLights) {
  if (!rgbBreathingEnabled_) return;
  if (blockedByTemporaryLights || speechLightActive_) return;
  if (lastRgbBreathingFrameMs_ != 0 && nowMs - lastRgbBreathingFrameMs_ < RGB_BREATH_FRAME_MS) return;
  lastRgbBreathingFrameMs_ = nowMs;

  uint32_t phase = (nowMs - rgbBreathingStartedMs_) % RGB_BREATH_CYCLE_MS;
  uint32_t halfCycle = RGB_BREATH_CYCLE_MS / 2;
  uint32_t ramp = phase < halfCycle ? phase : (RGB_BREATH_CYCLE_MS - phase);
  uint8_t breathBrightness = RGB_BREATH_MIN_BRIGHTNESS +
      (uint8_t)((uint32_t)(RGB_BREATH_MAX_BRIGHTNESS - RGB_BREATH_MIN_BRIGHTNESS) * ramp / halfCycle);

  RgbColor frame[RGB_LED_COUNT];
  for (uint8_t index = 0; index < RGB_LED_COUNT; ++index) {
    frame[index] = scaleRgb(configuredRgb_[index], breathBrightness);
  }
  setRgbEffectFrame(frame, false);
}

void RobotHardware::updateRgbMarquee(uint32_t nowMs, bool blockedByTemporaryLights) {
  if (!rgbMarqueeEnabled_) return;
  if (blockedByTemporaryLights || speechLightActive_) return;
  if (lastRgbMarqueeFrameMs_ != 0 && nowMs - lastRgbMarqueeFrameMs_ < RGB_MARQUEE_FRAME_MS) return;
  lastRgbMarqueeFrameMs_ = nowMs;

  RgbColor frame[RGB_LED_COUNT];
  for (uint8_t index = 0; index < RGB_LED_COUNT; ++index) {
    frame[index] = rainbowColor(index + rgbMarqueeOffset_);
  }
  rgbMarqueeOffset_ = (uint8_t)((rgbMarqueeOffset_ + 1) % RGB_LED_COUNT);
  setRgbEffectFrame(frame, false);
}

void RobotHardware::showRgbRainbow() {
  RgbColor frame[RGB_LED_COUNT];
  for (uint8_t index = 0; index < RGB_LED_COUNT; ++index) {
    frame[index] = rainbowColor(index);
  }
  setRgbEffectFrame(frame, false);
}

void RobotHardware::updateRgbManualEffects(uint32_t nowMs, bool blockedByTemporaryLights) {
  if (rgbTemporaryEffectActive_) return;
  if (rgbMarqueeEnabled_) {
    updateRgbMarquee(nowMs, blockedByTemporaryLights);
    return;
  }
  if (rgbBreathingEnabled_) {
    updateRgbBreathing(nowMs, blockedByTemporaryLights);
    return;
  }
  if (rgbRainbowEnabled_ && !blockedByTemporaryLights && !speechLightActive_) {
    showRgbRainbow();
  }
}

void RobotHardware::setRgbPresetName(const char* presetName) {
  const char* normalized = presetName == nullptr ? "manual" : presetName;
  if (strcmp(rgbPresetName_, normalized) != 0) {
    rgbConfigRevision_++;
  }
  rgbPresetName_ = normalized;
}

void RobotHardware::captureStandbyRgb() {
  rgbTemporaryRestorePending_ = false;
  rgbTemporaryRestoreAtMs_ = 0;
  standbyRgbBrightness_ = rgbBrightness_;
  standbyRgbBreathingEnabled_ = rgbBreathingEnabled_;
  standbyRgbMarqueeEnabled_ = rgbMarqueeEnabled_;
  standbyRgbRainbowEnabled_ = rgbRainbowEnabled_;
  standbyRgbPresetName_ = rgbPresetName_ == nullptr ? "manual" : rgbPresetName_;
  for (uint8_t index = 0; index < RGB_LED_COUNT; ++index) {
    standbyRgb_[index] = configuredRgb_[index];
  }
}

void RobotHardware::restoreStandbyRgb(uint32_t nowMs) {
  bool configChanged = rgbBrightness_ != standbyRgbBrightness_ ||
      rgbBreathingEnabled_ != standbyRgbBreathingEnabled_ ||
      rgbMarqueeEnabled_ != standbyRgbMarqueeEnabled_ ||
      rgbRainbowEnabled_ != standbyRgbRainbowEnabled_ ||
      strcmp(rgbPresetName_ == nullptr ? "manual" : rgbPresetName_, standbyRgbPresetName_ == nullptr ? "manual" : standbyRgbPresetName_) != 0;

  for (uint8_t index = 0; index < RGB_LED_COUNT; ++index) {
    if (configuredRgb_[index].r != standbyRgb_[index].r ||
        configuredRgb_[index].g != standbyRgb_[index].g ||
        configuredRgb_[index].b != standbyRgb_[index].b) {
      configChanged = true;
      break;
    }
  }

  if (!configChanged && !rgbTemporaryEffectActive_ && !speechLightActive_ && !rgbTemporaryRestorePending_) return;

  rgbTemporaryRestorePending_ = false;
  rgbTemporaryRestoreAtMs_ = 0;
  if (configChanged) rgbConfigRevision_++;
  for (uint8_t index = 0; index < RGB_LED_COUNT; ++index) {
    configuredRgb_[index] = standbyRgb_[index];
  }
  rgbBrightness_ = standbyRgbBrightness_;
  rgbBreathingEnabled_ = standbyRgbBreathingEnabled_;
  rgbMarqueeEnabled_ = standbyRgbMarqueeEnabled_;
  rgbRainbowEnabled_ = standbyRgbRainbowEnabled_;
  rgbPresetName_ = standbyRgbPresetName_ == nullptr ? "manual" : standbyRgbPresetName_;
  rgbEffectActive_ = false;
  rgbTemporaryEffectActive_ = false;
  speechLightActive_ = false;
  lastRgbBreathingFrameMs_ = 0;
  lastRgbMarqueeFrameMs_ = 0;

  if (rgbBreathingEnabled_) {
    rgbBreathingStartedMs_ = nowMs;
    updateRgbBreathing(nowMs, false);
    return;
  }
  if (rgbMarqueeEnabled_) {
    rgbMarqueeOffset_ = 0;
    updateRgbMarquee(nowMs, false);
    return;
  }
  if (rgbRainbowEnabled_) {
    showRgbRainbow();
    return;
  }
  restoreConfiguredRgb();
}

void RobotHardware::markTemporaryRgbOverride(uint32_t nowMs, uint32_t durationMs) {
  rgbTemporaryRestorePending_ = true;
  rgbTemporaryRestoreAtMs_ = nowMs + durationMs;
}

void RobotHardware::extendTemporaryRgbOverride(uint32_t nowMs, uint32_t durationMs) {
  if (!rgbTemporaryRestorePending_) return;
  rgbTemporaryRestoreAtMs_ = nowMs + durationMs;
}

void RobotHardware::updateTemporaryRgbOverride(uint32_t nowMs) {
  if (!rgbTemporaryRestorePending_) return;
  if ((int32_t)(nowMs - rgbTemporaryRestoreAtMs_) < 0) return;
  restoreStandbyRgb(nowMs);
}

RgbSnapshot RobotHardware::snapshotRgb() const {
  RgbSnapshot snapshot = {};
  snapshot.revision = rgbConfigRevision_;
  snapshot.brightness = rgbBrightness_;
  snapshot.breathingEnabled = rgbBreathingEnabled_;
  snapshot.marqueeEnabled = rgbMarqueeEnabled_;
  snapshot.rainbowEnabled = rgbRainbowEnabled_;
  snapshot.presetName = rgbPresetName_;
  for (uint8_t index = 0; index < RGB_LED_COUNT; ++index) {
    snapshot.colors[index] = configuredRgb_[index];
  }
  return snapshot;
}

void RobotHardware::restoreRgbSnapshot(const RgbSnapshot& snapshot) {
  rgbEffectActive_ = false;
  rgbTemporaryEffectActive_ = false;
  speechLightActive_ = false;
  if (snapshot.revision == rgbConfigRevision_) {
    for (uint8_t index = 0; index < RGB_LED_COUNT; ++index) {
      configuredRgb_[index] = snapshot.colors[index];
    }
    rgbBrightness_ = snapshot.brightness;
    rgbBreathingEnabled_ = snapshot.breathingEnabled;
    rgbMarqueeEnabled_ = snapshot.marqueeEnabled;
    rgbRainbowEnabled_ = snapshot.rainbowEnabled;
    rgbPresetName_ = snapshot.presetName == nullptr ? "manual" : snapshot.presetName;
  }
  restoreConfiguredRgb();
}

void RobotHardware::restoreConfiguredRgb() {
  rgbEffectActive_ = false;
  rgbTemporaryEffectActive_ = false;
  speechLightActive_ = false;
  for (uint8_t index = 0; index < RGB_LED_COUNT; ++index) {
    writeRgb(index, configuredRgb_[index]);
  }
  refreshRgb();
}

void RobotHardware::setRgbEffectFrame(const RgbColor colors[RGB_LED_COUNT], bool temporary) {
  rgbEffectActive_ = true;
  rgbTemporaryEffectActive_ = temporary;
  for (uint8_t index = 0; index < RGB_LED_COUNT; ++index) {
    displayedRgb_[index] = colors[index];
    RgbColor scaled = scaleRgb(colors[index], rgbBrightness_);
    M5StackChan.setRgbColor(index, scaled.r, scaled.g, scaled.b);
  }
  refreshRgb();
}

void RobotHardware::applySpeechLightLevel(float audioLevel, bool blockedByEmotionLights) {
  if (blockedByEmotionLights) return;
  audioLevel = constrain(audioLevel, 0.0f, 1.0f);
  if (audioLevel <= 0.01f) {
    if (speechLightActive_) restoreConfiguredRgb();
    return;
  }

  RgbColor frame[RGB_LED_COUNT];
  uint8_t brightness = (uint8_t)constrain(48 + (int)(audioLevel * 207.0f), 48, 255);
  for (uint8_t index = 0; index < RGB_LED_COUNT; ++index) {
    frame[index] = scaleRgb(configuredRgb_[index], brightness);
  }
  speechLightActive_ = true;
  setRgbEffectFrame(frame);
}

void RobotHardware::setRgbAll(uint8_t r, uint8_t g, uint8_t b) {
  setRgbRange(RGB_LEFT_START, RGB_RIGHT_END, r, g, b);
}

void RobotHardware::setRgbLeft(uint8_t r, uint8_t g, uint8_t b) {
  setRgbRange(RGB_LEFT_START, RGB_LEFT_END, r, g, b);
}

void RobotHardware::setRgbRight(uint8_t r, uint8_t g, uint8_t b) {
  setRgbRange(RGB_RIGHT_START, RGB_RIGHT_END, r, g, b);
}

void RobotHardware::setRgbOne(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
  if (index >= RGB_LED_COUNT) return;
  configuredRgb_[index] = {r, g, b};
  rgbConfigRevision_++;
  if (!rgbEffectActive_) {
    writeRgb(index, configuredRgb_[index]);
    refreshRgb();
  }
}

void RobotHardware::setRgbRange(uint8_t start, uint8_t end, uint8_t r, uint8_t g, uint8_t b) {
  if (start > end || end >= RGB_LED_COUNT) return;
  for (uint8_t index = start; index <= end; ++index) {
    configuredRgb_[index] = {r, g, b};
  }
  rgbConfigRevision_++;
  if (!rgbEffectActive_) {
    for (uint8_t index = start; index <= end; ++index) {
      writeRgb(index, configuredRgb_[index]);
    }
    refreshRgb();
  }
}

String RobotHardware::rgbJson() const {
  String json = "{";
  json += "\"brightness\":" + String(rgbBrightness_);
  json += ",\"breathing\":" + String(rgbBreathingEnabled_ ? "true" : "false");
  json += ",\"marquee\":" + String(rgbMarqueeEnabled_ ? "true" : "false");
  json += ",\"rainbow\":" + String(rgbRainbowEnabled_ ? "true" : "false");
  json += ",\"preset\":\"" + String(rgbPresetName_) + "\"";
  json += ",\"color\":\"" + rgbColorToHex(configuredRgb_[RGB_LEFT_START]) + "\"";
  json += ",\"leftColor\":\"" + rgbColorToHex(configuredRgb_[RGB_LEFT_START]) + "\"";
  json += ",\"rightColor\":\"" + rgbColorToHex(configuredRgb_[RGB_RIGHT_START]) + "\"";
  json += ",\"effectActive\":" + String(rgbEffectActive_ ? "true" : "false");
  json += ",\"temporaryEffectActive\":" + String(rgbTemporaryEffectActive_ ? "true" : "false");
  json += ",\"speechActive\":" + String(speechLightActive_ ? "true" : "false");
  json += ",\"standby\":{";
  json += "\"brightness\":" + String(standbyRgbBrightness_);
  json += ",\"breathing\":" + String(standbyRgbBreathingEnabled_ ? "true" : "false");
  json += ",\"marquee\":" + String(standbyRgbMarqueeEnabled_ ? "true" : "false");
  json += ",\"rainbow\":" + String(standbyRgbRainbowEnabled_ ? "true" : "false");
  json += ",\"preset\":\"" + String(standbyRgbPresetName_) + "\"";
  json += ",\"color\":\"" + rgbColorToHex(standbyRgb_[RGB_LEFT_START]) + "\"";
  json += ",\"leftColor\":\"" + rgbColorToHex(standbyRgb_[RGB_LEFT_START]) + "\"";
  json += ",\"rightColor\":\"" + rgbColorToHex(standbyRgb_[RGB_RIGHT_START]) + "\"";
  json += "}";
  json += "}";
  return json;
}

String RobotHardware::statusJson(const FaceStateSnapshot& face, const String& clockJson) const {
  String json = "{";
  json += "\"ok\":true";
  json += ",\"uptimeMs\":" + String(millis());
  json += ",\"networkMode\":\"sta\"";
  json += ",\"ip\":\"" + WiFi.localIP().toString() + "\"";
  json += ",\"wifiStatus\":" + String((int)WiFi.status());
  json += ",\"rssi\":" + String(WiFi.RSSI());
  json += ",\"hardware\":{\"ready\":" + String(ready_ ? "true" : "false");
  json += ",\"initStatus\":\"" + String(initStatus_) + "\"";
  json += "}";
  json += ",\"face\":{\"mood\":\"" + String(face.mood) + "\"";
  json += ",\"eyeWidthScale\":" + String(face.style.eyeWidthScale, 2);
  json += ",\"eyeHeightScale\":" + String(face.style.eyeHeightScale, 2);
  json += ",\"cornerRadius\":" + String(face.style.cornerRadius);
  json += ",\"eyeOffsetX\":" + String(face.style.eyeOffsetX);
  json += ",\"eyeOffsetY\":" + String(face.style.eyeOffsetY);
  json += ",\"eyeSpacing\":" + String(face.style.eyeSpacing);
  json += ",\"eyeColor\":\"" + color565ToHex(face.style.eyeColor) + "\"";
  json += ",\"glowEnabled\":" + String(face.style.glowEnabled ? "true" : "false");
  json += ",\"glowStrength\":" + String(face.style.glowStrength);
  json += ",\"scanlinesEnabled\":" + String(face.style.scanlinesEnabled ? "true" : "false");
  json += ",\"scanlineSpacing\":" + String(face.style.scanlineSpacing);
  json += ",\"scanlineAlpha\":" + String(face.style.scanlineAlpha);
  json += ",\"audioLevel\":" + String(face.react.audioLevel, 2);
  json += ",\"lookX\":" + String(face.react.lookX, 2);
  json += ",\"lookY\":" + String(face.react.lookY, 2);
  json += ",\"subtitle\":{";
  json += "\"enabled\":" + String(face.subtitle.enabled ? "true" : "false");
  json += ",\"text\":\"" + jsonEscape(face.subtitle.text) + "\"";
  json += ",\"color\":\"" + color565ToHex(face.subtitle.color) + "\"";
  json += ",\"showDateTime\":" + String(face.subtitle.showDateTime ? "true" : "false");
  json += "}";
  json += ",\"assistantStatusLine\":{";
  json += "\"enabled\":" + String(face.assistantStatusLine.enabled ? "true" : "false");
  json += ",\"text\":\"" + jsonEscape(face.assistantStatusLine.text) + "\"";
  json += ",\"color\":\"" + color565ToHex(face.assistantStatusLine.color) + "\"";
  json += "}";
  json += "}";
  json += ",\"motion\":" + motionJson();
  json += ",\"power\":" + powerJson();
  json += ",\"rgb\":" + rgbJson();
  json += ",\"clock\":" + clockJson;
  json += ",\"firmware\":{";
  json += "\"version\":\"" + String(ROBOT_FIRMWARE_VERSION) + "\"";
  json += ",\"sketchSize\":" + String(ESP.getSketchSize());
  json += ",\"freeSketchSpace\":" + String(ESP.getFreeSketchSpace());
  json += ",\"flashChipSize\":" + String(ESP.getFlashChipSize());
  json += "}";
  json += ",\"system\":{";
  json += "\"chipModel\":\"" + jsonEscape(String(ESP.getChipModel())) + "\"";
  json += ",\"sdkVersion\":\"" + jsonEscape(String(ESP.getSdkVersion())) + "\"";
  json += ",\"freeHeap\":" + String(ESP.getFreeHeap());
  json += ",\"heapSize\":" + String(ESP.getHeapSize());
  json += ",\"psramSize\":" + String(ESP.getPsramSize());
  json += ",\"freePsram\":" + String(ESP.getFreePsram());
  json += "}";
  json += "}";
  return json;
}

TouchState RobotHardware::readTouchState() {
  const auto& intensities = M5StackChan.TouchSensor.getIntensities();
  TouchState touch = {
    intensities[0],
    intensities[1],
    intensities[2],
    intensities[0] > 0 || intensities[1] > 0 || intensities[2] > 0,
  };
  return touch;
}

ScreenTouchState RobotHardware::readScreenTouchState() {
  ScreenTouchState screen = {false, -1, -1, 0};
  if (!M5.Touch.isEnabled()) return screen;

  screen.count = M5.Touch.getCount();
  for (uint8_t index = 0; index < screen.count; ++index) {
    const auto& detail = M5.Touch.getDetail(index);
    if (!screen.pressed || detail.isPressed()) {
      screen.x = detail.x;
      screen.y = detail.y;
    }
    if (detail.isPressed()) {
      screen.pressed = true;
      break;
    }
  }
  return screen;
}

String RobotHardware::touchJson() {
  TouchState touch = readTouchState();
  ScreenTouchState screen = readScreenTouchState();
  bool swipeForward = M5StackChan.TouchSensor.wasSwipedForward();
  bool swipeBackward = M5StackChan.TouchSensor.wasSwipedBackward();
  String json = "{\"front\":" + String(touch.front);
  json += ",\"middle\":" + String(touch.middle);
  json += ",\"back\":" + String(touch.back);
  json += ",\"pressed\":" + String(touch.pressed ? "true" : "false");
  json += ",\"swiped\":" + String((swipeForward || swipeBackward) ? "true" : "false");
  json += ",\"swipeForward\":" + String(swipeForward ? "true" : "false");
  json += ",\"swipeBackward\":" + String(swipeBackward ? "true" : "false");
  json += ",\"screenPressed\":" + String(screen.pressed ? "true" : "false");
  json += ",\"screenX\":" + String(screen.x);
  json += ",\"screenY\":" + String(screen.y);
  json += ",\"screenCount\":" + String(screen.count);
  json += ",\"screen\":{\"pressed\":" + String(screen.pressed ? "true" : "false");
  json += ",\"x\":" + String(screen.x);
  json += ",\"y\":" + String(screen.y);
  json += ",\"count\":" + String(screen.count);
  json += "}";
  json += "}";
  return json;
}

ProximityState RobotHardware::readProximityState() {
  ProximityState state = lastProximityState_;
  state.available = proximitySensorAvailable_;
  state.enabled = proximitySensorEnabled_;
  state.lastReadAtMs = millis();
  if (!proximitySensorAvailable_ || !proximitySensorEnabled_) {
    lastProximityState_ = state;
    return state;
  }

  uint8_t psBytes[2] = {0, 0};
  if (M5.In_I2C.readRegister(LTR553_I2C_ADDRESS, LTR553_REG_PS_DATA_0, psBytes, sizeof(psBytes), LTR553_I2C_FREQ)) {
    state.proximityRaw = (uint16_t)psBytes[0] | (uint16_t)((psBytes[1] & 0x07) << 8);
  }

  uint8_t alsBytes[4] = {0, 0, 0, 0};
  if (M5.In_I2C.readRegister(LTR553_I2C_ADDRESS, LTR553_REG_ALS_DATA_CH1_0, alsBytes, sizeof(alsBytes), LTR553_I2C_FREQ)) {
    uint16_t channel1 = (uint16_t)alsBytes[0] | (uint16_t)(alsBytes[1] << 8);
    uint16_t channel0 = (uint16_t)alsBytes[2] | (uint16_t)(alsBytes[3] << 8);
    state.ambientRaw = (uint32_t)channel0 + (uint32_t)channel1;
  }

  state.near = state.proximityRaw >= LTR553_DEFAULT_NEAR_RAW;
  lastProximityState_ = state;
  return state;
}

String RobotHardware::proximityJson() {
  ProximityState state = readProximityState();
  String json = "{\"ok\":true";
  json += ",\"available\":" + String(state.available ? "true" : "false");
  json += ",\"enabled\":" + String(state.enabled ? "true" : "false");
  json += ",\"near\":" + String(state.near ? "true" : "false");
  json += ",\"proximityRaw\":" + String(state.proximityRaw);
  json += ",\"ambientRaw\":" + String(state.ambientRaw);
  json += ",\"address\":\"0x23\"";
  json += ",\"lastReadAtMs\":" + String(state.lastReadAtMs);
  json += "}";
  return json;
}

String RobotHardware::audioStatusJson() const {
  String json = "{\"ok\":true";
  json += ",\"speakerEnabled\":" + String(M5.Speaker.isEnabled() ? "true" : "false");
  json += ",\"playing\":" + String(M5.Speaker.isPlaying() ? "true" : "false");
  json += ",\"volume\":" + String(M5.Speaker.getVolume());
  json += ",\"lastFrequency\":" + String(lastToneFrequency_);
  json += ",\"lastDurationMs\":" + String(lastToneDurationMs_);
  json += "}";
  return json;
}

String RobotHardware::playToneJson(int frequency, int durationMs, int volume) {
  frequency = constrain(frequency, 80, 5000);
  durationMs = constrain(durationMs, 40, 3000);
  volume = constrain(volume, 0, 255);
  while (M5.Mic.isRecording()) delay(1);
  M5.Mic.end();
  bool speakerStarted = M5.Speaker.begin();
  M5.Speaker.setVolume((uint8_t)volume);
  bool toneStarted = M5.Speaker.tone((float)frequency, (uint32_t)durationMs, 0, true);
  lastToneFrequency_ = frequency;
  lastToneDurationMs_ = durationMs;
  lastSpeakerVolume_ = volume;

  String json = "{\"ok\":";
  json += String((speakerStarted && toneStarted) ? "true" : "false");
  json += ",\"frequency\":" + String(frequency);
  json += ",\"durationMs\":" + String(durationMs);
  json += ",\"volume\":" + String(volume);
  json += ",\"speakerStarted\":" + String(speakerStarted ? "true" : "false");
  json += ",\"toneStarted\":" + String(toneStarted ? "true" : "false");
  json += "}";
  return json;
}

String RobotHardware::stopAudioJson() {
  M5.Speaker.stop();
  String json = "{\"ok\":true";
  json += ",\"playing\":" + String(M5.Speaker.isPlaying() ? "true" : "false");
  json += ",\"volume\":" + String(lastSpeakerVolume_);
  json += "}";
  return json;
}

String RobotHardware::micSampleJson(int sampleCount, int sampleRate) {
  sampleCount = constrain(sampleCount, 64, 512);
  sampleRate = constrain(sampleRate, 8000, 24000);
  uint32_t nowMs = millis();
  if (lastMicSampleJson_.length() > 0 &&
      sampleCount == lastMicSampleCount_ &&
      sampleRate == lastMicSampleRate_ &&
      (uint32_t)(nowMs - lastMicSampleAtMs_) < MIC_SAMPLE_MIN_INTERVAL_MS) {
    String cached = lastMicSampleJson_;
    cached.remove(cached.length() - 1);
    cached += ",\"cached\":true}";
    return cached;
  }

  int16_t samples[512];
  memset(samples, 0, sizeof(samples));

  M5.Speaker.stop();
  bool micStarted = M5.Mic.isEnabled();
  if (!M5.Mic.isEnabled()) {
    auto micConfig = M5.Mic.config();
    micConfig.sample_rate = sampleRate;
    micConfig.over_sampling = 1;
    micConfig.magnification = 32;
    micConfig.noise_filter_level = 0;
    M5.Mic.config(micConfig);
    micStarted = M5.Mic.begin();
  }
  bool available = micStarted && M5.Mic.isEnabled();
  bool accepted = false;
  if (available) {
    accepted = M5.Mic.record(samples, (size_t)sampleCount, (uint32_t)sampleRate);
    uint32_t waitMs = (uint32_t)sampleCount * 1000UL / (uint32_t)sampleRate + 50;
    if (waitMs < 70) waitMs = 70;
    if (waitMs > 220) waitMs = 220;
    uint32_t startMs = millis();
    while (M5.Mic.isRecording() && millis() - startMs < waitMs) {
      delay(2);
    }
  }

  int peak = 0;
  uint32_t sumAbs = 0;
  double sumSquares = 0.0;
  if (accepted) {
    for (int i = 0; i < sampleCount; ++i) {
      int value = abs((int)samples[i]);
      if (value > peak) peak = value;
      sumAbs += (uint32_t)value;
      sumSquares += (double)value * (double)value;
    }
  }
  float average = accepted ? (float)sumAbs / (float)sampleCount : 0.0f;
  float rms = accepted ? sqrtf((float)(sumSquares / (double)sampleCount)) : 0.0f;
  float level = constrain(rms / 32768.0f, 0.0f, 1.0f);

  String json = "{\"ok\":";
  json += String((available && accepted) ? "true" : "false");
  json += ",\"micEnabled\":" + String(M5.Mic.isEnabled() ? "true" : "false");
  json += ",\"recordAccepted\":" + String(accepted ? "true" : "false");
  json += ",\"sampleRate\":" + String(sampleRate);
  json += ",\"samples\":" + String(sampleCount);
  json += ",\"peak\":" + String(peak);
  json += ",\"average\":" + String(average, 1);
  json += ",\"rms\":" + String(rms, 1);
  json += ",\"level\":" + String(level, 3);
  json += ",\"cached\":false";
  json += "}";
  lastMicSampleAtMs_ = nowMs;
  lastMicSampleCount_ = sampleCount;
  lastMicSampleRate_ = sampleRate;
  lastMicSampleJson_ = json;
  return json;
}

ImuSample RobotHardware::readImuSample() const {
  ImuSample sample = {};
  sample.available = M5.Imu.isEnabled();
  if (sample.available) {
    M5.Imu.getAccel(&sample.accelX, &sample.accelY, &sample.accelZ);
    M5.Imu.getGyro(&sample.gyroX, &sample.gyroY, &sample.gyroZ);
  }
  sample.accelMagnitudeG = sqrtf(sample.accelX * sample.accelX + sample.accelY * sample.accelY + sample.accelZ * sample.accelZ);
  sample.gyroMagnitudeDps = sqrtf(sample.gyroX * sample.gyroX + sample.gyroY * sample.gyroY + sample.gyroZ * sample.gyroZ);
  return sample;
}

String RobotHardware::imuJson() const {
  ImuSample sample = readImuSample();
  String json = "{\"available\":" + String(sample.available ? "true" : "false");
  json += ",\"accelX\":" + String(sample.accelX, 3);
  json += ",\"accelY\":" + String(sample.accelY, 3);
  json += ",\"accelZ\":" + String(sample.accelZ, 3);
  json += ",\"gyroX\":" + String(sample.gyroX, 3);
  json += ",\"gyroY\":" + String(sample.gyroY, 3);
  json += ",\"gyroZ\":" + String(sample.gyroZ, 3);
  json += ",\"accelMagnitudeG\":" + String(sample.accelMagnitudeG, 3);
  json += ",\"gyroMagnitudeDps\":" + String(sample.gyroMagnitudeDps, 1);
  json += "}";
  return json;
}

String RobotHardware::powerJson() const {
  float voltage = batteryVoltage();
  float current = batteryCurrent();
  String json = "{\"available\":true";
  json += ",\"voltage\":" + String(voltage, 3);
  json += ",\"current\":" + String(current, 3);
  json += ",\"percent\":" + String(batteryPercent());
  json += "}";
  return json;
}

float RobotHardware::batteryVoltage() const {
  return M5StackChan.getBatteryVoltage();
}

float RobotHardware::batteryCurrent() const {
  return M5StackChan.getBatteryCurrent();
}

int RobotHardware::batteryPercent() const {
  float voltage = batteryVoltage();
  if (voltage <= 0.1f) return -1;
  int percent = (int)((voltage - 3.30f) * 100.0f / 0.90f);
  return constrain(percent, 0, 100);
}

String RobotHardware::motionJson() const {
  String json = "{\"yaw\":" + String(lastMotion_.yaw);
  json += ",\"pitch\":" + String(lastMotion_.pitch);
  json += ",\"speed\":" + String(lastMotion_.speed);
  json += ",\"velocity\":" + String(lastMotion_.velocity);
  json += ",\"restPitch\":" + String(restPitch_);
  json += ",\"restPitchDefault\":" + String(DEFAULT_REST_PITCH);
  json += "}";
  return json;
}

MotionState RobotHardware::currentMotion() const {
  return lastMotion_;
}
