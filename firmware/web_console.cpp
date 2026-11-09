#include "web_console.h"

String webConsoleHtml() {
  return R"HTML(
<!doctype html>
<html lang="zh-CN">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>桌面机器人控制台</title>
<style>
*{box-sizing:border-box}
html{scroll-behavior:smooth}
body{margin:0;background:#080d11;color:#d6f3ff;font-family:system-ui,-apple-system,Segoe UI,sans-serif;font-size:12px;line-height:1.28}
main{max-width:1240px;margin:auto;padding:8px;display:grid;gap:7px}
header{display:grid;gap:6px}
section{border:1px solid #1e3a47;background:#101820;padding:8px;border-radius:6px;box-shadow:0 6px 18px rgba(0,0,0,.14)}
h1{font-size:18px;margin:0;color:#e3f8ff}h2{font-size:14px;margin:0;color:#7df2df}
.top-nav{position:sticky;top:0;z-index:4;display:flex;gap:4px;flex-wrap:wrap;border:1px solid #1e3a47;background:rgba(8,13,17,.94);backdrop-filter:blur(10px);border-radius:6px;padding:4px}
.top-nav a{color:#bfefff;text-decoration:none;border:1px solid #2a5364;background:#071016;border-radius:999px;padding:3px 6px;font-size:11px}
.panel-heading{display:flex;align-items:end;justify-content:space-between;gap:8px;margin-bottom:6px}
.section-kicker{font-size:10px;color:#86aebb;margin-bottom:2px}
.panel-grid{display:grid;grid-template-columns:minmax(0,1.1fr) minmax(220px,.9fr);gap:7px 8px;align-items:start}
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(160px,1fr));gap:6px 8px;align-items:end}
label{display:grid;gap:3px;font-size:11px;color:#a8c7d3;min-width:0}
input,select,button{width:100%;min-width:0;background:#071016;color:#e6fbff;border:1px solid #2a5364;border-radius:5px;padding:4px 6px;font:inherit}
input[type=range]{padding:0;width:100%;min-width:0;accent-color:#1683ff}
input[type=color]{height:28px;padding:3px;cursor:pointer}button{cursor:pointer;width:auto;font-weight:650}button:hover,.top-nav a:hover{border-color:#58a6ff;color:#fff}button:focus-visible,input:focus-visible,select:focus-visible{outline:2px solid #58a6ff;outline-offset:2px}.primary{background:#0f3a35;border-color:#35cdbb;color:#f0fffc}.subtle{background:#0b151b}.danger{background:#211017;border-color:#754052;color:#ffdbe3}.row{display:flex;gap:5px;flex-wrap:wrap}.mono{font-family:ui-monospace,Menlo,monospace;color:#9ce7ff;white-space:pre-wrap}
input[type=number]{width:58px;text-align:left}
.control-line{display:grid;grid-template-columns:minmax(0,1fr) 58px;gap:5px;align-items:center}
.field-wide{grid-column:span 2}
.actions{margin-top:6px}
.actions button{min-width:64px}
.status-summary{display:flex;align-items:center;gap:5px;flex-wrap:wrap;border:1px solid #1e3a47;background:#101820;border-radius:6px;padding:6px 8px;font-size:12px}
.status-dot{width:7px;height:7px;border-radius:50%;background:#34f59a;box-shadow:0 0 10px #34f59a}
.status-pill{border:1px solid #2a5364;background:#071016;border-radius:999px;padding:3px 6px;color:#bfefff}
.status-details{font-size:11px;color:#a8c7d3}
.status-details summary{cursor:pointer;color:#7df2df;display:inline-block;margin-bottom:4px}
.status-json{max-height:110px;overflow:auto;border:1px solid #1e3a47;background:#071016;border-radius:5px;padding:6px;margin:0}
.swatches{display:flex;gap:4px;align-items:center;flex-wrap:wrap;min-height:28px}
.swatch{width:20px;height:20px;border-radius:50%;border:2px solid #2a5364;background:var(--c);padding:0;box-shadow:0 0 10px color-mix(in srgb,var(--c),transparent 45%)}
.swatch:focus{outline:2px solid #58a6ff;outline-offset:2px}
.swatch-row{display:grid;grid-template-columns:54px minmax(0,1fr);gap:6px;align-items:center;margin-top:6px}
.swatch-row span{font-size:11px;color:#a8c7d3}
.metric-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(96px,1fr));gap:5px;margin:5px 0}
.metric{border:1px solid #1e3a47;background:#071016;border-radius:5px;padding:5px 6px}
.metric b{display:block;font-size:10px;color:#7df2df;margin-bottom:2px}.metric span{font-size:13px;color:#e6fbff}
.camera-preview-box{display:grid;gap:5px;margin-top:6px}
.camera-preview-box img{width:min(100%,240px);aspect-ratio:1/1;object-fit:contain;image-rendering:pixelated;border:1px solid #1e3a47;background:#071016;border-radius:5px}
.camera-preview-status{font-size:11px;color:#a8c7d3}
.trigger-panel{padding:10px}
.trigger-panel .panel-heading{margin-bottom:6px}
.trigger-layout{display:grid;grid-template-columns:minmax(0,.9fr) minmax(360px,1.1fr);gap:8px 10px;align-items:start}
.trigger-settings{display:grid;grid-template-columns:repeat(auto-fit,minmax(150px,1fr));gap:6px 8px;align-items:end}
.trigger-settings label{gap:3px}
.trigger-settings input,.trigger-settings select{padding:4px 7px}
.trigger-status{grid-template-columns:repeat(auto-fit,minmax(96px,1fr));gap:5px;margin:0}
.trigger-status .metric{padding:5px 7px;min-height:46px}
.trigger-status .metric b{font-size:10px;margin-bottom:2px}.trigger-status .metric span{font-size:13px;line-height:1.25}
.trigger-actions{margin-top:6px}
.log-details{margin-top:6px;font-size:11px;color:#a8c7d3}
.log-details summary{cursor:pointer;display:inline-flex;align-items:center;gap:4px;border:1px solid #2a5364;background:#071016;border-radius:999px;padding:3px 6px;color:#bfefff}
.log-details[open] summary{margin-bottom:5px}
.panel-log{min-height:45px;max-height:110px;overflow:auto;border:1px solid #1e3a47;background:#071016;border-radius:5px;padding:6px;margin:0}
.meter{height:8px;border:1px solid #2a5364;background:#071016;border-radius:999px;overflow:hidden;margin-top:5px}
.meter-fill{height:100%;width:0;background:linear-gradient(90deg,#34f59a,#58a6ff);box-shadow:0 0 16px #34f59a}
.library-list{display:flex;gap:4px;flex-wrap:wrap;margin:5px 0}.library-list span{border:1px solid #2a5364;border-radius:999px;padding:2px 5px;color:#bfefff;background:#071016}
.tool-block{border:1px solid #1e3a47;background:#071016;border-radius:6px;padding:7px}.tool-block h3{font-size:12px;line-height:1.15;margin:0 0 5px;color:#7df2df}.emotion-layout{display:grid;grid-template-columns:minmax(230px,.95fr) minmax(270px,1.05fr);gap:7px;align-items:start}.emotion-stack{display:grid;gap:7px}.compact-actions{justify-content:flex-end}.compact-actions button{min-width:66px}.setting-grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:5px}.action-group{display:grid;gap:4px;margin-top:7px}.action-group:first-child{margin-top:0}.action-group-title{font-size:11px;color:#86aebb}.emotion-actions{display:grid;grid-template-columns:repeat(auto-fit,minmax(86px,1fr));gap:4px}.emotion-actions button{width:100%;min-height:28px;text-align:left}.helper-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(86px,1fr));gap:4px;margin-top:5px}.helper-grid button{width:100%;min-height:28px}.info-strip{display:grid;grid-template-columns:repeat(auto-fit,minmax(104px,1fr));gap:4px;margin-top:5px}.info-strip .metric{margin:0}.sound-details{margin-top:0}.sound-details summary{cursor:pointer;display:flex;align-items:center;justify-content:space-between;gap:6px;list-style:none}.sound-details summary::-webkit-details-marker{display:none}.sound-details[open] summary{margin-bottom:5px}.summary-title{font-size:12px;color:#7df2df;font-weight:700}.summary-meta{font-size:11px;color:#86aebb}
.slot-list{display:grid;gap:5px;margin-top:7px}
.slot-row{display:grid;grid-template-columns:minmax(100px,1fr) 56px 62px 62px;gap:4px;align-items:center;border:1px solid #1e3a47;background:#071016;border-radius:5px;padding:4px}
.slot-title{min-width:0}.slot-title b{display:block;color:#e6fbff;font-size:12px}.slot-title span{display:block;color:#86aebb;font-size:11px;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
.catalog-section{padding:0}.catalog-collapsible{display:block}.catalog-collapsible>summary{cursor:pointer;display:flex;align-items:end;justify-content:space-between;gap:8px;list-style:none;padding:8px}.catalog-collapsible>summary::-webkit-details-marker{display:none}.catalog-collapsible>summary::after{content:"展开";flex:0 0 auto;border:1px solid #2a5364;background:#071016;border-radius:999px;padding:3px 7px;color:#bfefff;font-size:11px;font-weight:650}.catalog-collapsible[open]>summary{border-bottom:1px solid #1e3a47;margin-bottom:7px}.catalog-collapsible[open]>summary::after{content:"收起"}.catalog-panel-body{display:grid;gap:7px;padding:0 8px 8px}.catalog-toolbar{justify-content:flex-end;margin:0}.catalog-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(280px,1fr));gap:7px}.catalog-list{display:grid;gap:4px}.catalog-row{border:1px solid #1e3a47;background:#071016;border-radius:5px;padding:5px}.catalog-row b{display:block;color:#e6fbff;font-size:12px}.catalog-row span{display:block;color:#86aebb;font-size:11px;line-height:1.25;margin-top:2px}.catalog-effects{display:flex;gap:3px;flex-wrap:wrap;margin-top:4px}.catalog-effects i{font-style:normal;border:1px solid #2a5364;border-radius:999px;padding:1px 4px;color:#bfefff}.catalog-actions{margin-top:4px}.catalog-actions button{width:100%;min-height:26px}
@media(max-width:900px){.panel-grid,.emotion-layout,.catalog-grid,.trigger-layout{grid-template-columns:1fr}}
@media(max-width:700px){main{padding:7px}.grid,.trigger-settings{grid-template-columns:1fr}.field-wide{grid-column:auto}.control-line{grid-template-columns:minmax(0,1fr) 58px}.slot-row{grid-template-columns:1fr 54px 60px 60px}section{padding:7px}h1{font-size:17px}.top-nav{position:static}}
@media(max-width:560px){.setting-grid{grid-template-columns:1fr}.panel-heading{align-items:stretch;flex-direction:column}.compact-actions{justify-content:flex-start}.compact-actions button{flex:1}}
</style>
</head>
<body>
<main>
<header>
<h1>桌面机器人固件控制台</h1>
<div class="status-summary" id="statusSummary">正在读取状态...</div>
<details class="status-details"><summary>详细状态</summary><pre class="mono status-json" id="statusDetail"></pre></details>
<nav class="top-nav"><a href="#panel-face">眼睛</a><a href="#panel-screen">画面</a><a href="#panel-subtitle">字幕</a><a href="#panel-assistant-status">助手信息</a><a href="#panel-debug">调试</a><a href="#panel-listening">聆听</a><a href="#panel-motion">动作</a><a href="#panel-triggers">触发</a><a href="#panel-emotions">情绪</a><a href="#panel-catalog">资产规则</a><a href="#panel-idle">待机</a><a href="#panel-lights">灯光</a><a href="#panel-sensors">传感器</a><a href="#panel-proximity">红外</a><a href="#panel-camera">摄像头</a><a href="#panel-audio">声音</a></nav>
</header>
<section id="panel-face">
<div class="panel-heading"><div><div class="section-kicker">视觉</div><h2>眼睛控制台</h2></div></div>
<div class="grid">
<label>表情<select id="mood"><option value="neutral">平静</option><option value="happy">开心</option><option value="angry">生气</option><option value="sad">难过</option><option value="sleeping">困倦</option><option value="tired">疲惫</option><option value="surprised">惊讶</option><option value="love">喜爱</option><option value="dizzy">眩晕</option><option value="wink">眨眼</option><option value="dead">无语</option><option value="thinking">思考</option><option value="curious">好奇</option><option value="sunglasses">墨镜</option><option value="loading">加载</option></select></label>
<label>眼睛颜色<input id="eyeColor" type="color" value="#00ffff"></label>
<div class="field-wide swatches"><button type="button" class="swatch" data-target="eyeColor" data-apply="face" data-color="#00ff66" title="荧光绿" style="--c:#00ff66"></button><button type="button" class="swatch" data-target="eyeColor" data-apply="face" data-color="#00ffff" title="青色" style="--c:#00ffff"></button><button type="button" class="swatch" data-target="eyeColor" data-apply="face" data-color="#58a6ff" title="蓝色" style="--c:#58a6ff"></button><button type="button" class="swatch" data-target="eyeColor" data-apply="face" data-color="#ff405c" title="红色" style="--c:#ff405c"></button><button type="button" class="swatch" data-target="eyeColor" data-apply="face" data-color="#ffcc33" title="黄色" style="--c:#ffcc33"></button><button type="button" class="swatch" data-target="eyeColor" data-apply="face" data-color="#ffffff" title="白色" style="--c:#ffffff"></button></div>
<label>眼睛宽度<div class="control-line"><input id="eyeWidthScale" type="range" min="0.35" max="2.2" step="0.01" value="1"><input id="eyeWidthScaleNum" type="number" min="0.35" max="2.2" step="0.01" value="1"></div></label>
<label>眼睛高度<div class="control-line"><input id="eyeHeightScale" type="range" min="0.15" max="2.2" step="0.01" value="1"><input id="eyeHeightScaleNum" type="number" min="0.15" max="2.2" step="0.01" value="1"></div></label>
<label>圆角<div class="control-line"><input id="cornerRadius" type="range" min="0" max="80" step="1" value="24"><input id="cornerRadiusNum" type="number" min="0" max="80" step="1" value="24"></div></label>
<label>水平位置<div class="control-line"><input id="eyeOffsetX" type="range" min="-90" max="90" step="1" value="0"><input id="eyeOffsetXNum" type="number" min="-90" max="90" step="1" value="0"></div></label>
<label>垂直位置<div class="control-line"><input id="eyeOffsetY" type="range" min="-90" max="90" step="1" value="0"><input id="eyeOffsetYNum" type="number" min="-90" max="90" step="1" value="0"></div></label>
<label>眼距<div class="control-line"><input id="eyeSpacing" type="range" min="48" max="220" step="1" value="112"><input id="eyeSpacingNum" type="number" min="48" max="220" step="1" value="112"></div></label>
<label>发光<select id="glowEnabled"><option value="1">开</option><option value="0">关</option></select></label>
<label>发光强度<div class="control-line"><input id="glowStrength" type="range" min="0" max="8" step="1" value="3"><input id="glowStrengthNum" type="number" min="0" max="8" step="1" value="3"></div></label>
<label>横纹<select id="scanlinesEnabled"><option value="0">关</option><option value="1">开</option></select></label>
<label>横纹间距<div class="control-line"><input id="scanlineSpacing" type="range" min="3" max="32" step="1" value="8"><input id="scanlineSpacingNum" type="number" min="3" max="32" step="1" value="8"></div></label>
<label>横纹透明度<div class="control-line"><input id="scanlineAlpha" type="range" min="0" max="100" step="1" value="70"><input id="scanlineAlphaNum" type="number" min="0" max="100" step="1" value="70"></div></label>
<label>视线 X<div class="control-line"><input id="lookX" type="range" min="-1" max="1" step="0.01" value="0"><input id="lookXNum" type="number" min="-1" max="1" step="0.01" value="0"></div></label>
<label>视线 Y<div class="control-line"><input id="lookY" type="range" min="-1" max="1" step="0.01" value="0"><input id="lookYNum" type="number" min="-1" max="1" step="0.01" value="0"></div></label>
<label>音频强度<div class="control-line"><input id="audioLevel" type="range" min="0" max="1" step="0.01" value="0"><input id="audioLevelNum" type="number" min="0" max="1" step="0.01" value="0"></div></label>
</div>
<div class="row actions"><button onclick="sendFace()">应用表情</button><button onclick="sendReact()">应用反应</button></div>
</section>
<section id="panel-screen">
<div class="panel-heading"><div><div class="section-kicker">全屏动画</div><h2>画面资产</h2></div><div class="row compact-actions"><button class="subtle" onclick="refreshScreenAsset()">刷新画面</button><button class="danger" onclick="clearScreenAsset()">停止画面</button></div></div>
<div class="panel-grid">
<div class="grid">
<label>画面资产<select id="screenAsset"><option value="none">无</option><option value="matrixRain">Matrix 数字雨</option></select></label>
<label>播放时长<input id="screenDuration" type="number" min="0" max="600000" step="1000" value="30000"></label>
<div class="field-wide row actions"><button class="primary" onclick="triggerMatrix()">启动 Matrix 屏保</button><button onclick="setScreenAsset()">应用画面资产</button></div>
</div>
<div>
<label>画面状态</label>
<div class="metric-grid"><div class="metric"><b>模式</b><span id="screenAssetMode">-</span></div><div class="metric"><b>状态</b><span id="screenAssetActive">-</span></div><div class="metric"><b>剩余</b><span id="screenAssetRemaining">-</span></div><div class="metric"><b>全屏</b><span id="screenAssetFullscreen">-</span></div></div>
</div>
</div>
<details class="log-details"><summary>调试输出</summary><pre class="mono panel-log" id="screenLog"></pre></details>
</section>
<section id="panel-subtitle">
<div class="panel-heading"><div><div class="section-kicker">屏幕底部</div><h2>字幕</h2></div></div>
<div class="grid">
<label class="field-wide">字幕文本<input id="subtitleText" type="text" maxlength="160" value="" placeholder="balabala..."></label>
<label>字幕显示<select id="subtitleEnabled"><option value="1">开</option><option value="0">关</option></select></label>
<label>日期时间<select id="subtitleDateTime"><option value="0">关</option><option value="1">开</option></select></label>
<label>字幕颜色<input id="subtitleColor" type="color" value="#00ff62"></label>
<div class="field-wide swatches"><button type="button" class="swatch" data-target="subtitleColor" data-apply="subtitle" data-color="#ffffff" title="字幕白色" style="--c:#ffffff"></button><button type="button" class="swatch" data-target="subtitleColor" data-apply="subtitle" data-color="#00ffff" title="字幕青色" style="--c:#00ffff"></button><button type="button" class="swatch" data-target="subtitleColor" data-apply="subtitle" data-color="#00ff66" title="字幕荧光绿" style="--c:#00ff66"></button><button type="button" class="swatch" data-target="subtitleColor" data-apply="subtitle" data-color="#58a6ff" title="字幕蓝色" style="--c:#58a6ff"></button><button type="button" class="swatch" data-target="subtitleColor" data-apply="subtitle" data-color="#ff405c" title="字幕红色" style="--c:#ff405c"></button><button type="button" class="swatch" data-target="subtitleColor" data-apply="subtitle" data-color="#ffcc33" title="字幕黄色" style="--c:#ffcc33"></button></div>
</div>
<div class="row actions"><button onclick="setSubtitle()">显示字幕</button><button onclick="clearSubtitle()">清除字幕</button></div>
<details class="log-details"><summary>调试输出</summary><pre class="mono panel-log" id="subtitleLog"></pre></details>
</section>
<section id="panel-assistant-status">
<div class="panel-heading"><div><div class="section-kicker">屏幕底部纯文字</div><h2>助手信息</h2></div><div class="row compact-actions"><button class="subtle" onclick="refreshAssistantStatus()">读取</button><button class="primary" onclick="setAssistantStatus()">保存</button></div></div>
<div class="grid">
<label>状态轮询<select id="assistantStatusEnabled"><option value="1">开</option><option value="0">关</option></select></label>
<label>信息显示<select id="assistantDisplayEnabled"><option value="1">开</option><option value="0">关</option></select></label>
<label>显示电量<select id="assistantBatteryEnabled"><option value="1">开</option><option value="0">关</option></select></label>
<label>文字颜色<input id="assistantStatusColor" type="color" value="#00ff62"></label>
<label>轮询间隔 ms<input id="assistantPollIntervalMs" type="number" min="1000" max="60000" step="500" value="5000"></label>
<label>事件显示秒数<input id="assistantEventTtlSeconds" type="number" min="3" max="120" step="1" value="12"></label>
<label>Codex 低量 %<input id="assistantLowCodexPercent" type="number" min="1" max="100" step="1" value="20"></label>
<label>电量低量 %<input id="assistantLowBatteryPercent" type="number" min="1" max="100" step="1" value="20"></label>
<label class="field-wide">状态源 URL<input id="assistantStatusUrl" type="text" value=""></label>
<div class="field-wide swatches"><button type="button" class="swatch" data-target="assistantStatusColor" data-apply="assistantStatus" data-color="#ffffff" title="助手白色" style="--c:#ffffff"></button><button type="button" class="swatch" data-target="assistantStatusColor" data-apply="assistantStatus" data-color="#00ffff" title="助手青色" style="--c:#00ffff"></button><button type="button" class="swatch" data-target="assistantStatusColor" data-apply="assistantStatus" data-color="#00ff66" title="助手荧光绿" style="--c:#00ff66"></button><button type="button" class="swatch" data-target="assistantStatusColor" data-apply="assistantStatus" data-color="#58a6ff" title="助手蓝色" style="--c:#58a6ff"></button><button type="button" class="swatch" data-target="assistantStatusColor" data-apply="assistantStatus" data-color="#ff405c" title="助手红色" style="--c:#ff405c"></button><button type="button" class="swatch" data-target="assistantStatusColor" data-apply="assistantStatus" data-color="#ffcc33" title="助手黄色" style="--c:#ffcc33"></button></div>
</div>
<div class="metric-grid"><div class="metric"><b>状态源</b><span id="assistantSourceOnline">-</span></div><div class="metric"><b>当前行</b><span id="assistantLastLine">-</span></div><div class="metric"><b>当前事件</b><span id="assistantEventMessage">-</span></div><div class="metric"><b>事件剩余</b><span id="assistantEventRemaining">-</span></div><div class="metric"><b>任务</b><span id="assistantCompletedToday">-</span></div><div class="metric"><b>低量告警</b><span id="assistantAlertState">-</span></div><div class="metric"><b>电池</b><span id="assistantBatteryState">-</span></div><div class="metric"><b>错误</b><span id="assistantLastError">-</span></div></div>
<div class="row actions"><button onclick="setAssistantSourceSettings()">保存事件规则</button><button onclick="clearAssistantSourceEvent()">清除当前事件</button><button onclick="sendAssistantSourceEvent('test_ok')">测试 OK</button><button onclick="sendAssistantSourceEvent('task_failed')">测试失败</button></div>
<details class="log-details"><summary>调试输出</summary><pre class="mono panel-log" id="assistantStatusLog"></pre></details>
</section>
<section id="panel-debug">
<div class="panel-heading"><div><div class="section-kicker">开发调试</div><h2>调试字幕</h2></div><div class="row compact-actions"><button class="subtle" onclick="refreshDebugSubtitle()">读取开关</button><button class="primary" onclick="setDebugSubtitle()">保存开关</button></div></div>
<div class="grid">
<label>唤醒监听字幕<select id="debugSubtitleListening"><option value="0">关</option><option value="1">开</option></select></label>
<label>API 返回字幕<select id="debugSubtitleApiResult"><option value="0">关</option><option value="1">开</option></select></label>
<label class="field-wide">API 示例<input id="debugSubtitleText" type="text" maxlength="160" value="{&quot;ok&quot;:true,&quot;action_id&quot;:&quot;nod_yes&quot;}"></label>
</div>
<div class="row actions"><button onclick="showDebugListening()">显示 Listening</button><button onclick="showDebugApiSample()">显示 API 示例</button></div>
<details class="log-details"><summary>调试输出</summary><pre class="mono panel-log" id="debugSubtitleLog"></pre></details>
</section>
<section id="panel-listening">
<div class="panel-heading"><div><div class="section-kicker">Wi-Fi 回调</div><h2>语音唤醒聆听</h2></div><div class="row compact-actions"><button class="subtle" onclick="refreshListening()">刷新</button><button class="primary" onclick="saveListening()">保存</button></div></div>
<div class="panel-grid">
<div class="grid">
<label>语音唤醒<select id="listeningVoiceWakeEnabled"><option value="0">关</option><option value="1">开</option></select></label>
<label>开关键唤醒<select id="listeningPowerButtonWakeEnabled"><option value="0">关</option><option value="1">开</option></select></label>
<label>聆听超时 ms<input id="listeningTimeoutMs" type="number" min="3000" max="30000" step="500" value="15000"></label>
<label class="field-wide">Host 回调地址<input id="listeningHostCallbackUrl" type="text" maxlength="128" placeholder="http://电脑局域网IP:8766/api/robot/wake-event"></label>
</div>
<div>
<label>聆听状态</label>
<div class="metric-grid"><div class="metric"><b>状态</b><span id="listeningState">-</span></div><div class="metric"><b>Wi-Fi</b><span id="listeningWifi">-</span></div><div class="metric"><b>剩余</b><span id="listeningRemaining">-</span></div><div class="metric"><b>已触发</b><span id="listeningWakeRequested">-</span></div></div>
</div>
</div>
<div class="row actions"><button onclick="startListening()">开始聆听</button><button onclick="stopListening()">停止聆听</button><button class="primary" onclick="wakeTestListening()">模拟唤醒回调</button></div>
<details class="log-details"><summary>调试输出</summary><pre class="mono panel-log" id="listeningLog"></pre></details>
</section>
<section id="panel-motion">
<div class="panel-heading"><div><div class="section-kicker">姿态</div><h2>动作控制</h2></div></div>
<div class="grid">
<label>水平舵机<div class="control-line"><input id="yaw" type="range" min="-1280" max="1280" step="10" value="0"><input id="yawNum" type="number" min="-1280" max="1280" step="10" value="0"></div></label>
<label>俯仰舵机<div class="control-line"><input id="pitch" type="range" min="0" max="900" step="10" value="120"><input id="pitchNum" type="number" min="0" max="900" step="10" value="120"></div></label>
<label>速度<div class="control-line"><input id="speed" type="range" min="0" max="1000" step="10" value="300"><input id="speedNum" type="number" min="0" max="1000" step="10" value="300"></div></label>
<label>水平旋转<div class="control-line"><input id="velocity" type="range" min="-1000" max="1000" step="10" value="0"><input id="velocityNum" type="number" min="-1000" max="1000" step="10" value="0"></div></label>
</div>
<div class="row actions"><button onclick="pose()">姿态</button><button onclick="home()">回中</button><button onclick="rotate()">旋转</button><button onclick="stopRotate()">停止旋转</button></div>
</section>
<section id="panel-sensors">
<div class="panel-heading"><div><div class="section-kicker">硬件读数</div><h2>传感器测试</h2></div></div>
<div class="grid">
<div>
<label>触摸板</label>
<div class="metric-grid"><div class="metric"><b>前</b><span id="touchFront">-</span></div><div class="metric"><b>中</b><span id="touchMiddle">-</span></div><div class="metric"><b>后</b><span id="touchBack">-</span></div><div class="metric"><b>手势</b><span id="touchGesture">-</span></div><div class="metric"><b>屏幕</b><span id="screenTouchPressed">-</span></div><div class="metric"><b>屏幕坐标</b><span id="screenTouchPoint">-</span></div></div>
<div class="row actions"><button onclick="readTouch()">读取触摸板</button></div>
<details class="log-details"><summary>调试输出</summary><pre class="mono panel-log" id="touchLog"></pre></details>
</div>
<div>
<label>惯性与电源</label>
<div class="row actions"><button onclick="readImu()">读取 IMU</button><button onclick="readPower()">读取电源</button></div>
<details class="log-details"><summary>调试输出</summary><pre class="mono panel-log" id="sensorLog"></pre></details>
</div>
</div>
</section>
<section id="panel-proximity">
<div class="panel-heading"><div><div class="section-kicker">LTR-553ALS-WA</div><h2>红外接口调试</h2></div></div>
<div class="metric-grid"><div class="metric"><b>模式</b><span id="proximityMode">-</span></div><div class="metric"><b>模块可用</b><span id="proximityAvailable">-</span></div><div class="metric"><b>传感器</b><span id="proximitySensorEnabled">-</span></div><div class="metric"><b>接近</b><span id="proximityNear">-</span></div><div class="metric"><b>接近值</b><span id="proximityRaw">-</span></div><div class="metric"><b>环境光</b><span id="proximityAmbient">-</span></div><div class="metric"><b>参考阈值</b><span id="proximityThreshold">-</span></div><div class="metric"><b>上次读取</b><span id="proximityLastRead">-</span></div></div>
<div class="row actions"><button onclick="refreshProximity()">刷新红外</button></div>
<details class="log-details"><summary>调试输出</summary><pre class="mono panel-log" id="proximityLog"></pre></details>
</section>
<section id="panel-camera">
<div class="panel-heading"><div><div class="section-kicker">本地视觉</div><h2>摄像头/人脸</h2></div></div>
<div class="panel-grid">
<div class="grid">
<label>本地人脸注意力<select id="cameraEnabled"><option value="1">开</option><option value="0">关</option></select></label>
<label>取证间隔 ms<input id="cameraCaptureInterval" type="number" min="3000" max="180000" step="1000" value="12000"></label>
<label>检测超时 ms<input id="cameraDetectTimeout" type="number" min="300" max="5000" step="100" value="1500"></label>
<label>回位稳定 ms<input id="cameraReturnSettle" type="number" min="80" max="2000" step="50" value="450"></label>
<label>复查稳定 ms<input id="cameraConfirmSettle" type="number" min="80" max="2000" step="50" value="250"></label>
<label>锁脸保持 ms<input id="cameraLockHold" type="number" min="1000" max="30000" step="500" value="4500"></label>
<label>冷却 ms<input id="cameraCooldown" type="number" min="3000" max="600000" step="1000" value="20000"></label>
<label>水平死区<input id="cameraCenterDeadZoneX" type="number" min="0.01" max="0.45" step="0.01" value="0.10"></label>
<label>垂直死区<input id="cameraCenterDeadZoneY" type="number" min="0.01" max="0.45" step="0.01" value="0.12"></label>
<label>Yaw 单步<input id="cameraYawStepMax" type="number" min="1" max="240" step="1" value="48"></label>
<label>Pitch 单步<input id="cameraPitchStepMax" type="number" min="1" max="180" step="1" value="28"></label>
<label>口头禅资源<input id="cameraCatchphraseAsset" type="text" value="halo"></label>
<label>口头禅后聆听<select id="cameraListenAfterCatchphrase"><option value="1">开</option><option value="0">关</option></select></label>
<label>检测反馈字幕<select id="cameraFeedbackSubtitleEnabled"><option value="1">开</option><option value="0">关</option></select></label>
</div>
<div>
<label>人脸状态</label>
<div class="metric-grid"><div class="metric"><b>模式</b><span id="cameraMode">-</span></div><div class="metric"><b>原因</b><span id="cameraReason">-</span></div><div class="metric"><b>取证 ID</b><span id="cameraSnapshotId">-</span></div><div class="metric"><b>检测</b><span id="cameraDetecting">-</span></div><div class="metric"><b>检测线程</b><span id="cameraDetectorTask">-</span></div><div class="metric"><b>最近帧</b><span id="cameraFrameSize">-</span></div><div class="metric"><b>复查</b><span id="cameraConfirming">-</span></div><div class="metric"><b>检测耗时</b><span id="cameraDetectDuration">-</span></div><div class="metric"><b>检测错误</b><span id="cameraDetectorError">-</span></div><div class="metric"><b>人脸框</b><span id="cameraFaceBox">-</span></div><div class="metric"><b>目标角度</b><span id="cameraTargetAngle">-</span></div><div class="metric"><b>冷却</b><span id="cameraCooldownRemaining">-</span></div><div class="metric"><b>口头禅</b><span id="cameraCatchphraseState">-</span></div></div>
<div class="camera-preview-box"><img id="cameraPreview" alt="摄像头预览帧"><span class="camera-preview-status" id="cameraPreviewStatus">未刷新</span></div>
</div>
</div>
<div class="row actions"><button onclick="setCamera()">应用摄像头设置</button><button onclick="setCameraCatchphrase()">保存口头禅</button><button onclick="refreshCamera()">刷新摄像头</button><button onclick="refreshCameraPreview()">刷新预览</button><button onclick="simulateCameraFace()">模拟有人脸</button><button onclick="simulateCameraNoFace()">模拟没人脸</button><button onclick="wakeCamera()">语音唤醒转向</button><button onclick="clearCamera()">清除人脸记忆</button></div>
<details class="log-details"><summary>调试输出</summary><pre class="mono panel-log" id="cameraLog"></pre></details>
</section>
<section id="panel-audio">
<div class="panel-heading"><div><div class="section-kicker">音频</div><h2>声音与麦克风</h2></div></div>
<div class="grid">
<label>测试音频率<div class="control-line"><input id="toneFrequency" type="range" min="80" max="5000" step="10" value="880"><input id="toneFrequencyNum" type="number" min="80" max="5000" step="10" value="880"></div></label>
<label>测试音时长<div class="control-line"><input id="toneDuration" type="range" min="40" max="3000" step="10" value="240"><input id="toneDurationNum" type="number" min="40" max="3000" step="10" value="240"></div></label>
<label>扬声器音量<div class="control-line"><input id="speakerVolume" type="range" min="0" max="255" step="1" value="96"><input id="speakerVolumeNum" type="number" min="0" max="255" step="1" value="96"></div></label>
<label>API 语音音量<div class="control-line"><input id="apiVoiceVolume" type="range" min="0" max="255" step="1" value="180"><input id="apiVoiceVolumeNum" type="number" min="0" max="255" step="1" value="180"></div></label>
<label>麦克风采样数<div class="control-line"><input id="micSamples" type="range" min="64" max="512" step="16" value="256"><input id="micSamplesNum" type="number" min="64" max="512" step="16" value="256"></div></label>
<label>麦克风采样率<div class="control-line"><input id="micSampleRate" type="range" min="8000" max="24000" step="1000" value="16000"><input id="micSampleRateNum" type="number" min="8000" max="24000" step="1000" value="16000"></div></label>
</div>
<div class="row actions"><button onclick="playTone()">播放测试音</button><button onclick="setApiVoiceVolume()">应用API音量</button><button onclick="stopAudio()">停止声音</button><button onclick="sampleMic()">采样麦克风</button></div>
<div class="meter"><div class="meter-fill" id="micLevelFill"></div></div>
<details class="log-details"><summary>调试输出</summary><pre class="mono panel-log" id="audioLog"></pre></details>
</section>
<section id="panel-idle">
<div class="panel-heading"><div><div class="section-kicker">空闲状态</div><h2>待机小行为库</h2></div></div>
<div class="library-list"><span>左右扫视</span><span>斜上方看</span><span>低头思考</span><span>向上看</span><span>回中停顿</span><span>近处注意</span><span>好奇回落</span><span>轻点头</span><span>偷看回中</span><span>困意回落</span><span>emo低头</span><span>疲惫回落</span><span>打瞌睡</span></div>
<div class="grid">
<label>待机动态<select id="idleEnabled"><option value="1">开</option><option value="0">关</option></select></label>
<label>待机仰角基准<div class="control-line"><input id="restPitch" type="range" min="0" max="900" step="10" value="120"><input id="restPitchNum" type="number" min="0" max="900" step="10" value="120"></div></label>
<div>
<label>当前行为</label>
<div class="metric-grid"><div class="metric"><b>序列</b><span id="idleBehavior">-</span></div><div class="metric"><b>步骤</b><span id="idleStep">-</span></div><div class="metric"><b>暂停</b><span id="idlePaused">-</span></div></div>
</div>
</div>
<div class="row actions"><button onclick="toggleIdle()">应用待机</button><button onclick="setRestPitch()">应用仰角</button><button onclick="nextIdle()">下一段行为</button><button onclick="refreshIdle()">刷新待机</button></div>
<div class="tool-block">
<h3>聊天状态</h3>
<div class="grid">
<label>聊天动作<select id="chatMotionEnabled"><option value="1">持续随机</option><option value="0">关</option></select></label>
<label>通话状态<select id="chatMotionStyle"><option value="natural">自然</option><option value="listening">听你说</option><option value="thinking">思考</option><option value="speaking">回答中</option><option value="settling">停顿</option></select></label>
<div>
<label>运行状态</label>
<div class="metric-grid"><div class="metric"><b>状态</b><span id="chatMotionState">等待随机</span></div></div>
</div>
</div>
<div class="row actions"><button class="primary" onclick="playChatMotionOnce()">播放聊天动作</button><button onclick="setChatMotion()">应用聊天状态</button><button class="danger" onclick="stopChatMotion()">停止聊天动作</button></div>
</div>
<details class="log-details"><summary>调试输出</summary><pre class="mono panel-log" id="idleLog"></pre></details>
</section>
<section id="panel-emotions">
<div class="panel-heading"><div><div class="section-kicker">手动播放</div><h2>情绪动作</h2></div><div class="row compact-actions"><button class="subtle" onclick="refreshEmotion()">刷新情绪</button><button class="danger" onclick="stopEmotion()">停止情绪动作</button></div></div>
<div class="emotion-layout">
<div class="emotion-stack">
<div class="tool-block">
<h3>当前状态</h3>
<div class="metric-grid"><div class="metric"><b>动作</b><span id="emotionTitle">-</span></div><div class="metric"><b>变体</b><span id="emotionVariant">-</span></div><div class="metric"><b>步骤</b><span id="emotionStep">-</span></div><div class="metric"><b>状态</b><span id="emotionActive">-</span></div><div class="metric"><b>灯效</b><span id="emotionLightsState">-</span></div></div>
</div>
<div class="tool-block">
<h3>快速播放</h3>
<div class="action-group"><div class="action-group-title">常用</div><div class="emotion-actions"><button class="primary" onclick="playEmotion('happy')">开心点头</button><button onclick="playEmotion('comfortableNod')">舒服点点头</button><button onclick="playEmotion('sad')">伤心低头</button><button onclick="playEmotion('angry')">生气摆头</button><button onclick="playEmotion('surprised')">惊讶后仰</button><button onclick="playEmotion('sleepWake')">睡眠醒来</button><button onclick="playEmotion('sleepStartle')">被惊醒</button><button onclick="playEmotion('thinking')">思考低头</button></div></div>
<div class="action-group"><div class="action-group-title">性格</div><div class="emotion-actions"><button onclick="playEmotion('curious')">好奇观察</button><button onclick="playEmotion('shy')">害羞躲闪</button><button onclick="playEmotion('cute')">撒娇求关注</button><button onclick="playEmotion('proud')">得意抬头</button><button onclick="playEmotion('confused')">困惑歪头</button><button onclick="playEmotion('scared')">紧张后缩</button><button onclick="playEmotion('celebrate')">庆祝欢呼</button></div></div>
<div class="action-group"><div class="action-group-title">测试</div><div class="emotion-actions"><button onclick="playEmotion('headTouchComboSpin')">摸头疯狂点头</button><button onclick="playEmotion('spin')">点头测试</button></div></div>
</div>
</div>
<div class="emotion-stack">
<div class="tool-block">
<h3>细调设置</h3>
<div class="action-group-title">情绪舵机编辑</div>
<div class="setting-grid">
<label>动作灯效<select id="emotionLights"><option value="1">开</option><option value="0">关</option></select></label>
<label>舵机速度<div class="control-line"><input id="emotionServoSpeed" type="range" min="0" max="1000" step="10" value="60"><input id="emotionServoSpeedNum" type="number" min="0" max="1000" step="10" value="60"></div></label>
<label>头X轴<div class="control-line"><input id="emotionServoYaw" type="range" min="-1280" max="1280" step="10" value="0"><input id="emotionServoYawNum" type="number" min="-1280" max="1280" step="10" value="0"></div></label>
<label>头Y轴<div class="control-line"><input id="emotionServoPitch" type="range" min="0" max="900" step="1" value="12"><input id="emotionServoPitchNum" type="number" min="0" max="900" step="1" value="12"></div></label>
</div>
<div class="helper-grid"><button class="primary" onclick="applyEmotionServoPose()">应用舵机</button><button onclick="applyEmotionServoPreset('sleeping',emotionSleepPitch,0.58)">睡觉最低</button><button onclick="applyEmotionServoPreset('tired',emotionTiredPitch,0.50)">疲惫略高</button></div>
<div class="info-strip"><div class="metric"><b>睡觉 / 疲惫</b><span>最低 0 / 略高 12</span></div><div class="metric"><b>伤心动作</b><span>斜下方低头</span></div></div>
</div>
<details class="tool-block sound-details">
<summary><span class="summary-title">情绪音效槽位</span><span class="summary-meta">上传 / 试听</span></summary>
<div class="grid">
<label class="field-wide">上传或替换音效<input id="emotionSoundUploadFile" type="file" accept="audio/*,.wav,.mp3,.raw,application/octet-stream"></label>
<div class="field-wide mono">可直接选 WAV/MP3/RAW；浏览器会转换为 RAW PCM / 8-bit unsigned / 16kHz / mono 后上传到对应槽位。</div>
</div>
<div class="slot-list" id="emotionSoundSlots"></div>
</details>
</div>
</div>
<details class="log-details"><summary>调试输出</summary><pre class="mono panel-log" id="emotionLog"></pre></details>
</section>
<section id="panel-catalog" class="catalog-section">
<details class="catalog-collapsible">
<summary class="catalog-summary"><div><div class="section-kicker">总览</div><h2>资产与规则</h2></div><span class="summary-meta">目录 / 规则 / 冲突</span></summary>
<div class="catalog-panel-body">
<div class="row compact-actions catalog-toolbar"><button class="subtle" onclick="refreshCatalog()">刷新目录</button></div>
<div class="catalog-grid">
<div class="tool-block"><h3>声音资产</h3><div class="catalog-list" id="catalogSoundAssets"></div></div>
<div class="tool-block"><h3>表情资产</h3><div class="catalog-list" id="catalogExpressionAssets"></div></div>
<div class="tool-block"><h3>画面资产</h3><div class="catalog-list" id="catalogScreenAssets"></div></div>
<div class="tool-block"><h3>动作资产</h3><div class="catalog-list" id="catalogMotionAssets"></div></div>
<div class="tool-block"><h3>灯光资产</h3><div class="catalog-list" id="catalogLightAssets"></div></div>
<div class="tool-block"><h3>行为资产</h3><div class="catalog-list" id="catalogBehaviorAssets"></div></div>
<div class="tool-block"><h3>弃用资产</h3><div class="catalog-list" id="catalogDeprecatedAssets"></div></div>
<div class="tool-block"><h3>逻辑规则</h3><div class="catalog-list" id="catalogLogicAssets"></div></div>
<div class="tool-block"><h3>关系提示</h3><div class="catalog-list" id="catalogConflicts"></div></div>
</div>
<details class="log-details"><summary>调试输出</summary><pre class="mono panel-log" id="catalogLog"></pre></details>
</div>
</details>
</section>
<section id="panel-triggers" class="trigger-panel">
<div class="panel-heading"><div><div class="section-kicker">自动反馈</div><h2>触发条件</h2></div></div>
<div class="trigger-layout">
<div class="trigger-settings">
<label>摸头触发舒服点点头<select id="triggerHeadTouchHappy"><option value="1">开</option><option value="0">关</option></select></label>
<label>10秒内摸5次疯狂点头，触发后5秒内再摸继续疯狂点头<select id="triggerHeadTouchCombo"><option value="1">开</option><option value="0">关</option></select></label>
<label>摸屏幕触发生气<select id="triggerScreenTouchAngry"><option value="1">开</option><option value="0">关</option></select></label>
<label>触发阈值<input id="headTouchThreshold" type="number" min="1" max="30" step="1" value="4"></label>
<label>陀螺仪/加速度触发<select id="triggerImuMotion"><option value="1">开</option><option value="0">关</option></select></label>
<label>倒置大摇生气 加速度<input id="angryShakeAccelDeltaG" type="number" min="0.02" max="3" step="0.01" value="0.55"></label>
<label>倒置大摇生气 角速度<input id="angryShakeGyroDps" type="number" min="5" max="1200" step="5" value="220"></label>
<label>倒置匹配阈值<input id="upsideDownGravityDotThreshold" type="number" min="-1" max="-0.05" step="0.01" value="-0.65"></label>
<label>随机情绪动作<select id="randomEmotionAction"><option value="0">关</option><option value="1">开</option></select></label>
<label>随机间隔最短<input id="randomEmotionMinInterval" type="number" min="10000" max="120000" step="5000" value="15000"></label>
<label>随机间隔最长<input id="randomEmotionMaxInterval" type="number" min="10000" max="120000" step="5000" value="45000"></label>
<label>夜间疲惫<select id="nightTriggerEnabled"><option value="1">开</option><option value="0">关</option></select></label>
<label>夜间开始<input id="nightStartHour" type="number" min="0" max="23" step="1" value="22"></label>
<label>夜间结束<input id="nightEndHour" type="number" min="0" max="23" step="1" value="8"></label>
<label>入睡等待<input id="nightSleepAfterMinutes" type="number" min="1" max="240" step="1" value="30"></label>
</div>
<div>
<label>实时触发状态</label>
<div class="metric-grid trigger-status"><div class="metric"><b>条件</b><span id="triggerCondition">-</span></div><div class="metric"><b>动作</b><span id="triggerAction">-</span></div><div class="metric"><b>头部状态</b><span id="triggerState">-</span></div><div class="metric"><b>屏幕状态</b><span id="screenTouchTriggerState">-</span></div><div class="metric"><b>屏幕坐标</b><span id="screenTouchTriggerPoint">-</span></div><div class="metric"><b>触摸峰值</b><span id="touchPeak">-</span></div><div class="metric"><b>阈值</b><span id="touchThresholdState">-</span></div><div class="metric"><b>摸头窗口</b><span id="triggerCooldown">-</span></div><div class="metric"><b>屏幕生气冷却</b><span id="screenTouchCooldown">-</span></div><div class="metric"><b>IMU状态</b><span id="imuTriggerState">-</span></div><div class="metric"><b>摇晃强度</b><span id="imuShakeStrength">-</span></div><div class="metric"><b>角速度</b><span id="imuGyroStrength">-</span></div><div class="metric"><b>倒置</b><span id="imuUpsideDownState">-</span></div><div class="metric"><b>IMU阈值</b><span id="imuThresholdState">-</span></div><div class="metric"><b>随机</b><span id="randomEmotionState">-</span></div><div class="metric"><b>随机间隔</b><span id="randomEmotionRange">-</span></div><div class="metric"><b>下次</b><span id="randomEmotionNext">-</span></div><div class="metric"><b>最近</b><span id="triggerLast">-</span></div><div class="metric"><b>随机动作</b><span id="randomEmotionLast">-</span></div><div class="metric"><b>日期时间</b><span id="clockDateTime">-</span></div><div class="metric"><b>夜间状态</b><span id="clockNightState">-</span></div><div class="metric"><b>睡眠状态</b><span id="clockSleepState">-</span></div><div class="metric"><b>无交互</b><span id="clockInteractionAge">-</span></div></div>
</div>
</div>
<div class="row actions trigger-actions"><button onclick="setTriggers()">应用触发条件</button><button onclick="refreshTriggers()">刷新触发条件</button></div>
<details class="log-details"><summary>调试输出</summary><pre class="mono panel-log" id="triggerLog"></pre></details>
</section>
<section id="panel-lights">
<div class="panel-heading"><div><div class="section-kicker">RGB</div><h2>待机灯光与传感器</h2></div></div>
<div class="grid">
<label>待机全部灯光<input id="rgbAll" type="color" value="#00ff62"></label>
<label>待机左侧灯光（面向屏幕）<input id="rgbLeft" type="color" value="#00ff62"></label>
<label>待机右侧灯光（面向屏幕）<input id="rgbRight" type="color" value="#00ff62"></label>
<label>亮度<div class="control-line"><input id="rgbBrightness" type="range" min="0" max="255" step="1" value="255"><input id="rgbBrightnessNum" type="number" min="0" max="255" step="1" value="255"></div></label>
<label>呼吸灯<select id="rgbBreathing"><option value="0">关</option><option value="1">开</option></select></label>
<label>跑马灯<select id="rgbMarquee"><option value="0">关</option><option value="1">开</option></select></label>
<label>七彩灯<select id="rgbRainbow"><option value="0">关</option><option value="1">开</option></select></label>
<label>灯光组合<select id="rgbPreset"><option value="manual">手动</option><option value="softGreenBreath">柔绿呼吸</option><option value="rainbowMarquee">彩虹跑马灯</option><option value="rainbowStatic">七彩常亮</option></select></label>
</div>
<div class="row actions"><button onclick="setRgb('all')">设置待机灯光</button><button onclick="setRgb('left')">设置左侧</button><button onclick="setRgb('right')">设置右侧</button><button onclick="setRgbEffects()">应用亮度/效果</button><button onclick="applyRgbPreset()">调用组合</button><button onclick="refreshStatus()">刷新状态</button></div>
<div class="swatch-row"><span>全部</span><div class="swatches"><button type="button" class="swatch" data-target="rgbAll" data-apply="rgb" data-side="all" data-color="#004020" title="全部柔绿" style="--c:#004020"></button><button type="button" class="swatch" data-target="rgbAll" data-apply="rgb" data-side="all" data-color="#00ff66" title="全部荧光绿" style="--c:#00ff66"></button><button type="button" class="swatch" data-target="rgbAll" data-apply="rgb" data-side="all" data-color="#00ffff" title="全部青色" style="--c:#00ffff"></button><button type="button" class="swatch" data-target="rgbAll" data-apply="rgb" data-side="all" data-color="#58a6ff" title="全部蓝色" style="--c:#58a6ff"></button><button type="button" class="swatch" data-target="rgbAll" data-apply="rgb" data-side="all" data-color="#ff405c" title="全部红色" style="--c:#ff405c"></button><button type="button" class="swatch" data-target="rgbAll" data-apply="rgb" data-side="all" data-color="#ffcc33" title="全部黄色" style="--c:#ffcc33"></button><button type="button" class="swatch" data-target="rgbAll" data-apply="rgb" data-side="all" data-color="#000000" title="全部关闭" style="--c:#000000"></button></div></div>
<div class="swatch-row"><span>左侧</span><div class="swatches"><button type="button" class="swatch" data-target="rgbLeft" data-apply="rgb" data-side="left" data-color="#004020" title="左侧柔绿" style="--c:#004020"></button><button type="button" class="swatch" data-target="rgbLeft" data-apply="rgb" data-side="left" data-color="#00ff66" title="左侧荧光绿" style="--c:#00ff66"></button><button type="button" class="swatch" data-target="rgbLeft" data-apply="rgb" data-side="left" data-color="#00ffff" title="左侧青色" style="--c:#00ffff"></button><button type="button" class="swatch" data-target="rgbLeft" data-apply="rgb" data-side="left" data-color="#58a6ff" title="左侧蓝色" style="--c:#58a6ff"></button><button type="button" class="swatch" data-target="rgbLeft" data-apply="rgb" data-side="left" data-color="#ff405c" title="左侧红色" style="--c:#ff405c"></button><button type="button" class="swatch" data-target="rgbLeft" data-apply="rgb" data-side="left" data-color="#ffcc33" title="左侧黄色" style="--c:#ffcc33"></button><button type="button" class="swatch" data-target="rgbLeft" data-apply="rgb" data-side="left" data-color="#000000" title="左侧关闭" style="--c:#000000"></button></div></div>
<div class="swatch-row"><span>右侧</span><div class="swatches"><button type="button" class="swatch" data-target="rgbRight" data-apply="rgb" data-side="right" data-color="#004020" title="右侧柔绿" style="--c:#004020"></button><button type="button" class="swatch" data-target="rgbRight" data-apply="rgb" data-side="right" data-color="#00ff66" title="右侧荧光绿" style="--c:#00ff66"></button><button type="button" class="swatch" data-target="rgbRight" data-apply="rgb" data-side="right" data-color="#00ffff" title="右侧青色" style="--c:#00ffff"></button><button type="button" class="swatch" data-target="rgbRight" data-apply="rgb" data-side="right" data-color="#58a6ff" title="右侧蓝色" style="--c:#58a6ff"></button><button type="button" class="swatch" data-target="rgbRight" data-apply="rgb" data-side="right" data-color="#ff405c" title="右侧红色" style="--c:#ff405c"></button><button type="button" class="swatch" data-target="rgbRight" data-apply="rgb" data-side="right" data-color="#ffcc33" title="右侧黄色" style="--c:#ffcc33"></button><button type="button" class="swatch" data-target="rgbRight" data-apply="rgb" data-side="right" data-color="#000000" title="右侧关闭" style="--c:#000000"></button></div></div>
<div class="mono" id="log"></div>
</section>
</main>
<script>
const $=id=>document.getElementById(id);
const moodText={neutral:'平静',happy:'开心',angry:'生气',sad:'难过',sleeping:'困倦',tired:'疲惫',surprised:'惊讶',love:'喜爱',dizzy:'眩晕',wink:'眨眼',dead:'无语',thinking:'思考',curious:'好奇',sunglasses:'墨镜',loading:'加载'};
const emotionSleepPitch=0,emotionTiredPitch=12;
const numberControls=['eyeWidthScale','eyeHeightScale','cornerRadius','eyeOffsetX','eyeOffsetY','eyeSpacing','glowStrength','scanlineSpacing','scanlineAlpha','lookX','lookY','audioLevel','yaw','pitch','speed','velocity','emotionServoYaw','emotionServoPitch','emotionServoSpeed','restPitch','rgbBrightness','toneFrequency','toneDuration','speakerVolume','apiVoiceVolume','micSamples','micSampleRate'];
const CHAT_MOTION_POSES=[
  {timeMs:0,yaw:0,pitchOffset:0,speed:190},
  {timeMs:360,yaw:18,pitchOffset:8,speed:190},
  {timeMs:760,yaw:-22,pitchOffset:-8,speed:185},
  {timeMs:1160,yaw:14,pitchOffset:10,speed:180},
  {timeMs:1560,yaw:-14,pitchOffset:-4,speed:175},
  {timeMs:1960,yaw:8,pitchOffset:4,speed:170},
  {timeMs:2400,yaw:0,pitchOffset:0,speed:165}
];
const CHAT_MOTION_STYLES={
  natural:{title:'自然',yawScale:1,pitchScale:1,speedScale:1,minDelayMs:3600,maxDelayMs:7200},
  listening:{title:'听你说',yawScale:.85,pitchScale:.8,speedScale:.9,minDelayMs:3600,maxDelayMs:7200},
  thinking:{title:'思考',yawScale:.65,pitchScale:.65,speedScale:.78,minDelayMs:5200,maxDelayMs:8200},
  speaking:{title:'回答中',yawScale:1.05,pitchScale:1,speedScale:1.12,minDelayMs:2600,maxDelayMs:4200},
  settling:{title:'停顿',yawScale:.55,pitchScale:.55,speedScale:.72,minDelayMs:6200,maxDelayMs:10000}
};
async function api(path, body){
  const opt=body?{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(body)}:{};
  const res=await fetch(path,opt); const txt=await res.text();
  try{return JSON.parse(txt)}catch{return {raw:txt,status:res.status}}
}
function uptimeText(ms){const s=Math.floor((ms||0)/1000),m=Math.floor(s/60),h=Math.floor(m/60);return h?`${h}时${m%60}分`:m?`${m}分${s%60}秒`:`${s}秒`}
function formatBytes(value){
  if(typeof value!=='number'||!Number.isFinite(value)||value<0)return '-';
  const units=['B','KB','MB','GB'];
  let size=value,unit=0;
  while(size>=1024&&unit<units.length-1){size/=1024;unit++}
  const digits=unit===0?0:size>=10?1:2;
  return `${size.toFixed(digits)}${units[unit]}`;
}
function firmwareSpaceText(s){
  const version=s.firmware?.version||'-';
  const sketch=formatBytes(s.firmware?.sketchSize);
  const free=formatBytes(s.firmware?.freeSketchSpace);
  if(sketch==='-'&&free==='-')return version;
  return `${version} · ${sketch} / 可更新 ${free}`;
}
function renderStatusSummary(s){
  const ok=s.ok&&s.hardware?.ready;
  const mood=moodText[s.face?.mood]||s.face?.mood||'-';
  const volt=s.power?.voltage?`${Number(s.power.voltage).toFixed(2)}V`:'-';
  const rssi=typeof s.rssi==='number'?`${s.rssi}dBm`:'-';
  $('statusSummary').innerHTML=`<span class="status-dot" style="background:${ok?'#34f59a':'#ff596a'};box-shadow:0 0 12px ${ok?'#34f59a':'#ff596a'}"></span><span>${ok?'在线':'异常'}</span><span class="status-pill">IP ${s.ip||'-'}</span><span class="status-pill">WiFi ${rssi}</span><span class="status-pill">表情 ${mood}</span><span class="status-pill">电池 ${volt}</span><span class="status-pill">固件 ${firmwareSpaceText(s)}</span><span class="status-pill">运行 ${uptimeText(s.uptimeMs)}</span>`;
}
function bool(id){return $(id).value==='1'}
function setValue(id,value){if(value!==undefined&&value!==null)$(id).value=value}
function setBoolValue(id,value){if(typeof value==='boolean')$(id).value=value?'1':'0'}
function syncNumberControls(){numberControls.forEach(id=>{const n=$(id+'Num');if(n)n.value=$(id).value})}
function bindNumberControl(id){
  const range=$(id),num=$(id+'Num');
  if(!range||!num)return;
  range.addEventListener('input',()=>{num.value=range.value});
  num.addEventListener('input',()=>{range.value=num.value});
}
function syncFaceControls(payload){
  const face=payload.face||payload;
  if(!face||!face.mood)return;
  setValue('mood',face.mood); setValue('eyeWidthScale',face.eyeWidthScale); setValue('eyeHeightScale',face.eyeHeightScale);
  setValue('cornerRadius',face.cornerRadius); setValue('eyeOffsetX',face.eyeOffsetX); setValue('eyeOffsetY',face.eyeOffsetY);
  setValue('eyeSpacing',face.eyeSpacing); setValue('eyeColor',face.eyeColor);
  setBoolValue('glowEnabled',face.glowEnabled); setValue('glowStrength',face.glowStrength);
  setBoolValue('scanlinesEnabled',face.scanlinesEnabled); setValue('scanlineSpacing',face.scanlineSpacing); setValue('scanlineAlpha',face.scanlineAlpha);
  setValue('lookX',face.lookX); setValue('lookY',face.lookY); setValue('audioLevel',face.audioLevel);
  syncNumberControls();
}
function syncSubtitleControls(payload){
  const face=payload.face||payload;
  const sub=face?.subtitle||payload.subtitle;
  if(!sub)return;
  setValue('subtitleText',sub.text||'');
  setBoolValue('subtitleEnabled',!!sub.enabled);
  setValue('subtitleColor',sub.color||'#00ff62');
  setBoolValue('subtitleDateTime',!!sub.showDateTime);
}
function syncAssistantStatusControls(s){
  if(!s)return;
  setBoolValue('assistantStatusEnabled',!!s.enabled);
  setBoolValue('assistantDisplayEnabled',!!s.displayEnabled);
  setBoolValue('assistantBatteryEnabled',!!s.batteryEnabled);
  setValue('assistantStatusColor',s.lineColor||'#00ff62');
  setValue('assistantPollIntervalMs',s.pollIntervalMs||5000);
  setValue('assistantEventTtlSeconds',s.eventTtlSeconds||12);
  setValue('assistantLowCodexPercent',s.lowCodexPercent||20);
  setValue('assistantLowBatteryPercent',s.lowBatteryPercent||20);
  setValue('assistantStatusUrl',s.statusUrl||'');
  $('assistantSourceOnline').textContent=s.sourceOnline?'在线':'离线';
  $('assistantLastLine').textContent=s.lastLine||'-';
  $('assistantEventMessage').textContent=s.eventActive?(s.eventMessage||s.eventType||'-'):'-';
  $('assistantEventRemaining').textContent=s.eventActive?`${s.eventRemainingSeconds||0}s / ${s.eventTtlSeconds||0}s`:`${s.eventTtlSeconds||0}s`;
  $('assistantCompletedToday').textContent=s.completedToday??'-';
  $('assistantAlertState').textContent=`Codex ${s.codexLow?'低':'正常'} · 电量 ${s.batteryLow?'低':'正常'}`;
  $('assistantBatteryState').textContent=typeof s.batteryPercent==='number'&&s.batteryPercent>=0?`${s.batteryPercent}%`:'-';
  $('assistantLastError').textContent=s.lastError||'-';
}
function syncMotionControls(payload){
  const motion=payload.motion||payload;
  if(!motion)return;
  setValue('yaw',motion.yaw);
  setValue('pitch',motion.pitch);
  setValue('speed',motion.speed);
  setValue('velocity',motion.velocity);
  setValue('restPitch',motion.restPitch);
  syncNumberControls();
}
function syncRgbControls(payload){
  const rgb=payload.rgb||payload;
  if(!rgb)return;
  setValue('rgbAll',rgb.color||'#00ff62');
  setValue('rgbLeft',rgb.leftColor||rgb.color||'#00ff62');
  setValue('rgbRight',rgb.rightColor||rgb.color||'#00ff62');
  setValue('rgbBrightness',rgb.brightness);
  setBoolValue('rgbBreathing',!!rgb.breathing);
  setBoolValue('rgbMarquee',!!rgb.marquee);
  setBoolValue('rgbRainbow',!!rgb.rainbow);
  setValue('rgbPreset',rgb.preset||'manual');
  syncNumberControls();
}
function syncEmotionServoControls(payload){
  const motion=payload.motion||payload;
  if(!motion)return;
  setValue('emotionServoYaw',motion.yaw);
  setValue('emotionServoPitch',motion.pitch);
  setValue('emotionServoSpeed',motion.speed);
  syncNumberControls();
}
function faceStyle(){
  return {
    mood:$('mood').value,
    eyeWidthScale:+$('eyeWidthScale').value, eyeHeightScale:+$('eyeHeightScale').value,
    cornerRadius:+$('cornerRadius').value, eyeOffsetX:+$('eyeOffsetX').value,
    eyeOffsetY:+$('eyeOffsetY').value, eyeSpacing:+$('eyeSpacing').value,
    eyeColor:$('eyeColor').value, glowEnabled:bool('glowEnabled'),
    glowStrength:+$('glowStrength').value, scanlinesEnabled:bool('scanlinesEnabled'),
    scanlineSpacing:+$('scanlineSpacing').value, scanlineAlpha:+$('scanlineAlpha').value
  };
}
async function selectMood(){const s=await api('/api/face/set',{mood:$('mood').value}); syncFaceControls(s); refreshStatus();}
async function sendFace(){const s=await api('/api/face/style',faceStyle()); syncFaceControls(s); refreshStatus();}
async function sendReact(){await api('/api/face/react',{lookX:+$('lookX').value,lookY:+$('lookY').value,audioLevel:+$('audioLevel').value}); refreshStatus();}
async function setSubtitle(){const s=await api('/api/subtitle/set',{text:$('subtitleText').value,enabled:bool('subtitleEnabled'),color:$('subtitleColor').value,showDateTime:bool('subtitleDateTime'),subtitleDateTime:bool('subtitleDateTime')});syncSubtitleControls(s);writeJson('subtitleLog',s);refreshStatus(false)}
async function clearSubtitle(){const s=await api('/api/subtitle/clear',{});syncSubtitleControls(s);writeJson('subtitleLog',s);refreshStatus(false)}
async function refreshAssistantStatus(){const s=await api('/api/assistant-display/status');syncAssistantStatusControls(s);writeJson('assistantStatusLog',s)}
async function setAssistantStatus(){const s=await api('/api/assistant-display/set',{enabled:bool('assistantStatusEnabled'),displayEnabled:bool('assistantDisplayEnabled'),batteryEnabled:bool('assistantBatteryEnabled'),lineColor:$('assistantStatusColor').value,assistantStatusUrl:$('assistantStatusUrl').value,pollIntervalMs:+$('assistantPollIntervalMs').value,lowCodexPercent:+$('assistantLowCodexPercent').value,lowBatteryPercent:+$('assistantLowBatteryPercent').value});syncAssistantStatusControls(s);writeJson('assistantStatusLog',s);refreshStatus(false);return s}
function assistantSourceBase(){const url=($('assistantStatusUrl').value||'').trim().replace(/\/$/,'');return url.endsWith('/api/assistant/status')?url.slice(0,-'/api/assistant/status'.length):url}
async function assistantSourceApi(path,payload){const base=assistantSourceBase();if(!base)throw new Error('状态源 URL 为空');const res=await fetch(base+path,{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(payload||{})});const text=await res.text();let data;try{data=JSON.parse(text)}catch{data={ok:false,raw:text,status:res.status}}if(!res.ok)throw new Error(data.error||`状态源 ${res.status}`);return data}
async function setAssistantSourceSettings(){try{const robot=await setAssistantStatus();const source=await assistantSourceApi('/api/assistant/settings',{eventTtlSeconds:+$('assistantEventTtlSeconds').value,lowCodexPercent:+$('assistantLowCodexPercent').value});writeJson('assistantStatusLog',{robot,source});refreshAssistantStatus()}catch(error){writeJson('assistantStatusLog',{ok:false,error:String(error)})}}
async function clearAssistantSourceEvent(){try{const source=await assistantSourceApi('/api/assistant/events/clear',{});writeJson('assistantStatusLog',source);refreshAssistantStatus()}catch(error){writeJson('assistantStatusLog',{ok:false,error:String(error)})}}
async function sendAssistantSourceEvent(type){try{const payload={type,ttlSeconds:+$('assistantEventTtlSeconds').value};if(type==='task_done')payload.count=1;const source=await assistantSourceApi('/api/assistant/events',payload);writeJson('assistantStatusLog',source);refreshAssistantStatus()}catch(error){writeJson('assistantStatusLog',{ok:false,error:String(error)})}}
function syncDebugSubtitleControls(s){if(!s)return;setBoolValue('debugSubtitleListening',!!s.listening);setBoolValue('debugSubtitleApiResult',!!s.apiResult)}
async function refreshDebugSubtitle(){const s=await api('/api/debug/subtitle/status');syncDebugSubtitleControls(s);writeJson('debugSubtitleLog',s)}
async function setDebugSubtitle(){const s=await api('/api/debug/subtitle/set',{listening:bool('debugSubtitleListening'),apiResult:bool('debugSubtitleApiResult')});syncDebugSubtitleControls(s);writeJson('debugSubtitleLog',s)}
async function showDebugListening(){const s=await api('/api/debug/subtitle/show',{kind:'listening',text:'Listening'});writeJson('debugSubtitleLog',s);refreshStatus(false)}
async function showDebugApiSample(){const s=await api('/api/debug/subtitle/show',{kind:'apiResult',text:$('debugSubtitleText').value||'{"ok":true}'});writeJson('debugSubtitleLog',s);refreshStatus(false)}
function syncListeningControls(s){
  if(!s)return;
  setBoolValue('listeningVoiceWakeEnabled',!!s.voiceWakeEnabled);
  setBoolValue('listeningPowerButtonWakeEnabled',!!s.powerButtonWakeEnabled);
  setValue('listeningHostCallbackUrl',s.hostCallbackUrl||'');
  setValue('listeningTimeoutMs',s.listenTimeoutMs);
  $('listeningState').textContent=s.active?'Listening':s.wifiConnected===false?'Wi-Fi 未连接':'待机';
  $('listeningWifi').textContent=s.wifiConnected?'已连接':'未连接';
  $('listeningRemaining').textContent=s.remainingMs?uptimeText(s.remainingMs):'-';
  $('listeningWakeRequested').textContent=s.listeningWakeRequested?'是':'否';
}
async function refreshListening(){const s=await api('/api/listening/status');syncListeningControls(s);writeJson('listeningLog',s)}
async function saveListening(){const s=await api('/api/listening/config',{voiceWakeEnabled:bool('listeningVoiceWakeEnabled'),powerButtonWakeEnabled:bool('listeningPowerButtonWakeEnabled'),hostCallbackUrl:$('listeningHostCallbackUrl').value,listenTimeoutMs:+$('listeningTimeoutMs').value});syncListeningControls(s);writeJson('listeningLog',s)}
async function startListening(){const s=await api('/api/listening/start',{listenTimeoutMs:+$('listeningTimeoutMs').value});syncListeningControls(s);writeJson('listeningLog',s);refreshStatus(false)}
async function stopListening(){const s=await api('/api/listening/stop',{});syncListeningControls(s);writeJson('listeningLog',s);refreshStatus(false)}
async function wakeTestListening(){const s=await api('/api/listening/wake-test',{source:'firmware-console',listenTimeoutMs:+$('listeningTimeoutMs').value});syncListeningControls(s);writeJson('listeningLog',s);refreshStatus(false)}
function renderScreenAsset(s){
  setValue('screenAsset',s.asset||'none');
  $('screenAssetMode').textContent=s.asset==='matrixRain'?'Matrix 数字雨':s.asset||'-';
  $('screenAssetActive').textContent=s.active?'播放中':'空闲';
  $('screenAssetRemaining').textContent=s.remainingMs?uptimeText(s.remainingMs):s.active?'持续':'-';
  $('screenAssetFullscreen').textContent=s.fullscreen?'是':'否';
}
async function refreshScreenAsset(){const s=await api('/api/screen/status');renderScreenAsset(s);writeJson('screenLog',s)}
async function setScreenAsset(){const s=await api('/api/screen/set',{asset:$('screenAsset').value,durationMs:+$('screenDuration').value});renderScreenAsset(s);writeJson('screenLog',s)}
async function triggerMatrix(){setValue('screenAsset','matrixRain');const s=await api('/api/screen/trigger',{asset:'matrixRain',durationMs:+$('screenDuration').value});renderScreenAsset(s);writeJson('screenLog',s)}
async function clearScreenAsset(){setValue('screenAsset','none');const s=await api('/api/screen/set',{asset:'none'});renderScreenAsset(s);writeJson('screenLog',s)}
async function pose(){const s=await api('/api/motion/pose',{yaw:+$('yaw').value,pitch:+$('pitch').value,speed:+$('speed').value});syncMotionControls(s);log(s)}
async function setRestPitch(){const s=await api('/api/motion/rest-pitch',{pitch:+$('restPitch').value,apply:true,speed:+$('speed').value});syncMotionControls(s);writeJson('idleLog',s);refreshStatus(false)}
async function home(){const s=await api('/api/motion/home',{speed:+$('speed').value});syncMotionControls(s);log(s)}
async function rotate(){const s=await api('/api/motion/rotate',{velocity:+$('velocity').value});syncMotionControls(s);log(s)}
async function stopRotate(){const s=await api('/api/motion/rotate',{velocity:0});syncMotionControls(s);log(s)}
function writeJson(id,x){$(id).textContent=JSON.stringify(x,null,2)}
async function readTouch(){
  const s=await api('/api/touch');
  $('touchFront').textContent=s.front??'-'; $('touchMiddle').textContent=s.middle??'-'; $('touchBack').textContent=s.back??'-';
  $('touchGesture').textContent=s.swipeForward?'前到后':s.swipeBackward?'后到前':s.pressed?'按下':'无';
  $('screenTouchPressed').textContent=s.screenPressed?'按下':'无';
  $('screenTouchPoint').textContent=s.screenPressed?`${s.screenX},${s.screenY}`:'-';
  writeJson('touchLog',s);
}
async function readImu(){writeJson('sensorLog',await api('/api/imu'))}
async function readPower(){writeJson('sensorLog',await api('/api/power'))}
function renderProximity(s){
  $('proximityMode').textContent=s.mode||'-';
  $('proximityAvailable').textContent=s.available?'可用':'不可用';
  $('proximitySensorEnabled').textContent=s.sensorEnabled?'开启':'关闭';
  $('proximityNear').textContent=s.near?'是':'否';
  $('proximityRaw').textContent=s.proximityRaw??'-';
  $('proximityAmbient').textContent=s.ambientRaw??'-';
  $('proximityThreshold').textContent=s.threshold??'-';
  $('proximityLastRead').textContent=typeof s.lastReadAtMs==='number'?uptimeText(s.lastReadAtMs):'-';
}
async function refreshProximity(){const s=await api('/api/proximity/status');renderProximity(s);writeJson('proximityLog',s)}
function renderCamera(s){
  setBoolValue('cameraEnabled',!!s.enabled);
  setValue('cameraCaptureInterval',s.captureIntervalMs);
  setValue('cameraDetectTimeout',s.detectTimeoutMs);
  setValue('cameraReturnSettle',s.returnSettleMs);
  setValue('cameraConfirmSettle',s.confirmSettleMs);
  setValue('cameraLockHold',s.lockHoldMs);
  setValue('cameraCooldown',s.cooldownMs);
  setValue('cameraCenterDeadZoneX',s.centerDeadZoneX);
  setValue('cameraCenterDeadZoneY',s.centerDeadZoneY);
  setValue('cameraYawStepMax',s.yawStepMax);
  setValue('cameraPitchStepMax',s.pitchStepMax);
  setValue('cameraCatchphraseAsset',s.catchphraseAssetId||'halo');
  setBoolValue('cameraListenAfterCatchphrase',!!s.listenAfterCatchphrase);
  setBoolValue('cameraFeedbackSubtitleEnabled',!!s.feedbackSubtitleEnabled);
  $('cameraMode').textContent=s.mode||'-';
  $('cameraReason').textContent=s.reason||'-';
  $('cameraSnapshotId').textContent=s.snapshotId??'-';
  const detector=s.detector||{};
  $('cameraDetecting').textContent=s.detecting?'检测中':(detector.cameraAvailable?'可用':'不可用');
  $('cameraDetectorTask').textContent=detector.initialized?(detector.taskRunning?'运行中':'待机'):'未初始化';
  $('cameraFrameSize').textContent=(detector.lastFrameWidth&&detector.lastFrameHeight)?`${detector.lastFrameWidth}x${detector.lastFrameHeight}`:'-';
  $('cameraConfirming').textContent=s.confirming?'复查中':'否';
  const detectMs=s.lastDetectDurationMs||detector.lastDetectDurationMs;
  $('cameraDetectDuration').textContent=detectMs?`${detectMs} ms`:'-';
  $('cameraDetectorError').textContent=detector.lastError||'-';
  $('cameraFaceBox').textContent=s.lastFaceDetected?`${Number(s.faceCenterX||0).toFixed(2)}, ${Number(s.faceCenterY||0).toFixed(2)} / ${Number(s.faceWidth||0).toFixed(2)}x${Number(s.faceHeight||0).toFixed(2)}`:'无';
  $('cameraTargetAngle').textContent=`${s.targetYaw??'-'} / ${s.targetPitch??'-'}`;
  $('cameraCooldownRemaining').textContent=s.cooldownRemainingMs?`${Math.ceil(s.cooldownRemainingMs/1000)}秒`:'-';
  $('cameraCatchphraseState').textContent=`${s.catchphraseAssetId||'halo'} / ${s.listenAfterCatchphrase?'聆听':'不聆听'}`;
}
async function refreshCamera(){const s=await api('/api/camera/status');renderCamera(s);writeJson('cameraLog',s)}
function refreshCameraPreview(){
  const img=$('cameraPreview');
  const status=$('cameraPreviewStatus');
  if(!img||!status)return;
  status.textContent='刷新中';
  img.src='/api/camera/frame.bmp?ts='+Date.now();
}
async function setCamera(){const s=await api('/api/camera/set',{enabled:bool('cameraEnabled'),captureIntervalMs:+$('cameraCaptureInterval').value,detectTimeoutMs:+$('cameraDetectTimeout').value,returnSettleMs:+$('cameraReturnSettle').value,confirmSettleMs:+$('cameraConfirmSettle').value,lockHoldMs:+$('cameraLockHold').value,cooldownMs:+$('cameraCooldown').value,centerDeadZoneX:+$('cameraCenterDeadZoneX').value,centerDeadZoneY:+$('cameraCenterDeadZoneY').value,yawStepMax:+$('cameraYawStepMax').value,pitchStepMax:+$('cameraPitchStepMax').value,catchphraseAssetId:$('cameraCatchphraseAsset').value,listenAfterCatchphrase:bool('cameraListenAfterCatchphrase'),feedbackSubtitleEnabled:bool('cameraFeedbackSubtitleEnabled')});renderCamera(s);writeJson('cameraLog',s)}
async function setCameraCatchphrase(){const s=await api('/api/camera/catchphrase',{assetId:$('cameraCatchphraseAsset').value,listenAfter:bool('cameraListenAfterCatchphrase')});renderCamera(s);writeJson('cameraLog',s)}
async function simulateCameraFace(){const s=await api('/api/camera/face-report',{scanId:+$('cameraSnapshotId').textContent||0,faceDetected:true,force:true,confidence:0.86,faceCenterX:0.55,faceCenterY:0.48,faceWidth:0.24,faceHeight:0.32});renderCamera(s);writeJson('cameraLog',s)}
async function simulateCameraNoFace(){const s=await api('/api/camera/face-report',{scanId:+$('cameraSnapshotId').textContent||0,faceDetected:false,force:true,confidence:0});renderCamera(s);writeJson('cameraLog',s)}
async function wakeCamera(){const s=await api('/api/camera/wake',{source:'console'});writeJson('cameraLog',s);refreshCamera()}
async function clearCamera(){const s=await api('/api/camera/clear',{});renderCamera(s);writeJson('cameraLog',s)}
function syncApiVoiceVolume(s){if(typeof s.volume==='number'){setValue('apiVoiceVolume',s.volume);syncNumberControls()}}
async function refreshApiVoiceVolume(){const s=await api('/api/mumble/status');syncApiVoiceVolume(s);return s}
async function setApiVoiceVolume(){const s=await api('/api/mumble/set',{volume:+$('apiVoiceVolume').value});syncApiVoiceVolume(s);writeJson('audioLog',s);return s}
async function playTone(){writeJson('audioLog',await api('/api/audio/tone',{frequency:+$('toneFrequency').value,durationMs:+$('toneDuration').value,volume:+$('speakerVolume').value}))}
async function stopAudio(){writeJson('audioLog',await api('/api/audio/stop',{}))}
async function sampleMic(){
  const s=await api('/api/mic/sample',{samples:+$('micSamples').value,sampleRate:+$('micSampleRate').value});
  $('micLevelFill').style.width=`${Math.round(Math.min(1,Math.max(0,s.level||0))*100)}%`;
  writeJson('audioLog',s);
}
function renderIdle(s){
  setBoolValue('idleEnabled',!!s.enabled);
  $('idleBehavior').textContent=s.behavior||'-';
  $('idleStep').textContent=s.step||'-';
  $('idlePaused').textContent=s.pausedMs?`${Math.ceil(s.pausedMs/1000)}秒`:'否';
}
async function refreshIdle(){const s=await api('/api/idle/status');renderIdle(s);writeJson('idleLog',s)}
async function toggleIdle(){const s=await api('/api/idle/set',{enabled:bool('idleEnabled')});renderIdle(s);writeJson('idleLog',s)}
async function nextIdle(){const s=await api('/api/idle/next',{});renderIdle(s);writeJson('idleLog',s)}
function sleep(ms){return new Promise(resolve=>setTimeout(resolve,ms))}
function activeChatMotionStyle(){return CHAT_MOTION_STYLES[$('chatMotionStyle').value]||CHAT_MOTION_STYLES.natural}
function chatMotionPose(point,style){
  const rest=+$('restPitch').value||120;
  return {
    yaw:Math.round(point.yaw*style.yawScale),
    pitch:Math.max(0,Math.min(900,rest+Math.round(point.pitchOffset*style.pitchScale))),
    speed:Math.max(0,Math.min(1000,Math.round(point.speed*style.speedScale)))
  };
}
function chatMotionDelayMs(style){return Math.round(style.minDelayMs+Math.random()*(style.maxDelayMs-style.minDelayMs))}
let chatMotionPlayToken=0,chatMotionLoopToken=0,chatMotionPlaying=false,chatMotionLoopRunning=false;
function setChatMotionState(text){$('chatMotionState').textContent=text}
async function playChatMotionOnce(){
  if(chatMotionPlaying)return;
  const token=++chatMotionPlayToken;
  chatMotionPlaying=true;
  const style=activeChatMotionStyle();
  let previous=0,lastResult=null;
  setChatMotionState(`${style.title} · 播放中`);
  try{
    for(const point of CHAT_MOTION_POSES){
      if(token!==chatMotionPlayToken)return;
      const delay=Math.max(0,point.timeMs-previous);
      if(delay)await sleep(delay);
      previous=point.timeMs;
      lastResult=await api('/api/motion/pose',chatMotionPose(point,style));
      syncMotionControls(lastResult);
    }
    writeJson('idleLog',{ok:true,module:'chatMotion',asset:'chat_soft_idle',style:$('chatMotionStyle').value,result:lastResult});
    setChatMotionState($('chatMotionEnabled').value==='1'?'等待随机':'已播放一次');
    refreshStatus(false);
  }finally{
    chatMotionPlaying=false;
  }
}
async function runChatMotionLoop(){
  if(chatMotionLoopRunning)return;
  chatMotionLoopRunning=true;
  const token=++chatMotionLoopToken;
  while($('chatMotionEnabled').value==='1'&&token===chatMotionLoopToken){
    const style=activeChatMotionStyle();
    const delay=chatMotionDelayMs(style);
    setChatMotionState(`${style.title} · 等待 ${Math.ceil(delay/1000)}秒`);
    await sleep(delay);
    if($('chatMotionEnabled').value==='1'&&token===chatMotionLoopToken)await playChatMotionOnce();
  }
  chatMotionLoopRunning=false;
}
function setChatMotion(){
  if($('chatMotionEnabled').value==='1')runChatMotionLoop();
  else stopChatMotion();
}
function stopChatMotion(){
  $('chatMotionEnabled').value='0';
  chatMotionLoopToken++;
  chatMotionPlayToken++;
  setChatMotionState('已停止');
}
function renderEmotion(s){
  $('emotionTitle').textContent=s.title||'-';
  $('emotionVariant').textContent=s.variantTitle||s.variant||'-';
  $('emotionStep').textContent=s.step||'-';
  $('emotionActive').textContent=s.active?'播放中':'空闲';
  $('emotionLightsState').textContent=s.lightsActive?'播放中':s.lightsEnabled?'开启':'关闭';
  if(typeof s.lightsEnabled==='boolean')setBoolValue('emotionLights',s.lightsEnabled);
}
async function applyEmotionServoPose(){
  const s=await api('/api/motion/pose',{yaw:+$('emotionServoYaw').value,pitch:+$('emotionServoPitch').value,speed:+$('emotionServoSpeed').value});
  syncMotionControls(s);syncEmotionServoControls(s);writeJson('emotionLog',s);refreshStatus(false)
}
async function applyEmotionServoPreset(mood,pitch,lookY){
  setValue('emotionServoYaw',0);setValue('emotionServoPitch',pitch);setValue('emotionServoSpeed',60);syncNumberControls();
  const motion=await api('/api/motion/pose',{yaw:0,pitch,speed:+$('emotionServoSpeed').value});
  const faceMood=await api('/api/face/set',{mood});
  const react=await api('/api/face/react',{lookX:0,lookY,audioLevel:0});
  syncMotionControls(motion);syncEmotionServoControls(motion);writeJson('emotionLog',{ok:motion.ok!==false,mood,pitch,motion,faceMood,react});refreshStatus(false)
}
async function refreshEmotion(){const s=await api('/api/emotion/status');renderEmotion(s);writeJson('emotionLog',s)}
async function playEmotion(name){const s=await api('/api/emotion/play',{name,lights:bool('emotionLights')});renderEmotion(s);writeJson('emotionLog',s);refreshStatus(false);refreshEmotionSounds()}
async function stopEmotion(){const s=await api('/api/emotion/stop',{});renderEmotion(s);writeJson('emotionLog',s)}
function renderEmotionSounds(s){
  const slots=s.slots||[];
  $('emotionSoundSlots').innerHTML=slots.map(slot=>`<div class="slot-row"><div class="slot-title"><b>${slot.title}</b><span>${slot.hasAudio?`${slot.uploadedName||slot.name} · ${Math.round((slot.audioBytes||0)/1024)}KB`:'未上传音效'}</span></div><select onchange="setEmotionSound('${slot.name}',this.value==='1')"><option value="1"${slot.enabled?' selected':''}>开</option><option value="0"${!slot.enabled?' selected':''}>关</option></select><button onclick="uploadEmotionSound('${slot.name}')">上传</button><button onclick="previewEmotionSound('${slot.name}')">试听</button></div>`).join('');
}
async function refreshEmotionSounds(){const s=await api('/api/emotion/sound-slots');renderEmotionSounds(s)}
async function setEmotionSound(slot,enabled){const s=await api('/api/emotion/sound-slot/set',{slot,enabled});renderEmotionSounds(s);writeJson('emotionLog',s)}
async function convertAudioFileToRawU8(file){
  const buffer=await file.arrayBuffer();
  if(file.name.toLowerCase().endsWith('.raw'))return new Uint8Array(buffer);
  const Ctx=window.AudioContext||window.webkitAudioContext;
  const ctx=new Ctx();
  const decoded=await ctx.decodeAudioData(buffer.slice(0));
  const targetRate=16000;
  const length=Math.max(1,Math.floor(decoded.duration*targetRate));
  const raw=new Uint8Array(length);
  const channels=Array.from({length:decoded.numberOfChannels},(_,i)=>decoded.getChannelData(i));
  for(let i=0;i<length;i++){
    const pos=i*decoded.sampleRate/targetRate;
    const index=Math.min(Math.floor(pos),decoded.length-1);
    const next=Math.min(index+1,decoded.length-1);
    const frac=pos-index;
    let sample=0;
    channels.forEach(ch=>{sample+=ch[index]*(1-frac)+ch[next]*frac});
    sample=Math.max(-1,Math.min(1,sample/channels.length));
    raw[i]=Math.max(0,Math.min(255,Math.round((sample*0.5+0.5)*255)));
  }
  await ctx.close?.();
  return raw;
}
async function uploadEmotionSound(slot){
  const file=$('emotionSoundUploadFile').files[0];
  if(!file){writeJson('emotionLog',{ok:false,error:'请选择音频文件'});return}
  try{
    const raw=await convertAudioFileToRawU8(file);
    const form=new FormData();
    const name=file.name.replace(/\.[^.]*$/,'')+'.raw';
    form.append('file',new Blob([raw],{type:'application/octet-stream'}),name);
    const res=await fetch(`/api/emotion/sound-slot/upload?slot=${encodeURIComponent(slot)}`,{method:'POST',body:form});
    const text=await res.text(); let payload; try{payload=JSON.parse(text)}catch{payload={ok:false,raw:text,status:res.status}};
    writeJson('emotionLog',payload); refreshEmotionSounds();
  }catch(error){writeJson('emotionLog',{ok:false,error:String(error)})}
}
async function previewEmotionSound(slot){const s=await api('/api/emotion/sound-slot/play',{slot});writeJson('emotionLog',s);refreshEmotionSounds()}
function catalogEffects(effects){return (effects||[]).map(effect=>`<i>${effect}</i>`).join('')}
function catalogAttr(value){return String(value||'').replace(/&/g,'&amp;').replace(/"/g,'&quot;').replace(/</g,'&lt;')}
function catalogInvokePayload(id){
  const payload={id};
  if(id==='behavior.robot_action.nod_yes'||id==='behavior.robot_action.shake_no'||id==='motion.robot_action.nod_yes'||id==='motion.robot_action.shake_no')payload.sound=false;
  return payload;
}
async function catalogInvokeAsset(id){
  const result=await api('/api/asset/invoke',catalogInvokePayload(id));
  writeJson('catalogLog',result);
  refreshStatus();
}
function wireCatalogInvokes(){
  document.querySelectorAll('[data-asset-id]').forEach(button=>{
    button.onclick=()=>catalogInvokeAsset(button.dataset.assetId);
  });
}
function catalogRow(entry){
  const title=entry.title||entry.id||'-';
  const owner=entry.owner?` · ${entry.owner}`:'';
  const meta=`${entry.id||'-'}${owner}`;
  const actions=entry.playable&&entry.category!=='logicAsset'?`<div class="catalog-actions"><button type="button" data-asset-id="${catalogAttr(entry.id)}">调用</button></div>`:'';
  return `<div class="catalog-row"><b>${title}</b><span>${meta}</span><div class="catalog-effects">${catalogEffects(entry.effects)}</div>${actions}</div>`;
}
function renderCatalog(catalog,conflicts){
  const categories=catalog.categories||{};
  const sounds=categories.soundAssets||catalog.soundAssets||[];
  const expressions=categories.expressionAssets||catalog.expressionAssets||[];
  const screens=categories.screenAssets||catalog.screenAssets||[];
  const motions=categories.motionAssets||catalog.motionAssets||[];
  const lights=categories.lightAssets||catalog.lightAssets||[];
  const behavior=categories.behaviorAssets||catalog.behaviorAssets||[];
  const deprecatedAssets=categories.deprecatedAssets||catalog.deprecatedAssets||[];
  const logic=categories.logicAssets||catalog.logicAssets||[];
  $('catalogSoundAssets').innerHTML=sounds.map(catalogRow).join('');
  $('catalogExpressionAssets').innerHTML=expressions.map(catalogRow).join('');
  $('catalogScreenAssets').innerHTML=screens.map(catalogRow).join('');
  $('catalogMotionAssets').innerHTML=motions.map(catalogRow).join('');
  $('catalogLightAssets').innerHTML=lights.map(catalogRow).join('');
  $('catalogBehaviorAssets').innerHTML=behavior.map(catalogRow).join('');
  $('catalogDeprecatedAssets').innerHTML=deprecatedAssets.length?deprecatedAssets.map(catalogRow).join(''):'<div class="catalog-row"><b>无</b><span>当前没有弃用资产</span></div>';
  $('catalogLogicAssets').innerHTML=logic.map(catalogRow).join('');
  $('catalogConflicts').innerHTML=(conflicts.conflicts||[]).map(item=>`<div class="catalog-row"><b>${item.source} → ${item.target}</b><span>${item.reason||''}</span></div>`).join('');
  wireCatalogInvokes();
}
async function refreshCatalog(){
  const catalog=await api('/api/catalog');
  const conflicts=await api('/api/catalog/conflicts');
  renderCatalog(catalog,conflicts);
  writeJson('catalogLog',{catalog,conflicts});
}
function renderTriggers(s){
  setBoolValue('triggerHeadTouchHappy',!!s.headTouchHappyEnabled);
  setBoolValue('triggerHeadTouchCombo',!!s.headTouchComboEnabled);
  setBoolValue('triggerScreenTouchAngry',!!s.screenTouchAngryEnabled);
  setBoolValue('triggerImuMotion',!!s.imuMotionEnabled);
  setBoolValue('randomEmotionAction',!!s.randomEmotionEnabled);
  setValue('headTouchThreshold',s.headTouchThreshold);
  setValue('angryShakeAccelDeltaG',s.angryShakeAccelDeltaG);
  setValue('angryShakeGyroDps',s.angryShakeGyroDps);
  setValue('upsideDownGravityDotThreshold',s.upsideDownGravityDotThreshold);
  setValue('randomEmotionMinInterval',s.randomEmotionMinIntervalMs);
  setValue('randomEmotionMaxInterval',s.randomEmotionMaxIntervalMs);
  $('triggerCondition').textContent=s.condition||'摸头';
  $('triggerAction').textContent=s.action||'舒服点点头';
  $('triggerState').textContent=s.headTouchHappyEnabled?(s.headTouchPressed?'摸到':'待命'):'关闭';
  $('screenTouchTriggerState').textContent=s.screenTouchAngryEnabled?(s.screenTouchPressed?'按下':'待命'):'关闭';
  $('screenTouchTriggerPoint').textContent=s.screenTouchPressed?`${s.screenTouchX},${s.screenTouchY}`:'-';
  $('touchPeak').textContent=s.headTouchPeak??'-';
  $('touchThresholdState').textContent=s.headTouchThreshold??'-';
  $('triggerCooldown').textContent=s.headTouchComboRepeatWindowRemainingMs?`五连摸续演窗口 ${Math.ceil(s.headTouchComboRepeatWindowRemainingMs/1000)}秒`:s.headTouchComboCooldownMs?`五连摸疯狂点头冷却 ${Math.ceil(s.headTouchComboCooldownMs/1000)}秒`:s.cooldownMs?`${Math.ceil(s.cooldownMs/1000)}秒`:'无冷却';
  $('screenTouchCooldown').textContent=s.screenTouchCooldownMs?`${Math.ceil(s.screenTouchCooldownMs/1000)}秒`:'无冷却';
  $('imuTriggerState').textContent=s.imuMotionEnabled?(s.imuAvailable?'待命':'无IMU'):'关闭';
  $('imuShakeStrength').textContent=typeof s.imuShakeAccelerationDeltaG==='number'?`${s.imuShakeAccelerationDeltaG.toFixed(3)}g`:'-';
  $('imuGyroStrength').textContent=typeof s.imuGyroMagnitudeDps==='number'?`${Math.round(s.imuGyroMagnitudeDps)}dps`:'-';
  const gravityDot=typeof s.imuGravityDot==='number'?s.imuGravityDot.toFixed(2):'-';
  $('imuUpsideDownState').textContent=`${s.imuUpsideDown?'倒置':'正向'} · 匹配 ${gravityDot}`;
  $('imuThresholdState').textContent=`生气 ${s.angryShakeAccelDeltaG??'-'}g/${s.angryShakeGyroDps??'-'}dps · 倒置 ${s.upsideDownGravityDotThreshold??'-'}`;
  $('randomEmotionState').textContent=s.randomEmotionEnabled?'开启':'关闭';
  $('randomEmotionRange').textContent=`${Math.round((s.randomEmotionMinIntervalMs||0)/1000)}-${Math.round((s.randomEmotionMaxIntervalMs||0)/1000)}秒`;
  $('randomEmotionNext').textContent=s.nextRandomEmotionInMs?`${Math.ceil(s.nextRandomEmotionInMs/1000)}秒`:'-';
  $('triggerLast').textContent=s.lastTrigger==='headTouchComfortableNod'||s.lastTrigger==='headTouchHappy'?'摸头触发舒服点点头':s.lastTrigger==='headTouchComboSpin'?'摸头五连触发疯狂点头':s.lastTrigger==='screenTouchAngry'?'摸屏幕触发生气':s.lastTrigger==='imuAngryShake'?'倒置大摇生气':s.lastTrigger==='randomEmotion'?'随机情绪动作':'无';
  $('randomEmotionLast').textContent=s.lastRandomEmotion&&s.lastRandomEmotion!=='none'?s.lastRandomEmotion:'-';
}
async function refreshTriggers(){const s=await api('/api/triggers/status');renderTriggers(s);writeJson('triggerLog',s)}
async function setTriggers(){const s=await api('/api/triggers/set',{headTouchHappyEnabled:bool('triggerHeadTouchHappy'),headTouchComboEnabled:bool('triggerHeadTouchCombo'),screenTouchAngryEnabled:bool('triggerScreenTouchAngry'),headTouchThreshold:+$('headTouchThreshold').value,imuMotionEnabled:bool('triggerImuMotion'),angryShakeAccelDeltaG:+$('angryShakeAccelDeltaG').value,angryShakeGyroDps:+$('angryShakeGyroDps').value,upsideDownGravityDotThreshold:+$('upsideDownGravityDotThreshold').value,randomEmotionEnabled:bool('randomEmotionAction'),randomEmotionMinIntervalMs:+$('randomEmotionMinInterval').value,randomEmotionMaxIntervalMs:+$('randomEmotionMaxInterval').value});renderTriggers(s);writeJson('triggerLog',s)}
function renderClock(s){
  if(!s)return;
  setBoolValue('subtitleDateTime',!!s.showDateTimeSubtitle);
  setBoolValue('nightTriggerEnabled',!!s.nightEnabled);
  setValue('nightStartHour',s.nightStartHour);
  setValue('nightEndHour',s.nightEndHour);
  setValue('nightSleepAfterMinutes',Math.max(1,Math.round((s.sleepAfterMs||1800000)/60000)));
  $('clockDateTime').textContent=s.dateTimeText||'-';
  $('clockNightState').textContent=s.nightEnabled?(s.isNight?'夜晚':'白天'):'关闭';
  $('clockSleepState').textContent=s.sleeping?'睡眠中':'清醒';
  $('clockInteractionAge').textContent=s.interactionAgeMs?`${Math.ceil(s.interactionAgeMs/1000)}秒`:'刚刚';
}
async function refreshClock(){const s=await api('/api/clock/status');renderClock(s);writeJson('triggerLog',s)}
async function setClock(){const s=await api('/api/clock/set',{showDateTimeSubtitle:bool('subtitleDateTime'),nightEnabled:bool('nightTriggerEnabled'),nightStartHour:+$('nightStartHour').value,nightEndHour:+$('nightEndHour').value,sleepAfterMs:+$('nightSleepAfterMinutes').value*60000});renderClock(s);writeJson('triggerLog',s);refreshStatus(false)}
function rgbInputForSide(side){return side==='left'?'rgbLeft':side==='right'?'rgbRight':'rgbAll'}
function rgbEffectPayload(){return {brightness:+$('rgbBrightness').value,breathing:bool('rgbBreathing'),marquee:bool('rgbMarquee'),rainbow:bool('rgbRainbow')}}
function rgbStandbyPayload(payload){return {...payload,standby:true}}
async function setRgb(side='all'){const c=$(rgbInputForSide(side)).value;if(side==='all'){$('rgbLeft').value=c;$('rgbRight').value=c}const s=await api('/api/rgb',rgbStandbyPayload({color:c,side,...rgbEffectPayload()}));syncRgbControls(s.rgb||s);log(s)}
async function setRgbEffects(){const s=await api('/api/rgb',rgbStandbyPayload(rgbEffectPayload()));syncRgbControls(s.rgb||s);log(s);refreshStatus(false)}
async function setRgbBreathing(){return setRgbEffects()}
async function applyRgbPreset(){const s=await api('/api/rgb',rgbStandbyPayload({preset:$('rgbPreset').value}));syncRgbControls(s.rgb||s);log(s);refreshStatus(false)}
function applySwatchColor(btn){$(btn.dataset.target).value=btn.dataset.color;if(btn.dataset.apply==='rgb')setRgb(btn.dataset.side||'all');else if(btn.dataset.apply==='subtitle')setSubtitle();else if(btn.dataset.apply==='assistantStatus')setAssistantStatus();else sendFace()}
async function refreshStatus(syncControls=false){const s=await api('/api/status'); renderStatusSummary(s); $('statusDetail').textContent=JSON.stringify(s,null,2); if(s.clock)renderClock(s.clock); if(s.rgb)syncRgbControls(s.rgb); if(syncControls){syncFaceControls(s);syncSubtitleControls(s);syncMotionControls(s);syncEmotionServoControls(s)}}
function log(x){$('log').textContent=JSON.stringify(x,null,2)}
bindNumberControl('eyeWidthScale'); bindNumberControl('eyeHeightScale'); bindNumberControl('cornerRadius'); bindNumberControl('eyeOffsetX'); bindNumberControl('eyeOffsetY'); bindNumberControl('eyeSpacing'); bindNumberControl('glowStrength'); bindNumberControl('scanlineSpacing'); bindNumberControl('scanlineAlpha'); bindNumberControl('lookX'); bindNumberControl('lookY'); bindNumberControl('audioLevel'); bindNumberControl('yaw'); bindNumberControl('pitch'); bindNumberControl('speed'); bindNumberControl('velocity'); bindNumberControl('emotionServoYaw'); bindNumberControl('emotionServoPitch'); bindNumberControl('emotionServoSpeed'); bindNumberControl('restPitch'); bindNumberControl('rgbBrightness'); bindNumberControl('toneFrequency'); bindNumberControl('toneDuration'); bindNumberControl('speakerVolume'); bindNumberControl('apiVoiceVolume'); bindNumberControl('micSamples'); bindNumberControl('micSampleRate');
document.querySelectorAll('.swatch').forEach(btn=>btn.addEventListener('click',()=>applySwatchColor(btn)));
$('cameraPreview')?.addEventListener('load',()=>$('cameraPreviewStatus').textContent='已刷新 '+new Date().toLocaleTimeString());
$('cameraPreview')?.addEventListener('error',()=>$('cameraPreviewStatus').textContent='预览不可用或检测中');
document.querySelectorAll('input,select').forEach(el=>el.addEventListener('change',()=>{
  const isIdleControl=el.id==='idleEnabled';
  const isRestPitchControl=['restPitch','restPitchNum'].includes(el.id);
  const isEmotionServoControl=['emotionServoYaw','emotionServoYawNum','emotionServoPitch','emotionServoPitchNum','emotionServoSpeed','emotionServoSpeedNum'].includes(el.id);
  const isTriggerControl=['triggerHeadTouchHappy','triggerHeadTouchCombo','triggerScreenTouchAngry','headTouchThreshold','triggerImuMotion','angryShakeAccelDeltaG','angryShakeGyroDps','upsideDownGravityDotThreshold','randomEmotionAction','randomEmotionMinInterval','randomEmotionMaxInterval'].includes(el.id);
  const isClockControl=['subtitleDateTime','nightTriggerEnabled','nightStartHour','nightEndHour','nightSleepAfterMinutes'].includes(el.id);
  const isCameraControl=['cameraEnabled','cameraCaptureInterval','cameraDetectTimeout','cameraReturnSettle','cameraConfirmSettle','cameraLockHold','cameraCooldown','cameraCenterDeadZoneX','cameraCenterDeadZoneY','cameraYawStepMax','cameraPitchStepMax','cameraCatchphraseAsset','cameraListenAfterCatchphrase','cameraFeedbackSubtitleEnabled'].includes(el.id);
  const isSubtitleControl=['subtitleText','subtitleEnabled','subtitleColor'].includes(el.id);
  const isAssistantStatusControl=['assistantStatusEnabled','assistantDisplayEnabled','assistantBatteryEnabled','assistantStatusColor','assistantPollIntervalMs','assistantStatusUrl','assistantLowBatteryPercent'].includes(el.id);
  const isAssistantSourceControl=['assistantEventTtlSeconds','assistantLowCodexPercent'].includes(el.id);
  const isDebugSubtitleControl=['debugSubtitleListening','debugSubtitleApiResult'].includes(el.id);
  const isListeningControl=['listeningVoiceWakeEnabled','listeningPowerButtonWakeEnabled','listeningHostCallbackUrl','listeningTimeoutMs'].includes(el.id);
  const isScreenControl=['screenAsset','screenDuration'].includes(el.id);
  const isChatMotionControl=['chatMotionEnabled','chatMotionStyle'].includes(el.id);
  const isApiVoiceVolumeControl=['apiVoiceVolume','apiVoiceVolumeNum'].includes(el.id);
  const isRgbControl=['rgbBrightness','rgbBrightnessNum','rgbBreathing','rgbMarquee','rgbRainbow','rgbPreset'].includes(el.id);
  if(isIdleControl)toggleIdle();
  else if(isRestPitchControl)setRestPitch();
  else if(isEmotionServoControl)applyEmotionServoPose();
  else if(isTriggerControl)setTriggers();
  else if(isClockControl)setClock();
  else if(isCameraControl)setCamera();
  else if(el.id==='proximityThreshold')refreshProximity();
  else if(isSubtitleControl)setSubtitle();
  else if(isAssistantSourceControl)setAssistantSourceSettings();
  else if(isAssistantStatusControl)setAssistantStatus();
  else if(isDebugSubtitleControl)setDebugSubtitle();
  else if(isListeningControl)saveListening();
  else if(isScreenControl)setScreenAsset();
  else if(isChatMotionControl)setChatMotion();
  else if(isApiVoiceVolumeControl)setApiVoiceVolume();
  else if(el.id==='rgbPreset')applyRgbPreset();
  else if(isRgbControl)setRgbEffects();
  else if(el.closest('section')?.querySelector('h2')?.textContent==='眼睛控制台')el.id==='mood'?selectMood():(el.id.startsWith('look')||el.id.startsWith('audio')?sendReact():sendFace())
}));
setInterval(()=>refreshStatus(false),5000); setInterval(()=>refreshAssistantStatus(),5000); setInterval(()=>refreshScreenAsset(),2500); setInterval(()=>refreshListening(),2500); setInterval(()=>refreshIdle(),5000); setInterval(()=>refreshEmotion(),3000); setInterval(()=>refreshTriggers(),3000); setInterval(()=>refreshClock(),3000); setInterval(()=>refreshProximity(),2500); setInterval(()=>refreshCamera(),2500); setInterval(()=>refreshCameraPreview(),5000); setInterval(()=>refreshEmotionSounds(),5000); setInterval(()=>refreshCatalog(),7000); refreshStatus(true); refreshAssistantStatus(); refreshScreenAsset(); refreshDebugSubtitle(); refreshListening(); refreshIdle(); refreshEmotion(); refreshTriggers(); refreshClock(); refreshProximity(); refreshCamera(); refreshCameraPreview(); refreshApiVoiceVolume(); refreshEmotionSounds(); refreshCatalog(); setChatMotion();
</script>
</body>
</html>
)HTML";
}
