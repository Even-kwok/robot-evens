#include "proximity_tracker.h"

namespace {

constexpr uint16_t DEBUG_NEAR_THRESHOLD = 180;

}  // namespace

void ProximityTrackerController::begin() {
  lastState_ = {};
  lastReadAtMs_ = 0;
}

void ProximityTrackerController::update(RobotHardware& hardware, uint32_t nowMs) {
  lastState_ = hardware.readProximityState();
  lastReadAtMs_ = nowMs;
}

bool ProximityTrackerController::active() const {
  return false;
}

String ProximityTrackerController::statusJson() const {
  String json = "{\"ok\":true";
  json += ",\"mode\":\"debug\"";
  json += ",\"active\":false";
  json += ",\"available\":" + String(lastState_.available ? "true" : "false");
  json += ",\"sensorEnabled\":" + String(lastState_.enabled ? "true" : "false");
  json += ",\"near\":" + String(lastState_.proximityRaw >= DEBUG_NEAR_THRESHOLD ? "true" : "false");
  json += ",\"proximityRaw\":" + String(lastState_.proximityRaw);
  json += ",\"ambientRaw\":" + String(lastState_.ambientRaw);
  json += ",\"threshold\":" + String(DEBUG_NEAR_THRESHOLD);
  json += ",\"lastReadAtMs\":" + String(lastReadAtMs_);
  json += "}";
  return json;
}
