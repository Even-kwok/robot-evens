#pragma once

#include <Arduino.h>
#include "robot_hardware.h"

class ProximityTrackerController {
public:
  void begin();
  void update(RobotHardware& hardware, uint32_t nowMs);
  bool active() const;
  String statusJson() const;

private:
  ProximityState lastState_ = {};
  uint32_t lastReadAtMs_ = 0;
};
