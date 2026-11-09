#pragma once

#include <Arduino.h>
#include <WiFiUdp.h>
#include "idle_behavior.h"
#include "robot_clock.h"
#include "robot_hardware.h"

constexpr uint16_t MOTION_STREAM_DEFAULT_PORT = 4210;
constexpr uint32_t MOTION_STREAM_IDLE_HOLD_MS = 900;

class MotionStreamReceiver {
public:
  bool begin(uint16_t port = MOTION_STREAM_DEFAULT_PORT);
  void update(RobotHardware& hardware, IdleBehaviorController& idle, RobotClock& clock, uint32_t nowMs);
  String statusJson(uint32_t nowMs) const;

private:
  WiFiUDP udp_;
  bool started_ = false;
  uint16_t port_ = MOTION_STREAM_DEFAULT_PORT;
  uint32_t packetsReceived_ = 0;
  uint32_t invalidPackets_ = 0;
  uint32_t lastPacketAtMs_ = 0;
  uint16_t latestSequence_ = 0;
  int latestYaw_ = 0;
  int latestPitch_ = DEFAULT_REST_PITCH;
  int latestSpeed_ = 300;

  bool parsePosePacket(const uint8_t* packet, int length);
};
