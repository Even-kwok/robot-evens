#include "local_face_detector.h"

#if __has_include("config.local.h")
#include "config.local.h"
#endif
#include "config.example.h"

#if ROBOT_LOCAL_FACE_DETECTION_ENABLED
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#endif

#if ROBOT_LOCAL_FACE_DETECTION_ENABLED && __has_include(<esp_camera.h>)
#include <esp_camera.h>
#define ROBOT_FACE_CAMERA_HEADERS_AVAILABLE 1
#else
#define ROBOT_FACE_CAMERA_HEADERS_AVAILABLE 0
#endif

#if ROBOT_FACE_CAMERA_HEADERS_AVAILABLE && ROBOT_LOCAL_FACE_DETECTION_USE_ESP_WHO && __has_include("human_face_detect_msr01.hpp") && __has_include("human_face_detect_mnp01.hpp")
#include "human_face_detect_msr01.hpp"
#include "human_face_detect_mnp01.hpp"
#define ROBOT_FACE_INFERENCE_AVAILABLE 1
#else
#define ROBOT_FACE_INFERENCE_AVAILABLE 0
#endif

namespace {

String jsonQuoteFace(const String& value) {
  String out = "\"";
  for (uint16_t i = 0; i < value.length(); ++i) {
    char c = value.charAt(i);
    if (c == '"' || c == '\\') out += "\\";
    out += c;
  }
  out += "\"";
  return out;
}

float clampUnit(float value) {
  if (value < 0.0f) return 0.0f;
  if (value > 1.0f) return 1.0f;
  return value;
}

#if ROBOT_FACE_CAMERA_HEADERS_AVAILABLE
void putLe16(uint8_t* data, uint8_t offset, uint16_t value) {
  data[offset] = value & 0xff;
  data[offset + 1] = (value >> 8) & 0xff;
}

void putLe32(uint8_t* data, uint8_t offset, uint32_t value) {
  data[offset] = value & 0xff;
  data[offset + 1] = (value >> 8) & 0xff;
  data[offset + 2] = (value >> 16) & 0xff;
  data[offset + 3] = (value >> 24) & 0xff;
}

bool cameraPinsConfigured() {
  return ROBOT_LOCAL_FACE_CAMERA_PIN_D0 >= 0 &&
      ROBOT_LOCAL_FACE_CAMERA_PIN_D1 >= 0 &&
      ROBOT_LOCAL_FACE_CAMERA_PIN_D2 >= 0 &&
      ROBOT_LOCAL_FACE_CAMERA_PIN_D3 >= 0 &&
      ROBOT_LOCAL_FACE_CAMERA_PIN_D4 >= 0 &&
      ROBOT_LOCAL_FACE_CAMERA_PIN_D5 >= 0 &&
      ROBOT_LOCAL_FACE_CAMERA_PIN_D6 >= 0 &&
      ROBOT_LOCAL_FACE_CAMERA_PIN_D7 >= 0 &&
      ROBOT_LOCAL_FACE_CAMERA_PIN_PCLK >= 0 &&
      ROBOT_LOCAL_FACE_CAMERA_PIN_VSYNC >= 0 &&
      ROBOT_LOCAL_FACE_CAMERA_PIN_HREF >= 0 &&
      ROBOT_LOCAL_FACE_CAMERA_PIN_SIOD >= 0 &&
      ROBOT_LOCAL_FACE_CAMERA_PIN_SIOC >= 0;
}

bool tryInitCameraDriver(String& error) {
  if (!ROBOT_LOCAL_FACE_CAMERA_AUTO_INIT) {
    error = "camera auto init disabled";
    return false;
  }
  if (!cameraPinsConfigured()) {
    error = "camera pins not configured";
    return false;
  }

  camera_config_t config = {};
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = ROBOT_LOCAL_FACE_CAMERA_PIN_D0;
  config.pin_d1 = ROBOT_LOCAL_FACE_CAMERA_PIN_D1;
  config.pin_d2 = ROBOT_LOCAL_FACE_CAMERA_PIN_D2;
  config.pin_d3 = ROBOT_LOCAL_FACE_CAMERA_PIN_D3;
  config.pin_d4 = ROBOT_LOCAL_FACE_CAMERA_PIN_D4;
  config.pin_d5 = ROBOT_LOCAL_FACE_CAMERA_PIN_D5;
  config.pin_d6 = ROBOT_LOCAL_FACE_CAMERA_PIN_D6;
  config.pin_d7 = ROBOT_LOCAL_FACE_CAMERA_PIN_D7;
  config.pin_xclk = ROBOT_LOCAL_FACE_CAMERA_PIN_XCLK;
  config.pin_pclk = ROBOT_LOCAL_FACE_CAMERA_PIN_PCLK;
  config.pin_vsync = ROBOT_LOCAL_FACE_CAMERA_PIN_VSYNC;
  config.pin_href = ROBOT_LOCAL_FACE_CAMERA_PIN_HREF;
  config.pin_sccb_sda = ROBOT_LOCAL_FACE_CAMERA_PIN_SIOD;
  config.pin_sccb_scl = ROBOT_LOCAL_FACE_CAMERA_PIN_SIOC;
  config.pin_pwdn = ROBOT_LOCAL_FACE_CAMERA_PIN_PWDN;
  config.pin_reset = ROBOT_LOCAL_FACE_CAMERA_PIN_RESET;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_240X240;
  config.pixel_format = PIXFORMAT_RGB565;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = psramFound() ? CAMERA_FB_IN_PSRAM : CAMERA_FB_IN_DRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err == ESP_OK || err == ESP_ERR_INVALID_STATE) return true;
  error = "camera init failed 0x" + String((uint32_t)err, HEX);
  return false;
}
#endif

#if ROBOT_FACE_INFERENCE_AVAILABLE
LocalFaceResult runFaceInference(uint32_t snapshotId, uint32_t requestedAtMs) {
  LocalFaceResult result;
  result.snapshotId = snapshotId;

  camera_fb_t* frame = esp_camera_fb_get();
  if (frame == nullptr) {
    result.durationMs = millis() - requestedAtMs;
    result.error = "camera frame unavailable";
    return result;
  }

  result.frameWidth = frame->width;
  result.frameHeight = frame->height;
  if (frame->format != PIXFORMAT_RGB565) {
    result.durationMs = millis() - requestedAtMs;
    result.error = "unsupported frame format";
    esp_camera_fb_return(frame);
    return result;
  }

  HumanFaceDetectMSR01 stage1(0.1F, 0.5F, 10, 0.2F);
  HumanFaceDetectMNP01 stage2(0.5F, 0.3F, 5);
  std::list<dl::detect::result_t>& candidates = stage1.infer((uint16_t*)frame->buf, {(int)frame->height, (int)frame->width, 3});
  std::list<dl::detect::result_t>& faces = stage2.infer((uint16_t*)frame->buf, {(int)frame->height, (int)frame->width, 3}, candidates);

  if (faces.empty()) {
    result.durationMs = millis() - requestedAtMs;
    result.error = "no face";
    esp_camera_fb_return(frame);
    return result;
  }

  const dl::detect::result_t* best = nullptr;
  for (const auto& face : faces) {
    if (face.box.size() < 4) continue;
    if (best == nullptr || face.score > best->score) best = &face;
  }
  if (best == nullptr) {
    result.durationMs = millis() - requestedAtMs;
    result.error = "no valid face box";
    esp_camera_fb_return(frame);
    return result;
  }

  int left = constrain(best->box[0], 0, (int)frame->width - 1);
  int top = constrain(best->box[1], 0, (int)frame->height - 1);
  int right = constrain(best->box[2], left, (int)frame->width - 1);
  int bottom = constrain(best->box[3], top, (int)frame->height - 1);
  int boxWidth = right - left + 1;
  int boxHeight = bottom - top + 1;

  result.faceDetected = true;
  result.confidence = best->score;
  result.faceCenterX = ((float)left + (float)boxWidth * 0.5f) / (float)frame->width;
  result.faceCenterY = ((float)top + (float)boxHeight * 0.5f) / (float)frame->height;
  result.faceWidth = (float)boxWidth / (float)frame->width;
  result.faceHeight = (float)boxHeight / (float)frame->height;
  result.durationMs = millis() - requestedAtMs;
  esp_camera_fb_return(frame);
  return result;
}
#endif

}  // namespace

void LocalFaceDetector::begin() {
  if (initialized_) return;
  initialized_ = true;
  cameraAvailable_ = false;
  detecting_ = false;
  taskRunning_ = false;
  hasResult_ = false;
  activeSnapshotId_ = 0;
  requestedAtMs_ = 0;
  lastDetectDurationMs_ = 0;
  lastError_ = "camera driver unavailable";
#if ROBOT_FACE_CAMERA_HEADERS_AVAILABLE
  String cameraError;
  cameraAvailable_ = tryInitCameraDriver(cameraError);
  lastError_ = cameraError.length() ? cameraError : "";
#endif
  lastResult_ = {};
}

bool LocalFaceDetector::initialized() const {
  return initialized_;
}

void LocalFaceDetector::update(uint32_t nowMs) {
  if (!detecting_) return;
  if (cameraAvailable_ && taskRunning_) return;
  if (!cameraAvailable_) {
    LocalFaceResult result;
    result.snapshotId = activeSnapshotId_;
    result.durationMs = nowMs - requestedAtMs_;
    result.error = "camera unavailable";
    injectResult(result, nowMs);
    return;
  }

#if ROBOT_FACE_INFERENCE_AVAILABLE
  return;
#endif

  LocalFaceResult result;
  result.snapshotId = activeSnapshotId_;
  result.durationMs = nowMs - requestedAtMs_;
  result.error =
#if ROBOT_FACE_INFERENCE_AVAILABLE
      "face inference hook not configured";
#else
      "face inference unavailable";
#endif
  injectResult(result, nowMs);
}

bool LocalFaceDetector::submitCapture(const LocalFaceRequest& request, uint32_t nowMs) {
  if (detecting_ || taskRunning_) return false;
  activeSnapshotId_ = request.snapshotId;
  requestedAtMs_ = nowMs;
  taskSnapshotId_ = request.snapshotId;
  taskRequestedAtMs_ = nowMs;
  detecting_ = true;
  hasResult_ = false;
  lastError_ = "";
#if ROBOT_FACE_INFERENCE_AVAILABLE
  if (cameraAvailable_) {
    taskRunning_ = true;
    BaseType_t created = xTaskCreatePinnedToCore(
      detectionTaskThunk,
      "face_detect",
      ROBOT_LOCAL_FACE_DETECTION_TASK_STACK,
      this,
      1,
      nullptr,
      ROBOT_LOCAL_FACE_DETECTION_TASK_CORE
    );
    if (created != pdPASS) {
      taskRunning_ = false;
      LocalFaceResult result;
      result.snapshotId = activeSnapshotId_;
      result.durationMs = nowMs - requestedAtMs_;
      result.error = "face task create failed";
      injectResult(result, nowMs);
    }
  }
#endif
  return true;
}

bool LocalFaceDetector::detecting() const {
  return detecting_;
}

bool LocalFaceDetector::cameraAvailable() const {
  return cameraAvailable_;
}

bool LocalFaceDetector::hasResult() const {
  return hasResult_;
}

bool LocalFaceDetector::takeResult(LocalFaceResult& result) {
  if (!hasResult_) return false;
  result = lastResult_;
  hasResult_ = false;
  return true;
}

void LocalFaceDetector::injectResult(const LocalFaceResult& result, uint32_t nowMs) {
  lastResult_ = result;
  lastResult_.confidence = clampUnit(lastResult_.confidence);
  lastResult_.faceCenterX = clampUnit(lastResult_.faceCenterX);
  lastResult_.faceCenterY = clampUnit(lastResult_.faceCenterY);
  lastResult_.faceWidth = clampUnit(lastResult_.faceWidth);
  lastResult_.faceHeight = clampUnit(lastResult_.faceHeight);
  if (lastResult_.durationMs == 0 && requestedAtMs_ > 0) {
    lastResult_.durationMs = nowMs - requestedAtMs_;
  }
  lastDetectDurationMs_ = lastResult_.durationMs;
  lastError_ = lastResult_.error;
  activeSnapshotId_ = lastResult_.snapshotId;
  detecting_ = false;
  taskRunning_ = false;
  hasResult_ = true;
}

void LocalFaceDetector::cancel(uint32_t nowMs) {
  (void)nowMs;
  detecting_ = false;
  hasResult_ = false;
  activeSnapshotId_ = 0;
}

bool LocalFaceDetector::streamPreviewBmp(WebServer& server, String& error) {
  if (!cameraAvailable_) {
    error = "camera unavailable";
    return false;
  }
  if (taskRunning_) {
    error = "face task running";
    return false;
  }

#if ROBOT_FACE_CAMERA_HEADERS_AVAILABLE
  camera_fb_t* frame = esp_camera_fb_get();
  if (frame == nullptr) {
    error = "camera frame unavailable";
    return false;
  }

  if (frame->format != PIXFORMAT_RGB565) {
    error = "unsupported frame format";
    esp_camera_fb_return(frame);
    return false;
  }

  const uint16_t width = frame->width;
  const uint16_t height = frame->height;
  const size_t expectedBytes = (size_t)width * (size_t)height * 2;
  if (width == 0 || height == 0 || expectedBytes > frame->len) {
    error = "short frame";
    esp_camera_fb_return(frame);
    return false;
  }

  const uint32_t rowSize = ((uint32_t)width * 3 + 3) & ~3UL;
  uint8_t row[1024] = {};
  if (rowSize > sizeof(row)) {
    error = "preview frame too wide";
    esp_camera_fb_return(frame);
    return false;
  }

  const uint32_t imageSize = rowSize * height;
  const uint32_t fileSize = 54 + imageSize;
  uint8_t header[54] = {};
  header[0] = 'B';
  header[1] = 'M';
  putLe32(header, 2, fileSize);
  putLe32(header, 10, 54);
  putLe32(header, 14, 40);
  putLe32(header, 18, width);
  putLe32(header, 22, height);
  putLe16(header, 26, 1);
  putLe16(header, 28, 24);
  putLe32(header, 34, imageSize);

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Cache-Control", "no-store");
  server.sendHeader("Content-Type", "image/bmp");
  server.setContentLength(fileSize);
  server.send(200, "image/bmp", "");

  WiFiClient client = server.client();
  client.write(header, sizeof(header));
  for (int y = (int)height - 1; y >= 0; --y) {
    const uint8_t* src = frame->buf + ((size_t)y * width * 2);
    for (uint16_t x = 0; x < width; ++x) {
      uint16_t pixel = (uint16_t)src[x * 2] | ((uint16_t)src[x * 2 + 1] << 8);
      uint8_t r = pixel & 0xf8;
      uint8_t g = ((pixel & 0x0007) << 5) | ((pixel & 0xe000) >> 11);
      uint8_t b = (pixel & 0x1f00) >> 5;
      row[x * 3] = b;
      row[x * 3 + 1] = g;
      row[x * 3 + 2] = r;
    }
    for (uint32_t i = (uint32_t)width * 3; i < rowSize; ++i) row[i] = 0;
    client.write(row, rowSize);
  }
  esp_camera_fb_return(frame);
  return true;
#else
  error = "camera headers unavailable";
  return false;
#endif
}

String LocalFaceDetector::statusJson() const {
  String json = "{\"cameraAvailable\":";
  json += cameraAvailable_ ? "true" : "false";
  json += ",\"initialized\":";
  json += initialized_ ? "true" : "false";
  json += ",\"inferenceAvailable\":";
  json += ROBOT_FACE_INFERENCE_AVAILABLE ? "true" : "false";
  json += ",\"detecting\":";
  json += detecting_ ? "true" : "false";
  json += ",\"taskRunning\":";
  json += taskRunning_ ? "true" : "false";
  json += ",\"snapshotId\":";
  json += String(activeSnapshotId_);
  json += ",\"lastDetectDurationMs\":";
  json += String(lastDetectDurationMs_);
  json += ",\"lastFrameWidth\":";
  json += String(lastResult_.frameWidth);
  json += ",\"lastFrameHeight\":";
  json += String(lastResult_.frameHeight);
  json += ",\"lastFaceDetected\":";
  json += lastResult_.faceDetected ? "true" : "false";
  json += ",\"lastConfidence\":";
  json += String(lastResult_.confidence, 2);
  json += ",\"lastError\":";
  json += jsonQuoteFace(lastError_);
  json += "}";
  return json;
}

void LocalFaceDetector::detectionTaskThunk(void* arg) {
  static_cast<LocalFaceDetector*>(arg)->runDetectionTask();
  vTaskDelete(nullptr);
}

void LocalFaceDetector::runDetectionTask() {
#if ROBOT_FACE_INFERENCE_AVAILABLE
  LocalFaceResult result = runFaceInference(taskSnapshotId_, taskRequestedAtMs_);
  injectResult(result, millis());
#else
  taskRunning_ = false;
#endif
}
