#pragma once

#include <Arduino.h>
#include "camera_attention.h"
#include "emotion_triggers.h"
#include "idle_behavior.h"
#include "mumble_player.h"
#include "robot_clock.h"

String catalogJson(const EmotionTriggerController& triggers, const IdleBehaviorController& idle, const MumblePlayer& mumble, const RobotClock& clock, const CameraAttentionController& camera);
String catalogAssetsJson(const MumblePlayer& mumble);
String catalogLogicJson(const EmotionTriggerController& triggers, const IdleBehaviorController& idle, const RobotClock& clock, const CameraAttentionController& camera);
String catalogConflictsJson();
