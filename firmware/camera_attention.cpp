#include "camera_attention.h"

#include <math.h>

namespace {

constexpr uint32_t MIN_CAPTURE_INTERVAL_MS = 3000;
constexpr uint32_t MAX_CAPTURE_INTERVAL_MS = 180000;
constexpr uint32_t MIN_DETECT_TIMEOUT_MS = 300;
constexpr uint32_t MAX_DETECT_TIMEOUT_MS = 5000;
constexpr uint32_t MIN_SETTLE_MS = 80;
constexpr uint32_t MAX_SETTLE_MS = 2000;
constexpr uint32_t MIN_LOCK_HOLD_MS = 1000;
constexpr uint32_t MAX_LOCK_HOLD_MS = 30000;
constexpr uint32_t MIN_COOLDOWN_MS = 3000;
constexpr uint32_t MAX_COOLDOWN_MS = 600000;
constexpr uint32_t DEFAULT_WAKE_HOLD_MS = 4200;
constexpr uint32_t FACE_FEEDBACK_SUBTITLE_MS = 2500;
constexpr uint32_t CAMERA_CAPTURE_FEEDBACK_SUBTITLE_MS = 900;
constexpr uint32_t CAMERA_NO_FACE_FEEDBACK_SUBTITLE_MS = 1100;

float clampFloatRange(float value, float minValue, float maxValue) {
  if (value < minValue) return minValue;
  if (value > maxValue) return maxValue;
  return value;
}

int16_t clampStep(float error, int16_t maxStep) {
  float step = error * (float)maxStep;
  if (step > maxStep) step = maxStep;
  if (step < -maxStep) step = -maxStep;
  return (int16_t)step;
}

String quoteCameraString(const String& value) {
  String out = "\"";
  for (uint16_t i = 0; i < value.length(); ++i) {
    char c = value.charAt(i);
    if (c == '"' || c == '\\') out += "\\";
    out += c;
  }
  out += "\"";
  return out;
}

}  // namespace

void CameraAttentionController::begin() {
  enabled_ = true;
  active_ = false;
  captureRequested_ = false;
  confirming_ = false;
  catchphrasePlayed_ = false;
  listenAfterCatchphrase_ = true;
  feedbackSubtitleEnabled_ = false;
  faceFeedbackSubtitleActive_ = false;
  mode_ = CameraAttentionMode::Idle;
  snapshot_ = {};
  lastFace_ = {};
  snapshotId_ = 0;
  nextCaptureAtMs_ = 0;
  modeUntilMs_ = 0;
  faceFeedbackUntilMs_ = 0;
  captureIntervalMs_ = 12000;
  detectTimeoutMs_ = 1500;
  returnSettleMs_ = 450;
  confirmSettleMs_ = 250;
  lockHoldMs_ = 4500;
  cooldownMs_ = 20000;
  centerDeadZoneX_ = 0.10f;
  centerDeadZoneY_ = 0.12f;
  yawStepMax_ = 48;
  pitchStepMax_ = 28;
  targetYaw_ = 0;
  targetPitch_ = DEFAULT_REST_PITCH;
  catchphraseAssetId_ = "halo";
  cameraFeedbackSubtitleText_ = "";
  lastReason_ = "boot";
}

void CameraAttentionController::setEnabled(bool enabled) {
  enabled_ = enabled;
  if (!enabled) {
    active_ = false;
    mode_ = CameraAttentionMode::Disabled;
  }
}

void CameraAttentionController::setCaptureIntervalMs(uint32_t value) {
  captureIntervalMs_ = constrain(value, MIN_CAPTURE_INTERVAL_MS, MAX_CAPTURE_INTERVAL_MS);
}

void CameraAttentionController::setDetectTimeoutMs(uint32_t value) {
  detectTimeoutMs_ = constrain(value, MIN_DETECT_TIMEOUT_MS, MAX_DETECT_TIMEOUT_MS);
}

void CameraAttentionController::setReturnSettleMs(uint32_t value) {
  returnSettleMs_ = constrain(value, MIN_SETTLE_MS, MAX_SETTLE_MS);
}

void CameraAttentionController::setConfirmSettleMs(uint32_t value) {
  confirmSettleMs_ = constrain(value, MIN_SETTLE_MS, MAX_SETTLE_MS);
}

void CameraAttentionController::setLockHoldMs(uint32_t value) {
  lockHoldMs_ = constrain(value, MIN_LOCK_HOLD_MS, MAX_LOCK_HOLD_MS);
}

void CameraAttentionController::setCooldownMs(uint32_t value) {
  cooldownMs_ = constrain(value, MIN_COOLDOWN_MS, MAX_COOLDOWN_MS);
}

void CameraAttentionController::setCenterDeadZone(float x, float y) {
  centerDeadZoneX_ = clampFloatRange(x, 0.01f, 0.45f);
  centerDeadZoneY_ = clampFloatRange(y, 0.01f, 0.45f);
}

void CameraAttentionController::setServoStepMax(int16_t yawStep, int16_t pitchStep) {
  yawStepMax_ = constrain(yawStep, 1, 240);
  pitchStepMax_ = constrain(pitchStep, 1, 180);
}

void CameraAttentionController::setCatchphrase(const String& assetId, bool listenAfter) {
  catchphraseAssetId_ = assetId.length() ? assetId : "halo";
  listenAfterCatchphrase_ = listenAfter;
}

void CameraAttentionController::setFeedbackSubtitleEnabled(bool enabled) {
  feedbackSubtitleEnabled_ = enabled;
}

bool CameraAttentionController::enabled() const {
  return enabled_;
}

bool CameraAttentionController::active() const {
  return active_;
}

uint32_t CameraAttentionController::captureIntervalMs() const {
  return captureIntervalMs_;
}

uint32_t CameraAttentionController::detectTimeoutMs() const {
  return detectTimeoutMs_;
}

uint32_t CameraAttentionController::returnSettleMs() const {
  return returnSettleMs_;
}

uint32_t CameraAttentionController::lockHoldMs() const {
  return lockHoldMs_;
}

uint32_t CameraAttentionController::cooldownMs() const {
  return cooldownMs_;
}

const char* CameraAttentionController::catchphraseAssetId() const {
  return catchphraseAssetId_.c_str();
}

bool CameraAttentionController::listenAfterCatchphrase() const {
  return listenAfterCatchphrase_;
}

bool CameraAttentionController::feedbackSubtitleEnabled() const {
  return feedbackSubtitleEnabled_;
}

const char* CameraAttentionController::modeName() const {
  switch (mode_) {
    case CameraAttentionMode::Idle: return "idle";
    case CameraAttentionMode::Detecting: return "detecting";
    case CameraAttentionMode::ReturningToSnapshot: return "returningToSnapshot";
    case CameraAttentionMode::Confirming: return "confirming";
    case CameraAttentionMode::FaceLock: return "faceLock";
    case CameraAttentionMode::Catchphrase: return "catchphrase";
    case CameraAttentionMode::Cooldown: return "cooldown";
    case CameraAttentionMode::Disabled: return "disabled";
    case CameraAttentionMode::Paused: return "paused";
    default: return "unknown";
  }
}

void CameraAttentionController::captureCurrentPose(RobotHardware& hardware, RobotFaceRenderer& face, LocalFaceDetector& detector, uint32_t nowMs) {
  MotionState motion = hardware.currentMotion();
  FaceReactState react = face.react();
  snapshotId_++;
  snapshot_ = {snapshotId_, (int16_t)motion.yaw, (int16_t)motion.pitch, react.lookX, react.lookY, nowMs};
  LocalFaceRequest request;
  request.snapshotId = snapshotId_;
  request.requestedAtMs = nowMs;
  captureRequested_ = detector.submitCapture(request, nowMs);
  active_ = captureRequested_;
  confirming_ = false;
  catchphrasePlayed_ = false;
  mode_ = captureRequested_ ? CameraAttentionMode::Detecting : CameraAttentionMode::Cooldown;
  modeUntilMs_ = nowMs + detectTimeoutMs_;
  lastReason_ = captureRequested_ ? "captureCurrentPose" : "detectorBusy";
  if (captureRequested_) showCameraFeedbackSubtitle(face, nowMs, "Looking...", CAMERA_CAPTURE_FEEDBACK_SUBTITLE_MS);
}

void CameraAttentionController::returnToSnapshot(RobotHardware& hardware, RobotFaceRenderer& face, IdleBehaviorController& idle, uint32_t nowMs) {
  targetYaw_ = snapshot_.yaw;
  targetPitch_ = snapshot_.pitch;
  showFaceFeedbackSubtitle(face, nowMs);
  face.setMood("curious");
  face.setReact(snapshot_.lookX, snapshot_.lookY, 0.08f);
  hardware.pose(targetYaw_, targetPitch_, 72);
  idle.pauseForManualControl(nowMs, returnSettleMs_ + confirmSettleMs_ + 500);
  active_ = true;
  mode_ = CameraAttentionMode::ReturningToSnapshot;
  modeUntilMs_ = nowMs + returnSettleMs_;
  lastReason_ = "firstFaceDetected";
}

void CameraAttentionController::startConfirmCapture(LocalFaceDetector& detector, uint32_t nowMs) {
  LocalFaceRequest request;
  request.snapshotId = snapshot_.id;
  request.requestedAtMs = nowMs;
  captureRequested_ = detector.submitCapture(request, nowMs);
  confirming_ = captureRequested_;
  mode_ = captureRequested_ ? CameraAttentionMode::Confirming : CameraAttentionMode::Cooldown;
  modeUntilMs_ = nowMs + detectTimeoutMs_;
  lastReason_ = captureRequested_ ? "confirmCapture" : "confirmDetectorBusy";
}

void CameraAttentionController::enterFaceLock(const LocalFaceResult& result, RobotHardware& hardware, RobotFaceRenderer& face, IdleBehaviorController& idle, uint32_t nowMs) {
  lastFace_ = result;
  targetYaw_ = snapshot_.yaw;
  targetPitch_ = snapshot_.pitch;
  active_ = true;
  captureRequested_ = false;
  confirming_ = false;
  catchphrasePlayed_ = false;
  mode_ = CameraAttentionMode::FaceLock;
  modeUntilMs_ = nowMs + lockHoldMs_;
  lastReason_ = "confirmedFace";
  showFaceFeedbackSubtitle(face, nowMs);
  applyFaceCentering(result, hardware, face, idle, nowMs);
}

void CameraAttentionController::applyFaceCentering(const LocalFaceResult& result, RobotHardware& hardware, RobotFaceRenderer& face, IdleBehaviorController& idle, uint32_t nowMs) {
  float errorX = result.faceCenterX - 0.5f;
  float errorY = result.faceCenterY - 0.5f;
  if (fabs(errorX) < centerDeadZoneX_) errorX = 0.0f;
  if (fabs(errorY) < centerDeadZoneY_) errorY = 0.0f;
  targetYaw_ = constrain(targetYaw_ + clampStep(errorX, yawStepMax_), YAW_MIN, YAW_MAX);
  targetPitch_ = constrain(targetPitch_ - clampStep(errorY, pitchStepMax_), PITCH_MIN, PITCH_MAX);
  face.setMood("curious");
  face.setReact(constrain(errorX * 1.2f, -0.45f, 0.45f), constrain(errorY * 1.0f, -0.35f, 0.35f), 0.12f);
  hardware.pose(targetYaw_, targetPitch_, 64);
  idle.pauseForManualControl(nowMs, 900);
}

void CameraAttentionController::showFaceFeedbackSubtitle(RobotFaceRenderer& face, uint32_t nowMs) {
  SubtitleState subtitle = face.subtitle();
  face.setSubtitle("Halo?", true, subtitle.color);
  cameraFeedbackSubtitleText_ = "Halo?";
  faceFeedbackSubtitleActive_ = true;
  faceFeedbackUntilMs_ = nowMs + FACE_FEEDBACK_SUBTITLE_MS;
}

void CameraAttentionController::showCameraFeedbackSubtitle(RobotFaceRenderer& face, uint32_t nowMs, const String& text, uint32_t durationMs) {
  if (!feedbackSubtitleEnabled_) return;
  SubtitleState subtitle = face.subtitle();
  face.setSubtitle(text, true, subtitle.color);
  cameraFeedbackSubtitleText_ = text;
  faceFeedbackSubtitleActive_ = true;
  faceFeedbackUntilMs_ = nowMs + durationMs;
}

void CameraAttentionController::clearFaceFeedbackSubtitle(RobotFaceRenderer& face, uint32_t nowMs) {
  if (!faceFeedbackSubtitleActive_) return;
  if (nowMs < faceFeedbackUntilMs_) return;
  SubtitleState subtitle = face.subtitle();
  if (subtitle.text == cameraFeedbackSubtitleText_) {
    face.clearSubtitle();
  }
  faceFeedbackSubtitleActive_ = false;
  cameraFeedbackSubtitleText_ = "";
}

void CameraAttentionController::playCatchphrase(MumblePlayer& mumble, uint32_t nowMs) {
  String slot = catchphraseAssetId_;
  if (slot == "halo") slot = "hello_human";
  if (slot.startsWith("sound.")) slot = slot.substring(String("sound.").length());
  mumble.playEmotionSlotJson(slot, nowMs, 0);
  catchphrasePlayed_ = true;
  mode_ = CameraAttentionMode::Catchphrase;
  lastReason_ = "catchphrase";
}

void CameraAttentionController::startListeningAfterCatchphrase(RobotClock& clock, uint32_t nowMs) {
  if (!listenAfterCatchphrase_) return;
  clock.markInteraction(nowMs);
  lastReason_ = "listenAfterCatchphrase";
}

void CameraAttentionController::enterCooldown(LocalFaceDetector& detector, uint32_t nowMs, const char* reason) {
  detector.cancel(nowMs);
  active_ = false;
  captureRequested_ = false;
  confirming_ = false;
  mode_ = CameraAttentionMode::Cooldown;
  modeUntilMs_ = nowMs + cooldownMs_;
  nextCaptureAtMs_ = modeUntilMs_;
  lastReason_ = reason;
}

void CameraAttentionController::update(RobotHardware& hardware, RobotFaceRenderer& face, IdleBehaviorController& idle, RobotClock& clock, MumblePlayer& mumble, LocalFaceDetector& detector, bool emotionActive, bool proximityActive, bool voiceBusy, uint32_t nowMs) {
  clearFaceFeedbackSubtitle(face, nowMs);
  detector.update(nowMs);
  if (!enabled_) {
    active_ = false;
    mode_ = CameraAttentionMode::Disabled;
    return;
  }
  if (!hardware.isReady() || emotionActive || proximityActive || voiceBusy) {
    if (!active_) mode_ = CameraAttentionMode::Paused;
    return;
  }

  LocalFaceResult result;
  if ((mode_ == CameraAttentionMode::Detecting || mode_ == CameraAttentionMode::Confirming) && detector.takeResult(result)) {
    if (result.snapshotId != snapshot_.id) {
      enterCooldown(detector, nowMs, "staleResult");
      return;
    }
    if (!result.faceDetected) {
      showCameraFeedbackSubtitle(face, nowMs, "No face", CAMERA_NO_FACE_FEEDBACK_SUBTITLE_MS);
      enterCooldown(detector, nowMs, confirming_ ? "confirmNoFace" : "noFace");
      return;
    }
    if (!confirming_) {
      lastFace_ = result;
      returnToSnapshot(hardware, face, idle, nowMs);
      return;
    }
    enterFaceLock(result, hardware, face, idle, nowMs);
    return;
  }

  if ((mode_ == CameraAttentionMode::Detecting || mode_ == CameraAttentionMode::Confirming) && nowMs >= modeUntilMs_) {
    enterCooldown(detector, nowMs, "detectTimeout");
    return;
  }

  if (mode_ == CameraAttentionMode::ReturningToSnapshot && nowMs >= modeUntilMs_) {
    startConfirmCapture(detector, nowMs);
    return;
  }

  if (mode_ == CameraAttentionMode::FaceLock) {
    if (!catchphrasePlayed_) {
      playCatchphrase(mumble, nowMs);
      startListeningAfterCatchphrase(clock, nowMs);
      return;
    }
    if (nowMs >= modeUntilMs_) {
      enterCooldown(detector, nowMs, "lockComplete");
      return;
    }
    applyFaceCentering(lastFace_, hardware, face, idle, nowMs);
    return;
  }

  if (mode_ == CameraAttentionMode::Catchphrase) {
    if (nowMs >= modeUntilMs_) {
      enterCooldown(detector, nowMs, "catchphraseComplete");
      return;
    }
    active_ = true;
    return;
  }

  if (mode_ == CameraAttentionMode::Cooldown && nowMs < modeUntilMs_) {
    return;
  }

  if (nowMs >= nextCaptureAtMs_) {
    captureCurrentPose(hardware, face, detector, nowMs);
  } else {
    active_ = false;
    mode_ = CameraAttentionMode::Idle;
  }
}

bool CameraAttentionController::reportFace(const CameraFaceReport& report, RobotHardware& hardware, RobotFaceRenderer& face, IdleBehaviorController& idle, RobotClock& clock, MumblePlayer& mumble, LocalFaceDetector& detector, bool emotionActive, uint32_t nowMs) {
  (void)mumble;
  if (emotionActive) {
    lastReason_ = "reportIgnoredDuringEmotion";
    return false;
  }
  LocalFaceResult result;
  result.snapshotId = report.scanId == 0 ? snapshot_.id : report.scanId;
  result.faceDetected = report.faceDetected;
  result.confidence = report.confidence;
  result.faceCenterX = report.faceCenterX;
  result.faceCenterY = report.faceCenterY;
  result.faceWidth = report.faceWidth;
  result.faceHeight = report.faceHeight;
  bool refreshingForcedFace = report.force && report.faceDetected && (mode_ == CameraAttentionMode::FaceLock || mode_ == CameraAttentionMode::Catchphrase);

  if (report.force) {
    if (!report.faceDetected) {
      enterCooldown(detector, nowMs, "syntheticNoFaceForced");
      return true;
    }
    if (snapshot_.id == 0 || (report.scanId > 0 && report.scanId != snapshot_.id)) {
      MotionState motion = hardware.currentMotion();
      FaceReactState react = face.react();
      uint32_t forcedSnapshotId = report.scanId > snapshotId_ ? report.scanId : snapshotId_ + 1;
      snapshotId_ = forcedSnapshotId;
      snapshot_ = {forcedSnapshotId, (int16_t)motion.yaw, (int16_t)motion.pitch, react.lookX, react.lookY, nowMs};
    }
    if (report.hasPoseOverride) {
      snapshot_.yaw = report.yaw;
      snapshot_.pitch = report.pitch;
    }
    result.snapshotId = snapshot_.id;
    detector.cancel(nowMs);
    clock.markInteraction(nowMs);
    if (refreshingForcedFace) {
      lastFace_ = result;
      active_ = true;
      captureRequested_ = false;
      confirming_ = false;
      modeUntilMs_ = nowMs + lockHoldMs_;
      lastReason_ = "syntheticFaceRefreshed";
      applyFaceCentering(result, hardware, face, idle, nowMs);
      return true;
    }
    enterFaceLock(result, hardware, face, idle, nowMs);
    lastReason_ = "syntheticFaceForced";
    return true;
  }

  if (report.hasPoseOverride) {
    targetYaw_ = report.yaw;
    targetPitch_ = report.pitch;
  }
  detector.injectResult(result, nowMs);
  lastReason_ = "syntheticFaceReport";
  return true;
}

String CameraAttentionController::handleVoiceWake(RobotHardware& hardware, RobotFaceRenderer& face, IdleBehaviorController& idle, RobotClock& clock, uint32_t nowMs, uint32_t holdMs) {
  clock.markInteraction(nowMs);
  targetYaw_ = lastFace_.faceDetected ? targetYaw_ : snapshot_.yaw;
  targetPitch_ = lastFace_.faceDetected ? targetPitch_ : snapshot_.pitch;
  uint32_t durationMs = holdMs > 0 ? constrain(holdMs, MIN_LOCK_HOLD_MS, MAX_LOCK_HOLD_MS) : DEFAULT_WAKE_HOLD_MS;
  face.setMood("curious");
  face.setReact(snapshot_.lookX, snapshot_.lookY, 0.10f);
  hardware.pose(targetYaw_, targetPitch_, 72);
  idle.pauseForManualControl(nowMs, durationMs + 200);
  active_ = true;
  mode_ = CameraAttentionMode::FaceLock;
  modeUntilMs_ = nowMs + durationMs;
  lastReason_ = "voiceWake";

  String json = "{\"ok\":true";
  json += ",\"usedFaceTarget\":" + String(lastFace_.faceDetected ? "true" : "false");
  json += ",\"lastFaceValid\":" + String(lastFace_.faceDetected ? "true" : "false");
  json += ",\"mode\":\"" + String(modeName()) + "\"";
  json += "}";
  return json;
}

void CameraAttentionController::clear(LocalFaceDetector& detector, uint32_t nowMs) {
  detector.cancel(nowMs);
  active_ = false;
  captureRequested_ = false;
  confirming_ = false;
  catchphrasePlayed_ = false;
  faceFeedbackSubtitleActive_ = false;
  cameraFeedbackSubtitleText_ = "";
  lastFace_ = {};
  mode_ = CameraAttentionMode::Idle;
  modeUntilMs_ = 0;
  nextCaptureAtMs_ = nowMs + captureIntervalMs_;
  lastReason_ = "cleared";
}

String CameraAttentionController::statusJson(const LocalFaceDetector& detector) const {
  uint32_t nowMs = millis();
  String json = "{\"ok\":true";
  json += ",\"enabled\":" + String(enabled_ ? "true" : "false");
  json += ",\"active\":" + String(active_ ? "true" : "false");
  json += ",\"mode\":\"" + String(modeName()) + "\"";
  json += ",\"reason\":" + quoteCameraString(String(lastReason_));
  json += ",\"snapshotId\":" + String(snapshotId_);
  json += ",\"captureRequested\":" + String(captureRequested_ ? "true" : "false");
  json += ",\"detecting\":" + String(detector.detecting() ? "true" : "false");
  json += ",\"confirming\":" + String(confirming_ ? "true" : "false");
  json += ",\"captureIntervalMs\":" + String(captureIntervalMs_);
  json += ",\"detectTimeoutMs\":" + String(detectTimeoutMs_);
  json += ",\"returnSettleMs\":" + String(returnSettleMs_);
  json += ",\"confirmSettleMs\":" + String(confirmSettleMs_);
  json += ",\"lockHoldMs\":" + String(lockHoldMs_);
  json += ",\"cooldownMs\":" + String(cooldownMs_);
  json += ",\"cooldownRemainingMs\":" + String(mode_ == CameraAttentionMode::Cooldown ? remainingMs(nowMs, modeUntilMs_) : 0);
  json += ",\"modeRemainingMs\":" + String(remainingMs(nowMs, modeUntilMs_));
  json += ",\"nextCaptureInMs\":" + String(remainingMs(nowMs, nextCaptureAtMs_));
  json += ",\"snapshotYaw\":" + String(snapshot_.yaw);
  json += ",\"snapshotPitch\":" + String(snapshot_.pitch);
  json += ",\"targetYaw\":" + String(targetYaw_);
  json += ",\"targetPitch\":" + String(targetPitch_);
  json += ",\"centerDeadZoneX\":" + String(centerDeadZoneX_, 2);
  json += ",\"centerDeadZoneY\":" + String(centerDeadZoneY_, 2);
  json += ",\"yawStepMax\":" + String(yawStepMax_);
  json += ",\"pitchStepMax\":" + String(pitchStepMax_);
  json += ",\"lastFaceDetected\":" + String(lastFace_.faceDetected ? "true" : "false");
  json += ",\"confidence\":" + String(lastFace_.confidence, 2);
  json += ",\"faceCenterX\":" + String(lastFace_.faceCenterX, 2);
  json += ",\"faceCenterY\":" + String(lastFace_.faceCenterY, 2);
  json += ",\"faceWidth\":" + String(lastFace_.faceWidth, 2);
  json += ",\"faceHeight\":" + String(lastFace_.faceHeight, 2);
  json += ",\"lastDetectDurationMs\":" + String(lastFace_.durationMs);
  json += ",\"catchphraseAssetId\":" + quoteCameraString(catchphraseAssetId_);
  json += ",\"catchphrasePlayed\":" + String(catchphrasePlayed_ ? "true" : "false");
  json += ",\"listenAfterCatchphrase\":" + String(listenAfterCatchphrase_ ? "true" : "false");
  json += ",\"feedbackSubtitleEnabled\":" + String(feedbackSubtitleEnabled_ ? "true" : "false");
  json += ",\"detector\":";
  json += detector.statusJson();
  json += "}";
  return json;
}

uint32_t CameraAttentionController::remainingMs(uint32_t nowMs, uint32_t targetMs) const {
  return nowMs < targetMs ? targetMs - nowMs : 0;
}
