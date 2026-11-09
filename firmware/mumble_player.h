#pragma once

#include <Arduino.h>
#include "face_renderer.h"

class RobotHardware;

struct EmotionSoundSlot {
  const char* name = "";
  const char* title = "";
  bool enabled = true;
  const uint8_t* audio = nullptr;
  size_t audioLength = 0;
  bool ownsAudio = false;
  const uint8_t* builtinAudio = nullptr;
  size_t builtinAudioLength = 0;
  const char* builtinName = "";
  String uploadedName = "";
};

class MumblePlayer {
public:
  static constexpr uint8_t MUMBLE_EMOTION_SLOT_COUNT = 15;

  void begin();
  void update(uint32_t nowMs, RobotHardware& hardware, bool allowAuto, bool blockedByEmotionLights);
  void setSpeedPercent(uint16_t value);
  void setApiVoiceVolume(uint8_t value);
  void setSegmentDurationMs(uint16_t value);
  void setEnabled(bool enabled);
  void setAutoEnabled(bool enabled, uint32_t nowMs);
  void setFullChancePercent(uint8_t value);
  bool autoEnabled() const;
  bool playing() const;
  String statusJson() const;
  bool uploadBegin(const String& filename, size_t expectedSize);
  bool uploadWrite(const uint8_t* data, size_t length);
  bool uploadEnd();
  void uploadAbort();
  String uploadStatusJson() const;
  String playRandomSegmentJson(uint32_t nowMs, uint32_t durationMs = 0);
  String playForSubtitleJson(const SubtitleState& subtitle, uint32_t nowMs);
  String playForEmotionJson(uint32_t nowMs, uint32_t durationMs);
  String playFullJson(uint32_t nowMs);
  String stopJson();
  String emotionSlotsJson() const;
  bool setEmotionSlotEnabled(const String& name, bool enabled);
  bool uploadEmotionSlotBegin(const String& name, const String& filename, size_t expectedSize);
  bool uploadEmotionSlotWrite(const uint8_t* data, size_t length);
  bool uploadEmotionSlotEnd();
  void uploadEmotionSlotAbort();
  String emotionSlotUploadStatusJson(const String& name) const;
  String playEmotionSlotJson(const String& name, uint32_t nowMs, uint32_t durationMs);

private:
  uint16_t speedPercent_ = 100;
  uint8_t apiVoiceVolume_ = 180;
  uint8_t emotionSoundVolume_ = 96;
  uint16_t segmentDurationMs_ = 1800;
  bool enabled_ = false;
  bool autoEnabled_ = false;
  uint8_t fullChancePercent_ = 18;
  uint32_t nextAutoAtMs_ = 0;
  uint32_t lastStartedAtMs_ = 0;
  uint32_t lastStartMs_ = 0;
  uint32_t lastDurationMs_ = 0;
  uint32_t lastSampleRate_ = 16000;
  uint32_t lastSampleCount_ = 0;
  const char* lastMode_ = "none";
  bool speechLightWasActive_ = false;
  uint8_t* uploadedAudio_ = nullptr;
  size_t uploadedAudioLength_ = 0;
  size_t uploadCapacity_ = 0;
  size_t uploadBytesWritten_ = 0;
  bool uploadActive_ = false;
  bool uploadFailed_ = false;
  String uploadedName_ = "";
  EmotionSoundSlot emotionSlots_[MUMBLE_EMOTION_SLOT_COUNT];
  int8_t activeEmotionUploadSlot_ = -1;
  size_t emotionUploadCapacity_ = 0;
  size_t emotionUploadBytesWritten_ = 0;
  bool emotionUploadActive_ = false;
  bool emotionUploadFailed_ = false;
  String lastEmotionSlot_ = "none";

  bool hasUploadedAudio() const;
  bool hasEmotionSlotAudio(const EmotionSoundSlot& slot) const;
  EmotionSoundSlot* findEmotionSoundSlot(const String& name);
  const EmotionSoundSlot* findEmotionSoundSlot(const String& name) const;
  uint32_t countUtf8Characters(const String& text) const;
  uint32_t durationForSubtitle(const SubtitleState& subtitle) const;
  uint32_t playbackSampleRate() const;
  uint32_t randomish(uint32_t nowMs, uint32_t salt) const;
  uint32_t autoDelayMs(uint32_t nowMs) const;
  void scheduleNextAuto(uint32_t nowMs);
  String disabledJson(const char* mode);
  String missingAudioJson(const char* mode);
  String missingEmotionSlotJson(const String& name) const;
  String disabledEmotionSlotJson(const EmotionSoundSlot& slot) const;
  String playSegmentJson(const char* mode, uint32_t nowMs, uint32_t durationMs);
  String playFullFromStartJson(uint32_t nowMs);
  String playBufferFromStartJson(const char* mode, const String& slotName, const uint8_t* audio, size_t audioLength, uint32_t nowMs, uint32_t requestedDurationMs);
  void updateSpeechLights(uint32_t nowMs, RobotHardware& hardware, bool blockedByEmotionLights);
  void resetEmotionSlots();
  void applyBuiltInEmotionSlotAudio(EmotionSoundSlot& slot);
  void releaseEmotionSlotAudio(EmotionSoundSlot& slot);
};
