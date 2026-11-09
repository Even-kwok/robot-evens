#include <Arduino.h>
#include <M5Unified.hpp>
#include <M5StackChan.h>
#include <WiFi.h>
#include <WebServer.h>

#if __has_include("config.local.h")
#include "config.local.h"
#endif
#include "config.example.h"
#include "api.h"
#include "assistant_status.h"
#include "camera_attention.h"
#include "emotion_actions.h"
#include "emotion_triggers.h"
#include "face_renderer.h"
#include "firmware_version.h"
#include "idle_behavior.h"
#include "local_face_detector.h"
#include "motion_stream.h"
#include "mumble_player.h"
#include "proximity_tracker.h"
#include "robot_clock.h"
#include "robot_hardware.h"
#include "wifi_manager.h"

WebServer server(80);
RobotFaceRenderer faceRenderer;
RobotHardware hardware;
IdleBehaviorController idleBehavior;
EmotionActionController emotionActions;
EmotionTriggerController emotionTriggers;
MumblePlayer mumble;
RobotClock robotClock;
ProximityTrackerController proximityTracker;
CameraAttentionController cameraAttention;
LocalFaceDetector localFaceDetector;
MotionStreamReceiver motionStream;
AssistantStatusDisplay assistantStatus;
uint32_t lastStatusLogMs = 0;
uint32_t networkOverlayUntilMs = 0;
uint32_t setupPortalScreenDrawMs = 0;
uint32_t lastWifiDisplayRevision = 0;
TaskHandle_t hardwareInitTaskHandle = nullptr;
RobotWifiManager wifiManager;

constexpr uint8_t WIFI_SETUP_CLICK_TARGET = 3;
constexpr uint32_t WIFI_SETUP_CLICK_WINDOW_MS = 1400;
constexpr uint32_t POWER_BUTTON_WAKE_DEFER_MS = WIFI_SETUP_CLICK_WINDOW_MS;
uint8_t powerButtonClickCount = 0;
uint32_t firstPowerButtonClickMs = 0;
uint32_t lastPowerButtonClickMs = 0;
bool powerButtonWakePending = false;

void showBootLine(const String& line) {
  Serial.println(line);
  M5.Display.println(line);
}

void drawSetupPortalScreen(bool force = false) {
  if (!wifiManager.setupPortalActive()) return;
  uint32_t nowMs = millis();
  if (!force && nowMs - setupPortalScreenDrawMs < 1000) return;
  setupPortalScreenDrawMs = nowMs;
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setTextSize(3);
  M5.Display.setCursor(8, 12);
  M5.Display.println("Desktop Robot");
  M5.Display.setTextSize(2);
  M5.Display.setCursor(8, 54);
  auto printSetupLine = [](const String& value) {
    String line = value;
    if (line.length() > 25) line = line.substring(0, 24) + "~";
    M5.Display.println(line);
  };
  printSetupLine("3x press: setup");
  printSetupLine("Wi-Fi: " + wifiManager.wifiStatusLabel());
  printSetupLine("State: " + String(wifiManager.statusName(WiFi.status())));
  String connectedSsid = wifiManager.currentSsid();
  printSetupLine(connectedSsid.length() ? "SSID: " + connectedSsid : "Target: " + wifiManager.configuredSsid());
  printSetupLine("Robot: " + wifiManager.firmwareAddress());
  printSetupLine("Version: " + String(ROBOT_FIRMWARE_VERSION));
  printSetupLine("AP: " + String(ROBOT_AP_SSID));
  printSetupLine("Pass: " + String(ROBOT_AP_PASSWORD));
  printSetupLine("Setup: 192.168.4.1");
}

void drawNetworkOverlay(uint32_t nowMs) {
  if (wifiManager.setupPortalActive()) return;
  if (WiFi.status() == WL_CONNECTED) return;
  if (nowMs > networkOverlayUntilMs) return;
  M5.Display.fillRect(0, 0, 320, 34, TFT_BLACK);
  M5.Display.setTextSize(1);
  M5.Display.setTextColor(TFT_ORANGE, TFT_BLACK);
  M5.Display.setCursor(4, 5);
  M5.Display.print("STA ");
  M5.Display.print(wifiManager.lastStatusSummary());
  M5.Display.setCursor(4, 20);
  M5.Display.print(wifiManager.lastScanSummary());
}

void restoreRobotFaceScreen(uint32_t nowMs) {
  M5.Display.fillScreen(TFT_BLACK);
  faceRenderer.begin(M5.Display);
  // Redraw immediately: the setup screen overwrites the framebuffer, so
  // waiting for the normal animation cadence can leave stale setup text on
  // screen after the hotspot is closed.
  faceRenderer.update(M5.Display, nowMs + 34);
}

void hardwareInitTask(void*) {
  hardware.setInitStatus("initializing");
  Serial.println("[hardware] StackChan init start");
  M5StackChan.begin();
  Serial.println("[hardware] StackChan begin done");
  M5StackChan.Motion.setAutoAngleSyncEnabled(false);
  hardware.begin();
  hardware.markReady();
  Serial.println("[hardware] ready");
  vTaskDelete(nullptr);
}

void startHardwareInit() {
  if (hardwareInitTaskHandle != nullptr) return;
  hardware.setInitStatus("queued");
  xTaskCreatePinnedToCore(
    hardwareInitTask,
    "robot_hw_init",
    8192,
    nullptr,
    1,
    &hardwareInitTaskHandle,
    0
  );
}

void connectWifi() {
  wifiManager.begin(ROBOT_WIFI_SSID, ROBOT_WIFI_PASSWORD, ROBOT_MDNS_NAME);
  bool connected = wifiManager.connectBoot();
  showBootLine(wifiManager.lastScanSummary());
  showBootLine(wifiManager.lastStatusSummary());
  if (!connected) showBootLine("3 presses: open Wi-Fi setup");
  lastWifiDisplayRevision = wifiManager.displayRevision();
  networkOverlayUntilMs = millis() + 30000;
}

void resetPowerButtonClickWindow() {
  powerButtonClickCount = 0;
  firstPowerButtonClickMs = 0;
  lastPowerButtonClickMs = 0;
  powerButtonWakePending = false;
}

void handlePowerButtonClick(uint32_t nowMs) {
  if (!firstPowerButtonClickMs || nowMs - firstPowerButtonClickMs > WIFI_SETUP_CLICK_WINDOW_MS) {
    powerButtonClickCount = 0;
    firstPowerButtonClickMs = nowMs;
  }
  ++powerButtonClickCount;
  lastPowerButtonClickMs = nowMs;
  powerButtonWakePending = true;
  if (powerButtonClickCount < WIFI_SETUP_CLICK_TARGET) return;

  resetPowerButtonClickWindow();
  if (wifiManager.setupPortalActive()) {
    wifiManager.closeSetupPortal();
    networkOverlayUntilMs = nowMs;
    restoreRobotFaceScreen(nowMs);
    showBootLine("Setup closed: robot face restored");
    lastWifiDisplayRevision = wifiManager.displayRevision();
    return;
  }
  if (!wifiManager.openSetupPortal("power-triple")) {
    showBootLine("Setup hotspot failed");
    restoreRobotFaceScreen(nowMs);
    return;
  }
  showBootLine("Setup hotspot ready");
  networkOverlayUntilMs = nowMs + 60000;
  lastWifiDisplayRevision = wifiManager.displayRevision();
  drawSetupPortalScreen(true);
}

void updatePowerButtonClickWindow(uint32_t nowMs) {
  if (!powerButtonWakePending) return;
  bool windowExpired = nowMs - firstPowerButtonClickMs > WIFI_SETUP_CLICK_WINDOW_MS;
  bool deferExpired = nowMs - lastPowerButtonClickMs > POWER_BUTTON_WAKE_DEFER_MS;
  if (!windowExpired && !deferExpired) return;
  if (powerButtonClickCount == 1) {
    handlePowerButtonWake(faceRenderer, idleBehavior, robotClock, nowMs);
  }
  resetPowerButtonClickWindow();
}

void syncWifiDisplayRevision(uint32_t nowMs) {
  uint32_t revision = wifiManager.displayRevision();
  if (revision == lastWifiDisplayRevision) return;
  lastWifiDisplayRevision = revision;
  networkOverlayUntilMs = nowMs + 30000;
  if (wifiManager.setupPortalActive()) drawSetupPortalScreen(true);
  if (!wifiManager.setupPortalActive()) restoreRobotFaceScreen(nowMs);
}

void setup() {
  Serial.begin(115200);
  delay(200);
  M5.begin();
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setTextSize(2);
  M5.Display.setCursor(8, 8);
  showBootLine("Desktop Robot boot " + String(ROBOT_FIRMWARE_VERSION));
  connectWifi();
  robotClock.begin();
  faceRenderer.setSubtitleDateTimeEnabled(robotClock.showDateTimeSubtitle());
  idleBehavior.begin(true);
  emotionActions.begin();
  emotionTriggers.begin();
  mumble.begin();
  proximityTracker.begin();
  cameraAttention.begin();
  motionStream.begin();
  assistantStatus.begin(
    ROBOT_ASSISTANT_STATUS_ENABLED,
    ROBOT_ASSISTANT_STATUS_URL,
    ROBOT_ASSISTANT_STATUS_POLL_MS,
    ROBOT_ASSISTANT_STATUS_LOW_CODEX_PERCENT
  );
  registerApiRoutes(server, faceRenderer, hardware, idleBehavior, emotionActions, emotionTriggers, mumble, robotClock, proximityTracker, cameraAttention, localFaceDetector, motionStream, assistantStatus, wifiManager);
  server.begin();
  showBootLine("HTTP ready");
  startHardwareInit();
  delay(1200);
  faceRenderer.begin(M5.Display);
}

void loop() {
  if (hardware.isReady()) {
    M5StackChan.update();
  } else {
    M5.update();
  }
  uint32_t nowMs = millis();
  if (M5.BtnPWR.wasClicked()) {
    handlePowerButtonClick(nowMs);
  }
  updatePowerButtonClickWindow(nowMs);
  motionStream.update(hardware, idleBehavior, robotClock, nowMs);
  server.handleClient();
  wifiManager.update(millis());
  nowMs = millis();
  syncWifiDisplayRevision(nowMs);
  robotClock.update(nowMs);
  faceRenderer.setDateTimeText(robotClock.localDateTimeText());
  if (hardware.isReady() && !localFaceDetector.initialized()) {
    localFaceDetector.begin();
  }
  bool voiceQuietMode = false;
  if (!voiceQuietMode) {
    emotionTriggers.update(hardware, emotionActions, idleBehavior, mumble, robotClock, nowMs);
    emotionActions.update(hardware, faceRenderer, idleBehavior, nowMs);
    proximityTracker.update(hardware, nowMs);
    bool voiceBusy = false;
    cameraAttention.update(hardware, faceRenderer, idleBehavior, robotClock, mumble, localFaceDetector, emotionActions.active(), proximityTracker.active(), voiceBusy, nowMs);
  }
  mumble.update(nowMs, hardware, hardware.isReady() && !voiceQuietMode && !emotionActions.active(), emotionActions.lightsActive());
  if (hardware.isReady()) {
    hardware.updateTemporaryRgbOverride(nowMs);
    hardware.updateRgbManualEffects(nowMs, emotionActions.lightsActive());
  }
  if (!voiceQuietMode && !emotionActions.active() && !proximityTracker.active() && !cameraAttention.active()) {
    idleBehavior.update(hardware, faceRenderer, robotClock, nowMs);
  }
  bool assistantStatusCanControlMood = !emotionActions.active() && !proximityTracker.active() && !cameraAttention.active();
  assistantStatus.update(faceRenderer, hardware, nowMs, assistantStatusCanControlMood);
  if (wifiManager.setupPortalActive()) {
    drawSetupPortalScreen(false);
  } else {
    faceRenderer.update(M5.Display, nowMs);
    drawNetworkOverlay(nowMs);
  }
  if (millis() - lastStatusLogMs > 5000) {
    lastStatusLogMs = millis();
    Serial.printf("status wifi=%d ip=%s rssi=%d hardware=%s\n",
                  WiFi.status(),
                  WiFi.localIP().toString().c_str(),
                  WiFi.RSSI(),
                  hardware.isReady() ? "ready" : "not_ready");
  }
}
