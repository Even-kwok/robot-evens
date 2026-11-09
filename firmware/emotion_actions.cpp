#include "emotion_actions.h"

#include <string.h>

namespace {

struct EmotionVariant {
  const char* name;
  const char* title;
  const EmotionStep* steps;
  uint8_t count;
};

struct EmotionAction {
  const char* name;
  const char* title;
  const EmotionVariant* variants;
  uint8_t variantCount;
};

constexpr int HEAD_TOUCH_NOD_CENTER_YAW = 0;
constexpr int HEAD_TOUCH_NOD_UP_PITCH = DEFAULT_REST_PITCH + 250;
constexpr int HEAD_TOUCH_NOD_DOWN_PITCH = PITCH_MIN + 58;
constexpr int COMFORTABLE_NOD_UP_PITCH = DEFAULT_REST_PITCH + 100;
constexpr int COMFORTABLE_NOD_DOWN_PITCH = PITCH_MIN + 72;
constexpr int COMFORTABLE_NOD_SECOND_UP_PITCH = DEFAULT_REST_PITCH + 82;
constexpr int COMFORTABLE_NOD_SECOND_DOWN_PITCH = PITCH_MIN + 82;
constexpr int ROBOT_ACTION_REFERENCE_CENTER_PITCH = DEFAULT_REST_PITCH - 4;
constexpr int ROBOT_ACTION_NOD_UP_PITCH = DEFAULT_REST_PITCH + 220;
constexpr int ROBOT_ACTION_NOD_DOWN_PITCH = PITCH_MIN + 58;
constexpr int ROBOT_ACTION_SHAKE_LEFT_YAW = -320;
constexpr int ROBOT_ACTION_SHAKE_RIGHT_YAW = 320;

const EmotionStep EMOTION_HAPPY_NOD[] = {
  {"happy", 0, 160, 0.00f, -0.18f, 0.42f, 120, 470, EmotionLightEffect::RainbowMarquee, "开心点头准备"},
  {"happy", 0, 335, 0.00f, -0.48f, 0.80f, 150, 520, EmotionLightEffect::RainbowMarquee, "疯狂上下点头-抬头"},
  {"happy", 0, 70, 0.00f, 0.42f, 0.86f, 150, 520, EmotionLightEffect::RainbowMarquee, "疯狂上下点头-低头"},
  {"happy", 0, 325, 0.00f, -0.42f, 0.75f, 135, 540, EmotionLightEffect::RainbowMarquee, "疯狂上下点头-再抬头"},
  {"happy", 0, 82, 0.00f, 0.36f, 0.70f, 135, 540, EmotionLightEffect::RainbowMarquee, "疯狂上下点头-再低头"},
  {"happy", 0, 128, 0.00f, -0.02f, 0.18f, 420, 260, EmotionLightEffect::RainbowMarquee, "开心回中停顿"},
};

const EmotionStep EMOTION_HAPPY_SWAY[] = {
  {"happy", 0, 150, 0.00f, -0.12f, 0.34f, 140, 360, EmotionLightEffect::RainbowMarquee, "开心左右准备"},
  {"happy", -116, 205, -0.28f, -0.24f, 0.62f, 210, 360, EmotionLightEffect::RainbowMarquee, "开心向左探"},
  {"happy", 116, 198, 0.28f, -0.22f, 0.62f, 210, 360, EmotionLightEffect::RainbowMarquee, "开心向右探"},
  {"happy", -72, 158, -0.16f, -0.06f, 0.46f, 180, 340, EmotionLightEffect::RainbowMarquee, "开心小回左"},
  {"happy", 72, 158, 0.16f, -0.06f, 0.46f, 180, 340, EmotionLightEffect::RainbowMarquee, "开心小回右"},
  {"happy", 0, 126, 0.00f, 0.00f, 0.18f, 360, 220, EmotionLightEffect::RainbowMarquee, "开心稳住"},
};

const EmotionStep EMOTION_HAPPY_BOUNCE[] = {
  {"happy", 0, 128, 0.00f, -0.10f, 0.36f, 120, 410, EmotionLightEffect::RainbowMarquee, "开心轻快准备"},
  {"happy", 32, 245, 0.08f, -0.34f, 0.58f, 170, 430, EmotionLightEffect::RainbowMarquee, "开心轻快抬头"},
  {"happy", -24, 98, -0.06f, 0.24f, 0.55f, 150, 430, EmotionLightEffect::RainbowMarquee, "开心轻快低头"},
  {"happy", 22, 230, 0.05f, -0.28f, 0.50f, 160, 420, EmotionLightEffect::RainbowMarquee, "开心再抬"},
  {"happy", 0, 122, 0.00f, 0.02f, 0.18f, 300, 240, EmotionLightEffect::RainbowMarquee, "开心短收尾"},
};

const EmotionStep EMOTION_HEAD_TOUCH_COMBO_NOD[] = {
  {"happy", HEAD_TOUCH_NOD_CENTER_YAW, HEAD_TOUCH_NOD_UP_PITCH, 0.00f, -0.44f, 0.72f, 220, 620, EmotionLightEffect::BlinkingMarquee, "全程跑马灯-疯狂点头-抬头准备"},
  {"happy", HEAD_TOUCH_NOD_CENTER_YAW, HEAD_TOUCH_NOD_DOWN_PITCH, 0.00f, 0.44f, 0.88f, 180, 660, EmotionLightEffect::BlinkingMarquee, "疯狂点头-低头"},
  {"happy", HEAD_TOUCH_NOD_CENTER_YAW, HEAD_TOUCH_NOD_UP_PITCH, 0.00f, -0.46f, 0.84f, 180, 660, EmotionLightEffect::BlinkingMarquee, "疯狂点头-抬头"},
  {"happy", HEAD_TOUCH_NOD_CENTER_YAW, HEAD_TOUCH_NOD_DOWN_PITCH + 14, 0.00f, 0.38f, 0.82f, 170, 660, EmotionLightEffect::BlinkingMarquee, "疯狂点头-再低头"},
  {"happy", HEAD_TOUCH_NOD_CENTER_YAW, HEAD_TOUCH_NOD_UP_PITCH - 24, 0.00f, -0.40f, 0.78f, 170, 650, EmotionLightEffect::BlinkingMarquee, "疯狂点头-再抬头"},
  {"happy", HEAD_TOUCH_NOD_CENTER_YAW, HEAD_TOUCH_NOD_DOWN_PITCH + 6, 0.00f, 0.34f, 0.76f, 170, 640, EmotionLightEffect::BlinkingMarquee, "疯狂点头-第三次低头"},
  {"happy", HEAD_TOUCH_NOD_CENTER_YAW, HEAD_TOUCH_NOD_UP_PITCH - 42, 0.00f, -0.30f, 0.64f, 180, 600, EmotionLightEffect::BlinkingMarquee, "疯狂点头-第三次抬头"},
  {"happy", HEAD_TOUCH_NOD_CENTER_YAW, DEFAULT_REST_PITCH, 0.00f, -0.02f, 0.28f, 580, 360, EmotionLightEffect::BlinkingMarquee, "跑马灯收尾"},
};

const EmotionStep EMOTION_COMFORTABLE_NOD[] = {
  {"love", 0, DEFAULT_REST_PITCH + 28, 0.00f, -0.10f, 0.16f, 90, 440, EmotionLightEffect::SoftPinkPulse, "舒服眯眼"},
  {"happy", 0, COMFORTABLE_NOD_UP_PITCH, 0.00f, -0.34f, 0.34f, 125, 640, EmotionLightEffect::BlinkingMarquee, "轻轻仰头"},
  {"love", 0, COMFORTABLE_NOD_DOWN_PITCH, 0.00f, 0.34f, 0.34f, 125, 660, EmotionLightEffect::SoftPinkPulse, "舒服点头"},
  {"happy", 0, COMFORTABLE_NOD_SECOND_UP_PITCH, 0.00f, -0.28f, 0.28f, 115, 640, EmotionLightEffect::BlinkingMarquee, "再蹭一下"},
  {"love", 0, COMFORTABLE_NOD_SECOND_DOWN_PITCH, 0.00f, 0.26f, 0.26f, 115, 620, EmotionLightEffect::SoftPinkPulse, "软软低头"},
  {"happy", 0, DEFAULT_REST_PITCH, 0.00f, 0.00f, 0.08f, 220, 380, EmotionLightEffect::BlinkingMarquee, "满足停住"},
};

const EmotionStep ROBOT_ACTION_NOD_YES[] = {
  {"happy", 0, ROBOT_ACTION_REFERENCE_CENTER_PITCH, 0.00f, -0.02f, 0.18f, 120, 520, EmotionLightEffect::WarmBounce, "点头确认-准备"},
  {"happy", 0, ROBOT_ACTION_NOD_DOWN_PITCH, 0.00f, 0.44f, 0.72f, 190, 620, EmotionLightEffect::WarmBounce, "点头确认-低头"},
  {"happy", 0, ROBOT_ACTION_NOD_UP_PITCH, 0.00f, -0.40f, 0.66f, 190, 620, EmotionLightEffect::WarmBounce, "点头确认-抬头"},
  {"happy", 0, ROBOT_ACTION_NOD_DOWN_PITCH + 12, 0.00f, 0.38f, 0.56f, 170, 600, EmotionLightEffect::WarmBounce, "点头确认-再次低头"},
  {"happy", 0, ROBOT_ACTION_NOD_UP_PITCH - 48, 0.00f, -0.28f, 0.42f, 180, 560, EmotionLightEffect::WarmBounce, "点头确认-回弹抬头"},
  {"happy", 0, ROBOT_ACTION_REFERENCE_CENTER_PITCH, 0.00f, -0.02f, 0.16f, 420, 300, EmotionLightEffect::WarmBounce, "点头确认-回中"},
};

const EmotionStep ROBOT_ACTION_SHAKE_NO[] = {
  {"curious", 0, ROBOT_ACTION_REFERENCE_CENTER_PITCH, 0.00f, 0.02f, 0.12f, 120, 480, EmotionLightEffect::BluePurpleBlink, "摇头拒绝-准备"},
  {"curious", ROBOT_ACTION_SHAKE_LEFT_YAW, ROBOT_ACTION_REFERENCE_CENTER_PITCH - 4, -0.62f, 0.04f, 0.28f, 190, 620, EmotionLightEffect::BluePurpleBlink, "摇头拒绝-看左"},
  {"curious", ROBOT_ACTION_SHAKE_RIGHT_YAW, ROBOT_ACTION_REFERENCE_CENTER_PITCH - 4, 0.62f, 0.04f, 0.28f, 190, 620, EmotionLightEffect::BluePurpleBlink, "摇头拒绝-看右"},
  {"curious", ROBOT_ACTION_SHAKE_LEFT_YAW + 70, ROBOT_ACTION_REFERENCE_CENTER_PITCH - 2, -0.50f, 0.04f, 0.22f, 170, 600, EmotionLightEffect::BluePurpleBlink, "摇头拒绝-再看左"},
  {"curious", ROBOT_ACTION_SHAKE_RIGHT_YAW - 92, ROBOT_ACTION_REFERENCE_CENTER_PITCH, 0.42f, 0.02f, 0.18f, 160, 560, EmotionLightEffect::BluePurpleBlink, "摇头拒绝-回摆右"},
  {"curious", 0, ROBOT_ACTION_REFERENCE_CENTER_PITCH, 0.00f, 0.00f, 0.08f, 420, 280, EmotionLightEffect::BluePurpleBlink, "摇头拒绝-回中"},
};

const EmotionStep EMOTION_SAD_DIAGONAL_DOWN[] = {
  {"sad", 0, 125, -0.18f, 0.24f, 0.00f, 260, 150, EmotionLightEffect::Breathing, "伤心眼神下沉"},
  {"sad", -135, 76, -0.36f, 0.50f, 0.00f, 820, 120, EmotionLightEffect::Breathing, "伤心斜下方低头"},
  {"sad", -165, 54, -0.42f, 0.58f, 0.00f, 1150, 75, EmotionLightEffect::Breathing, "斜下方低头停住"},
};

const EmotionStep EMOTION_SAD_SLOW_SINK[] = {
  {"sad", 0, 126, 0.00f, 0.18f, 0.00f, 360, 120, EmotionLightEffect::Breathing, "伤心慢慢垂眼"},
  {"sad", 0, 84, 0.00f, 0.42f, 0.00f, 960, 82, EmotionLightEffect::Breathing, "伤心缓慢低头"},
  {"sad", 18, 62, 0.04f, 0.54f, 0.00f, 900, 70, EmotionLightEffect::Breathing, "伤心停在低处"},
  {"sad", 0, 96, 0.00f, 0.30f, 0.00f, 520, 82, EmotionLightEffect::Breathing, "伤心轻轻回一点"},
};

const EmotionStep EMOTION_SAD_SIDE_SIGH[] = {
  {"sad", 0, 118, 0.12f, 0.26f, 0.00f, 300, 130, EmotionLightEffect::Breathing, "伤心侧看"},
  {"sad", 112, 88, 0.34f, 0.44f, 0.00f, 700, 105, EmotionLightEffect::Breathing, "伤心侧低头"},
  {"sad", 138, 70, 0.42f, 0.54f, 0.00f, 780, 80, EmotionLightEffect::Breathing, "伤心叹气停住"},
  {"sad", 62, 104, 0.22f, 0.30f, 0.00f, 460, 95, EmotionLightEffect::Breathing, "伤心侧回弹"},
  {"sad", 0, 118, 0.00f, 0.16f, 0.00f, 520, 90, EmotionLightEffect::Breathing, "伤心回中慢停"},
};

const EmotionStep EMOTION_ANGRY_SHAKE[] = {
  {"angry", 0, 150, 0.00f, 0.00f, 0.32f, 140, 420, EmotionLightEffect::RedPulse, "生气蓄力"},
  {"angry", -255, 145, -0.48f, 0.02f, 0.50f, 170, 560, EmotionLightEffect::RedPulse, "生气摆头左"},
  {"angry", 255, 145, 0.48f, 0.02f, 0.50f, 170, 560, EmotionLightEffect::RedPulse, "生气摆头右"},
  {"angry", -190, 150, -0.36f, 0.03f, 0.44f, 150, 540, EmotionLightEffect::RedPulse, "生气摆头左回"},
  {"angry", 0, 132, 0.00f, 0.00f, 0.18f, 520, 290, EmotionLightEffect::RedPulse, "生气盯住"},
};

const EmotionStep EMOTION_ANGRY_DOUBLE_CHECK[] = {
  {"angry", 0, 150, 0.00f, 0.00f, 0.30f, 160, 360, EmotionLightEffect::RedPulse, "生气压住火"},
  {"angry", -178, 126, -0.40f, 0.08f, 0.46f, 210, 460, EmotionLightEffect::RedPulse, "生气左瞪"},
  {"angry", 34, 136, 0.12f, 0.02f, 0.30f, 130, 390, EmotionLightEffect::RedPulse, "生气回眼"},
  {"angry", 188, 126, 0.40f, 0.08f, 0.46f, 210, 460, EmotionLightEffect::RedPulse, "生气右瞪"},
  {"angry", 0, 120, 0.00f, 0.12f, 0.20f, 560, 220, EmotionLightEffect::RedPulse, "生气压低盯住"},
};

const EmotionStep EMOTION_ANGRY_STOMP_STARE[] = {
  {"angry", 0, 178, 0.00f, -0.12f, 0.40f, 180, 420, EmotionLightEffect::RedPulse, "生气抬头憋气"},
  {"angry", 0, 78, 0.00f, 0.30f, 0.56f, 190, 520, EmotionLightEffect::RedPulse, "生气猛低头"},
  {"angry", -88, 112, -0.22f, 0.14f, 0.46f, 160, 440, EmotionLightEffect::RedPulse, "生气小甩左"},
  {"angry", 88, 112, 0.22f, 0.14f, 0.46f, 160, 440, EmotionLightEffect::RedPulse, "生气小甩右"},
  {"angry", 0, 116, 0.00f, 0.08f, 0.22f, 480, 260, EmotionLightEffect::RedPulse, "生气定住"},
};

const EmotionStep EMOTION_SURPRISED_RECOIL[] = {
  {"surprised", 0, 132, 0.00f, -0.34f, 0.72f, 120, 420, EmotionLightEffect::SurpriseFlash, "惊讶睁眼"},
  {"surprised", 48, 385, 0.10f, -0.56f, 0.82f, 430, 430, EmotionLightEffect::SurpriseFlash, "惊讶后仰"},
  {"surprised", 16, 260, 0.03f, -0.34f, 0.38f, 360, 240, EmotionLightEffect::SurpriseFlash, "惊讶回落"},
  {"surprised", 0, 130, 0.00f, -0.08f, 0.10f, 520, 180, EmotionLightEffect::SurpriseFlash, "惊讶停顿"},
};

const EmotionStep EMOTION_SURPRISED_QUICK_POP[] = {
  {"surprised", 0, 122, 0.00f, -0.26f, 0.62f, 90, 430, EmotionLightEffect::SurpriseFlash, "惊讶瞬间睁眼"},
  {"surprised", -28, 316, -0.08f, -0.50f, 0.76f, 210, 460, EmotionLightEffect::SurpriseFlash, "惊讶快速后弹"},
  {"surprised", 34, 238, 0.10f, -0.30f, 0.42f, 240, 280, EmotionLightEffect::SurpriseFlash, "惊讶回看"},
  {"surprised", 0, 132, 0.00f, -0.12f, 0.14f, 420, 180, EmotionLightEffect::SurpriseFlash, "惊讶缓住"},
};

const EmotionStep EMOTION_SURPRISED_SIDE_LOOK[] = {
  {"surprised", 0, 132, -0.20f, -0.30f, 0.58f, 120, 380, EmotionLightEffect::SurpriseFlash, "惊讶先看左"},
  {"surprised", -116, 290, -0.42f, -0.48f, 0.76f, 330, 390, EmotionLightEffect::SurpriseFlash, "惊讶左后仰"},
  {"surprised", 92, 260, 0.34f, -0.36f, 0.58f, 260, 340, EmotionLightEffect::SurpriseFlash, "惊讶转向右"},
  {"surprised", 0, 150, 0.00f, -0.10f, 0.16f, 500, 190, EmotionLightEffect::SurpriseFlash, "惊讶回中确认"},
};

const EmotionStep EMOTION_SLEEP_WAKE[] = {
  {"sleeping", 0, 54, 0.00f, 0.42f, 0.00f, 260, 90, EmotionLightEffect::Breathing, "睡眠醒来-还没反应"},
  {"tired", -18, 106, -0.06f, 0.18f, 0.03f, 260, 110, EmotionLightEffect::Breathing, "睡眠醒来-慢慢抬头"},
  {"surprised", 0, 186, 0.00f, -0.34f, 0.48f, 180, 360, EmotionLightEffect::SurpriseFlash, "睡眠醒来-发现靠近"},
  {"happy", 0, 132, 0.00f, -0.08f, 0.20f, 320, 220, EmotionLightEffect::WarmBounce, "睡眠醒来-开心确认"},
};

const EmotionStep EMOTION_SLEEP_STARTLE[] = {
  {"sleeping", 0, PITCH_MIN + 8, 0.00f, 0.58f, 0.00f, 90, 160, EmotionLightEffect::Breathing, "惊醒睡梦低头"},
  {"surprised", 0, DEFAULT_REST_PITCH + 280, 0.00f, -0.58f, 0.82f, 130, 720, EmotionLightEffect::SurpriseFlash, "惊醒快速抬头"},
  {"thinking", 0, DEFAULT_REST_PITCH + 210, 0.00f, -0.22f, 0.20f, 520, 260, EmotionLightEffect::ThinkingTravel, "惊醒懵一下停顿"},
  {"thinking", 18, DEFAULT_REST_PITCH + 188, 0.06f, -0.12f, 0.12f, 460, 220, EmotionLightEffect::ThinkingTravel, "惊醒懵逼想一下"},
  {"surprised", -92, DEFAULT_REST_PITCH + 158, -0.26f, -0.08f, 0.38f, 180, 380, EmotionLightEffect::SurpriseFlash, "惊醒小幅左右确认-左"},
  {"surprised", 92, DEFAULT_REST_PITCH + 158, 0.26f, -0.08f, 0.38f, 180, 380, EmotionLightEffect::SurpriseFlash, "惊醒小幅左右确认-右"},
  {"thinking", 0, DEFAULT_REST_PITCH + 144, 0.00f, 0.00f, 0.12f, 540, 220, EmotionLightEffect::ThinkingTravel, "惊醒回中停顿"},
  {"surprised", -62, DEFAULT_REST_PITCH + 150, -0.18f, -0.04f, 0.30f, 170, 350, EmotionLightEffect::SurpriseFlash, "惊醒再次小幅左右-左"},
  {"surprised", 62, DEFAULT_REST_PITCH + 150, 0.18f, -0.04f, 0.30f, 170, 350, EmotionLightEffect::SurpriseFlash, "惊醒再次小幅左右-右"},
  {"tired", 0, DEFAULT_REST_PITCH, 0.00f, 0.04f, 0.10f, 420, 220, EmotionLightEffect::Breathing, "惊醒回中收住"},
};

const EmotionStep EMOTION_IDLE_THOUGHT[] = {
  {"thinking", 0, 118, -0.10f, 0.20f, 0.03f, 420, 48, EmotionLightEffect::ThinkingTravel, "待机思绪-眼神下沉"},
  {"thinking", -24, 86, -0.16f, 0.40f, 0.02f, 940, 42, EmotionLightEffect::ThinkingTravel, "待机思绪-低头停顿"},
  {"thinking", -8, 104, -0.05f, 0.30f, 0.03f, 760, 38, EmotionLightEffect::ThinkingTravel, "待机思绪-小声嘀咕"},
  {"thinking", 0, 120, 0.00f, 0.00f, 0.05f, 680, 42, EmotionLightEffect::ThinkingTravel, "待机思绪-慢慢回中"},
};

const EmotionStep EMOTION_THINKING_DIP[] = {
  {"thinking", 0, 118, -0.16f, 0.34f, 0.00f, 260, 130, EmotionLightEffect::ThinkingTravel, "思考眼神下探"},
  {"thinking", -72, 68, -0.26f, 0.46f, 0.00f, 900, 82, EmotionLightEffect::ThinkingTravel, "思考低头"},
  {"thinking", -36, 94, -0.12f, 0.30f, 0.00f, 520, 70, EmotionLightEffect::ThinkingTravel, "思考小回弹"},
  {"thinking", 0, 116, 0.00f, 0.10f, 0.00f, 680, 85, EmotionLightEffect::ThinkingTravel, "思考回中"},
};

const EmotionStep EMOTION_THINKING_SIDE_TILT[] = {
  {"thinking", 0, 126, 0.16f, 0.16f, 0.00f, 280, 120, EmotionLightEffect::ThinkingTravel, "思考斜眼准备"},
  {"thinking", 92, 116, 0.36f, 0.18f, 0.00f, 520, 100, EmotionLightEffect::ThinkingTravel, "思考右歪"},
  {"thinking", 68, 82, 0.28f, 0.36f, 0.00f, 640, 82, EmotionLightEffect::ThinkingTravel, "思考低一点"},
  {"thinking", -24, 112, -0.06f, 0.14f, 0.00f, 460, 92, EmotionLightEffect::ThinkingTravel, "思考换方向"},
  {"thinking", 0, 120, 0.00f, 0.04f, 0.00f, 520, 88, EmotionLightEffect::ThinkingTravel, "思考回中"},
};

const EmotionStep EMOTION_THINKING_SCAN[] = {
  {"thinking", -78, 132, -0.34f, 0.12f, 0.00f, 360, 120, EmotionLightEffect::ThinkingTravel, "思考扫左"},
  {"thinking", 0, 92, 0.00f, 0.32f, 0.00f, 620, 84, EmotionLightEffect::ThinkingTravel, "思考看下"},
  {"thinking", 82, 132, 0.34f, 0.10f, 0.00f, 360, 120, EmotionLightEffect::ThinkingTravel, "思考扫右"},
  {"thinking", 32, 104, 0.12f, 0.22f, 0.00f, 520, 88, EmotionLightEffect::ThinkingTravel, "思考停一下"},
  {"thinking", 0, 118, 0.00f, 0.08f, 0.00f, 420, 92, EmotionLightEffect::ThinkingTravel, "思考收住"},
};

const EmotionStep EMOTION_CURIOUS_LOOK_AROUND[] = {
  {"curious", 0, 146, 0.00f, -0.16f, 0.16f, 140, 360, EmotionLightEffect::ThinkingTravel, "好奇先抬眼"},
  {"curious", -282, 238, -0.72f, -0.46f, 0.22f, 280, 560, EmotionLightEffect::ThinkingTravel, "好奇看左上"},
  {"curious", 286, 78, 0.72f, 0.52f, 0.22f, 320, 560, EmotionLightEffect::ThinkingTravel, "好奇扫到右下"},
  {"curious", -236, 72, -0.62f, 0.50f, 0.18f, 270, 520, EmotionLightEffect::ThinkingTravel, "好奇低头看左"},
  {"curious", 238, 272, 0.62f, -0.54f, 0.20f, 300, 540, EmotionLightEffect::ThinkingTravel, "好奇抬头看右"},
  {"curious", 0, 132, 0.00f, -0.04f, 0.08f, 420, 260, EmotionLightEffect::ThinkingTravel, "好奇回中确认"},
};

const EmotionStep EMOTION_SHY_HIDE[] = {
  {"love", -42, 138, -0.22f, 0.28f, 0.16f, 220, 180, EmotionLightEffect::SoftPinkPulse, "害羞眼神躲开"},
  {"happy", -112, 84, -0.42f, 0.46f, 0.08f, 520, 110, EmotionLightEffect::SoftPinkPulse, "害羞侧低头"},
  {"love", -54, 118, -0.20f, 0.26f, 0.20f, 360, 150, EmotionLightEffect::SoftPinkPulse, "偷偷看回来"},
  {"happy", 0, 130, 0.00f, 0.10f, 0.12f, 360, 160, EmotionLightEffect::SoftPinkPulse, "害羞回中"},
};

const EmotionStep EMOTION_SHY_PEEK_RIGHT[] = {
  {"love", 0, 126, 0.18f, 0.20f, 0.12f, 220, 160, EmotionLightEffect::SoftPinkPulse, "害羞偷看准备"},
  {"happy", 92, 92, 0.34f, 0.40f, 0.08f, 480, 105, EmotionLightEffect::SoftPinkPulse, "害羞右下躲"},
  {"love", 52, 116, 0.20f, 0.22f, 0.18f, 300, 140, EmotionLightEffect::SoftPinkPulse, "害羞露一点"},
  {"happy", -18, 130, -0.04f, 0.08f, 0.10f, 360, 150, EmotionLightEffect::SoftPinkPulse, "害羞收回来"},
};

const EmotionStep EMOTION_SHY_TINY_HIDE[] = {
  {"love", -22, 132, -0.12f, 0.18f, 0.14f, 180, 170, EmotionLightEffect::SoftPinkPulse, "害羞小躲"},
  {"happy", -64, 106, -0.26f, 0.32f, 0.08f, 360, 120, EmotionLightEffect::SoftPinkPulse, "害羞低头一点"},
  {"love", -38, 122, -0.14f, 0.18f, 0.18f, 260, 145, EmotionLightEffect::SoftPinkPulse, "害羞悄悄看"},
  {"happy", 24, 126, 0.08f, 0.10f, 0.12f, 280, 150, EmotionLightEffect::SoftPinkPulse, "害羞轻轻摆回"},
  {"love", 0, 124, 0.00f, 0.06f, 0.10f, 360, 130, EmotionLightEffect::SoftPinkPulse, "害羞停住"},
};

const EmotionStep EMOTION_CUTE_ATTENTION[] = {
  {"love", 0, 142, 0.00f, -0.20f, 0.24f, 180, 260, EmotionLightEffect::WarmBounce, "撒娇抬眼"},
  {"happy", -118, 130, -0.28f, -0.12f, 0.38f, 180, 360, EmotionLightEffect::WarmBounce, "撒娇左摆"},
  {"happy", 118, 130, 0.28f, -0.12f, 0.38f, 180, 360, EmotionLightEffect::WarmBounce, "撒娇右摆"},
  {"love", -54, 122, -0.12f, -0.24f, 0.30f, 170, 340, EmotionLightEffect::WarmBounce, "再靠近一点"},
  {"happy", 0, 128, 0.00f, -0.10f, 0.20f, 360, 220, EmotionLightEffect::WarmBounce, "撒娇停住"},
};

const EmotionStep EMOTION_CUTE_BOUNCE[] = {
  {"love", 0, 150, 0.00f, -0.22f, 0.28f, 150, 300, EmotionLightEffect::WarmBounce, "撒娇蹦一下"},
  {"happy", -54, 176, -0.16f, -0.30f, 0.42f, 170, 340, EmotionLightEffect::WarmBounce, "撒娇左上"},
  {"happy", 54, 176, 0.16f, -0.30f, 0.42f, 170, 340, EmotionLightEffect::WarmBounce, "撒娇右上"},
  {"love", 0, 110, 0.00f, 0.16f, 0.30f, 170, 320, EmotionLightEffect::WarmBounce, "撒娇压低"},
  {"happy", 0, 128, 0.00f, -0.08f, 0.18f, 330, 210, EmotionLightEffect::WarmBounce, "撒娇收尾"},
};

const EmotionStep EMOTION_CUTE_SIDE_WIGGLE[] = {
  {"love", -82, 126, -0.22f, -0.08f, 0.28f, 200, 300, EmotionLightEffect::WarmBounce, "撒娇靠左"},
  {"happy", 82, 126, 0.22f, -0.08f, 0.36f, 200, 300, EmotionLightEffect::WarmBounce, "撒娇靠右"},
  {"love", -96, 118, -0.28f, 0.02f, 0.30f, 180, 310, EmotionLightEffect::WarmBounce, "撒娇再左"},
  {"happy", 96, 118, 0.28f, 0.02f, 0.30f, 180, 310, EmotionLightEffect::WarmBounce, "撒娇再右"},
  {"love", 0, 122, 0.00f, -0.10f, 0.18f, 420, 190, EmotionLightEffect::WarmBounce, "撒娇看着你"},
};

const EmotionStep EMOTION_PROUD_SWAY[] = {
  {"happy", 0, 278, 0.00f, -0.34f, 0.30f, 260, 210, EmotionLightEffect::GoldPulse, "得意抬头"},
  {"happy", -78, 300, -0.18f, -0.38f, 0.42f, 260, 240, EmotionLightEffect::GoldPulse, "得意左晃"},
  {"happy", 78, 300, 0.18f, -0.38f, 0.42f, 260, 240, EmotionLightEffect::GoldPulse, "得意右晃"},
  {"happy", 0, 230, 0.00f, -0.18f, 0.24f, 500, 160, EmotionLightEffect::GoldPulse, "骄傲定格"},
};

const EmotionStep EMOTION_PROUD_CHIN_UP[] = {
  {"happy", 0, 190, 0.00f, -0.18f, 0.18f, 260, 180, EmotionLightEffect::GoldPulse, "得意先端住"},
  {"happy", 0, 330, 0.00f, -0.46f, 0.34f, 480, 170, EmotionLightEffect::GoldPulse, "得意高抬头"},
  {"happy", 34, 318, 0.08f, -0.42f, 0.34f, 260, 180, EmotionLightEffect::GoldPulse, "得意微偏右"},
  {"happy", -34, 318, -0.08f, -0.42f, 0.34f, 260, 180, EmotionLightEffect::GoldPulse, "得意微偏左"},
  {"happy", 0, 238, 0.00f, -0.18f, 0.20f, 520, 150, EmotionLightEffect::GoldPulse, "得意慢慢落下"},
};

const EmotionStep EMOTION_PROUD_SLOW_SWAY[] = {
  {"happy", -64, 260, -0.14f, -0.28f, 0.28f, 360, 180, EmotionLightEffect::GoldPulse, "得意慢晃左"},
  {"happy", 64, 280, 0.14f, -0.32f, 0.36f, 420, 180, EmotionLightEffect::GoldPulse, "得意慢晃右"},
  {"happy", -36, 300, -0.08f, -0.36f, 0.30f, 360, 170, EmotionLightEffect::GoldPulse, "得意回左"},
  {"happy", 0, 250, 0.00f, -0.20f, 0.20f, 560, 145, EmotionLightEffect::GoldPulse, "得意收住"},
};

const EmotionStep EMOTION_CONFUSED_TILT[] = {
  {"thinking", -96, 116, -0.38f, 0.04f, 0.06f, 320, 130, EmotionLightEffect::BluePurpleBlink, "困惑左歪头"},
  {"thinking", 96, 112, 0.38f, 0.06f, 0.06f, 360, 125, EmotionLightEffect::BluePurpleBlink, "困惑右歪头"},
  {"thinking", -42, 92, -0.18f, 0.24f, 0.04f, 520, 92, EmotionLightEffect::BluePurpleBlink, "低头想一下"},
  {"thinking", 0, 118, 0.00f, 0.08f, 0.02f, 460, 110, EmotionLightEffect::BluePurpleBlink, "困惑回中"},
};

const EmotionStep EMOTION_CONFUSED_DOUBLE_TILT[] = {
  {"thinking", -72, 138, -0.30f, -0.02f, 0.04f, 300, 130, EmotionLightEffect::BluePurpleBlink, "困惑先左"},
  {"thinking", -118, 94, -0.44f, 0.20f, 0.05f, 420, 100, EmotionLightEffect::BluePurpleBlink, "困惑左低"},
  {"thinking", 118, 94, 0.44f, 0.20f, 0.05f, 520, 100, EmotionLightEffect::BluePurpleBlink, "困惑换右低"},
  {"thinking", 36, 120, 0.14f, 0.08f, 0.03f, 360, 110, EmotionLightEffect::BluePurpleBlink, "困惑回一点"},
  {"thinking", 0, 118, 0.00f, 0.08f, 0.02f, 420, 110, EmotionLightEffect::BluePurpleBlink, "困惑回中"},
};

const EmotionStep EMOTION_CONFUSED_SMALL_CIRCLE[] = {
  {"thinking", -54, 112, -0.22f, 0.12f, 0.04f, 260, 120, EmotionLightEffect::BluePurpleBlink, "困惑绕左下"},
  {"thinking", 0, 82, 0.00f, 0.32f, 0.04f, 320, 100, EmotionLightEffect::BluePurpleBlink, "困惑绕下方"},
  {"thinking", 54, 112, 0.22f, 0.12f, 0.04f, 260, 120, EmotionLightEffect::BluePurpleBlink, "困惑绕右下"},
  {"thinking", 36, 148, 0.14f, -0.04f, 0.04f, 320, 110, EmotionLightEffect::BluePurpleBlink, "困惑抬眼"},
  {"thinking", 0, 118, 0.00f, 0.08f, 0.02f, 420, 105, EmotionLightEffect::BluePurpleBlink, "困惑停住"},
};

const EmotionStep EMOTION_SCARED_RECOIL[] = {
  {"surprised", 0, 138, 0.00f, -0.44f, 0.70f, 120, 420, EmotionLightEffect::ColdFlash, "紧张睁眼"},
  {"surprised", 42, 390, 0.20f, -0.62f, 0.78f, 240, 440, EmotionLightEffect::ColdFlash, "紧张后缩"},
  {"dizzy", -74, 340, -0.34f, -0.48f, 0.58f, 110, 520, EmotionLightEffect::ColdFlash, "紧张小抖左"},
  {"dizzy", 74, 330, 0.34f, -0.48f, 0.58f, 110, 520, EmotionLightEffect::ColdFlash, "紧张小抖右"},
  {"surprised", 0, 190, 0.00f, -0.22f, 0.28f, 420, 190, EmotionLightEffect::ColdFlash, "慢慢缓住"},
};

const EmotionStep EMOTION_SCARED_FREEZE[] = {
  {"surprised", 0, 132, 0.00f, -0.40f, 0.68f, 100, 430, EmotionLightEffect::ColdFlash, "紧张僵住"},
  {"surprised", -26, 360, -0.12f, -0.58f, 0.70f, 360, 260, EmotionLightEffect::ColdFlash, "紧张慢后仰"},
  {"dizzy", -34, 330, -0.18f, -0.50f, 0.42f, 180, 420, EmotionLightEffect::ColdFlash, "紧张抖一下"},
  {"surprised", 0, 210, 0.00f, -0.24f, 0.24f, 520, 170, EmotionLightEffect::ColdFlash, "紧张缓慢回来"},
};

const EmotionStep EMOTION_SCARED_DUCK[] = {
  {"surprised", 0, 140, 0.00f, -0.34f, 0.64f, 110, 430, EmotionLightEffect::ColdFlash, "紧张发现"},
  {"surprised", -86, 92, -0.34f, 0.30f, 0.58f, 220, 460, EmotionLightEffect::ColdFlash, "紧张躲左下"},
  {"dizzy", 72, 96, 0.30f, 0.28f, 0.52f, 180, 430, EmotionLightEffect::ColdFlash, "紧张换方向"},
  {"surprised", 20, 182, 0.08f, -0.16f, 0.28f, 380, 200, EmotionLightEffect::ColdFlash, "紧张抬起一点"},
  {"surprised", 0, 126, 0.00f, 0.00f, 0.14f, 460, 160, EmotionLightEffect::ColdFlash, "紧张观察"},
};

const EmotionStep EMOTION_CELEBRATE[] = {
  {"happy", 0, 170, 0.00f, -0.20f, 0.55f, 120, 470, EmotionLightEffect::CelebrationRainbow, "庆祝准备"},
  {"happy", -155, 305, -0.36f, -0.38f, 0.86f, 135, 560, EmotionLightEffect::CelebrationRainbow, "庆祝左上"},
  {"happy", 155, 88, 0.36f, 0.34f, 0.86f, 135, 560, EmotionLightEffect::CelebrationRainbow, "庆祝右下"},
  {"happy", -120, 90, -0.30f, 0.30f, 0.75f, 130, 560, EmotionLightEffect::CelebrationRainbow, "庆祝再左"},
  {"happy", 120, 310, 0.30f, -0.36f, 0.75f, 130, 560, EmotionLightEffect::CelebrationRainbow, "庆祝再右"},
  {"happy", 0, 130, 0.00f, -0.04f, 0.30f, 420, 260, EmotionLightEffect::CelebrationRainbow, "庆祝收尾"},
};

const EmotionStep EMOTION_CELEBRATE_BIG_ARC[] = {
  {"happy", 0, 150, 0.00f, -0.16f, 0.50f, 120, 420, EmotionLightEffect::CelebrationRainbow, "庆祝大弧线准备"},
  {"happy", -188, 330, -0.42f, -0.44f, 0.82f, 260, 520, EmotionLightEffect::CelebrationRainbow, "庆祝大弧线左上"},
  {"happy", 0, 88, 0.00f, 0.34f, 0.78f, 220, 520, EmotionLightEffect::CelebrationRainbow, "庆祝大弧线下落"},
  {"happy", 188, 330, 0.42f, -0.44f, 0.82f, 260, 520, EmotionLightEffect::CelebrationRainbow, "庆祝大弧线右上"},
  {"happy", 0, 132, 0.00f, -0.04f, 0.30f, 420, 240, EmotionLightEffect::CelebrationRainbow, "庆祝大弧线收尾"},
};

const EmotionStep EMOTION_CELEBRATE_FAST_SHAKE[] = {
  {"happy", 0, 160, 0.00f, -0.18f, 0.55f, 100, 480, EmotionLightEffect::CelebrationRainbow, "庆祝快摇准备"},
  {"happy", -128, 210, -0.28f, -0.26f, 0.78f, 130, 560, EmotionLightEffect::CelebrationRainbow, "庆祝快摇左"},
  {"happy", 128, 210, 0.28f, -0.26f, 0.78f, 130, 560, EmotionLightEffect::CelebrationRainbow, "庆祝快摇右"},
  {"happy", -118, 108, -0.26f, 0.24f, 0.72f, 120, 560, EmotionLightEffect::CelebrationRainbow, "庆祝快摇左下"},
  {"happy", 118, 108, 0.26f, 0.24f, 0.72f, 120, 560, EmotionLightEffect::CelebrationRainbow, "庆祝快摇右下"},
  {"happy", 0, 130, 0.00f, -0.04f, 0.32f, 360, 260, EmotionLightEffect::CelebrationRainbow, "庆祝快摇停住"},
};

#define STEP_COUNT(steps) (uint8_t)(sizeof(steps) / sizeof((steps)[0]))
#define VARIANT_COUNT(variants) (uint8_t)(sizeof(variants) / sizeof((variants)[0]))

const EmotionVariant EMOTION_HAPPY_VARIANTS[] = {
  {"nod", "疯狂点头", EMOTION_HAPPY_NOD, STEP_COUNT(EMOTION_HAPPY_NOD)},
  {"sway", "左右开心晃", EMOTION_HAPPY_SWAY, STEP_COUNT(EMOTION_HAPPY_SWAY)},
  {"bounce", "轻快雀跃", EMOTION_HAPPY_BOUNCE, STEP_COUNT(EMOTION_HAPPY_BOUNCE)},
};

const EmotionVariant EMOTION_HEAD_TOUCH_COMBO_VARIANTS[] = {
  {"nod", "五连摸疯狂点头", EMOTION_HEAD_TOUCH_COMBO_NOD, STEP_COUNT(EMOTION_HEAD_TOUCH_COMBO_NOD)},
};

const EmotionVariant EMOTION_COMFORTABLE_NOD_VARIANTS[] = {
  {"gentle", "小猫舒服点头", EMOTION_COMFORTABLE_NOD, STEP_COUNT(EMOTION_COMFORTABLE_NOD)},
};

const EmotionVariant ROBOT_ACTION_NOD_YES_VARIANTS[] = {
  {"nod_yes", "是 / 点头确认", ROBOT_ACTION_NOD_YES, STEP_COUNT(ROBOT_ACTION_NOD_YES)},
};

const EmotionVariant ROBOT_ACTION_SHAKE_NO_VARIANTS[] = {
  {"shake_no", "否 / 摇头拒绝", ROBOT_ACTION_SHAKE_NO, STEP_COUNT(ROBOT_ACTION_SHAKE_NO)},
};

const EmotionVariant EMOTION_SAD_VARIANTS[] = {
  {"diagonal", "斜下低头", EMOTION_SAD_DIAGONAL_DOWN, STEP_COUNT(EMOTION_SAD_DIAGONAL_DOWN)},
  {"slowSink", "缓慢下沉", EMOTION_SAD_SLOW_SINK, STEP_COUNT(EMOTION_SAD_SLOW_SINK)},
  {"sideSigh", "侧身叹气", EMOTION_SAD_SIDE_SIGH, STEP_COUNT(EMOTION_SAD_SIDE_SIGH)},
};

const EmotionVariant EMOTION_ANGRY_VARIANTS[] = {
  {"shake", "左右摆头", EMOTION_ANGRY_SHAKE, STEP_COUNT(EMOTION_ANGRY_SHAKE)},
  {"doubleCheck", "左右瞪视", EMOTION_ANGRY_DOUBLE_CHECK, STEP_COUNT(EMOTION_ANGRY_DOUBLE_CHECK)},
  {"stompStare", "猛低头盯住", EMOTION_ANGRY_STOMP_STARE, STEP_COUNT(EMOTION_ANGRY_STOMP_STARE)},
};

const EmotionVariant EMOTION_SURPRISED_VARIANTS[] = {
  {"recoil", "后仰惊讶", EMOTION_SURPRISED_RECOIL, STEP_COUNT(EMOTION_SURPRISED_RECOIL)},
  {"quickPop", "快速弹开", EMOTION_SURPRISED_QUICK_POP, STEP_COUNT(EMOTION_SURPRISED_QUICK_POP)},
  {"sideLook", "侧向确认", EMOTION_SURPRISED_SIDE_LOOK, STEP_COUNT(EMOTION_SURPRISED_SIDE_LOOK)},
};

const EmotionVariant EMOTION_SLEEP_WAKE_VARIANTS[] = {
  {"wake", "靠近唤醒", EMOTION_SLEEP_WAKE, STEP_COUNT(EMOTION_SLEEP_WAKE)},
};

const EmotionVariant EMOTION_SLEEP_STARTLE_VARIANTS[] = {
  {"startle", "触摸惊醒", EMOTION_SLEEP_STARTLE, STEP_COUNT(EMOTION_SLEEP_STARTLE)},
};

const EmotionVariant EMOTION_IDLE_THOUGHT_VARIANTS[] = {
  {"mumble", "低头嘀咕", EMOTION_IDLE_THOUGHT, STEP_COUNT(EMOTION_IDLE_THOUGHT)},
};

const EmotionVariant EMOTION_THINKING_VARIANTS[] = {
  {"dip", "低头思考", EMOTION_THINKING_DIP, STEP_COUNT(EMOTION_THINKING_DIP)},
  {"sideTilt", "侧歪思考", EMOTION_THINKING_SIDE_TILT, STEP_COUNT(EMOTION_THINKING_SIDE_TILT)},
  {"scan", "左右搜索", EMOTION_THINKING_SCAN, STEP_COUNT(EMOTION_THINKING_SCAN)},
};

const EmotionVariant EMOTION_CURIOUS_VARIANTS[] = {
  {"lookAround", "左右上下观察", EMOTION_CURIOUS_LOOK_AROUND, STEP_COUNT(EMOTION_CURIOUS_LOOK_AROUND)},
};

const EmotionVariant EMOTION_SHY_VARIANTS[] = {
  {"hide", "侧低头躲闪", EMOTION_SHY_HIDE, STEP_COUNT(EMOTION_SHY_HIDE)},
  {"peekRight", "右侧偷看", EMOTION_SHY_PEEK_RIGHT, STEP_COUNT(EMOTION_SHY_PEEK_RIGHT)},
  {"tinyHide", "小幅害羞", EMOTION_SHY_TINY_HIDE, STEP_COUNT(EMOTION_SHY_TINY_HIDE)},
};

const EmotionVariant EMOTION_CUTE_VARIANTS[] = {
  {"attention", "撒娇求关注", EMOTION_CUTE_ATTENTION, STEP_COUNT(EMOTION_CUTE_ATTENTION)},
  {"bounce", "软软蹦一下", EMOTION_CUTE_BOUNCE, STEP_COUNT(EMOTION_CUTE_BOUNCE)},
  {"wiggle", "左右撒娇晃", EMOTION_CUTE_SIDE_WIGGLE, STEP_COUNT(EMOTION_CUTE_SIDE_WIGGLE)},
};

const EmotionVariant EMOTION_PROUD_VARIANTS[] = {
  {"sway", "得意晃头", EMOTION_PROUD_SWAY, STEP_COUNT(EMOTION_PROUD_SWAY)},
  {"chinUp", "高抬头定格", EMOTION_PROUD_CHIN_UP, STEP_COUNT(EMOTION_PROUD_CHIN_UP)},
  {"slowSway", "慢慢得意晃", EMOTION_PROUD_SLOW_SWAY, STEP_COUNT(EMOTION_PROUD_SLOW_SWAY)},
};

const EmotionVariant EMOTION_CONFUSED_VARIANTS[] = {
  {"tilt", "左右歪头", EMOTION_CONFUSED_TILT, STEP_COUNT(EMOTION_CONFUSED_TILT)},
  {"doubleTilt", "两边确认", EMOTION_CONFUSED_DOUBLE_TILT, STEP_COUNT(EMOTION_CONFUSED_DOUBLE_TILT)},
  {"smallCircle", "小圈寻找", EMOTION_CONFUSED_SMALL_CIRCLE, STEP_COUNT(EMOTION_CONFUSED_SMALL_CIRCLE)},
};

const EmotionVariant EMOTION_SCARED_VARIANTS[] = {
  {"recoil", "紧张后缩", EMOTION_SCARED_RECOIL, STEP_COUNT(EMOTION_SCARED_RECOIL)},
  {"freeze", "僵住后退", EMOTION_SCARED_FREEZE, STEP_COUNT(EMOTION_SCARED_FREEZE)},
  {"duck", "躲到一边", EMOTION_SCARED_DUCK, STEP_COUNT(EMOTION_SCARED_DUCK)},
};

const EmotionVariant EMOTION_CELEBRATE_VARIANTS[] = {
  {"cheer", "庆祝欢呼", EMOTION_CELEBRATE, STEP_COUNT(EMOTION_CELEBRATE)},
  {"bigArc", "大弧线庆祝", EMOTION_CELEBRATE_BIG_ARC, STEP_COUNT(EMOTION_CELEBRATE_BIG_ARC)},
  {"fastShake", "快摇庆祝", EMOTION_CELEBRATE_FAST_SHAKE, STEP_COUNT(EMOTION_CELEBRATE_FAST_SHAKE)},
};

const EmotionAction EMOTION_ACTIONS[] = {
  {"comfortableNod", "舒服点点头", EMOTION_COMFORTABLE_NOD_VARIANTS, VARIANT_COUNT(EMOTION_COMFORTABLE_NOD_VARIANTS)},
  {"nod_yes", "是 / 点头确认", ROBOT_ACTION_NOD_YES_VARIANTS, VARIANT_COUNT(ROBOT_ACTION_NOD_YES_VARIANTS)},
  {"shake_no", "否 / 摇头拒绝", ROBOT_ACTION_SHAKE_NO_VARIANTS, VARIANT_COUNT(ROBOT_ACTION_SHAKE_NO_VARIANTS)},
  {"happy", "开心点头", EMOTION_HAPPY_VARIANTS, VARIANT_COUNT(EMOTION_HAPPY_VARIANTS)},
  {"headTouchComboSpin", "摸头疯狂点头", EMOTION_HEAD_TOUCH_COMBO_VARIANTS, VARIANT_COUNT(EMOTION_HEAD_TOUCH_COMBO_VARIANTS)},
  {"spin", "点头测试", EMOTION_HEAD_TOUCH_COMBO_VARIANTS, VARIANT_COUNT(EMOTION_HEAD_TOUCH_COMBO_VARIANTS)},
  {"sad", "伤心低头", EMOTION_SAD_VARIANTS, VARIANT_COUNT(EMOTION_SAD_VARIANTS)},
  {"angry", "生气摆头", EMOTION_ANGRY_VARIANTS, VARIANT_COUNT(EMOTION_ANGRY_VARIANTS)},
  {"surprised", "惊讶后仰", EMOTION_SURPRISED_VARIANTS, VARIANT_COUNT(EMOTION_SURPRISED_VARIANTS)},
  {"sleepWake", "睡眠醒来", EMOTION_SLEEP_WAKE_VARIANTS, VARIANT_COUNT(EMOTION_SLEEP_WAKE_VARIANTS)},
  {"sleepStartle", "被惊醒", EMOTION_SLEEP_STARTLE_VARIANTS, VARIANT_COUNT(EMOTION_SLEEP_STARTLE_VARIANTS)},
  {"idleThought", "待机思绪", EMOTION_IDLE_THOUGHT_VARIANTS, VARIANT_COUNT(EMOTION_IDLE_THOUGHT_VARIANTS)},
  {"thinking", "思考低头", EMOTION_THINKING_VARIANTS, VARIANT_COUNT(EMOTION_THINKING_VARIANTS)},
  {"curious", "好奇观察", EMOTION_CURIOUS_VARIANTS, VARIANT_COUNT(EMOTION_CURIOUS_VARIANTS)},
  {"shy", "害羞躲闪", EMOTION_SHY_VARIANTS, VARIANT_COUNT(EMOTION_SHY_VARIANTS)},
  {"cute", "撒娇求关注", EMOTION_CUTE_VARIANTS, VARIANT_COUNT(EMOTION_CUTE_VARIANTS)},
  {"proud", "得意抬头", EMOTION_PROUD_VARIANTS, VARIANT_COUNT(EMOTION_PROUD_VARIANTS)},
  {"confused", "困惑歪头", EMOTION_CONFUSED_VARIANTS, VARIANT_COUNT(EMOTION_CONFUSED_VARIANTS)},
  {"scared", "紧张后缩", EMOTION_SCARED_VARIANTS, VARIANT_COUNT(EMOTION_SCARED_VARIANTS)},
  {"celebrate", "庆祝欢呼", EMOTION_CELEBRATE_VARIANTS, VARIANT_COUNT(EMOTION_CELEBRATE_VARIANTS)},
};

constexpr uint8_t EMOTION_ACTION_COUNT = sizeof(EMOTION_ACTIONS) / sizeof(EMOTION_ACTIONS[0]);

String emotionLibraryJson() {
  String json = "[";
  for (uint8_t i = 0; i < EMOTION_ACTION_COUNT; ++i) {
    if (i > 0) json += ",";
    json += "\"";
    json += EMOTION_ACTIONS[i].title;
    json += "\"";
  }
  json += "]";
  return json;
}

const EmotionAction* findEmotionAction(const String& name) {
  for (uint8_t i = 0; i < EMOTION_ACTION_COUNT; ++i) {
    if (name == EMOTION_ACTIONS[i].name) return &EMOTION_ACTIONS[i];
  }
  return nullptr;
}

uint8_t chooseVariantIndex(const EmotionAction& action, uint32_t nowMs, uint32_t playCount) {
  if (action.variantCount <= 1) return 0;
  uint32_t hash = nowMs ^ (playCount * 2654435761UL);
  const char* cursor = action.name;
  while (*cursor) {
    hash ^= (uint8_t)(*cursor);
    hash *= 16777619UL;
    cursor++;
  }
  hash ^= hash >> 16;
  return (uint8_t)(hash % action.variantCount);
}

RgbColor wheelColor(uint8_t position) {
  position = position % RGB_LED_COUNT;
  const RgbColor colors[RGB_LED_COUNT] = {
    {255, 0, 0},
    {255, 96, 0},
    {255, 220, 0},
    {96, 255, 0},
    {0, 255, 80},
    {0, 255, 220},
    {0, 180, 255},
    {0, 72, 255},
    {80, 0, 255},
    {180, 0, 255},
    {255, 0, 180},
    {255, 0, 72},
  };
  return colors[position];
}

RgbColor dimColor(const RgbColor& color, uint8_t brightness) {
  return {
    (uint8_t)((uint16_t)color.r * brightness / 255),
    (uint8_t)((uint16_t)color.g * brightness / 255),
    (uint8_t)((uint16_t)color.b * brightness / 255),
  };
}

}  // namespace

void EmotionActionController::begin() {
  active_ = false;
  steps_ = nullptr;
  stepCount_ = 0;
  stepIndex_ = 0;
  variantIndex_ = 0;
  playCount_ = 0;
  nextStepAtMs_ = 0;
  durationMs_ = 0;
  rotationStopped_ = false;
  lightsEnabled_ = true;
  lightSnapshotCaptured_ = false;
  rgbSnapshot_ = {};
  actionName_ = "none";
  actionTitle_ = "未播放";
  variantName_ = "none";
  variantTitle_ = "未选择";
  currentStepLabel_ = "待机";
}

bool EmotionActionController::play(const String& name, uint32_t nowMs, bool lightsEnabled) {
  const EmotionAction* action = findEmotionAction(name);
  if (action == nullptr) return false;
  if (action->variantCount == 0) return false;
  variantIndex_ = chooseVariantIndex(*action, nowMs, playCount_);
  const EmotionVariant& selectedVariant = action->variants[variantIndex_];
  steps_ = selectedVariant.steps;
  stepCount_ = selectedVariant.count;
  stepIndex_ = 0;
  nextStepAtMs_ = nowMs;
  durationMs_ = 500;
  for (uint8_t i = 0; i < stepCount_; ++i) {
    durationMs_ += steps_[i].durationMs;
  }
  active_ = true;
  rotationStopped_ = false;
  lightsEnabled_ = lightsEnabled;
  lightSnapshotCaptured_ = false;
  actionName_ = action->name;
  actionTitle_ = action->title;
  variantName_ = selectedVariant.name;
  variantTitle_ = selectedVariant.title;
  currentStepLabel_ = "准备播放";
  playCount_++;
  return true;
}

void EmotionActionController::stop(RobotHardware& hardware, uint32_t nowMs) {
  restoreEmotionLights(hardware);
  hardware.rotateX(0);
  active_ = false;
  steps_ = nullptr;
  stepCount_ = 0;
  stepIndex_ = 0;
  variantIndex_ = 0;
  nextStepAtMs_ = nowMs;
  durationMs_ = 0;
  rotationStopped_ = false;
  variantName_ = "none";
  variantTitle_ = "未选择";
  currentStepLabel_ = "已停止";
}

void EmotionActionController::update(RobotHardware& hardware, RobotFaceRenderer& face, IdleBehaviorController& idle, uint32_t nowMs) {
  if (!active_) return;
  idle.pauseForManualControl(nowMs, 1400);
  if (!hardware.isReady()) return;
  if (!rotationStopped_) {
    hardware.rotateX(0);
    rotationStopped_ = true;
  }
  if (nowMs < nextStepAtMs_) return;

  if (steps_ == nullptr || stepIndex_ >= stepCount_) {
    restoreEmotionLights(hardware);
    hardware.rotateX(0);
    active_ = false;
    idle.pauseForManualControl(nowMs, 1200);
    currentStepLabel_ = "完成";
    return;
  }

  const EmotionStep& step = steps_[stepIndex_];
  applyStep(hardware, face, idle, step, nowMs);
  nextStepAtMs_ = nowMs + step.durationMs;
  stepIndex_++;
  if (stepIndex_ >= stepCount_) {
    nextStepAtMs_ += 500;
  }
}

bool EmotionActionController::active() const {
  return active_;
}

bool EmotionActionController::isActiveAction(const char* name) const {
  if (name == nullptr) return false;
  return active_ && strcmp(actionName_, name) == 0;
}

bool EmotionActionController::lightsActive() const {
  return active_ && lightsEnabled_;
}

uint32_t EmotionActionController::durationMs() const {
  return durationMs_;
}

String EmotionActionController::statusJson() const {
  uint32_t nowMs = millis();
  uint32_t nextInMs = nowMs < nextStepAtMs_ ? nextStepAtMs_ - nowMs : 0;
  String json = "{\"ok\":true";
  json += ",\"active\":" + String(active_ ? "true" : "false");
  json += ",\"lightsEnabled\":" + String(lightsEnabled_ ? "true" : "false");
  json += ",\"lightsActive\":" + String(lightsActive() ? "true" : "false");
  json += ",\"action\":\"" + String(actionName_) + "\"";
  json += ",\"title\":\"" + String(actionTitle_) + "\"";
  json += ",\"variant\":\"" + String(variantName_) + "\"";
  json += ",\"variantTitle\":\"" + String(variantTitle_) + "\"";
  json += ",\"variantIndex\":" + String(variantIndex_);
  json += ",\"step\":\"" + String(currentStepLabel_) + "\"";
  json += ",\"stepIndex\":" + String(stepIndex_);
  json += ",\"stepCount\":" + String(stepCount_);
  json += ",\"nextInMs\":" + String(nextInMs);
  json += ",\"durationMs\":" + String(durationMs_);
  json += ",\"library\":" + emotionLibraryJson();
  json += "}";
  return json;
}

bool EmotionActionController::isKnownAction(const String& name) {
  return findEmotionAction(name) != nullptr;
}

void EmotionActionController::restoreEmotionLights(RobotHardware& hardware) {
  if (lightSnapshotCaptured_) {
    hardware.restoreRgbSnapshot(rgbSnapshot_);
  }
  lightSnapshotCaptured_ = false;
}

void EmotionActionController::applyEmotionLight(RobotHardware& hardware, EmotionLightEffect effect, uint8_t frameIndex) {
  if (!lightsEnabled_ || effect == EmotionLightEffect::None) return;
  if (!lightSnapshotCaptured_) {
    rgbSnapshot_ = hardware.snapshotRgb();
    lightSnapshotCaptured_ = true;
  }

  RgbColor frame[RGB_LED_COUNT] = {};
  for (uint8_t index = 0; index < RGB_LED_COUNT; ++index) {
    switch (effect) {
      case EmotionLightEffect::RainbowMarquee:
        frame[index] = wheelColor(index + frameIndex * 2);
        break;
      case EmotionLightEffect::Breathing: {
        uint8_t brightness = (frameIndex % 2 == 0) ? 58 : 128;
        frame[index] = dimColor(rgbSnapshot_.colors[index], brightness);
        break;
      }
      case EmotionLightEffect::RedPulse:
        frame[index] = ((index < RGB_LEFT_END + 1) == (frameIndex % 2 == 0)) ? RgbColor{255, 0, 0} : RgbColor{34, 0, 0};
        break;
      case EmotionLightEffect::SurpriseFlash:
        frame[index] = (frameIndex % 2 == 0) ? RgbColor{255, 255, 255} : RgbColor{0, 220, 255};
        break;
      case EmotionLightEffect::ThinkingTravel:
        frame[index] = ((index + RGB_LED_COUNT - frameIndex) % RGB_LED_COUNT < 3) ? RgbColor{88, 72, 255} : RgbColor{8, 0, 36};
        break;
      case EmotionLightEffect::SoftPinkPulse: {
        uint8_t brightness = (frameIndex % 2 == 0) ? 70 : 180;
        frame[index] = dimColor(RgbColor{255, 80, 190}, brightness);
        break;
      }
      case EmotionLightEffect::WarmBounce:
        frame[index] = ((index + frameIndex) % 3 == 0) ? RgbColor{255, 174, 64} : RgbColor{255, 72, 160};
        break;
      case EmotionLightEffect::GoldPulse: {
        uint8_t brightness = (frameIndex % 2 == 0) ? 100 : 255;
        frame[index] = dimColor(RgbColor{255, 190, 24}, brightness);
        break;
      }
      case EmotionLightEffect::BluePurpleBlink:
        frame[index] = (frameIndex % 2 == 0) ? RgbColor{58, 120, 255} : RgbColor{150, 64, 255};
        break;
      case EmotionLightEffect::ColdFlash:
        frame[index] = (frameIndex % 2 == 0) ? RgbColor{220, 255, 255} : RgbColor{36, 96, 255};
        break;
      case EmotionLightEffect::CelebrationRainbow:
        frame[index] = wheelColor(index + frameIndex * 3);
        break;
      case EmotionLightEffect::BlinkingMarquee: {
        bool lit = ((index + frameIndex) % 3) == 0;
        uint8_t brightness = (frameIndex % 2 == 0) ? 255 : 96;
        frame[index] = lit ? dimColor(wheelColor(index + frameIndex * 4), brightness) : RgbColor{0, 0, 0};
        break;
      }
      case EmotionLightEffect::None:
        frame[index] = rgbSnapshot_.colors[index];
        break;
    }
  }
  hardware.setRgbEffectFrame(frame);
}

void EmotionActionController::applyStep(RobotHardware& hardware, RobotFaceRenderer& face, IdleBehaviorController& idle, const EmotionStep& step, uint32_t nowMs) {
  idle.pauseForManualControl(nowMs, step.durationMs + 1400);
  face.setMood(String(step.mood));
  face.setReact(step.lookX, step.lookY, step.audioLevel);
  if (step.rotateVelocity != 0) {
    hardware.prepareYawRotation();
    hardware.pitchWithRestPitchOffset(step.pitch, step.speed);
    hardware.rotateX(step.rotateVelocity);
  } else {
    hardware.rotateX(0);
    hardware.poseWithRestPitchOffset(step.yaw, step.pitch, step.speed);
  }
  applyEmotionLight(hardware, step.lightEffect, stepIndex_);
  currentStepLabel_ = step.label;
}
