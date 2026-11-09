#include "face_renderer.h"

namespace {

const char* const MOOD_NAMES[] = {
  "neutral",
  "happy",
  "angry",
  "sad",
  "sleeping",
  "tired",
  "surprised",
  "love",
  "dizzy",
  "wink",
  "dead",
  "thinking",
  "curious",
  "sunglasses",
  "loading",
};

const char* const SCREEN_ASSET_NAMES[] = {
  "none",
  "matrixRain",
};

const char MATRIX_RAIN_GLYPHS[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ$+-*/<>[]{}";

constexpr uint16_t DEFAULT_EYE_COLOR = 0x07EC;
constexpr bool DEFAULT_GLOW_ENABLED = false;
constexpr uint8_t DEFAULT_GLOW_STRENGTH = 3;
constexpr bool DEFAULT_SCANLINES_ENABLED = true;
constexpr uint8_t DEFAULT_SCANLINE_SPACING = 5;
constexpr uint8_t DEFAULT_SCANLINE_ALPHA = 20;
constexpr uint32_t BLINK_INTERVAL_MS = 3200;
constexpr uint32_t BLINK_CLOSE_MS = 80;
constexpr uint32_t BLINK_HOLD_MS = 45;
constexpr uint32_t BLINK_OPEN_MS = 100;
constexpr float BLINK_CLOSED_SCALE = 0.18f;
constexpr int16_t SUBTITLE_BAR_HEIGHT = 32;
constexpr int16_t SUBTITLE_PADDING_X = 6;
constexpr size_t SUBTITLE_MAX_LENGTH = 160;
constexpr uint16_t MATRIX_RAIN_GREEN = 0x07E0;
constexpr uint16_t MATRIX_RAIN_HEAD = 0xEFFE;
constexpr uint8_t MATRIX_RAIN_COLUMN_WIDTH = 8;
constexpr uint8_t MATRIX_RAIN_ROW_HEIGHT = 10;

FaceStyle makeDefaultStyle(float eyeWidthScale, float eyeHeightScale, int16_t cornerRadius, int16_t eyeOffsetX, int16_t eyeOffsetY, int16_t eyeSpacing) {
  FaceStyle style;
  style.eyeWidthScale = eyeWidthScale;
  style.eyeHeightScale = eyeHeightScale;
  style.cornerRadius = cornerRadius;
  style.eyeOffsetX = eyeOffsetX;
  style.eyeOffsetY = eyeOffsetY;
  style.eyeSpacing = eyeSpacing;
  style.eyeColor = DEFAULT_EYE_COLOR;
  style.glowEnabled = DEFAULT_GLOW_ENABLED;
  style.glowStrength = DEFAULT_GLOW_STRENGTH;
  style.scanlinesEnabled = DEFAULT_SCANLINES_ENABLED;
  style.scanlineSpacing = DEFAULT_SCANLINE_SPACING;
  style.scanlineAlpha = DEFAULT_SCANLINE_ALPHA;
  return style;
}

float clampFloat(float value, float low, float high) {
  if (value < low) return low;
  if (value > high) return high;
  return value;
}

uint16_t dim565(uint16_t color, uint8_t percent) {
  uint8_t r = (color >> 11) & 0x1F;
  uint8_t g = (color >> 5) & 0x3F;
  uint8_t b = color & 0x1F;
  r = (uint8_t)((uint16_t)r * percent / 100);
  g = (uint8_t)((uint16_t)g * percent / 100);
  b = (uint8_t)((uint16_t)b * percent / 100);
  return (uint16_t)((r << 11) | (g << 5) | b);
}

String colorToHex(uint16_t color) {
  uint8_t r = (uint8_t)(((color >> 11) & 0x1F) * 255 / 31);
  uint8_t g = (uint8_t)(((color >> 5) & 0x3F) * 255 / 63);
  uint8_t b = (uint8_t)((color & 0x1F) * 255 / 31);
  char buf[8];
  snprintf(buf, sizeof(buf), "#%02x%02x%02x", r, g, b);
  return String(buf);
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

void RobotFaceRenderer::begin(LGFX_Device& display) {
  initializeDefaultStyles();
  ensureFrame(display);
}

void RobotFaceRenderer::update(LGFX_Device& display, uint32_t nowMs) {
  if (nowMs - lastFrameMs_ < 33) return;
  if (!ensureFrame(display)) return;
  lastFrameMs_ = nowMs;
  frame_.fillSprite(TFT_BLACK);
  if (screenAssetUntilMs_ != 0 && (int32_t)(nowMs - screenAssetUntilMs_) >= 0) clearScreenAsset();
  if (screenAssetActive(nowMs)) drawMatrixRain(frame_, nowMs);
  else {
    drawEyes(frame_, nowMs);
    drawSubtitle(frame_);
    drawAssistantStatusLine(frame_);
    if (style_.scanlinesEnabled) drawScanlines(frame_);
  }
  display.startWrite();
  frame_.pushSprite(&display, 0, 0);
  display.endWrite();
}

bool RobotFaceRenderer::ensureFrame(LGFX_Device& display) {
  int16_t width = display.width();
  int16_t height = display.height();
  if (frameReady_ && frameWidth_ == width && frameHeight_ == height) return true;
  frame_.deleteSprite();
  frame_.setPsram(true);
  frame_.setColorDepth(16);
  frameReady_ = frame_.createSprite(width, height) != nullptr;
  frameWidth_ = width;
  frameHeight_ = height;
  return frameReady_;
}

bool RobotFaceRenderer::setMood(const String& moodNameValue) {
  initializeDefaultStyles();
  FaceMood next;
  if (!parseMood(moodNameValue, next)) {
    mood_ = FaceMood::Neutral;
    style_ = moodStyles_[(uint8_t)mood_];
    return false;
  }
  mood_ = next;
  style_ = moodStyles_[(uint8_t)mood_];
  return true;
}

void RobotFaceRenderer::setStyle(const FaceStyle& style) {
  initializeDefaultStyles();
  FaceStyle next = style;
  clampStyle(next);
  copyGlobalEffects(next);
  moodStyles_[(uint8_t)mood_] = next;
  style_ = moodStyles_[(uint8_t)mood_];
}

void RobotFaceRenderer::patchStyle(const FaceStyle& style) {
  setStyle(style);
}

void RobotFaceRenderer::setReact(float lookX, float lookY, float audioLevel) {
  react_.lookX = clampFloat(lookX, -1.0f, 1.0f);
  react_.lookY = clampFloat(lookY, -1.0f, 1.0f);
  react_.audioLevel = clampFloat(audioLevel, 0.0f, 1.0f);
}

void RobotFaceRenderer::setSubtitle(const String& text, bool enabled, uint16_t color) {
  subtitle_.text = text;
  if (subtitle_.text.length() > SUBTITLE_MAX_LENGTH) {
    subtitle_.text = subtitle_.text.substring(0, SUBTITLE_MAX_LENGTH);
  }
  subtitle_.enabled = enabled;
  subtitle_.color = color;
}

void RobotFaceRenderer::setSubtitleEnabled(bool enabled) {
  subtitle_.enabled = enabled;
}

void RobotFaceRenderer::setSubtitleColor(uint16_t color) {
  subtitle_.color = color;
}

void RobotFaceRenderer::setSubtitleDateTimeEnabled(bool enabled) {
  subtitle_.showDateTime = enabled;
}

void RobotFaceRenderer::setDateTimeText(const String& text) {
  dateTimeText_ = text;
}

void RobotFaceRenderer::clearSubtitle() {
  subtitle_.text = "";
  subtitle_.enabled = false;
}

void RobotFaceRenderer::setAssistantStatusLine(const String& text, bool enabled, uint16_t color) {
  assistantStatusLine_.text = text;
  if (assistantStatusLine_.text.length() > SUBTITLE_MAX_LENGTH) {
    assistantStatusLine_.text = assistantStatusLine_.text.substring(0, SUBTITLE_MAX_LENGTH);
  }
  assistantStatusLine_.enabled = enabled;
  assistantStatusLine_.color = color;
}

void RobotFaceRenderer::clearAssistantStatusLine() {
  assistantStatusLine_.text = "";
  assistantStatusLine_.enabled = false;
}

FaceStyle RobotFaceRenderer::style() const {
  return style_;
}

FaceReactState RobotFaceRenderer::react() const {
  return react_;
}

SubtitleState RobotFaceRenderer::subtitle() const {
  return subtitle_;
}

AssistantStatusLineState RobotFaceRenderer::assistantStatusLine() const {
  return assistantStatusLine_;
}

FaceStateSnapshot RobotFaceRenderer::snapshot() const {
  return {moodName(mood_), style_, react_, subtitle_, assistantStatusLine_};
}

String RobotFaceRenderer::stateJson() const {
  String json = "{";
  json += "\"mood\":\"" + String(moodName(mood_)) + "\"";
  json += ",\"eyeWidthScale\":" + String(style_.eyeWidthScale, 2);
  json += ",\"eyeHeightScale\":" + String(style_.eyeHeightScale, 2);
  json += ",\"cornerRadius\":" + String(style_.cornerRadius);
  json += ",\"eyeOffsetX\":" + String(style_.eyeOffsetX);
  json += ",\"eyeOffsetY\":" + String(style_.eyeOffsetY);
  json += ",\"eyeSpacing\":" + String(style_.eyeSpacing);
  json += ",\"eyeColor\":\"" + colorToHex(style_.eyeColor) + "\"";
  json += ",\"glowEnabled\":" + String(style_.glowEnabled ? "true" : "false");
  json += ",\"glowStrength\":" + String(style_.glowStrength);
  json += ",\"scanlinesEnabled\":" + String(style_.scanlinesEnabled ? "true" : "false");
  json += ",\"scanlineSpacing\":" + String(style_.scanlineSpacing);
  json += ",\"scanlineAlpha\":" + String(style_.scanlineAlpha);
  json += ",\"lookX\":" + String(react_.lookX, 2);
  json += ",\"lookY\":" + String(react_.lookY, 2);
  json += ",\"audioLevel\":" + String(react_.audioLevel, 2);
  json += ",\"subtitle\":{";
  json += "\"enabled\":" + String(subtitle_.enabled ? "true" : "false");
  json += ",\"text\":\"" + jsonEscape(subtitle_.text) + "\"";
  json += ",\"color\":\"" + colorToHex(subtitle_.color) + "\"";
  json += ",\"showDateTime\":" + String(subtitle_.showDateTime ? "true" : "false");
  json += ",\"dateTimeText\":\"" + jsonEscape(dateTimeText_) + "\"";
  json += "}";
  json += ",\"assistantStatusLine\":{";
  json += "\"enabled\":" + String(assistantStatusLine_.enabled ? "true" : "false");
  json += ",\"text\":\"" + jsonEscape(assistantStatusLine_.text) + "\"";
  json += ",\"color\":\"" + colorToHex(assistantStatusLine_.color) + "\"";
  json += "}";
  json += "}";
  return json;
}

bool RobotFaceRenderer::setScreenAsset(const String& assetName, uint32_t nowMs, uint32_t durationMs) {
  ScreenAssetMode next;
  if (!parseScreenAsset(assetName, next)) return false;
  screenAsset_ = next;
  if (screenAsset_ == ScreenAssetMode::None) {
    screenAssetStartedMs_ = 0;
    screenAssetUntilMs_ = 0;
    return true;
  }
  screenAssetStartedMs_ = nowMs;
  screenAssetUntilMs_ = durationMs > 0 ? nowMs + durationMs : 0;
  return true;
}

void RobotFaceRenderer::clearScreenAsset() {
  screenAsset_ = ScreenAssetMode::None;
  screenAssetStartedMs_ = 0;
  screenAssetUntilMs_ = 0;
}

String RobotFaceRenderer::screenAssetJson(uint32_t nowMs) const {
  const bool active = screenAssetActive(nowMs);
  uint32_t remainingMs = 0;
  uint32_t durationMs = 0;
  if (screenAssetUntilMs_ != 0 && active) {
    remainingMs = screenAssetUntilMs_ - nowMs;
    durationMs = screenAssetUntilMs_ - screenAssetStartedMs_;
  }
  String json = "{\"ok\":true";
  json += ",\"asset\":\"" + String(screenAssetName(screenAsset_)) + "\"";
  json += ",\"mode\":\"" + String(screenAssetName(screenAsset_)) + "\"";
  json += ",\"active\":" + String(active ? "true" : "false");
  json += ",\"startedMs\":" + String(screenAssetStartedMs_);
  json += ",\"durationMs\":" + String(durationMs);
  json += ",\"remainingMs\":" + String(remainingMs);
  json += ",\"fullscreen\":" + String(screenAsset_ != ScreenAssetMode::None ? "true" : "false");
  json += "}";
  return json;
}

const char* RobotFaceRenderer::moodName(FaceMood mood) {
  return MOOD_NAMES[(uint8_t)mood];
}

bool RobotFaceRenderer::parseMood(const String& name, FaceMood& mood) {
  for (uint8_t i = 0; i < sizeof(MOOD_NAMES) / sizeof(MOOD_NAMES[0]); ++i) {
    if (name == MOOD_NAMES[i]) {
      mood = (FaceMood)i;
      return true;
    }
  }
  return false;
}

const char* RobotFaceRenderer::screenAssetName(ScreenAssetMode mode) {
  return SCREEN_ASSET_NAMES[(uint8_t)mode];
}

bool RobotFaceRenderer::parseScreenAsset(const String& name, ScreenAssetMode& mode) {
  String value = name;
  value.trim();
  if (value == "none" || value == "off" || value == "clear" || value == "") {
    mode = ScreenAssetMode::None;
    return true;
  }
  if (value == "matrixRain" || value == "matrix_rain" || value == "screen.matrix_rain") {
    mode = ScreenAssetMode::MatrixRain;
    return true;
  }
  return false;
}

void RobotFaceRenderer::initializeDefaultStyles() {
  if (stylesInitialized_) return;
  stylesInitialized_ = true;

  moodStyles_[(uint8_t)FaceMood::Neutral] = makeDefaultStyle(1.28f, 1.19f, 36, 13, -35, 112);
  moodStyles_[(uint8_t)FaceMood::Happy] = makeDefaultStyle(0.35f, 1.33f, 53, 0, -51, 105);
  moodStyles_[(uint8_t)FaceMood::Angry] = makeDefaultStyle(0.87f, 1.16f, 15, 0, -35, 100);
  moodStyles_[(uint8_t)FaceMood::Sad] = makeDefaultStyle(1.0f, 1.0f, 24, 0, -35, 112);
  moodStyles_[(uint8_t)FaceMood::Sleeping] = makeDefaultStyle(1.0f, 1.0f, 24, 0, -28, 112);
  moodStyles_[(uint8_t)FaceMood::Tired] = makeDefaultStyle(1.19f, 1.56f, 18, 0, -57, 112);
  moodStyles_[(uint8_t)FaceMood::Surprised] = makeDefaultStyle(0.58f, 0.35f, 24, 0, -38, 101);
  moodStyles_[(uint8_t)FaceMood::Love] = makeDefaultStyle(1.93f, 1.55f, 24, 0, -37, 108);
  moodStyles_[(uint8_t)FaceMood::Dizzy] = makeDefaultStyle(1.88f, 1.94f, 24, 0, -25, 112);
  moodStyles_[(uint8_t)FaceMood::Wink] = makeDefaultStyle(1.0f, 1.0f, 24, 0, -38, 112);
  moodStyles_[(uint8_t)FaceMood::Dead] = makeDefaultStyle(1.08f, 1.0f, 24, 0, -36, 112);
  moodStyles_[(uint8_t)FaceMood::Thinking] = makeDefaultStyle(1.0f, 1.0f, 24, 0, -36, 112);
  moodStyles_[(uint8_t)FaceMood::Curious] = makeDefaultStyle(0.83f, 0.42f, 30, 0, -46, 90);
  moodStyles_[(uint8_t)FaceMood::Sunglasses] = makeDefaultStyle(1.16f, 0.64f, 18, 0, -39, 104);
  moodStyles_[(uint8_t)FaceMood::Loading] = makeDefaultStyle(1.0f, 1.0f, 24, 0, -33, 112);

  for (uint8_t i = 0; i < MOOD_COUNT; ++i) {
    clampStyle(moodStyles_[i]);
  }
  style_ = moodStyles_[(uint8_t)mood_];
}

void RobotFaceRenderer::clampStyle(FaceStyle& style) {
  style.eyeWidthScale = clampFloat(style.eyeWidthScale, 0.35f, 2.2f);
  style.eyeHeightScale = clampFloat(style.eyeHeightScale, 0.15f, 2.2f);
  style.cornerRadius = constrain(style.cornerRadius, 0, 80);
  style.eyeOffsetX = constrain(style.eyeOffsetX, -90, 90);
  style.eyeOffsetY = constrain(style.eyeOffsetY, -90, 90);
  style.eyeSpacing = constrain(style.eyeSpacing, 48, 220);
  style.glowStrength = constrain(style.glowStrength, 0, 8);
  style.scanlineSpacing = constrain(style.scanlineSpacing, 3, 32);
  style.scanlineAlpha = constrain(style.scanlineAlpha, 0, 100);
}

void RobotFaceRenderer::copyGlobalEffects(const FaceStyle& style) {
  for (uint8_t i = 0; i < MOOD_COUNT; ++i) {
    moodStyles_[i].eyeColor = style.eyeColor;
    moodStyles_[i].glowEnabled = style.glowEnabled;
    moodStyles_[i].glowStrength = style.glowStrength;
    moodStyles_[i].scanlinesEnabled = style.scanlinesEnabled;
    moodStyles_[i].scanlineSpacing = style.scanlineSpacing;
    moodStyles_[i].scanlineAlpha = style.scanlineAlpha;
  }
}

bool RobotFaceRenderer::screenAssetActive(uint32_t nowMs) const {
  if (screenAsset_ == ScreenAssetMode::None) return false;
  return screenAssetUntilMs_ == 0 || (int32_t)(screenAssetUntilMs_ - nowMs) > 0;
}

float RobotFaceRenderer::blinkHeightScale(uint32_t nowMs) const {
  uint32_t phase = (nowMs + 1100) % BLINK_INTERVAL_MS;
  if (phase < BLINK_CLOSE_MS) {
    float t = (float)phase / (float)BLINK_CLOSE_MS;
    return 1.0f - (1.0f - BLINK_CLOSED_SCALE) * t;
  }
  phase -= BLINK_CLOSE_MS;
  if (phase < BLINK_HOLD_MS) return BLINK_CLOSED_SCALE;
  phase -= BLINK_HOLD_MS;
  if (phase < BLINK_OPEN_MS) {
    float t = (float)phase / (float)BLINK_OPEN_MS;
    return BLINK_CLOSED_SCALE + (1.0f - BLINK_CLOSED_SCALE) * t;
  }
  return 1.0f;
}

void RobotFaceRenderer::drawMatrixRain(LGFX_Sprite& canvas, uint32_t nowMs) {
  canvas.setFont(&fonts::Font0);
  canvas.setTextWrap(false);
  canvas.setTextSize(1);
  const uint8_t glyphCount = (uint8_t)(sizeof(MATRIX_RAIN_GLYPHS) - 1);
  const int columnCount = max<int>(1, frameWidth_ / MATRIX_RAIN_COLUMN_WIDTH);
  const int trailRows = frameHeight_ / MATRIX_RAIN_ROW_HEIGHT + 7;
  for (int column = 0; column <= columnCount; ++column) {
    const uint32_t seed = (uint32_t)column * 1103515245UL + 12345UL;
    const uint16_t speed = 42 + (uint16_t)(seed % 62);
    const int x = column * MATRIX_RAIN_COLUMN_WIDTH + (int)((seed >> 9) % 3);
    const int travel = frameHeight_ + MATRIX_RAIN_ROW_HEIGHT * 8;
    const int headY = (int)(((nowMs / speed) * MATRIX_RAIN_ROW_HEIGHT + (seed % travel)) % travel) - MATRIX_RAIN_ROW_HEIGHT * 4;
    for (int trail = 0; trail < trailRows; ++trail) {
      const int y = headY - trail * MATRIX_RAIN_ROW_HEIGHT;
      if (y < -MATRIX_RAIN_ROW_HEIGHT || y >= frameHeight_) continue;
      const uint8_t intensity = trail == 0 ? 100 : (uint8_t)constrain(86 - trail * 7, 12, 72);
      const uint16_t color = trail == 0 ? MATRIX_RAIN_HEAD : dim565(MATRIX_RAIN_GREEN, intensity);
      const uint8_t glyphIndex = (uint8_t)((seed + nowMs / 105 + trail * 11) % glyphCount);
      canvas.setTextColor(color, TFT_BLACK);
      canvas.setCursor(x, y);
      canvas.print(MATRIX_RAIN_GLYPHS[glyphIndex]);
    }
  }
}

void RobotFaceRenderer::drawEyes(LGFX_Sprite& canvas, uint32_t nowMs) {
  const int centerX = frameWidth_ / 2 + style_.eyeOffsetX;
  const int centerY = frameHeight_ / 2 + style_.eyeOffsetY;
  const float breath = sinf(nowMs / 500.0f) * 3.0f;
  const float audioBounce = react_.audioLevel * 22.0f;
  const float blinkScale = blinkHeightScale(nowMs);
  int baseW = (int)(72 * style_.eyeWidthScale + audioBounce * 0.35f);
  int baseH = (int)((54 + breath + audioBounce) * style_.eyeHeightScale);
  int blinkH = max(3, (int)(baseH * blinkScale));
  int lookDx = (int)(react_.lookX * 24.0f);
  int lookDy = (int)(react_.lookY * 20.0f);
  int leftX = centerX - style_.eyeSpacing / 2 + lookDx;
  int rightX = centerX + style_.eyeSpacing / 2 + lookDx;
  int y = centerY + lookDy;
  drawOneEye(canvas, true, leftX, y, baseW, blinkH, nowMs, blinkScale);
  drawOneEye(canvas, false, rightX, y, baseW, blinkH, nowMs, blinkScale);
}

void RobotFaceRenderer::drawSubtitle(LGFX_Sprite& canvas) {
  String text = "";
  if (subtitle_.enabled && subtitle_.text.length() > 0) {
    text = subtitle_.text;
  } else if (subtitle_.showDateTime && dateTimeText_.length() > 0) {
    text = dateTimeText_;
  }
  if (text.length() == 0) return;
  uint8_t slot = assistantStatusLine_.enabled && assistantStatusLine_.text.length() > 0 ? 1 : 0;
  drawPlainBottomText(canvas, text, subtitle_.color, slot);
}

void RobotFaceRenderer::drawAssistantStatusLine(LGFX_Sprite& canvas) {
  if (!assistantStatusLine_.enabled || assistantStatusLine_.text.length() == 0) return;
  drawPlainBottomText(canvas, assistantStatusLine_.text, assistantStatusLine_.color, 0);
}

void RobotFaceRenderer::drawPlainBottomText(LGFX_Sprite& canvas, const String& text, uint16_t color, uint8_t slotFromBottom) {
  const int16_t barY = frameHeight_ - SUBTITLE_BAR_HEIGHT * (slotFromBottom + 1);
  const int16_t maxTextWidth = frameWidth_ - SUBTITLE_PADDING_X * 2;
  canvas.setFont(&fonts::Font0);
  canvas.setTextWrap(false);
  canvas.setTextSize(2);
  canvas.setTextColor(color);
  int16_t textWidth = canvas.textWidth(text);
  uint8_t textSize = 2;
  if (textWidth > maxTextWidth) {
    textSize = 1;
    canvas.setTextSize(1);
    textWidth = canvas.textWidth(text);
  }
  const int16_t textHeight = 8 * textSize;
  const int16_t x = max<int16_t>(SUBTITLE_PADDING_X, (frameWidth_ - textWidth) / 2);
  const int16_t y = barY + (SUBTITLE_BAR_HEIGHT - textHeight) / 2;
  canvas.setCursor(x, y);
  canvas.print(text);
}

void RobotFaceRenderer::drawOneEye(LGFX_Sprite& canvas, bool left, int cx, int cy, int w, int h, uint32_t nowMs, float blinkScale) {
  const uint16_t color = style_.eyeColor;
  if (blinkScale <= BLINK_CLOSED_SCALE) {
    drawBlinkEye(canvas, cx, cy, w + 18, color);
    return;
  }

  const bool roundedEyeGlow =
      mood_ == FaceMood::Neutral ||
      mood_ == FaceMood::Surprised ||
      mood_ == FaceMood::Wink ||
      mood_ == FaceMood::Thinking ||
      mood_ == FaceMood::Curious;
  if (style_.glowEnabled && roundedEyeGlow) drawGlow(canvas, cx, cy, w, h);

  switch (mood_) {
    case FaceMood::Happy:
      if (style_.glowEnabled) drawHappyEye(canvas, cx, cy + 8, w + 28, h + 12, dim565(color, 24 + style_.glowStrength * 4));
      drawHappyEye(canvas, cx, cy + 8, w + 14, h, color);
      break;
    case FaceMood::Angry: {
      int angryOutwardOffset = 8;
      int angryEyeWidth = w + 8;
      int angryGlowWidth = w + 14;
      int angryCx = cx + (left ? -angryOutwardOffset : angryOutwardOffset);
      if (style_.glowEnabled) drawAngryEye(canvas, left, angryCx, cy, angryGlowWidth, h + 8, dim565(color, 24 + style_.glowStrength * 4));
      drawAngryEye(canvas, left, angryCx, cy, angryEyeWidth, h, color);
      break;
    }
    case FaceMood::Sad:
      if (style_.glowEnabled) drawSadEye(canvas, cx, cy + 2, w + 26, h + 16, dim565(color, 24 + style_.glowStrength * 4));
      drawSadEye(canvas, cx, cy + 2, w + 14, h + 8, color);
      break;
    case FaceMood::Sleeping:
      drawLineEye(canvas, cx, cy, w + 24, 2, color);
      break;
    case FaceMood::Tired:
      if (style_.glowEnabled) drawTiredEye(canvas, cx, cy + 2, w + 24, h + 6, dim565(color, 24 + style_.glowStrength * 4));
      drawTiredEye(canvas, cx, cy + 2, w + 10, h, color);
      break;
    case FaceMood::Surprised:
      drawRoundedEye(canvas, cx, cy, w * 3 / 4, h * 5 / 4, style_.cornerRadius, color);
      break;
    case FaceMood::Love:
      drawHeartEye(canvas, cx, cy - 5, w, h, color);
      break;
    case FaceMood::Dizzy:
      drawSpiralEye(canvas, cx, cy, min(w, h) / 2, nowMs, color);
      break;
    case FaceMood::Wink:
      if (!left && (nowMs % 1400) > 800) drawLineEye(canvas, cx, cy, w + 18, 2, color);
      else drawRoundedEye(canvas, cx, cy, w, h, style_.cornerRadius, color);
      break;
    case FaceMood::Dead:
      drawXEye(canvas, cx, cy, min(w, h), color);
      break;
    case FaceMood::Thinking: {
      int driftX = (int)(cosf(nowMs / 420.0f) * 12);
      int driftY = (int)(sinf(nowMs / 420.0f) * 8);
      drawRoundedEye(canvas, cx + driftX, cy + driftY, w, h, style_.cornerRadius, color);
      break;
    }
    case FaceMood::Curious:
      drawCuriousEye(canvas, left, cx, cy, w + 12, h + 14, color);
      break;
    case FaceMood::Sunglasses:
      drawSunglassesEye(canvas, left, cx, cy, w, h, color);
      break;
    case FaceMood::Loading:
      drawLoadingEye(canvas, cx, cy, min(w, h) / 2, nowMs, color);
      break;
    case FaceMood::Neutral:
    default:
      drawRoundedEye(canvas, cx, cy, w, h, style_.cornerRadius, color);
      break;
  }
}

void RobotFaceRenderer::drawRoundedEye(LGFX_Sprite& canvas, int cx, int cy, int w, int h, int radius, uint16_t color) {
  int x = cx - w / 2;
  int y = cy - h / 2;
  canvas.fillRoundRect(x, y, w, max(2, h), constrain(radius, 0, max(1, h / 2)), color);
}

void RobotFaceRenderer::drawBlinkEye(LGFX_Sprite& canvas, int cx, int cy, int w, uint16_t color) {
  drawThickLine(canvas, cx - w / 2, cy, cx + w / 2, cy, 5, color);
}

void RobotFaceRenderer::drawHappyEye(LGFX_Sprite& canvas, int cx, int cy, int w, int h, uint16_t color) {
  int happyArcThickness = constrain(h / 3, 14, 26);
  int happyArcHeight = constrain(h * 2 / 3, 28, 54);
  int happyArcWidth = constrain(w + 10, 62, 118);
  int arcCenterY = cy + happyArcHeight / 3;
  drawThickArc(canvas, cx, arcCenterY, happyArcWidth, happyArcHeight, PI, PI * 2.0f, happyArcThickness, color);
}

void RobotFaceRenderer::drawCuriousEye(LGFX_Sprite& canvas, bool left, int cx, int cy, int w, int h, uint16_t color) {
  int curiousOpenBias = left ? 12 : -6;
  int curiousSquintBias = left ? 6 : -10;
  int curiousVerticalBias = left ? -5 : 5;
  int curiousW = constrain(w + curiousOpenBias, 44, 112);
  int curiousH = constrain(h + curiousSquintBias, 24, 86);
  int curiousCy = cy + curiousVerticalBias;
  int radius = constrain(style_.cornerRadius + 10, 12, max(12, curiousH / 2));
  drawRoundedEye(canvas, cx, curiousCy, curiousW, curiousH, radius, color);
}

void RobotFaceRenderer::drawSunglassesEye(LGFX_Sprite& canvas, bool left, int cx, int cy, int w, int h, uint16_t color) {
  const int lensW = max(2, w);
  const int lensH = max(2, h);
  const int lensX = cx - lensW / 2;
  const int lensY = cy - lensH / 2;
  const int bottomY = lensY + lensH;
  const int browH = max(6, lensH / 5);
  const int outerWing = max(8, lensW / 6);
  const int noseDrop = max(8, lensH / 3);
  const int topOuterX = left ? lensX - outerWing : lensX + lensW + outerWing;
  const int topInnerX = left ? lensX + lensW : lensX;
  const int noseInnerX = left ? lensX + lensW - max(4, lensW / 8) : lensX + max(4, lensW / 8);
  const int lowerInnerX = left ? lensX + lensW - max(8, lensW / 5) : lensX + max(8, lensW / 5);
  const int lowerOuterX = left ? lensX + max(4, lensW / 9) : lensX + lensW - max(4, lensW / 9);
  const uint16_t lensFillColor = dim565(color, 72);
  const uint16_t lensShadowColor = dim565(color, 46);
  const uint16_t browColor = dim565(color, 90);
  const uint16_t highlightColor = TFT_WHITE;

  canvas.fillTriangle(topOuterX, lensY + browH, topInnerX, lensY, noseInnerX, lensY + noseDrop, lensFillColor);
  canvas.fillTriangle(topOuterX, lensY + browH, noseInnerX, lensY + noseDrop, lowerInnerX, bottomY - browH / 2, lensFillColor);
  canvas.fillTriangle(topOuterX, lensY + browH, lowerInnerX, bottomY - browH / 2, lowerOuterX, bottomY, lensFillColor);
  canvas.fillCircle(lowerOuterX, bottomY - browH / 2, max(2, browH / 2), lensFillColor);
  canvas.fillCircle(lowerInnerX, bottomY - browH / 2, max(2, browH / 2), lensFillColor);

  canvas.fillTriangle(topOuterX, lensY, topInnerX, lensY, topOuterX, lensY + browH, browColor);
  canvas.fillTriangle(topInnerX, lensY, noseInnerX, lensY + noseDrop, topOuterX, lensY + browH, browColor);

  const int shadowY = lensY + lensH / 2;
  const int shadowOuterX = left ? lensX : lensX + lensW;
  const int shadowInnerX = left ? lensX + lensW - max(6, lensW / 6) : lensX + max(6, lensW / 6);
  canvas.fillTriangle(shadowOuterX, shadowY, shadowInnerX, shadowY, lowerInnerX, bottomY - browH / 2, lensShadowColor);
  canvas.fillTriangle(shadowOuterX, shadowY, lowerInnerX, bottomY - browH / 2, lowerOuterX, bottomY, lensShadowColor);

  canvas.drawLine(lensX + max(5, lensW / 5), lensY + max(3, lensH / 6), lensX + max(12, lensW / 2), lensY + max(4, lensH / 6), highlightColor);
  canvas.drawLine(lensX + max(7, lensW / 5), lensY + max(5, lensH / 6 + 2), lensX + max(10, lensW / 3), lensY + max(6, lensH / 6 + 3), dim565(highlightColor, 62));
  canvas.fillCircle(topInnerX + (left ? -max(4, lensW / 10) : max(4, lensW / 10)), lensY + max(3, browH / 2), max(1, browH / 5), highlightColor);
  canvas.fillCircle(topInnerX + (left ? -max(7, lensW / 6) : max(7, lensW / 6)), lensY + max(3, browH / 2), max(1, browH / 5), highlightColor);

  const int bridgeW = max(8, style_.eyeSpacing - lensW);
  const int bridgeH = max(4, lensH / 6);
  const int bridgeX = left ? lensX + lensW - 1 : lensX - bridgeW + 1;
  const int bridgeY = cy - bridgeH / 2;
  canvas.fillRoundRect(bridgeX, bridgeY, bridgeW, bridgeH, max(2, bridgeH / 2), browColor);
}

void RobotFaceRenderer::drawAngryEye(LGFX_Sprite& canvas, bool left, int cx, int cy, int w, int h, uint16_t color) {
  int halfW = max(18, w / 2);
  int slopeY = constrain(h / 3, 12, 34);
  int thickness = constrain(h / 3, 12, 28);
  if (left) {
    drawThickLine(canvas, cx - halfW, cy - slopeY, cx + halfW, cy + slopeY, thickness, color);
  } else {
    drawThickLine(canvas, cx - halfW, cy + slopeY, cx + halfW, cy - slopeY, thickness, color);
  }
}

void RobotFaceRenderer::drawSadEye(LGFX_Sprite& canvas, int cx, int cy, int w, int h, uint16_t color) {
  int radius = constrain(min(w / 2, h * 3 / 4), 8, 80);
  int topY = cy - radius / 3;
  for (int y = 0; y <= radius; ++y) {
    int dx = (int)sqrtf((float)(radius * radius - y * y));
    canvas.drawFastHLine(cx - dx, topY + y, dx * 2 + 1, color);
  }
}

void RobotFaceRenderer::drawTiredEye(LGFX_Sprite& canvas, int cx, int cy, int w, int h, uint16_t color) {
  int tiredArcHeight = constrain(h * 2 / 3, 10, 34);
  int tiredArcWidth = constrain(w + 8, 44, 118);
  int tiredDowncastY = cy + max(1, h / 8);
  int lidY = tiredDowncastY - tiredArcHeight / 5;
  drawSadEye(canvas, cx, tiredDowncastY, tiredArcWidth, tiredArcHeight, dim565(color, 78));
  drawThickLine(canvas, cx - tiredArcWidth / 2 + 5, lidY, cx + tiredArcWidth / 2 - 5, lidY + max(1, tiredArcHeight / 10), max(2, tiredArcHeight / 8), color);
}

void RobotFaceRenderer::drawThickArc(LGFX_Sprite& canvas, int cx, int cy, int w, int h, float startRad, float endRad, int thickness, uint16_t color) {
  int steps = 30;
  int prevX = 0;
  int prevY = 0;
  for (int i = 0; i <= steps; ++i) {
    float t = (float)i / steps;
    float a = startRad + (endRad - startRad) * t;
    int px = cx + (int)(cosf(a) * w * 0.5f);
    int py = cy + (int)(sinf(a) * h * 0.5f);
    if (i == 0 || i == steps) drawRoundCap(canvas, px, py, thickness, color);
    if (i > 0) drawThickLine(canvas, prevX, prevY, px, py, thickness, color);
    prevX = px;
    prevY = py;
  }
}

void RobotFaceRenderer::drawLineEye(LGFX_Sprite& canvas, int cx, int cy, int w, int, uint16_t color) {
  canvas.drawLine(cx - w / 2, cy, cx + w / 2, cy, color);
  canvas.drawLine(cx - w / 2, cy + 1, cx + w / 2, cy + 1, color);
  canvas.drawLine(cx - w / 2, cy - 1, cx + w / 2, cy - 1, color);
}

void RobotFaceRenderer::drawThickLine(LGFX_Sprite& canvas, int x0, int y0, int x1, int y1, int thickness, uint16_t color) {
  int radius = max(1, thickness / 2);
  int steps = max(abs(x1 - x0), abs(y1 - y0));
  int step = max(1, radius / 2);
  for (int i = 0; i <= steps; i += step) {
    float t = steps == 0 ? 0.0f : (float)i / (float)steps;
    int x = x0 + (int)((x1 - x0) * t);
    int y = y0 + (int)((y1 - y0) * t);
    canvas.fillCircle(x, y, radius, color);
  }
  canvas.fillCircle(x1, y1, radius, color);
}

void RobotFaceRenderer::drawRoundCap(LGFX_Sprite& canvas, int x, int y, int thickness, uint16_t color) {
  canvas.fillCircle(x, y, max(1, thickness / 2), color);
}

void RobotFaceRenderer::drawHeartEye(LGFX_Sprite& canvas, int cx, int cy, int w, int h, uint16_t color) {
  int r = min(w, h) / 4;
  canvas.fillCircle(cx - r, cy - r / 2, r, color);
  canvas.fillCircle(cx + r, cy - r / 2, r, color);
  canvas.fillTriangle(cx - 2 * r, cy, cx + 2 * r, cy, cx, cy + 2 * r, color);
}

void RobotFaceRenderer::drawSpiralEye(LGFX_Sprite& canvas, int cx, int cy, int r, uint32_t nowMs, uint16_t color) {
  float phase = nowMs / 160.0f;
  int spiralThickness = constrain(r / 8, 4, 7);
  int lastX = cx;
  int lastY = cy;
  for (float a = 0.0f; a < 7.0f; a += 0.18f) {
    float rr = (a / 7.0f) * r;
    int x = cx + (int)(cosf(a + phase) * rr);
    int y = cy + (int)(sinf(a + phase) * rr);
    drawThickLine(canvas, lastX, lastY, x, y, spiralThickness, color);
    lastX = x;
    lastY = y;
  }
}

void RobotFaceRenderer::drawXEye(LGFX_Sprite& canvas, int cx, int cy, int s, uint16_t color) {
  int d = s / 2;
  canvas.drawLine(cx - d, cy - d, cx + d, cy + d, color);
  canvas.drawLine(cx + d, cy - d, cx - d, cy + d, color);
  canvas.drawLine(cx - d, cy - d + 1, cx + d, cy + d + 1, color);
  canvas.drawLine(cx + d, cy - d + 1, cx - d, cy + d + 1, color);
}

void RobotFaceRenderer::drawLoadingEye(LGFX_Sprite& canvas, int cx, int cy, int r, uint32_t nowMs, uint16_t color) {
  float phase = nowMs / 220.0f;
  for (int i = 0; i < 9; ++i) {
    float a = phase + i * 0.55f;
    uint16_t c = dim565(color, 35 + i * 7);
    canvas.fillCircle(cx + (int)(cosf(a) * r), cy + (int)(sinf(a) * r), 3 + i / 4, c);
  }
}

void RobotFaceRenderer::drawGlow(LGFX_Sprite& canvas, int cx, int cy, int w, int h) {
  uint8_t strength = style_.glowStrength;
  if (strength == 0) return;
  for (uint8_t i = 0; i < strength; ++i) {
    uint16_t c = dim565(style_.eyeColor, 12 + i * 6);
    canvas.drawRoundRect(cx - w / 2 - 4 - i * 3, cy - h / 2 - 4 - i * 3, w + 8 + i * 6, h + 8 + i * 6, style_.cornerRadius + i * 2, c);
  }
}

void RobotFaceRenderer::drawScanlines(LGFX_Sprite& canvas) {
  uint16_t c = dim565(style_.eyeColor, style_.scanlineAlpha);
  for (int y = 0; y < frameHeight_; y += style_.scanlineSpacing) {
    canvas.drawFastHLine(0, y, frameWidth_, c);
  }
}
