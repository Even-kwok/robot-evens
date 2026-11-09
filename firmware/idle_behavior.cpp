#include "idle_behavior.h"
#include <string.h>

namespace {

struct IdleBehavior {
  const char* name;
  const IdleStep* steps;
  uint8_t count;
};

const IdleStep BEHAVIOR_SCAN_LEFT_RIGHT[] = {
  {0, 142, -0.30f, -0.02f, 0.03f, 300, 86, "眼睛先看/视线微扫左"},
  {-180, 150, -0.44f, 0.00f, 0.04f, 680, 92, "头部慢跟左"},
  {-240, 150, -0.38f, 0.01f, 0.03f, 760, 86, "头部跟随左停"},
  {-168, 138, -0.20f, 0.00f, 0.02f, 420, 62, "轻微回弹左"},
  {0, 124, 0.0f, 0.0f, 0.05f, 980, 58, "自然停顿/呼吸停顿"},
  {0, 142, 0.30f, -0.02f, 0.03f, 300, 86, "视线微扫右"},
  {180, 150, 0.44f, 0.00f, 0.04f, 680, 92, "头部慢跟右"},
  {240, 150, 0.38f, 0.01f, 0.03f, 760, 86, "头部跟随右停"},
  {168, 138, 0.20f, 0.00f, 0.02f, 420, 62, "轻微回弹右"},
  {0, 124, 0.0f, 0.0f, 0.05f, 1080, 58, "自然停顿/呼吸停顿"},
};

const IdleStep BEHAVIOR_DIAGONAL_UP[] = {
  {0, 136, -0.30f, -0.34f, 0.03f, 300, 76, "视线微扫左上"},
  {-78, 246, -0.24f, -0.30f, 0.04f, 700, 74, "头部慢跟左上"},
  {-104, 274, -0.18f, -0.24f, 0.03f, 560, 68, "头部跟随左上"},
  {-72, 238, -0.12f, -0.18f, 0.02f, 360, 50, "轻微回弹左上"},
  {0, 142, 0.0f, -0.06f, 0.05f, 940, 56, "自然停顿/呼吸停顿"},
  {0, 136, 0.28f, -0.32f, 0.03f, 300, 76, "视线微扫右上"},
  {74, 238, 0.22f, -0.28f, 0.04f, 680, 74, "头部慢跟右上"},
  {96, 260, 0.16f, -0.22f, 0.03f, 520, 66, "头部跟随右上"},
  {64, 226, 0.10f, -0.16f, 0.02f, 360, 50, "轻微回弹右上"},
  {0, 138, 0.0f, 0.0f, 0.05f, 980, 56, "自然停顿/呼吸停顿"},
};

const IdleStep BEHAVIOR_THINK_DOWN[] = {
  {0, 120, -0.12f, 0.34f, 0.03f, 320, 70, "视线微扫下方"},
  {-38, 82, -0.18f, 0.38f, 0.02f, 760, 58, "头部慢跟低头思考"},
  {-56, 64, -0.20f, 0.44f, 0.02f, 840, 48, "二次确认下方"},
  {-28, 88, -0.08f, 0.26f, 0.03f, 420, 46, "轻微回弹思考"},
  {0, 116, 0.0f, 0.0f, 0.05f, 1520, 50, "自然停顿/呼吸停顿"},
};

const IdleStep BEHAVIOR_LOOK_UP[] = {
  {0, 130, 0.0f, -0.40f, 0.03f, 300, 72, "视线微扫上方"},
  {24, 270, 0.06f, -0.34f, 0.04f, 720, 66, "头部慢跟向上看"},
  {38, 320, 0.08f, -0.38f, 0.03f, 720, 62, "头部跟随向上看"},
  {18, 280, 0.03f, -0.25f, 0.02f, 420, 44, "轻微回弹向上"},
  {0, 126, 0.0f, 0.0f, 0.05f, 1220, 50, "自然停顿/呼吸停顿"},
};

const IdleStep BEHAVIOR_CENTER_PAUSE[] = {
  {0, 122, 0.08f, 0.01f, 0.03f, 320, 54, "视线微扫近处"},
  {34, 130, 0.06f, 0.01f, 0.03f, 620, 48, "头部慢跟小幅观察"},
  {22, 126, 0.02f, 0.00f, 0.02f, 360, 40, "轻微回弹观察"},
  {-18, 124, -0.05f, 0.01f, 0.03f, 520, 42, "二次确认近处"},
  {0, 120, 0.0f, 0.0f, 0.05f, 1620, 44, "自然停顿/呼吸停顿"},
};

const IdleStep BEHAVIOR_SOFT_ATTENTION[] = {
  {0, 124, 0.12f, 0.02f, 0.03f, 340, 52, "视线微扫近处"},
  {46, 132, 0.16f, 0.02f, 0.04f, 620, 50, "头部慢跟近处"},
  {28, 128, 0.08f, 0.01f, 0.03f, 400, 42, "轻微回弹近处"},
  {-22, 126, -0.08f, 0.02f, 0.03f, 540, 42, "二次确认近处"},
  {0, 122, 0.0f, 0.0f, 0.05f, 1040, 44, "呼吸停顿近处"},
};

const IdleStep BEHAVIOR_CURIOUS_SETTLE[] = {
  {0, 128, 0.26f, -0.14f, 0.03f, 320, 58, "视线微扫右上"},
  {64, 204, 0.20f, -0.22f, 0.04f, 660, 58, "头部慢跟右上"},
  {84, 224, 0.15f, -0.18f, 0.03f, 520, 50, "头部跟随右上停"},
  {48, 186, 0.08f, -0.10f, 0.02f, 380, 42, "轻微回弹右上"},
  {0, 124, -0.10f, 0.08f, 0.03f, 520, 44, "二次确认回中"},
  {0, 120, 0.0f, 0.0f, 0.05f, 1120, 44, "呼吸停顿回中"},
};

const IdleStep BEHAVIOR_MICRO_NOD[] = {
  {0, 126, 0.00f, 0.04f, 0.04f, 280, 62, "轻点头准备"},
  {0, 178, 0.00f, -0.12f, 0.05f, 420, 66, "轻点头抬起"},
  {0, 96, 0.00f, 0.18f, 0.04f, 420, 62, "轻点头落下"},
  {16, 124, 0.04f, 0.02f, 0.04f, 520, 46, "轻点头小停"},
  {0, 120, 0.00f, 0.00f, 0.05f, 980, 42, "轻点头回中"},
};

const IdleStep BEHAVIOR_SHY_PEEK[] = {
  {0, 122, -0.16f, 0.18f, 0.03f, 320, 52, "偷看前眼神躲开"},
  {-64, 92, -0.28f, 0.26f, 0.03f, 700, 48, "偷看左下"},
  {-32, 112, -0.14f, 0.12f, 0.04f, 460, 44, "偷偷看回来"},
  {42, 126, 0.18f, 0.00f, 0.04f, 560, 46, "偷看右侧确认"},
  {0, 120, 0.00f, 0.00f, 0.05f, 1040, 42, "偷看回中"},
};

const IdleStep BEHAVIOR_SLEEPY_SETTLE[] = {
  {0, 118, 0.00f, 0.22f, 0.02f, 420, 42, "困意下垂"},
  {-18, 102, -0.06f, 0.28f, 0.02f, 760, 36, "慢慢低头"},
  {-8, 112, -0.02f, 0.16f, 0.03f, 620, 34, "困意微回弹"},
  {16, 126, 0.08f, 0.04f, 0.03f, 520, 40, "醒一下"},
  {0, 120, 0.00f, 0.00f, 0.04f, 1200, 38, "困意回中"},
};

const IdleStep BEHAVIOR_WIDE_ARC_SCAN[] = {
  {0, 134, -0.38f, -0.20f, 0.03f, 300, 76, "多角度环视/眼睛先看左上"},
  {-300, 286, -0.46f, -0.34f, 0.04f, 760, 76, "头部跟随左上"},
  {-300, 92, -0.46f, 0.22f, 0.03f, 760, 68, "斜下观察左侧"},
  {-108, 118, -0.08f, 0.20f, 0.03f, 520, 58, "眼神先犹豫回中"},
  {300, 286, 0.46f, -0.32f, 0.04f, 760, 76, "抬眼想一下右上"},
  {300, 92, 0.46f, 0.22f, 0.03f, 760, 68, "斜下观察右侧"},
  {96, 132, 0.18f, 0.02f, 0.03f, 460, 54, "轻微回弹多角度"},
  {0, 122, 0.00f, 0.00f, 0.05f, 1160, 46, "自然停顿/多角度环视收住"},
};

const IdleStep BEHAVIOR_LAYERED_ATTENTION[] = {
  {0, 124, 0.10f, 0.08f, 0.03f, 300, 54, "远近切换/近处确认"},
  {28, 148, 0.04f, -0.04f, 0.04f, 520, 52, "头部慢跟远一点"},
  {-24, 112, -0.18f, 0.22f, 0.02f, 620, 46, "斜下观察近处"},
  {0, 132, 0.22f, -0.12f, 0.03f, 420, 48, "远处再看右上"},
  {48, 188, 0.18f, -0.18f, 0.04f, 620, 54, "头部跟随远处"},
  {-12, 118, -0.04f, 0.20f, 0.03f, 520, 44, "眼神先犹豫近处"},
  {0, 120, 0.00f, 0.00f, 0.05f, 1180, 44, "呼吸停顿远近切换"},
};

const IdleStep BEHAVIOR_THOUGHTFUL_PONDER[] = {
  {0, 124, -0.10f, 0.26f, 0.03f, 320, 54, "眼神先犹豫"},
  {-34, 92, -0.18f, 0.36f, 0.02f, 680, 48, "斜下观察/思考落点"},
  {-62, 76, -0.22f, 0.46f, 0.02f, 940, 42, "思考停顿左下"},
  {0, 104, 0.00f, 0.38f, 0.02f, 760, 40, "思考停顿正下方"},
  {46, 118, 0.18f, 0.22f, 0.03f, 520, 44, "抬眼想一下"},
  {18, 146, 0.10f, -0.08f, 0.04f, 460, 48, "想完轻抬眼"},
  {0, 120, 0.00f, 0.00f, 0.05f, 1380, 42, "自然停顿/思考收住"},
};

const IdleStep BEHAVIOR_PROXIMITY_SWEEP[] = {
  {0, 156, -0.10f, -0.06f, 0.04f, 360, 88, "红外巡扫准备"},
  {-240, 210, -0.42f, -0.12f, 0.04f, 820, 96, "红外巡扫左中"},
  {-360, 272, -0.48f, -0.30f, 0.04f, 980, 104, "红外巡扫左远"},
  {-320, 82, -0.46f, 0.26f, 0.03f, 900, 96, "红外巡扫左下"},
  {0, 146, 0.00f, 0.00f, 0.04f, 540, 78, "红外巡扫回中"},
  {240, 210, 0.42f, -0.12f, 0.04f, 820, 96, "红外巡扫右中"},
  {360, 272, 0.48f, -0.30f, 0.04f, 980, 104, "红外巡扫右远"},
  {320, 82, 0.46f, 0.26f, 0.03f, 900, 96, "红外巡扫右下"},
  {0, 122, 0.00f, 0.00f, 0.05f, 960, 58, "红外巡扫收住"},
};

const IdleStep BEHAVIOR_EMO_SAD[] = {
  {0, 118, -0.12f, 0.30f, 0.01f, 420, 46, "emo低头眼神下沉"},
  {-42, 86, -0.24f, 0.48f, 0.01f, 860, 38, "emo低头斜下停住"},
  {-18, 72, -0.14f, 0.56f, 0.01f, 980, 32, "emo低头沉默"},
  {12, 96, 0.06f, 0.38f, 0.02f, 620, 38, "emo低头轻轻回一点"},
  {0, 118, 0.00f, 0.18f, 0.02f, 1160, 34, "emo低头回中"},
};

const IdleStep BEHAVIOR_TIRED_DRIFT[] = {
  {0, 116, 0.00f, 0.22f, 0.02f, 520, 40, "疲惫回落准备"},
  {-24, 88, -0.12f, 0.42f, 0.02f, 880, 34, "疲惫回落低头"},
  {-58, 64, -0.26f, 0.58f, 0.01f, 1120, 28, "疲惫低头打盹"},
  {18, 94, 0.08f, 0.34f, 0.02f, 620, 36, "疲惫回落醒一下"},
  {0, 118, 0.00f, 0.10f, 0.03f, 1180, 36, "疲惫回落回中"},
};

const IdleStep BEHAVIOR_DOZE_SLEEP[] = {
  {0, 102, 0.00f, 0.34f, 0.01f, 760, 30, "打瞌睡眼皮沉"},
  {-18, 54, -0.08f, 0.58f, 0.00f, 1260, 24, "打瞌睡低头"},
  {0, PITCH_MIN + 10, 0.00f, 0.62f, 0.00f, 1800, 22, "待机睡觉"},
  {22, PITCH_MIN + 18, 0.08f, 0.56f, 0.00f, 1280, 24, "打瞌睡栽头"},
  {0, 110, 0.00f, 0.24f, 0.02f, 720, 34, "打瞌睡微醒"},
};

const IdleBehavior IDLE_BEHAVIORS[] = {
  {"左右扫视", BEHAVIOR_SCAN_LEFT_RIGHT, (uint8_t)(sizeof(BEHAVIOR_SCAN_LEFT_RIGHT) / sizeof(BEHAVIOR_SCAN_LEFT_RIGHT[0]))},
  {"斜上方看", BEHAVIOR_DIAGONAL_UP, (uint8_t)(sizeof(BEHAVIOR_DIAGONAL_UP) / sizeof(BEHAVIOR_DIAGONAL_UP[0]))},
  {"低头思考", BEHAVIOR_THINK_DOWN, (uint8_t)(sizeof(BEHAVIOR_THINK_DOWN) / sizeof(BEHAVIOR_THINK_DOWN[0]))},
  {"向上看", BEHAVIOR_LOOK_UP, (uint8_t)(sizeof(BEHAVIOR_LOOK_UP) / sizeof(BEHAVIOR_LOOK_UP[0]))},
  {"回中停顿", BEHAVIOR_CENTER_PAUSE, (uint8_t)(sizeof(BEHAVIOR_CENTER_PAUSE) / sizeof(BEHAVIOR_CENTER_PAUSE[0]))},
  {"近处注意", BEHAVIOR_SOFT_ATTENTION, (uint8_t)(sizeof(BEHAVIOR_SOFT_ATTENTION) / sizeof(BEHAVIOR_SOFT_ATTENTION[0]))},
  {"好奇回落", BEHAVIOR_CURIOUS_SETTLE, (uint8_t)(sizeof(BEHAVIOR_CURIOUS_SETTLE) / sizeof(BEHAVIOR_CURIOUS_SETTLE[0]))},
  {"轻点头", BEHAVIOR_MICRO_NOD, (uint8_t)(sizeof(BEHAVIOR_MICRO_NOD) / sizeof(BEHAVIOR_MICRO_NOD[0]))},
  {"偷看回中", BEHAVIOR_SHY_PEEK, (uint8_t)(sizeof(BEHAVIOR_SHY_PEEK) / sizeof(BEHAVIOR_SHY_PEEK[0]))},
  {"困意回落", BEHAVIOR_SLEEPY_SETTLE, (uint8_t)(sizeof(BEHAVIOR_SLEEPY_SETTLE) / sizeof(BEHAVIOR_SLEEPY_SETTLE[0]))},
  {"多角度环视", BEHAVIOR_WIDE_ARC_SCAN, (uint8_t)(sizeof(BEHAVIOR_WIDE_ARC_SCAN) / sizeof(BEHAVIOR_WIDE_ARC_SCAN[0]))},
  {"远近切换", BEHAVIOR_LAYERED_ATTENTION, (uint8_t)(sizeof(BEHAVIOR_LAYERED_ATTENTION) / sizeof(BEHAVIOR_LAYERED_ATTENTION[0]))},
  {"思考停顿", BEHAVIOR_THOUGHTFUL_PONDER, (uint8_t)(sizeof(BEHAVIOR_THOUGHTFUL_PONDER) / sizeof(BEHAVIOR_THOUGHTFUL_PONDER[0]))},
  {"红外巡扫", BEHAVIOR_PROXIMITY_SWEEP, (uint8_t)(sizeof(BEHAVIOR_PROXIMITY_SWEEP) / sizeof(BEHAVIOR_PROXIMITY_SWEEP[0]))},
  {"emo低头", BEHAVIOR_EMO_SAD, (uint8_t)(sizeof(BEHAVIOR_EMO_SAD) / sizeof(BEHAVIOR_EMO_SAD[0]))},
  {"疲惫回落", BEHAVIOR_TIRED_DRIFT, (uint8_t)(sizeof(BEHAVIOR_TIRED_DRIFT) / sizeof(BEHAVIOR_TIRED_DRIFT[0]))},
  {"打瞌睡", BEHAVIOR_DOZE_SLEEP, (uint8_t)(sizeof(BEHAVIOR_DOZE_SLEEP) / sizeof(BEHAVIOR_DOZE_SLEEP[0]))},
};

constexpr uint8_t IDLE_BEHAVIOR_COUNT = sizeof(IDLE_BEHAVIORS) / sizeof(IDLE_BEHAVIORS[0]);
constexpr uint32_t NIGHT_DOZE_INTERVAL_MS = 8500;
constexpr uint16_t NIGHT_TIRED_STEP_MS = 1800;
constexpr uint16_t NIGHT_SLEEP_STEP_MS = 4200;
constexpr int16_t NIGHT_SLEEP_PITCH = PITCH_MIN;
constexpr int16_t NIGHT_TIRED_PITCH = PITCH_MIN + 12;
constexpr int16_t NIGHT_TIRED_STARTLE_PITCH = DEFAULT_REST_PITCH + 18;
constexpr uint16_t NIGHT_SLEEP_SPEED = 32;

const IdleStep NIGHT_TIRED_IDLE_SEQUENCE[] = {
  {0, NIGHT_TIRED_PITCH, 0.00f, 0.54f, 0.02f, 2100, NIGHT_SLEEP_SPEED + 6, "疲惫低头打盹"},
  {-248, NIGHT_TIRED_PITCH + 16, -0.50f, 0.56f, 0.02f, 1850, NIGHT_SLEEP_SPEED + 12, "疲惫左栽头"},
  {-322, NIGHT_TIRED_PITCH + 28, -0.58f, 0.50f, 0.02f, 1650, NIGHT_SLEEP_SPEED + 10, "疲惫左右大角度打盹-左"},
  {-86, NIGHT_TIRED_PITCH + 10, -0.24f, 0.58f, 0.02f, 1480, NIGHT_SLEEP_SPEED + 8, "疲惫低头打盹"},
  {18, NIGHT_TIRED_STARTLE_PITCH, 0.06f, -0.08f, 0.07f, 360, NIGHT_SLEEP_SPEED + 46, "疲惫惊跳一下"},
  {-22, NIGHT_TIRED_PITCH + 12, -0.08f, 0.52f, 0.03f, 960, NIGHT_SLEEP_SPEED + 18, "疲惫惊跳回落"},
  {264, NIGHT_TIRED_PITCH + 18, 0.52f, 0.56f, 0.02f, 1900, NIGHT_SLEEP_SPEED + 12, "疲惫右栽头"},
  {326, NIGHT_TIRED_PITCH + 30, 0.58f, 0.50f, 0.02f, 1680, NIGHT_SLEEP_SPEED + 10, "疲惫左右大角度打盹-右"},
  {72, NIGHT_TIRED_PITCH + 8, 0.18f, 0.58f, 0.02f, 1520, NIGHT_SLEEP_SPEED + 8, "疲惫低头打盹"},
  {0, NIGHT_TIRED_PITCH + 6, 0.00f, 0.56f, 0.02f, 2300, NIGHT_SLEEP_SPEED + 6, "疲惫低头停住"},
};

constexpr uint8_t NIGHT_TIRED_SEQUENCE_COUNT = sizeof(NIGHT_TIRED_IDLE_SEQUENCE) / sizeof(NIGHT_TIRED_IDLE_SEQUENCE[0]);

bool idleLabelContains(const IdleStep& step, const char* needle) {
  return step.label != nullptr && strstr(step.label, needle) != nullptr;
}

const char* idleMoodForStep(const IdleStep& step) {
  if (idleLabelContains(step, "睡觉") || idleLabelContains(step, "睡着") || idleLabelContains(step, "打瞌睡")) {
    return "sleeping";
  }
  if (idleLabelContains(step, "疲惫") || idleLabelContains(step, "困意")) {
    return "tired";
  }
  if (idleLabelContains(step, "emo") || idleLabelContains(step, "伤心")) {
    return "sad";
  }
  if (idleLabelContains(step, "思考") || idleLabelContains(step, "犹豫") || idleLabelContains(step, "想一下")) {
    return "thinking";
  }
  return "neutral";
}

bool idleWakeableForStep(const IdleStep& step) {
  return idleLabelContains(step, "睡觉") ||
         idleLabelContains(step, "睡着") ||
         idleLabelContains(step, "打瞌睡") ||
         idleLabelContains(step, "打盹") ||
         idleLabelContains(step, "栽头");
}

}  // namespace

void IdleBehaviorController::begin(bool enabled) {
  enabled_ = enabled;
  behaviorIndex_ = 0;
  stepIndex_ = 0;
  nextStepAtMs_ = 0;
  pausedUntilMs_ = 0;
  lastAppliedYaw_ = 0;
  lastAppliedPitch_ = 120;
  nextNightStepAtMs_ = 0;
  nightStepIndex_ = 0;
  currentStepWakeableByTouch_ = false;
}

void IdleBehaviorController::update(RobotHardware& hardware, RobotFaceRenderer& face, RobotClock& clock, uint32_t nowMs) {
  if (!enabled_ || !hardware.isReady()) return;
  if ((!clock.isNightNow() || clock.nightAwakeActive(nowMs)) && clock.sleeping()) clock.setSleeping(false, nowMs);
  if (nowMs < pausedUntilMs_) return;
  hardware.restoreStandbyRgb(nowMs);
  if (clock.isNightNow() && !clock.nightAwakeActive(nowMs)) {
    applyNightStep(hardware, face, clock, nowMs);
    return;
  }
  if (nowMs < nextStepAtMs_) return;

  const IdleBehavior& behavior = IDLE_BEHAVIORS[behaviorIndex_];
  const IdleStep& step = behavior.steps[stepIndex_];
  currentBehaviorName_ = behavior.name;
  currentStepLabel_ = step.label;
  applyStep(hardware, face, step);

  nextStepAtMs_ = nowMs + step.durationMs;
  stepIndex_++;
  if (stepIndex_ >= behavior.count) {
    stepIndex_ = 0;
    behaviorIndex_ = chooseNextBehavior(nowMs);
    nextStepAtMs_ += naturalPauseJitterMs(nowMs);
  }
}

void IdleBehaviorController::setEnabled(bool enabled) {
  enabled_ = enabled;
  if (enabled_) nextStepAtMs_ = 0;
}

void IdleBehaviorController::pauseForManualControl(uint32_t nowMs, uint32_t durationMs) {
  pausedUntilMs_ = nowMs + durationMs;
  nextStepAtMs_ = pausedUntilMs_;
  currentStepWakeableByTouch_ = false;
}

void IdleBehaviorController::triggerNext(uint32_t nowMs) {
  behaviorIndex_ = chooseNextBehavior(nowMs);
  stepIndex_ = 0;
  pausedUntilMs_ = 0;
  nextStepAtMs_ = nowMs;
}

bool IdleBehaviorController::enabled() const {
  return enabled_;
}

const char* IdleBehaviorController::currentStepLabel() const {
  return currentStepLabel_;
}

bool IdleBehaviorController::currentStepWakeableByTouch() const {
  return currentStepWakeableByTouch_;
}

String IdleBehaviorController::statusJson() const {
  uint32_t nowMs = millis();
  uint32_t pausedMs = nowMs < pausedUntilMs_ ? pausedUntilMs_ - nowMs : 0;
  uint32_t nextInMs = nowMs < nextStepAtMs_ ? nextStepAtMs_ - nowMs : 0;
  String json = "{\"ok\":true";
  json += ",\"enabled\":" + String(enabled_ ? "true" : "false");
  json += ",\"active\":" + String((enabled_ && pausedMs == 0) ? "true" : "false");
  json += ",\"behavior\":\"" + String(currentBehaviorName_) + "\"";
  json += ",\"step\":\"" + String(currentStepLabel_) + "\"";
  json += ",\"wakeableByTouch\":" + String(currentStepWakeableByTouch_ ? "true" : "false");
  json += ",\"nextInMs\":" + String(nextInMs);
  json += ",\"pausedMs\":" + String(pausedMs);
  json += ",\"library\":[\"左右扫视\",\"斜上方看\",\"低头思考\",\"向上看\",\"回中停顿\",\"近处注意\",\"好奇回落\",\"轻点头\",\"偷看回中\",\"困意回落\",\"多角度环视\",\"远近切换\",\"思考停顿\",\"红外巡扫\",\"emo低头\",\"疲惫回落\",\"打瞌睡\"]";
  json += "}";
  return json;
}

uint8_t IdleBehaviorController::chooseNextBehavior(uint32_t nowMs) const {
  uint8_t stride = 1 + (uint8_t)((nowMs / 1379 + abs(lastAppliedYaw_) / 40 + lastAppliedPitch_ / 60) % (IDLE_BEHAVIOR_COUNT - 1));
  uint8_t next = (behaviorIndex_ + stride) % IDLE_BEHAVIOR_COUNT;
  if (next == behaviorIndex_) next = (next + 1) % IDLE_BEHAVIOR_COUNT;
  return next;
}

uint16_t IdleBehaviorController::naturalPauseJitterMs(uint32_t nowMs) const {
  return 650 + (uint16_t)((nowMs / 97 + abs(lastAppliedYaw_) + lastAppliedPitch_) % 6) * 170;
}

void IdleBehaviorController::applyStep(RobotHardware& hardware, RobotFaceRenderer& face, const IdleStep& step) {
  hardware.poseWithRestPitchOffset(step.yaw, step.pitch, step.speed);
  face.setMood(idleMoodForStep(step));
  face.setReact(step.lookX, step.lookY, step.ambientLevel);
  lastAppliedYaw_ = step.yaw;
  lastAppliedPitch_ = step.pitch;
  currentStepWakeableByTouch_ = idleWakeableForStep(step);
}

void IdleBehaviorController::applyNightStep(RobotHardware& hardware, RobotFaceRenderer& face, RobotClock& clock, uint32_t nowMs) {
  if (nowMs < nextNightStepAtMs_) return;

  currentBehaviorName_ = "夜间疲惫";
  if (clock.shouldSleep(nowMs)) {
    clock.setSleeping(true, nowMs);
    currentStepLabel_ = "长时间无交互入睡";
    hardware.pose(0, NIGHT_SLEEP_PITCH, NIGHT_SLEEP_SPEED);
    face.setMood("sleeping");
    face.setReact(0.0f, 0.58f, 0.01f);
    lastAppliedYaw_ = 0;
    lastAppliedPitch_ = NIGHT_SLEEP_PITCH;
    currentStepWakeableByTouch_ = true;
    nextNightStepAtMs_ = nowMs + NIGHT_SLEEP_STEP_MS;
    return;
  }

  clock.setSleeping(false, nowMs);
  uint8_t tiredCyclePhase = nightStepIndex_ % (NIGHT_TIRED_SEQUENCE_COUNT + 3);
  bool dozing = tiredCyclePhase == NIGHT_TIRED_SEQUENCE_COUNT && nowMs >= NIGHT_DOZE_INTERVAL_MS;
  if (dozing) {
    currentStepLabel_ = "频繁睡着";
    hardware.pose(0, NIGHT_SLEEP_PITCH + 8, NIGHT_SLEEP_SPEED);
    face.setMood("sleeping");
    face.setReact(0.0f, 0.52f, 0.02f);
    lastAppliedYaw_ = 0;
    lastAppliedPitch_ = NIGHT_SLEEP_PITCH + 8;
    currentStepWakeableByTouch_ = true;
    nextNightStepAtMs_ = nowMs + NIGHT_TIRED_STEP_MS;
  } else {
    uint8_t sequenceIndex = nightStepIndex_ % NIGHT_TIRED_SEQUENCE_COUNT;
    const IdleStep& step = NIGHT_TIRED_IDLE_SEQUENCE[sequenceIndex];
    currentStepLabel_ = step.label;
    hardware.poseWithRestPitchOffset(step.yaw, step.pitch, step.speed);
    face.setMood("tired");
    face.setReact(step.lookX, step.lookY, step.ambientLevel);
    lastAppliedYaw_ = step.yaw;
    lastAppliedPitch_ = step.pitch;
    currentStepWakeableByTouch_ = idleWakeableForStep(step);
    nextNightStepAtMs_ = nowMs + step.durationMs;
  }
  nightStepIndex_++;
}
