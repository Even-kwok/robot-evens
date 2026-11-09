#include "mumble_player.h"

#include <M5Unified.hpp>
#include <esp_heap_caps.h>
#include "emotion_sound_assets.h"
#include "robot_hardware.h"

namespace {
constexpr uint8_t MUMBLE_CHANNEL = 1;
constexpr uint16_t MUMBLE_MIN_SPEED_PERCENT = 60;
constexpr uint16_t MUMBLE_MAX_SPEED_PERCENT = 160;
constexpr uint16_t MUMBLE_MIN_SEGMENT_MS = 500;
constexpr uint16_t MUMBLE_MAX_SEGMENT_MS = 6000;
constexpr uint32_t MUMBLE_MIN_AUTO_DELAY_MS = 45000;
constexpr uint32_t MUMBLE_MAX_AUTO_DELAY_MS = 120000;
constexpr uint32_t MUMBLE_AUDIO_SAMPLE_RATE = 16000;
constexpr size_t MUMBLE_MAX_UPLOAD_BYTES = 320000;
constexpr const char* MUMBLE_AUDIO_FORMAT = "raw-u8-16k-mono";

struct EmotionSoundSlotDefinition {
  const char* name;
  const char* title;
};

const EmotionSoundSlotDefinition EMOTION_SOUND_SLOT_DEFAULTS[] = {
  {"comfortableNod", "舒服点点头"},
  {"happy", "开心点头"},
  {"headTouchComboSpin", "摸头疯狂点头"},
  {"sad", "伤心低头"},
  {"angry", "生气摆头"},
  {"surprised", "惊讶后仰"},
  {"thinking", "思考低头"},
  {"idleThought", "待机嘀咕"},
  {"curious", "好奇观察"},
  {"shy", "害羞躲闪"},
  {"cute", "撒娇求关注"},
  {"proud", "得意抬头"},
  {"confused", "困惑歪头"},
  {"scared", "紧张后缩"},
  {"celebrate", "庆祝欢呼"},
};

const EmotionSoundAsset* findBuiltInEmotionSoundAsset(const String& name) {
  for (size_t i = 0; i < EMOTION_SOUND_ASSET_COUNT; ++i) {
    if (name == EMOTION_SOUND_ASSETS[i].name) return &EMOTION_SOUND_ASSETS[i];
  }
  return nullptr;
}

uint32_t clampDurationMs(uint32_t value, uint32_t fallback) {
  if (value == 0) value = fallback;
  if (value < MUMBLE_MIN_SEGMENT_MS) value = MUMBLE_MIN_SEGMENT_MS;
  if (value > MUMBLE_MAX_SEGMENT_MS) value = MUMBLE_MAX_SEGMENT_MS;
  return value;
}

String jsonEscapeLocal(const String& value) {
  String out;
  out.reserve(value.length() + 8);
  for (uint32_t index = 0; index < value.length(); ++index) {
    char c = value[index];
    if (c == '"' || c == '\\') {
      out += "\\";
      out += c;
    } else if (c == '\n') {
      out += "\\n";
    } else if (c == '\r') {
      out += "\\r";
    } else {
      out += c;
    }
  }
  return out;
}
}  // namespace

void MumblePlayer::begin() {
  speedPercent_ = 100;
  apiVoiceVolume_ = 180;
  emotionSoundVolume_ = 96;
  segmentDurationMs_ = 1800;
  enabled_ = false;
  autoEnabled_ = false;
  fullChancePercent_ = 18;
  nextAutoAtMs_ = 0;
  lastStartedAtMs_ = 0;
  lastStartMs_ = 0;
  lastDurationMs_ = 0;
  lastSampleRate_ = MUMBLE_AUDIO_SAMPLE_RATE;
  lastSampleCount_ = 0;
  lastMode_ = "none";
  speechLightWasActive_ = false;
  uploadedAudio_ = nullptr;
  uploadedAudioLength_ = 0;
  uploadCapacity_ = 0;
  uploadBytesWritten_ = 0;
  uploadActive_ = false;
  uploadFailed_ = false;
  uploadedName_ = "";
  resetEmotionSlots();
  activeEmotionUploadSlot_ = -1;
  emotionUploadCapacity_ = 0;
  emotionUploadBytesWritten_ = 0;
  emotionUploadActive_ = false;
  emotionUploadFailed_ = false;
  lastEmotionSlot_ = "none";
}

void MumblePlayer::update(uint32_t nowMs, RobotHardware& hardware, bool allowAuto, bool blockedByEmotionLights) {
  updateSpeechLights(nowMs, hardware, blockedByEmotionLights);
  if (!enabled_) return;
  if (!hasUploadedAudio()) return;
  if (!autoEnabled_ || !allowAuto || nextAutoAtMs_ == 0 || nowMs < nextAutoAtMs_) return;
  if (M5.Speaker.isPlaying()) {
    nextAutoAtMs_ = nowMs + 5000;
    return;
  }
  if ((randomish(nowMs, 17) % 100) < fullChancePercent_) {
    playFullJson(nowMs);
  } else {
    playRandomSegmentJson(nowMs);
  }
  scheduleNextAuto(nowMs);
}

void MumblePlayer::setSpeedPercent(uint16_t value) {
  speedPercent_ = constrain(value, MUMBLE_MIN_SPEED_PERCENT, MUMBLE_MAX_SPEED_PERCENT);
}

void MumblePlayer::setApiVoiceVolume(uint8_t value) {
  apiVoiceVolume_ = value;
}

void MumblePlayer::setSegmentDurationMs(uint16_t value) {
  segmentDurationMs_ = (uint16_t)clampDurationMs(value, segmentDurationMs_);
}

void MumblePlayer::setEnabled(bool enabled) {
  enabled_ = enabled;
  if (!enabled_) {
    autoEnabled_ = false;
    nextAutoAtMs_ = 0;
    M5.Speaker.stop(MUMBLE_CHANNEL);
    lastMode_ = "disabled";
    lastDurationMs_ = 0;
    lastSampleCount_ = 0;
  }
}

void MumblePlayer::setAutoEnabled(bool enabled, uint32_t nowMs) {
  autoEnabled_ = enabled_ && enabled;
  if (autoEnabled_) {
    scheduleNextAuto(nowMs);
  } else {
    nextAutoAtMs_ = 0;
  }
}

void MumblePlayer::setFullChancePercent(uint8_t value) {
  fullChancePercent_ = constrain(value, 0, 100);
}

bool MumblePlayer::autoEnabled() const {
  return autoEnabled_;
}

bool MumblePlayer::playing() const {
  return M5.Speaker.isPlaying();
}

String MumblePlayer::statusJson() const {
  uint32_t nowMs = millis();
  uint32_t nextAutoInMs = autoEnabled_ && nextAutoAtMs_ > nowMs ? nextAutoAtMs_ - nowMs : 0;
  uint32_t audioDurationMs = hasUploadedAudio() ? (uint32_t)((uint64_t)uploadedAudioLength_ * 1000UL / MUMBLE_AUDIO_SAMPLE_RATE) : 0;
  String json = "{\"ok\":true";
  json += ",\"available\":true";
  json += ",\"enabled\":" + String(enabled_ ? "true" : "false");
  json += ",\"hasAudio\":" + String(hasUploadedAudio() ? "true" : "false");
  json += ",\"playing\":" + String(M5.Speaker.isPlaying() ? "true" : "false");
  json += ",\"autoEnabled\":" + String(autoEnabled_ ? "true" : "false");
  json += ",\"speedPercent\":" + String(speedPercent_);
  json += ",\"volume\":" + String(apiVoiceVolume_);
  json += ",\"apiVoiceVolume\":" + String(apiVoiceVolume_);
  json += ",\"emotionSoundVolume\":" + String(emotionSoundVolume_);
  json += ",\"segmentDurationMs\":" + String(segmentDurationMs_);
  json += ",\"fullChancePercent\":" + String(fullChancePercent_);
  json += ",\"nextAutoInMs\":" + String(nextAutoInMs);
  json += ",\"format\":\"" + String(MUMBLE_AUDIO_FORMAT) + "\"";
  json += ",\"sampleRateRequired\":" + String(MUMBLE_AUDIO_SAMPLE_RATE);
  json += ",\"maxUploadBytes\":" + String(MUMBLE_MAX_UPLOAD_BYTES);
  json += ",\"uploadedName\":\"" + uploadedName_ + "\"";
  json += ",\"audioDurationMs\":" + String(audioDurationMs);
  json += ",\"audioBytes\":" + String(uploadedAudioLength_);
  json += ",\"sampleRate\":" + String(lastSampleRate_);
  json += ",\"lastMode\":\"" + String(lastMode_) + "\"";
  json += ",\"lastEmotionSlot\":\"" + jsonEscapeLocal(lastEmotionSlot_) + "\"";
  json += ",\"lastStartMs\":" + String(lastStartMs_);
  json += ",\"lastDurationMs\":" + String(lastDurationMs_);
  json += ",\"lastSampleCount\":" + String(lastSampleCount_);
  json += "}";
  return json;
}

bool MumblePlayer::uploadBegin(const String& filename, size_t expectedSize) {
  uploadAbort();
  if (expectedSize > MUMBLE_MAX_UPLOAD_BYTES) {
    uploadFailed_ = true;
    return false;
  }
  size_t capacity = expectedSize == 0 ? MUMBLE_MAX_UPLOAD_BYTES : expectedSize;
  uploadedAudio_ = (uint8_t*)heap_caps_malloc(capacity, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (uploadedAudio_ == nullptr) {
    uploadedAudio_ = (uint8_t*)heap_caps_malloc(capacity, MALLOC_CAP_8BIT);
  }
  if (uploadedAudio_ == nullptr) {
    uploadFailed_ = true;
    return false;
  }
  uploadedName_ = filename;
  uploadCapacity_ = capacity;
  uploadBytesWritten_ = 0;
  uploadedAudioLength_ = 0;
  uploadActive_ = true;
  uploadFailed_ = false;
  return true;
}

bool MumblePlayer::uploadWrite(const uint8_t* data, size_t length) {
  if (!uploadActive_ || uploadFailed_ || uploadedAudio_ == nullptr) return false;
  if (uploadBytesWritten_ + length > uploadCapacity_) {
    uploadFailed_ = true;
    return false;
  }
  memcpy(uploadedAudio_ + uploadBytesWritten_, data, length);
  uploadBytesWritten_ += length;
  return true;
}

bool MumblePlayer::uploadEnd() {
  if (!uploadActive_ || uploadFailed_ || uploadedAudio_ == nullptr || uploadBytesWritten_ == 0) {
    uploadAbort();
    uploadFailed_ = true;
    return false;
  }
  uploadedAudioLength_ = uploadBytesWritten_;
  uploadActive_ = false;
  uploadFailed_ = false;
  lastMode_ = "uploaded";
  lastDurationMs_ = 0;
  lastSampleCount_ = 0;
  return true;
}

void MumblePlayer::uploadAbort() {
  if (uploadedAudio_ != nullptr) {
    heap_caps_free(uploadedAudio_);
  }
  uploadedAudio_ = nullptr;
  uploadedAudioLength_ = 0;
  uploadCapacity_ = 0;
  uploadBytesWritten_ = 0;
  uploadActive_ = false;
}

String MumblePlayer::uploadStatusJson() const {
  String json = "{\"ok\":";
  json += String(hasUploadedAudio() && !uploadFailed_ ? "true" : "false");
  json += ",\"format\":\"" + String(MUMBLE_AUDIO_FORMAT) + "\"";
  json += ",\"sampleRateRequired\":" + String(MUMBLE_AUDIO_SAMPLE_RATE);
  json += ",\"maxUploadBytes\":" + String(MUMBLE_MAX_UPLOAD_BYTES);
  json += ",\"uploadedBytes\":" + String(uploadedAudioLength_);
  json += ",\"uploadActive\":" + String(uploadActive_ ? "true" : "false");
  json += ",\"uploadFailed\":" + String(uploadFailed_ ? "true" : "false");
  json += ",\"uploadedName\":\"" + uploadedName_ + "\"";
  json += "}";
  return json;
}

String MumblePlayer::playRandomSegmentJson(uint32_t nowMs, uint32_t durationMs) {
  if (!enabled_) return disabledJson("random");
  if (!hasUploadedAudio()) return missingAudioJson("random");
  return playSegmentJson("random", nowMs, clampDurationMs(durationMs, segmentDurationMs_));
}

String MumblePlayer::playForSubtitleJson(const SubtitleState& subtitle, uint32_t nowMs) {
  if (!enabled_) return disabledJson("subtitle");
  if (!hasUploadedAudio()) return missingAudioJson("subtitle");
  return playSegmentJson("subtitle", nowMs, durationForSubtitle(subtitle));
}

String MumblePlayer::playForEmotionJson(uint32_t nowMs, uint32_t durationMs) {
  if (!enabled_) return disabledJson("emotion");
  if (!hasUploadedAudio()) return missingAudioJson("emotion");
  return playSegmentJson("emotion", nowMs, clampDurationMs(durationMs, segmentDurationMs_));
}

String MumblePlayer::playFullJson(uint32_t nowMs) {
  if (!enabled_) return disabledJson("full");
  if (!hasUploadedAudio()) return missingAudioJson("full");
  return playFullFromStartJson(nowMs);
}

String MumblePlayer::stopJson() {
  M5.Speaker.stop(MUMBLE_CHANNEL);
  lastMode_ = "stopped";
  lastDurationMs_ = 0;
  lastSampleCount_ = 0;
  String json = statusJson();
  return json;
}

String MumblePlayer::emotionSlotsJson() const {
  String json = "{\"ok\":true";
  json += ",\"format\":\"" + String(MUMBLE_AUDIO_FORMAT) + "\"";
  json += ",\"sampleRateRequired\":" + String(MUMBLE_AUDIO_SAMPLE_RATE);
  json += ",\"maxUploadBytes\":" + String(MUMBLE_MAX_UPLOAD_BYTES);
  json += ",\"lastEmotionSlot\":\"" + jsonEscapeLocal(lastEmotionSlot_) + "\"";
  json += ",\"slots\":[";
  for (uint8_t i = 0; i < MUMBLE_EMOTION_SLOT_COUNT; ++i) {
    const EmotionSoundSlot& slot = emotionSlots_[i];
    if (i > 0) json += ",";
    uint32_t audioDurationMs = hasEmotionSlotAudio(slot) ? (uint32_t)((uint64_t)slot.audioLength * 1000UL / MUMBLE_AUDIO_SAMPLE_RATE) : 0;
    json += "{\"name\":\"" + String(slot.name) + "\"";
    json += ",\"title\":\"" + String(slot.title) + "\"";
    json += ",\"enabled\":" + String(slot.enabled ? "true" : "false");
    json += ",\"hasAudio\":" + String(hasEmotionSlotAudio(slot) ? "true" : "false");
    json += ",\"audioBytes\":" + String((uint32_t)slot.audioLength);
    json += ",\"audioDurationMs\":" + String(audioDurationMs);
    json += ",\"uploadedName\":\"" + jsonEscapeLocal(slot.uploadedName) + "\"";
    json += "}";
  }
  json += "]}";
  return json;
}

bool MumblePlayer::setEmotionSlotEnabled(const String& name, bool enabled) {
  EmotionSoundSlot* slot = findEmotionSoundSlot(name);
  if (slot == nullptr) return false;
  slot->enabled = enabled;
  if (!enabled && lastEmotionSlot_ == String(slot->name)) {
    M5.Speaker.stop(MUMBLE_CHANNEL);
  }
  return true;
}

bool MumblePlayer::uploadEmotionSlotBegin(const String& name, const String& filename, size_t expectedSize) {
  uploadEmotionSlotAbort();
  EmotionSoundSlot* slot = findEmotionSoundSlot(name);
  if (slot == nullptr || expectedSize > MUMBLE_MAX_UPLOAD_BYTES) {
    emotionUploadFailed_ = true;
    return false;
  }
  size_t capacity = expectedSize == 0 ? MUMBLE_MAX_UPLOAD_BYTES : expectedSize;
  releaseEmotionSlotAudio(*slot);
  uint8_t* uploaded = (uint8_t*)heap_caps_malloc(capacity, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (uploaded == nullptr) {
    uploaded = (uint8_t*)heap_caps_malloc(capacity, MALLOC_CAP_8BIT);
  }
  if (uploaded == nullptr) {
    applyBuiltInEmotionSlotAudio(*slot);
    emotionUploadFailed_ = true;
    return false;
  }
  slot->audio = uploaded;
  slot->ownsAudio = true;
  slot->uploadedName = filename;
  slot->audioLength = 0;
  activeEmotionUploadSlot_ = (int8_t)(slot - emotionSlots_);
  emotionUploadCapacity_ = capacity;
  emotionUploadBytesWritten_ = 0;
  emotionUploadActive_ = true;
  emotionUploadFailed_ = false;
  return true;
}

bool MumblePlayer::uploadEmotionSlotWrite(const uint8_t* data, size_t length) {
  if (!emotionUploadActive_ || emotionUploadFailed_ || activeEmotionUploadSlot_ < 0) return false;
  EmotionSoundSlot& slot = emotionSlots_[activeEmotionUploadSlot_];
  if (!slot.ownsAudio || slot.audio == nullptr || emotionUploadBytesWritten_ + length > emotionUploadCapacity_) {
    emotionUploadFailed_ = true;
    return false;
  }
  uint8_t* writableAudio = const_cast<uint8_t*>(slot.audio);
  memcpy(writableAudio + emotionUploadBytesWritten_, data, length);
  emotionUploadBytesWritten_ += length;
  return true;
}

bool MumblePlayer::uploadEmotionSlotEnd() {
  if (!emotionUploadActive_ || emotionUploadFailed_ || activeEmotionUploadSlot_ < 0 || emotionUploadBytesWritten_ == 0) {
    uploadEmotionSlotAbort();
    emotionUploadFailed_ = true;
    return false;
  }
  EmotionSoundSlot& slot = emotionSlots_[activeEmotionUploadSlot_];
  slot.audioLength = emotionUploadBytesWritten_;
  emotionUploadActive_ = false;
  emotionUploadFailed_ = false;
  activeEmotionUploadSlot_ = -1;
  emotionUploadCapacity_ = 0;
  emotionUploadBytesWritten_ = 0;
  lastMode_ = "emotionSlotUploaded";
  lastDurationMs_ = 0;
  lastSampleCount_ = 0;
  return true;
}

void MumblePlayer::uploadEmotionSlotAbort() {
  if (emotionUploadActive_ && activeEmotionUploadSlot_ >= 0) {
    EmotionSoundSlot& slot = emotionSlots_[activeEmotionUploadSlot_];
    if (slot.ownsAudio && slot.audio != nullptr && slot.audioLength == 0) {
      heap_caps_free(const_cast<uint8_t*>(slot.audio));
      slot.audio = nullptr;
      slot.ownsAudio = false;
      slot.uploadedName = "";
    }
    if (!hasEmotionSlotAudio(slot)) applyBuiltInEmotionSlotAudio(slot);
  }
  activeEmotionUploadSlot_ = -1;
  emotionUploadCapacity_ = 0;
  emotionUploadBytesWritten_ = 0;
  emotionUploadActive_ = false;
}

String MumblePlayer::emotionSlotUploadStatusJson(const String& name) const {
  const EmotionSoundSlot* slot = findEmotionSoundSlot(name);
  String json = "{\"ok\":";
  json += String((slot != nullptr && hasEmotionSlotAudio(*slot) && !emotionUploadFailed_) ? "true" : "false");
  json += ",\"format\":\"" + String(MUMBLE_AUDIO_FORMAT) + "\"";
  json += ",\"sampleRateRequired\":" + String(MUMBLE_AUDIO_SAMPLE_RATE);
  json += ",\"maxUploadBytes\":" + String(MUMBLE_MAX_UPLOAD_BYTES);
  json += ",\"uploadActive\":" + String(emotionUploadActive_ ? "true" : "false");
  json += ",\"uploadFailed\":" + String(emotionUploadFailed_ ? "true" : "false");
  if (slot != nullptr) {
    json += ",\"slot\":\"" + String(slot->name) + "\"";
    json += ",\"title\":\"" + String(slot->title) + "\"";
    json += ",\"enabled\":" + String(slot->enabled ? "true" : "false");
    json += ",\"hasAudio\":" + String(hasEmotionSlotAudio(*slot) ? "true" : "false");
    json += ",\"uploadedBytes\":" + String((uint32_t)slot->audioLength);
    json += ",\"uploadedName\":\"" + jsonEscapeLocal(slot->uploadedName) + "\"";
  } else {
    json += ",\"slot\":\"" + jsonEscapeLocal(name) + "\"";
    json += ",\"error\":\"unknown emotion sound slot\"";
  }
  json += "}";
  return json;
}

String MumblePlayer::playEmotionSlotJson(const String& name, uint32_t nowMs, uint32_t durationMs) {
  EmotionSoundSlot* slot = findEmotionSoundSlot(name);
  if (slot == nullptr) {
    String json = "{\"ok\":false";
    json += ",\"slot\":\"" + jsonEscapeLocal(name) + "\"";
    json += ",\"error\":\"unknown emotion sound slot\"";
    json += "}";
    return json;
  }
  if (!slot->enabled) return disabledEmotionSlotJson(*slot);
  if (!hasEmotionSlotAudio(*slot)) return missingEmotionSlotJson(name);
  return playBufferFromStartJson("emotionSlot", String(slot->name), slot->audio, slot->audioLength, nowMs, durationMs);
}

bool MumblePlayer::hasUploadedAudio() const {
  return uploadedAudio_ != nullptr && uploadedAudioLength_ > 0;
}

bool MumblePlayer::hasEmotionSlotAudio(const EmotionSoundSlot& slot) const {
  return slot.audio != nullptr && slot.audioLength > 0;
}

EmotionSoundSlot* MumblePlayer::findEmotionSoundSlot(const String& name) {
  for (uint8_t i = 0; i < MUMBLE_EMOTION_SLOT_COUNT; ++i) {
    if (name == emotionSlots_[i].name) return &emotionSlots_[i];
  }
  return nullptr;
}

const EmotionSoundSlot* MumblePlayer::findEmotionSoundSlot(const String& name) const {
  for (uint8_t i = 0; i < MUMBLE_EMOTION_SLOT_COUNT; ++i) {
    if (name == emotionSlots_[i].name) return &emotionSlots_[i];
  }
  return nullptr;
}

uint32_t MumblePlayer::countUtf8Characters(const String& text) const {
  uint32_t count = 0;
  for (uint32_t index = 0; index < text.length(); ++index) {
    uint8_t c = (uint8_t)text[index];
    if ((c & 0xC0) != 0x80) count++;
  }
  return count;
}

uint32_t MumblePlayer::durationForSubtitle(const SubtitleState& subtitle) const {
  if (!subtitle.enabled || subtitle.text.length() == 0) return segmentDurationMs_;
  uint32_t chars = countUtf8Characters(subtitle.text);
  uint32_t estimatedMs = chars * 150;
  return clampDurationMs(estimatedMs, segmentDurationMs_);
}

uint32_t MumblePlayer::playbackSampleRate() const {
  return MUMBLE_AUDIO_SAMPLE_RATE * speedPercent_ / 100;
}

uint32_t MumblePlayer::randomish(uint32_t nowMs, uint32_t salt) const {
  uint32_t x = nowMs ^ (lastStartedAtMs_ * 1103515245UL) ^ (lastStartMs_ * 2654435761UL) ^ salt;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  return x;
}

uint32_t MumblePlayer::autoDelayMs(uint32_t nowMs) const {
  uint32_t span = MUMBLE_MAX_AUTO_DELAY_MS - MUMBLE_MIN_AUTO_DELAY_MS + 1;
  return MUMBLE_MIN_AUTO_DELAY_MS + (randomish(nowMs, 29) % span);
}

void MumblePlayer::scheduleNextAuto(uint32_t nowMs) {
  nextAutoAtMs_ = nowMs + autoDelayMs(nowMs);
}

String MumblePlayer::disabledJson(const char* mode) {
  String json = "{\"ok\":false";
  json += ",\"enabled\":false";
  json += ",\"mode\":\"" + String(mode) + "\"";
  json += ",\"error\":\"mumble disabled\"";
  json += "}";
  return json;
}

String MumblePlayer::missingAudioJson(const char* mode) {
  String json = "{\"ok\":false";
  json += ",\"enabled\":" + String(enabled_ ? "true" : "false");
  json += ",\"hasAudio\":false";
  json += ",\"mode\":\"" + String(mode) + "\"";
  json += ",\"format\":\"" + String(MUMBLE_AUDIO_FORMAT) + "\"";
  json += ",\"error\":\"mumble audio not uploaded\"";
  json += "}";
  return json;
}

String MumblePlayer::missingEmotionSlotJson(const String& name) const {
  String json = "{\"ok\":false";
  json += ",\"slot\":\"" + jsonEscapeLocal(name) + "\"";
  json += ",\"hasAudio\":false";
  json += ",\"mode\":\"emotionSlot\"";
  json += ",\"format\":\"" + String(MUMBLE_AUDIO_FORMAT) + "\"";
  json += ",\"error\":\"emotion sound slot not uploaded\"";
  json += "}";
  return json;
}

String MumblePlayer::disabledEmotionSlotJson(const EmotionSoundSlot& slot) const {
  String json = "{\"ok\":false";
  json += ",\"slot\":\"" + String(slot.name) + "\"";
  json += ",\"title\":\"" + String(slot.title) + "\"";
  json += ",\"enabled\":false";
  json += ",\"mode\":\"emotionSlot\"";
  json += ",\"error\":\"emotion sound slot disabled\"";
  json += "}";
  return json;
}

String MumblePlayer::playSegmentJson(const char* mode, uint32_t nowMs, uint32_t durationMs) {
  if (!enabled_) return disabledJson(mode);
  if (!hasUploadedAudio()) return missingAudioJson(mode);
  uint32_t effectiveRate = playbackSampleRate();
  uint32_t sampleCount = (uint32_t)((uint64_t)durationMs * effectiveRate / 1000UL);
  if (sampleCount == 0) sampleCount = 1;
  if (sampleCount > uploadedAudioLength_) sampleCount = uploadedAudioLength_;
  uint32_t maxStartSample = uploadedAudioLength_ > sampleCount ? uploadedAudioLength_ - sampleCount : 0;
  uint32_t startSample = maxStartSample > 0 ? randomish(nowMs, durationMs) % (maxStartSample + 1) : 0;
  while (M5.Mic.isRecording()) delay(1);
  M5.Mic.end();
  bool speakerStarted = M5.Speaker.begin();
  M5.Speaker.setVolume(apiVoiceVolume_);
  bool started = M5.Speaker.playRaw(uploadedAudio_ + startSample, sampleCount, effectiveRate, false, 1, MUMBLE_CHANNEL, true);
  lastStartedAtMs_ = nowMs;
  lastStartMs_ = (uint32_t)((uint64_t)startSample * 1000UL / MUMBLE_AUDIO_SAMPLE_RATE);
  lastDurationMs_ = durationMs;
  lastSampleRate_ = effectiveRate;
  lastSampleCount_ = sampleCount;
  lastMode_ = mode;

  String json = "{\"ok\":";
  json += String((speakerStarted && started) ? "true" : "false");
  json += ",\"mode\":\"" + String(mode) + "\"";
  json += ",\"startMs\":" + String(lastStartMs_);
  json += ",\"durationMs\":" + String(lastDurationMs_);
  json += ",\"speedPercent\":" + String(speedPercent_);
  json += ",\"sampleRate\":" + String(lastSampleRate_);
  json += ",\"sampleCount\":" + String(lastSampleCount_);
  json += ",\"volume\":" + String(apiVoiceVolume_);
  json += ",\"speakerStarted\":" + String(speakerStarted ? "true" : "false");
  json += ",\"playRawStarted\":" + String(started ? "true" : "false");
  json += "}";
  return json;
}

String MumblePlayer::playFullFromStartJson(uint32_t nowMs) {
  if (!hasUploadedAudio()) return missingAudioJson("full");
  uint32_t effectiveRate = playbackSampleRate();
  uint32_t playbackMs = (uint32_t)((uint64_t)uploadedAudioLength_ * 1000UL / effectiveRate);
  while (M5.Mic.isRecording()) delay(1);
  M5.Mic.end();
  bool speakerStarted = M5.Speaker.begin();
  M5.Speaker.setVolume(apiVoiceVolume_);
  bool started = M5.Speaker.playRaw(uploadedAudio_, uploadedAudioLength_, effectiveRate, false, 1, MUMBLE_CHANNEL, true);
  lastStartedAtMs_ = nowMs;
  lastStartMs_ = 0;
  lastDurationMs_ = playbackMs;
  lastSampleRate_ = effectiveRate;
  lastSampleCount_ = uploadedAudioLength_;
  lastMode_ = "full";

  String json = "{\"ok\":";
  json += String((speakerStarted && started) ? "true" : "false");
  json += ",\"mode\":\"full\"";
  json += ",\"startMs\":0";
  json += ",\"durationMs\":" + String(lastDurationMs_);
  json += ",\"speedPercent\":" + String(speedPercent_);
  json += ",\"sampleRate\":" + String(lastSampleRate_);
  json += ",\"sampleCount\":" + String(lastSampleCount_);
  json += ",\"volume\":" + String(apiVoiceVolume_);
  json += ",\"speakerStarted\":" + String(speakerStarted ? "true" : "false");
  json += ",\"playRawStarted\":" + String(started ? "true" : "false");
  json += "}";
  return json;
}

String MumblePlayer::playBufferFromStartJson(const char* mode, const String& slotName, const uint8_t* audio, size_t audioLength, uint32_t nowMs, uint32_t requestedDurationMs) {
  if (audio == nullptr || audioLength == 0) return missingEmotionSlotJson(slotName);
  uint32_t effectiveRate = playbackSampleRate();
  uint32_t playbackMs = (uint32_t)((uint64_t)audioLength * 1000UL / effectiveRate);
  while (M5.Mic.isRecording()) delay(1);
  M5.Mic.end();
  bool speakerStarted = M5.Speaker.begin();
  M5.Speaker.setVolume(emotionSoundVolume_);
  bool started = M5.Speaker.playRaw(const_cast<uint8_t*>(audio), audioLength, effectiveRate, false, 1, MUMBLE_CHANNEL, true);
  lastStartedAtMs_ = nowMs;
  lastStartMs_ = 0;
  lastDurationMs_ = playbackMs;
  lastSampleRate_ = effectiveRate;
  lastSampleCount_ = (uint32_t)audioLength;
  lastMode_ = mode;
  lastEmotionSlot_ = slotName;

  String json = "{\"ok\":";
  json += String((speakerStarted && started) ? "true" : "false");
  json += ",\"mode\":\"" + String(mode) + "\"";
  json += ",\"slot\":\"" + jsonEscapeLocal(slotName) + "\"";
  json += ",\"startMs\":0";
  json += ",\"durationMs\":" + String(lastDurationMs_);
  json += ",\"requestedDurationMs\":" + String(requestedDurationMs);
  json += ",\"speedPercent\":" + String(speedPercent_);
  json += ",\"sampleRate\":" + String(lastSampleRate_);
  json += ",\"sampleCount\":" + String(lastSampleCount_);
  json += ",\"volume\":" + String(emotionSoundVolume_);
  json += ",\"emotionSoundVolume\":" + String(emotionSoundVolume_);
  json += ",\"speakerStarted\":" + String(speakerStarted ? "true" : "false");
  json += ",\"playRawStarted\":" + String(started ? "true" : "false");
  json += "}";
  return json;
}

void MumblePlayer::updateSpeechLights(uint32_t nowMs, RobotHardware& hardware, bool blockedByEmotionLights) {
  bool playing = M5.Speaker.isPlaying() && lastDurationMs_ > 0;
  if (!playing) {
    if (speechLightWasActive_) {
      hardware.applySpeechLightLevel(0.0f, false);
      speechLightWasActive_ = false;
    }
    return;
  }
  if (blockedByEmotionLights) return;
  uint32_t elapsedMs = nowMs - lastStartedAtMs_;
  uint8_t phase = (uint8_t)((elapsedMs / 90) % 6);
  float level = phase < 3 ? (0.35f + phase * 0.22f) : (0.95f - (phase - 3) * 0.22f);
  hardware.applySpeechLightLevel(level, false);
  speechLightWasActive_ = true;
}

void MumblePlayer::resetEmotionSlots() {
  for (uint8_t i = 0; i < MUMBLE_EMOTION_SLOT_COUNT; ++i) {
    releaseEmotionSlotAudio(emotionSlots_[i]);
    emotionSlots_[i].name = EMOTION_SOUND_SLOT_DEFAULTS[i].name;
    emotionSlots_[i].title = EMOTION_SOUND_SLOT_DEFAULTS[i].title;
    emotionSlots_[i].enabled = true;
    const EmotionSoundAsset* asset = findBuiltInEmotionSoundAsset(String(emotionSlots_[i].name));
    emotionSlots_[i].builtinAudio = asset != nullptr ? asset->data : nullptr;
    emotionSlots_[i].builtinAudioLength = asset != nullptr ? asset->length : 0;
    emotionSlots_[i].builtinName = asset != nullptr ? asset->filename : "";
    applyBuiltInEmotionSlotAudio(emotionSlots_[i]);
  }
}

void MumblePlayer::applyBuiltInEmotionSlotAudio(EmotionSoundSlot& slot) {
  slot.audio = slot.builtinAudio;
  slot.audioLength = slot.builtinAudioLength;
  slot.ownsAudio = false;
  if (slot.builtinAudio != nullptr && slot.builtinAudioLength > 0) {
    slot.uploadedName = "builtin:" + String(slot.builtinName);
  } else {
    slot.uploadedName = "";
  }
}

void MumblePlayer::releaseEmotionSlotAudio(EmotionSoundSlot& slot) {
  if (slot.ownsAudio && slot.audio != nullptr) {
    heap_caps_free(const_cast<uint8_t*>(slot.audio));
  }
  slot.audio = nullptr;
  slot.audioLength = 0;
  slot.ownsAudio = false;
}
