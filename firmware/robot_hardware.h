#pragma once

#include <Arduino.h>
#include "face_renderer.h"

constexpr int YAW_MIN = -1280;
constexpr int YAW_MAX = 1280;
constexpr int PITCH_MIN = 0;
constexpr int PITCH_MAX = 900;
constexpr int DEFAULT_REST_PITCH = 120;
constexpr int ROTATE_MIN = -1000;
constexpr int ROTATE_MAX = 1000;
constexpr int SPEED_MIN = 0;
constexpr int SPEED_MAX = 1000;
// Left/right are defined while facing the robot screen.
constexpr uint8_t RGB_LED_COUNT = 12;
constexpr uint8_t RGB_LEFT_START = 0;
constexpr uint8_t RGB_LEFT_END = 5;
constexpr uint8_t RGB_RIGHT_START = 6;
constexpr uint8_t RGB_RIGHT_END = 11;
constexpr uint32_t RGB_TEMPORARY_RESTORE_MS = 2500;

struct MotionState {
  int yaw;
  int pitch;
  int speed;
  int velocity;
};

struct RgbColor {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

constexpr RgbColor DEFAULT_RGB_COLOR = {0, 255, 98};
constexpr uint8_t DEFAULT_RGB_BRIGHTNESS = 60;

struct RgbSnapshot {
  RgbColor colors[RGB_LED_COUNT];
  uint8_t brightness;
  bool breathingEnabled;
  bool marqueeEnabled;
  bool rainbowEnabled;
  const char* presetName;
  uint32_t revision;
};

struct TouchState {
  int front;
  int middle;
  int back;
  bool pressed;
};

struct ScreenTouchState {
  bool pressed;
  int x;
  int y;
  uint8_t count;
};

struct ProximityState {
  bool available;
  bool enabled;
  bool near;
  uint16_t proximityRaw;
  uint32_t ambientRaw;
  uint32_t lastReadAtMs;
};

struct ImuSample {
  bool available;
  float accelX;
  float accelY;
  float accelZ;
  float gyroX;
  float gyroY;
  float gyroZ;
  float accelMagnitudeG;
  float gyroMagnitudeDps;
};

class RobotHardware {
public:
  void begin();
  void setInitStatus(const char* status);
  void markReady();
  bool isReady() const;
  String unavailableJson() const;
  MotionState pose(int yaw, int pitch, int speed);
  MotionState poseWithRestPitchOffset(int yaw, int pitch, int speed);
  MotionState pitchWithRestPitchOffset(int pitch, int speed);
  MotionState prepareYawRotation();
  MotionState rotateX(int velocity);
  MotionState home(int speed);
  void setRestPitch(int pitch);
  int restPitch() const;
  int restPitchOffset(int pitch) const;
  void setRgbAll(uint8_t r, uint8_t g, uint8_t b);
  void setRgbLeft(uint8_t r, uint8_t g, uint8_t b);
  void setRgbRight(uint8_t r, uint8_t g, uint8_t b);
  void setRgbOne(uint8_t index, uint8_t r, uint8_t g, uint8_t b);
  void setRgbBrightness(uint8_t brightness);
  uint8_t rgbBrightness() const;
  void setRgbBreathingEnabled(bool enabled, uint32_t nowMs, bool blockedByTemporaryLights = false);
  bool rgbBreathingEnabled() const;
  void setRgbMarqueeEnabled(bool enabled, uint32_t nowMs, bool blockedByTemporaryLights = false);
  bool rgbMarqueeEnabled() const;
  void setRgbRainbowEnabled(bool enabled, uint32_t nowMs, bool blockedByTemporaryLights = false);
  bool rgbRainbowEnabled() const;
  void updateRgbBreathing(uint32_t nowMs, bool blockedByTemporaryLights);
  void updateRgbManualEffects(uint32_t nowMs, bool blockedByTemporaryLights);
  void setRgbPresetName(const char* presetName);
  void captureStandbyRgb();
  void restoreStandbyRgb(uint32_t nowMs);
  void markTemporaryRgbOverride(uint32_t nowMs, uint32_t durationMs = RGB_TEMPORARY_RESTORE_MS);
  void extendTemporaryRgbOverride(uint32_t nowMs, uint32_t durationMs = RGB_TEMPORARY_RESTORE_MS);
  void updateTemporaryRgbOverride(uint32_t nowMs);
  RgbSnapshot snapshotRgb() const;
  void restoreRgbSnapshot(const RgbSnapshot& snapshot);
  void restoreConfiguredRgb();
  void setRgbEffectFrame(const RgbColor colors[RGB_LED_COUNT], bool temporary = true);
  void applySpeechLightLevel(float audioLevel, bool blockedByEmotionLights);
  String rgbJson() const;
  String statusJson(const FaceStateSnapshot& face, const String& clockJson) const;
  TouchState readTouchState();
  ScreenTouchState readScreenTouchState();
  ProximityState readProximityState();
  String touchJson();
  String proximityJson();
  String audioStatusJson() const;
  String playToneJson(int frequency, int durationMs, int volume);
  String stopAudioJson();
  String micSampleJson(int sampleCount, int sampleRate);
  ImuSample readImuSample() const;
  String imuJson() const;
  String powerJson() const;
  float batteryVoltage() const;
  float batteryCurrent() const;
  int batteryPercent() const;
  String motionJson() const;
  MotionState currentMotion() const;

private:
  MotionState lastMotion_ = {0, DEFAULT_REST_PITCH, 0, 0};
  int restPitch_ = DEFAULT_REST_PITCH;
  int lastToneFrequency_ = 880;
  int lastToneDurationMs_ = 240;
  int lastSpeakerVolume_ = 96;
  bool ready_ = false;
  const char* initStatus_ = "not_started";
  RgbColor configuredRgb_[RGB_LED_COUNT] = {};
  RgbColor displayedRgb_[RGB_LED_COUNT] = {};
  RgbColor standbyRgb_[RGB_LED_COUNT] = {};
  uint32_t rgbConfigRevision_ = 0;
  uint8_t rgbBrightness_ = DEFAULT_RGB_BRIGHTNESS;
  uint8_t standbyRgbBrightness_ = DEFAULT_RGB_BRIGHTNESS;
  bool rgbEffectActive_ = false;
  bool rgbTemporaryEffectActive_ = false;
  bool rgbTemporaryRestorePending_ = false;
  bool speechLightActive_ = false;
  bool rgbBreathingEnabled_ = false;
  bool rgbMarqueeEnabled_ = false;
  bool rgbRainbowEnabled_ = false;
  bool standbyRgbBreathingEnabled_ = false;
  bool standbyRgbMarqueeEnabled_ = false;
  bool standbyRgbRainbowEnabled_ = false;
  uint32_t rgbBreathingStartedMs_ = 0;
  uint32_t lastRgbBreathingFrameMs_ = 0;
  uint32_t lastRgbMarqueeFrameMs_ = 0;
  uint32_t rgbTemporaryRestoreAtMs_ = 0;
  uint8_t rgbMarqueeOffset_ = 0;
  const char* rgbPresetName_ = "manual";
  const char* standbyRgbPresetName_ = "manual";
  bool proximitySensorAvailable_ = false;
  bool proximitySensorEnabled_ = false;
  ProximityState lastProximityState_ = {};
  uint32_t lastMicSampleAtMs_ = 0;
  int lastMicSampleCount_ = 0;
  int lastMicSampleRate_ = 0;
  String lastMicSampleJson_ = "";

  void beginProximitySensor();
  void setRgbRange(uint8_t start, uint8_t end, uint8_t r, uint8_t g, uint8_t b);
  void writeRgb(uint8_t index, const RgbColor& color);
  void refreshRgb();
  void updateRgbMarquee(uint32_t nowMs, bool blockedByTemporaryLights);
  void showRgbRainbow();
  static RgbColor scaleRgb(const RgbColor& color, uint8_t brightness);
};
