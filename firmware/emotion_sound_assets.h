#pragma once

#include <Arduino.h>
#include <stddef.h>

struct EmotionSoundAsset {
  const char* name;
  const char* title;
  const char* filename;
  const uint8_t* data;
  size_t length;
};

extern const EmotionSoundAsset EMOTION_SOUND_ASSETS[];
extern const size_t EMOTION_SOUND_ASSET_COUNT;
