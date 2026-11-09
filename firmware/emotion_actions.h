#pragma once

#include <Arduino.h>
#include "face_renderer.h"
#include "idle_behavior.h"
#include "robot_hardware.h"

enum class EmotionLightEffect : uint8_t {
  None,
  RainbowMarquee,
  Breathing,
  RedPulse,
  SurpriseFlash,
  ThinkingTravel,
  SoftPinkPulse,
  WarmBounce,
  GoldPulse,
  BluePurpleBlink,
  ColdFlash,
  CelebrationRainbow,
  BlinkingMarquee,
};

struct EmotionStep {
  const char* mood;
  int16_t yaw;
  int16_t pitch;
  float lookX;
  float lookY;
  float audioLevel;
  uint16_t durationMs;
  uint16_t speed;
  EmotionLightEffect lightEffect;
  const char* label;
  int16_t rotateVelocity = 0;
};

class EmotionActionController {
public:
  void begin();
  bool play(const String& name, uint32_t nowMs, bool lightsEnabled = true);
  void stop(RobotHardware& hardware, uint32_t nowMs);
  void update(RobotHardware& hardware, RobotFaceRenderer& face, IdleBehaviorController& idle, uint32_t nowMs);
  bool active() const;
  bool isActiveAction(const char* name) const;
  bool lightsActive() const;
  uint32_t durationMs() const;
  String statusJson() const;
  static bool isKnownAction(const String& name);

private:
  const EmotionStep* steps_ = nullptr;
  uint8_t stepCount_ = 0;
  uint8_t stepIndex_ = 0;
  uint8_t variantIndex_ = 0;
  uint32_t playCount_ = 0;
  uint32_t nextStepAtMs_ = 0;
  uint32_t durationMs_ = 0;
  bool active_ = false;
  bool rotationStopped_ = false;
  bool lightsEnabled_ = true;
  bool lightSnapshotCaptured_ = false;
  RgbSnapshot rgbSnapshot_ = {};
  const char* actionName_ = "none";
  const char* actionTitle_ = "未播放";
  const char* variantName_ = "none";
  const char* variantTitle_ = "未选择";
  const char* currentStepLabel_ = "待机";

  void applyStep(RobotHardware& hardware, RobotFaceRenderer& face, IdleBehaviorController& idle, const EmotionStep& step, uint32_t nowMs);
  void restoreEmotionLights(RobotHardware& hardware);
  void applyEmotionLight(RobotHardware& hardware, EmotionLightEffect effect, uint8_t frameIndex);
};
