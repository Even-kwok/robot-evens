#include "motion_stream.h"

namespace {

constexpr uint8_t MOTION_STREAM_MAGIC_0 = 'R';
constexpr uint8_t MOTION_STREAM_MAGIC_1 = 'E';
constexpr uint8_t MOTION_STREAM_VERSION = 1;
constexpr uint8_t MOTION_STREAM_PACKET_BYTES = 12;
constexpr uint8_t MOTION_STREAM_MAX_PACKET_BYTES = 64;
constexpr uint8_t MOTION_STREAM_MAX_PACKETS_PER_UPDATE = 12;

uint16_t readUInt16(const uint8_t* packet, uint8_t offset) {
  return ((uint16_t)packet[offset] << 8) | packet[offset + 1];
}

int16_t readInt16(const uint8_t* packet, uint8_t offset) {
  return (int16_t)readUInt16(packet, offset);
}

}  // namespace

bool MotionStreamReceiver::begin(uint16_t port) {
  port_ = port;
  started_ = udp_.begin(port_) == 1;
  return started_;
}

void MotionStreamReceiver::update(RobotHardware& hardware, IdleBehaviorController& idle, RobotClock& clock, uint32_t nowMs) {
  if (!started_ || !hardware.isReady()) return;

  bool hasLatestPose = false;
  uint8_t packet[MOTION_STREAM_MAX_PACKET_BYTES] = {};
  uint8_t processed = 0;
  int packetSize = udp_.parsePacket();
  while (packetSize > 0 && processed < MOTION_STREAM_MAX_PACKETS_PER_UPDATE) {
    int bytesRead = udp_.read(packet, sizeof(packet));
    if (parsePosePacket(packet, bytesRead)) {
      packetsReceived_++;
      lastPacketAtMs_ = nowMs;
      hasLatestPose = true;
    } else {
      invalidPackets_++;
    }
    processed++;
    packetSize = udp_.parsePacket();
  }

  if (!hasLatestPose) return;
  clock.markInteraction(nowMs);
  idle.pauseForManualControl(nowMs, MOTION_STREAM_IDLE_HOLD_MS);
  hardware.extendTemporaryRgbOverride(nowMs);
  hardware.pose(latestYaw_, latestPitch_, latestSpeed_);
}

String MotionStreamReceiver::statusJson(uint32_t nowMs) const {
  String json = "{\"ok\":true";
  json += ",\"enabled\":" + String(started_ ? "true" : "false");
  json += ",\"port\":" + String(port_);
  json += ",\"packetsReceived\":" + String(packetsReceived_);
  json += ",\"invalidPackets\":" + String(invalidPackets_);
  json += ",\"lastPacketAgeMs\":";
  if (lastPacketAtMs_ == 0) {
    json += "null";
  } else {
    json += String(nowMs - lastPacketAtMs_);
  }
  json += ",\"sequence\":" + String(latestSequence_);
  json += ",\"yaw\":" + String(latestYaw_);
  json += ",\"pitch\":" + String(latestPitch_);
  json += ",\"speed\":" + String(latestSpeed_);
  json += "}";
  return json;
}

bool MotionStreamReceiver::parsePosePacket(const uint8_t* packet, int length) {
  if (length != MOTION_STREAM_PACKET_BYTES) return false;
  if (packet[0] != MOTION_STREAM_MAGIC_0 || packet[1] != MOTION_STREAM_MAGIC_1) return false;
  if (packet[2] != MOTION_STREAM_VERSION) return false;

  latestSequence_ = readUInt16(packet, 4);
  latestYaw_ = constrain((int)readInt16(packet, 6), YAW_MIN, YAW_MAX);
  latestPitch_ = constrain((int)readInt16(packet, 8), PITCH_MIN, PITCH_MAX);
  latestSpeed_ = constrain((int)readUInt16(packet, 10), SPEED_MIN, SPEED_MAX);
  return true;
}
