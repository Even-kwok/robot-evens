#pragma once

#include <Arduino.h>
#include <WebServer.h>

struct LocalFaceRequest {
  uint32_t snapshotId = 0;
  uint32_t requestedAtMs = 0;
};

struct LocalFaceResult {
  uint32_t snapshotId = 0;
  bool faceDetected = false;
  float confidence = 0.0f;
  float faceCenterX = 0.5f;
  float faceCenterY = 0.5f;
  float faceWidth = 0.0f;
  float faceHeight = 0.0f;
  uint16_t frameWidth = 0;
  uint16_t frameHeight = 0;
  uint32_t durationMs = 0;
  String error = "";
};

class LocalFaceDetector {
public:
  void begin();
  bool initialized() const;
  void update(uint32_t nowMs);
  bool submitCapture(const LocalFaceRequest& request, uint32_t nowMs);
  bool detecting() const;
  bool cameraAvailable() const;
  bool hasResult() const;
  bool takeResult(LocalFaceResult& result);
  void injectResult(const LocalFaceResult& result, uint32_t nowMs);
  void cancel(uint32_t nowMs);
  bool streamPreviewBmp(WebServer& server, String& error);
  String statusJson() const;

private:
  static void detectionTaskThunk(void* arg);
  void runDetectionTask();

  bool initialized_ = false;
  bool cameraAvailable_ = false;
  bool detecting_ = false;
  bool taskRunning_ = false;
  bool hasResult_ = false;
  uint32_t activeSnapshotId_ = 0;
  uint32_t requestedAtMs_ = 0;
  uint32_t taskSnapshotId_ = 0;
  uint32_t taskRequestedAtMs_ = 0;
  uint32_t lastDetectDurationMs_ = 0;
  String lastError_ = "";
  LocalFaceResult lastResult_ = {};
};
