#pragma once

#include <Arduino.h>
#include "face_renderer.h"
#include "idle_behavior.h"
#include "local_face_detector.h"
#include "mumble_player.h"
#include "robot_clock.h"
#include "robot_hardware.h"

struct CameraFaceReport {
  uint32_t scanId = 0;
  bool faceDetected = false;
  float confidence = 0.0f;
  float faceCenterX = 0.5f;
  float faceCenterY = 0.5f;
  float faceWidth = 0.0f;
  float faceHeight = 0.0f;
  bool force = false;
  bool hasPoseOverride = false;
  int16_t yaw = 0;
  int16_t pitch = DEFAULT_REST_PITCH;
};

struct FaceSnapshot {
  uint32_t id = 0;
  int16_t yaw = 0;
  int16_t pitch = DEFAULT_REST_PITCH;
  float lookX = 0.0f;
  float lookY = 0.0f;
  uint32_t capturedAtMs = 0;
};

enum class CameraAttentionMode {
  Idle,
  Detecting,
  ReturningToSnapshot,
  Confirming,
  FaceLock,
  Catchphrase,
  Cooldown,
  Disabled,
  Paused,
};

class CameraAttentionController {
public:
  void begin();
  void update(RobotHardware& hardware, RobotFaceRenderer& face, IdleBehaviorController& idle, RobotClock& clock, MumblePlayer& mumble, LocalFaceDetector& detector, bool emotionActive, bool proximityActive, bool voiceBusy, uint32_t nowMs);
  bool reportFace(const CameraFaceReport& report, RobotHardware& hardware, RobotFaceRenderer& face, IdleBehaviorController& idle, RobotClock& clock, MumblePlayer& mumble, LocalFaceDetector& detector, bool emotionActive, uint32_t nowMs);
  String handleVoiceWake(RobotHardware& hardware, RobotFaceRenderer& face, IdleBehaviorController& idle, RobotClock& clock, uint32_t nowMs, uint32_t holdMs = 0);
  void clear(LocalFaceDetector& detector, uint32_t nowMs);
  void setEnabled(bool enabled);
  void setCaptureIntervalMs(uint32_t value);
  void setDetectTimeoutMs(uint32_t value);
  void setReturnSettleMs(uint32_t value);
  void setConfirmSettleMs(uint32_t value);
  void setLockHoldMs(uint32_t value);
  void setCooldownMs(uint32_t value);
  void setCenterDeadZone(float x, float y);
  void setServoStepMax(int16_t yawStep, int16_t pitchStep);
  void setCatchphrase(const String& assetId, bool listenAfter);
  void setFeedbackSubtitleEnabled(bool enabled);
  bool enabled() const;
  bool active() const;
  uint32_t captureIntervalMs() const;
  uint32_t detectTimeoutMs() const;
  uint32_t returnSettleMs() const;
  uint32_t lockHoldMs() const;
  uint32_t cooldownMs() const;
  const char* catchphraseAssetId() const;
  bool listenAfterCatchphrase() const;
  bool feedbackSubtitleEnabled() const;
  String statusJson(const LocalFaceDetector& detector) const;

private:
  void captureCurrentPose(RobotHardware& hardware, RobotFaceRenderer& face, LocalFaceDetector& detector, uint32_t nowMs);
  void returnToSnapshot(RobotHardware& hardware, RobotFaceRenderer& face, IdleBehaviorController& idle, uint32_t nowMs);
  void startConfirmCapture(LocalFaceDetector& detector, uint32_t nowMs);
  void enterFaceLock(const LocalFaceResult& result, RobotHardware& hardware, RobotFaceRenderer& face, IdleBehaviorController& idle, uint32_t nowMs);
  void applyFaceCentering(const LocalFaceResult& result, RobotHardware& hardware, RobotFaceRenderer& face, IdleBehaviorController& idle, uint32_t nowMs);
  void showFaceFeedbackSubtitle(RobotFaceRenderer& face, uint32_t nowMs);
  void showCameraFeedbackSubtitle(RobotFaceRenderer& face, uint32_t nowMs, const String& text, uint32_t durationMs);
  void clearFaceFeedbackSubtitle(RobotFaceRenderer& face, uint32_t nowMs);
  void playCatchphrase(MumblePlayer& mumble, uint32_t nowMs);
  void startListeningAfterCatchphrase(RobotClock& clock, uint32_t nowMs);
  void enterCooldown(LocalFaceDetector& detector, uint32_t nowMs, const char* reason);
  const char* modeName() const;
  uint32_t remainingMs(uint32_t nowMs, uint32_t targetMs) const;

  bool enabled_ = true;
  bool active_ = false;
  bool captureRequested_ = false;
  bool confirming_ = false;
  bool catchphrasePlayed_ = false;
  bool listenAfterCatchphrase_ = true;
  bool feedbackSubtitleEnabled_ = false;
  bool faceFeedbackSubtitleActive_ = false;
  CameraAttentionMode mode_ = CameraAttentionMode::Idle;
  FaceSnapshot snapshot_ = {};
  LocalFaceResult lastFace_ = {};
  uint32_t snapshotId_ = 0;
  uint32_t nextCaptureAtMs_ = 0;
  uint32_t modeUntilMs_ = 0;
  uint32_t faceFeedbackUntilMs_ = 0;
  uint32_t captureIntervalMs_ = 12000;
  uint32_t detectTimeoutMs_ = 1500;
  uint32_t returnSettleMs_ = 450;
  uint32_t confirmSettleMs_ = 250;
  uint32_t lockHoldMs_ = 4500;
  uint32_t cooldownMs_ = 20000;
  float centerDeadZoneX_ = 0.10f;
  float centerDeadZoneY_ = 0.12f;
  int16_t yawStepMax_ = 48;
  int16_t pitchStepMax_ = 28;
  int16_t targetYaw_ = 0;
  int16_t targetPitch_ = DEFAULT_REST_PITCH;
  String catchphraseAssetId_ = "halo";
  String cameraFeedbackSubtitleText_ = "";
  const char* lastReason_ = "boot";
};
