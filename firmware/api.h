#pragma once

#include <WebServer.h>
#include "assistant_status.h"
#include "camera_attention.h"
#include "emotion_actions.h"
#include "emotion_triggers.h"
#include "face_renderer.h"
#include "idle_behavior.h"
#include "local_face_detector.h"
#include "motion_stream.h"
#include "mumble_player.h"
#include "proximity_tracker.h"
#include "robot_hardware.h"
#include "robot_clock.h"
#include "wifi_manager.h"

void registerApiRoutes(WebServer& server, RobotFaceRenderer& face, RobotHardware& hardware, IdleBehaviorController& idle, EmotionActionController& emotion, EmotionTriggerController& triggers, MumblePlayer& mumble, RobotClock& clock, ProximityTrackerController& proximity, CameraAttentionController& camera, LocalFaceDetector& faceDetector, MotionStreamReceiver& motionStream, AssistantStatusDisplay& assistantStatus, RobotWifiManager& wifiManager);
bool handlePowerButtonWake(RobotFaceRenderer& face, IdleBehaviorController& idle, RobotClock& clock, uint32_t nowMs);
