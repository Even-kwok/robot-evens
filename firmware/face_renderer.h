#pragma once

#include <Arduino.h>
#include <M5GFX.h>

enum class FaceMood : uint8_t {
  Neutral,
  Happy,
  Angry,
  Sad,
  Sleeping,
  Tired,
  Surprised,
  Love,
  Dizzy,
  Wink,
  Dead,
  Thinking,
  Curious,
  Sunglasses,
  Loading,
};

enum class ScreenAssetMode : uint8_t {
  None,
  MatrixRain,
};

struct FaceStyle {
  float eyeWidthScale = 1.0f;
  float eyeHeightScale = 1.0f;
  int16_t cornerRadius = 24;
  int16_t eyeOffsetX = 0;
  int16_t eyeOffsetY = 0;
  int16_t eyeSpacing = 112;
  uint16_t eyeColor = 0x07EC;
  bool glowEnabled = false;
  uint8_t glowStrength = 3;
  bool scanlinesEnabled = true;
  uint8_t scanlineSpacing = 5;
  uint8_t scanlineAlpha = 20;
};

struct FaceReactState {
  float audioLevel = 0.0f;
  float lookX = 0.0f;
  float lookY = 0.0f;
};

struct SubtitleState {
  bool enabled = true;
  String text = "";
  uint16_t color = 0x07EC;
  bool showDateTime = false;
};

struct AssistantStatusLineState {
  bool enabled = false;
  String text = "";
  uint16_t color = 0x07EC;
};

struct FaceStateSnapshot {
  const char* mood;
  FaceStyle style;
  FaceReactState react;
  SubtitleState subtitle;
  AssistantStatusLineState assistantStatusLine;
};

class RobotFaceRenderer {
public:
  void begin(LGFX_Device& display);
  void update(LGFX_Device& display, uint32_t nowMs);
  bool setMood(const String& moodName);
  void setStyle(const FaceStyle& style);
  void patchStyle(const FaceStyle& style);
  void setReact(float lookX, float lookY, float audioLevel);
  void setSubtitle(const String& text, bool enabled, uint16_t color);
  void setSubtitleEnabled(bool enabled);
  void setSubtitleColor(uint16_t color);
  void setSubtitleDateTimeEnabled(bool enabled);
  void setDateTimeText(const String& text);
  void clearSubtitle();
  void setAssistantStatusLine(const String& text, bool enabled, uint16_t color);
  void clearAssistantStatusLine();
  FaceStyle style() const;
  FaceReactState react() const;
  SubtitleState subtitle() const;
  AssistantStatusLineState assistantStatusLine() const;
  FaceStateSnapshot snapshot() const;
  String stateJson() const;
  bool setScreenAsset(const String& assetName, uint32_t nowMs, uint32_t durationMs = 0);
  void clearScreenAsset();
  String screenAssetJson(uint32_t nowMs) const;
  static const char* moodName(FaceMood mood);
  static bool parseMood(const String& name, FaceMood& mood);
  static const char* screenAssetName(ScreenAssetMode mode);
  static bool parseScreenAsset(const String& name, ScreenAssetMode& mode);

private:
  static constexpr uint8_t MOOD_COUNT = 15;

  LGFX_Sprite frame_;
  FaceMood mood_ = FaceMood::Neutral;
  FaceStyle style_;
  FaceStyle moodStyles_[MOOD_COUNT];
  FaceReactState react_;
  SubtitleState subtitle_;
  AssistantStatusLineState assistantStatusLine_;
  String dateTimeText_;
  ScreenAssetMode screenAsset_ = ScreenAssetMode::None;
  uint32_t screenAssetStartedMs_ = 0;
  uint32_t screenAssetUntilMs_ = 0;
  uint32_t lastFrameMs_ = 0;
  bool stylesInitialized_ = false;
  bool frameReady_ = false;
  int16_t frameWidth_ = 320;
  int16_t frameHeight_ = 240;

  bool ensureFrame(LGFX_Device& display);
  void initializeDefaultStyles();
  void clampStyle(FaceStyle& style);
  void copyGlobalEffects(const FaceStyle& style);
  bool screenAssetActive(uint32_t nowMs) const;
  float blinkHeightScale(uint32_t nowMs) const;
  void drawMatrixRain(LGFX_Sprite& canvas, uint32_t nowMs);
  void drawEyes(LGFX_Sprite& canvas, uint32_t nowMs);
  void drawSubtitle(LGFX_Sprite& canvas);
  void drawAssistantStatusLine(LGFX_Sprite& canvas);
  void drawPlainBottomText(LGFX_Sprite& canvas, const String& text, uint16_t color, uint8_t slotFromBottom);
  void drawOneEye(LGFX_Sprite& canvas, bool left, int cx, int cy, int w, int h, uint32_t nowMs, float blinkScale);
  void drawRoundedEye(LGFX_Sprite& canvas, int cx, int cy, int w, int h, int radius, uint16_t color);
  void drawBlinkEye(LGFX_Sprite& canvas, int cx, int cy, int w, uint16_t color);
  void drawHappyEye(LGFX_Sprite& canvas, int cx, int cy, int w, int h, uint16_t color);
  void drawCuriousEye(LGFX_Sprite& canvas, bool left, int cx, int cy, int w, int h, uint16_t color);
  void drawAngryEye(LGFX_Sprite& canvas, bool left, int cx, int cy, int w, int h, uint16_t color);
  void drawSadEye(LGFX_Sprite& canvas, int cx, int cy, int w, int h, uint16_t color);
  void drawTiredEye(LGFX_Sprite& canvas, int cx, int cy, int w, int h, uint16_t color);
  void drawSunglassesEye(LGFX_Sprite& canvas, bool left, int cx, int cy, int w, int h, uint16_t color);
  void drawLineEye(LGFX_Sprite& canvas, int cx, int cy, int w, int h, uint16_t color);
  void drawThickArc(LGFX_Sprite& canvas, int cx, int cy, int w, int h, float startRad, float endRad, int thickness, uint16_t color);
  void drawThickLine(LGFX_Sprite& canvas, int x0, int y0, int x1, int y1, int thickness, uint16_t color);
  void drawRoundCap(LGFX_Sprite& canvas, int x, int y, int thickness, uint16_t color);
  void drawHeartEye(LGFX_Sprite& canvas, int cx, int cy, int w, int h, uint16_t color);
  void drawSpiralEye(LGFX_Sprite& canvas, int cx, int cy, int r, uint32_t nowMs, uint16_t color);
  void drawXEye(LGFX_Sprite& canvas, int cx, int cy, int s, uint16_t color);
  void drawLoadingEye(LGFX_Sprite& canvas, int cx, int cy, int r, uint32_t nowMs, uint16_t color);
  void drawGlow(LGFX_Sprite& canvas, int cx, int cy, int w, int h);
  void drawScanlines(LGFX_Sprite& canvas);
};
