#include "wifi_manager.h"

#include <esp_wifi.h>
#include <ESPmDNS.h>
#include <Preferences.h>

#if __has_include("config.local.h")
#include "config.local.h"
#endif
#include "config.example.h"

namespace {

constexpr uint32_t WIFI_CONNECT_POLL_MS = 250;
constexpr uint32_t WIFI_RESTORE_TIMEOUT_MS = 6000;

Preferences prefs_;

}  // namespace

void RobotWifiManager::begin(const char* fallbackSsid, const char* fallbackPassword, const char* mdnsName) {
  fallbackSsid_ = fallbackSsid ? String(fallbackSsid) : "";
  fallbackPassword_ = fallbackPassword ? String(fallbackPassword) : "";
  mdnsName_ = mdnsName ? String(mdnsName) : "desktop-robot";
  prefs_.begin("robot_wifi", false);
  String savedSsid;
  String savedPass;
  savedCredentialsAvailable_ = loadSavedCredentials(savedSsid, savedPass);
}

void RobotWifiManager::configureStation() {
  WiFi.mode(setupPortalActive_ ? WIFI_AP_STA : WIFI_STA);
  // The ESP-IDF world-safe default may only scan channels 1–11. Explicitly
  // support the local 2.4 GHz range (1–13) before trying the saved network.
  esp_wifi_set_country_code("CN", true);
  WiFi.setSleep(false);
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  WiFi.setHostname(mdnsName_.c_str());
}

bool RobotWifiManager::loadSavedCredentials(String& ssid, String& pass) {
  ssid = prefs_.getString("ssid", "");
  pass = prefs_.getString("pass", "");
  ssid.trim();
  return ssid.length() > 0;
}

void RobotWifiManager::saveCredentials(const String& ssid, const String& pass) {
  prefs_.putString("ssid", ssid);
  prefs_.putString("pass", pass);
  savedCredentialsAvailable_ = true;
}

void RobotWifiManager::clearSavedCredentials() {
  prefs_.remove("ssid");
  prefs_.remove("pass");
  savedCredentialsAvailable_ = false;
}

bool RobotWifiManager::validateCredentials(const String& ssid, const String& pass, String& error) const {
  if (!ssid.length()) {
    error = "empty ssid";
    return false;
  }
  if (ssid.length() > 32) {
    error = "ssid too long";
    return false;
  }
  if (pass.length() > 64) {
    error = "password too long";
    return false;
  }
  return true;
}

const char* RobotWifiManager::statusName(wl_status_t status) const {
  switch (status) {
    case WL_IDLE_STATUS: return "IDLE";
    case WL_NO_SSID_AVAIL: return "NO_SSID";
    case WL_SCAN_COMPLETED: return "SCAN_DONE";
    case WL_CONNECTED: return "CONNECTED";
    case WL_CONNECT_FAILED: return "CONNECT_FAILED";
    case WL_CONNECTION_LOST: return "CONNECTION_LOST";
    case WL_DISCONNECTED: return "DISCONNECTED";
    default: return "UNKNOWN";
  }
}

String RobotWifiManager::jsonQuote(const String& value) const {
  String out = "\"";
  for (uint16_t i = 0; i < value.length(); ++i) {
    char c = value.charAt(i);
    if (c == '"' || c == '\\') out += "\\";
    if (c == '\n') {
      out += "\\n";
    } else if (c == '\r') {
      out += "\\r";
    } else {
      out += c;
    }
  }
  out += "\"";
  return out;
}

void RobotWifiManager::bumpDisplayRevision() {
  ++displayRevision_;
}

bool RobotWifiManager::startMdns() {
  MDNS.end();
  if (!MDNS.begin(mdnsName_.c_str())) return false;
  MDNS.addService("http", "tcp", 80);
  return true;
}

bool RobotWifiManager::openSetupPortal(const String& reason) {
  // A manual setup request may follow a failed STA attempt. Give the radio a
  // clean AP+STA transition so the hotspot is discoverable on nearby phones.
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_AP_STA);
  delay(150);
  bool started = WiFi.softAP(ROBOT_AP_SSID, ROBOT_AP_PASSWORD);
  setupPortalActive_ = started;
  setupReason_ = reason;
  if (started) {
    if (reason != "connect-failed") {
      lastStatusSummary_ = "Setup AP: " + WiFi.softAPIP().toString();
    }
  } else {
    lastStatusSummary_ = "Setup AP failed";
  }
  bumpDisplayRevision();
  return started;
}

void RobotWifiManager::closeSetupPortal() {
  if (!setupPortalActive_) return;
  WiFi.softAPdisconnect(true);
  setupPortalActive_ = false;
  setupPortalClosePending_ = false;
  setupPortalCloseAtMs_ = 0;
  setupReason_ = "";
  if (WiFi.status() == WL_CONNECTED) WiFi.mode(WIFI_STA);
  bumpDisplayRevision();
}

void RobotWifiManager::requestSetupPortalClose(uint32_t delayMs) {
  if (!setupPortalActive_) return;
  setupPortalClosePending_ = true;
  setupPortalCloseAtMs_ = millis() + delayMs;
  lastStatusSummary_ = "Setup hotspot closing";
  bumpDisplayRevision();
}

void RobotWifiManager::update(uint32_t nowMs) {
  if (!setupPortalClosePending_) return;
  if (static_cast<int32_t>(nowMs - setupPortalCloseAtMs_) < 0) return;
  closeSetupPortal();
}

void RobotWifiManager::scanForSsid(const String& ssid) {
  lastScanSummary_ = "WiFi scanning";
  int networkCount = WiFi.scanNetworks(false, true);
  int bestRssi = -999;
  int bestChannel = -1;
  int bestAuth = -1;
  for (int i = 0; i < networkCount; ++i) {
    if (WiFi.SSID(i) == ssid && WiFi.RSSI(i) > bestRssi) {
      bestRssi = WiFi.RSSI(i);
      bestChannel = WiFi.channel(i);
      bestAuth = (int)WiFi.encryptionType(i);
    }
  }
  lastAuth_ = bestAuth;
  if (bestChannel >= 0) {
    lastScanSummary_ = "SSID found rssi=" + String(bestRssi) + " ch=" + String(bestChannel) + " auth=" + String(bestAuth);
  } else {
    lastScanSummary_ = "SSID not found; scanned=" + String(networkCount);
  }
  WiFi.scanDelete();
  bumpDisplayRevision();
}

bool RobotWifiManager::connectToNetwork(const String& ssid, const String& pass, uint32_t timeoutMs) {
  configureStation();
  WiFi.disconnect(false, false);
  delay(100);
  if (!pass.length()) {
    WiFi.begin(ssid.c_str());
  } else {
    WiFi.begin(ssid.c_str(), pass.c_str());
  }

  uint32_t startedMs = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startedMs < timeoutMs) {
    delay(WIFI_CONNECT_POLL_MS);
  }

  if (WiFi.status() == WL_CONNECTED) {
    activeSsid_ = ssid;
    bool mdnsStarted = startMdns();
    lastStatusSummary_ = "IP: " + WiFi.localIP().toString();
    if (mdnsStarted) {
      lastStatusSummary_ += " mDNS: " + mdnsName_ + ".local";
    }
    bumpDisplayRevision();
    return true;
  }

  wl_status_t status = WiFi.status();
  lastStatusSummary_ = String("WiFi failed: ") + statusName(status) + " (" + String((int)status) + ")";
  bumpDisplayRevision();
  return false;
}

bool RobotWifiManager::connectBoot(uint32_t timeoutMs) {
  String ssid;
  String pass;
  bool usingSavedCredentials = loadSavedCredentials(ssid, pass);
  if (usingSavedCredentials) {
    credentialSource_ = "saved";
    savedCredentialsAvailable_ = true;
  } else {
    ssid = fallbackSsid_;
    pass = fallbackPassword_;
    credentialSource_ = "fallback";
    savedCredentialsAvailable_ = false;
  }
  activeSsid_ = ssid;
  lastScanSummary_ = "SSID entered manually";
  if (connectToNetwork(ssid, pass, timeoutMs)) return true;

  // A stale saved network must not prevent the local fallback configuration
  // from reconnecting the robot after a firmware update.
  if (usingSavedCredentials && fallbackSsid_.length() && fallbackSsid_ != ssid) {
    credentialSource_ = "fallback";
    activeSsid_ = fallbackSsid_;
    return connectToNetwork(fallbackSsid_, fallbackPassword_, timeoutMs);
  }
  return false;
}

String RobotWifiManager::statusJsonWithExtra(const String& extra, bool ok) const {
  bool connected = WiFi.status() == WL_CONNECTED;
  String ssid = connected ? WiFi.SSID() : activeSsid_;
  String json = "{\"ok\":";
  json += ok ? "true" : "false";
  json += ",\"connected\":" + String(connected ? "true" : "false");
  json += ",\"networkMode\":" + jsonQuote(setupPortalActive_ ? "setup" : "sta");
  json += ",\"setupPortalActive\":" + String(setupPortalActive_ ? "true" : "false");
  json += ",\"setupReason\":" + jsonQuote(setupReason_);
  json += ",\"ssid\":" + jsonQuote(ssid);
  json += ",\"ip\":" + jsonQuote(connected ? WiFi.localIP().toString() : String(""));
  json += ",\"deviceName\":\"Desktop Robot\"";
  json += ",\"firmwareUrl\":" + jsonQuote(firmwareAddress());
  json += ",\"mdnsUrl\":" + jsonQuote(String("http://") + mdnsName_ + ".local");
  json += ",\"apSsid\":" + jsonQuote(ROBOT_AP_SSID);
  json += ",\"apIp\":" + jsonQuote(setupPortalActive_ ? WiFi.softAPIP().toString() : String(""));
  json += ",\"setupUrl\":" + jsonQuote(setupPortalActive_ ? String("http://") + WiFi.softAPIP().toString() : String("/setup"));
  json += ",\"rssi\":" + String(connected ? WiFi.RSSI() : 0);
  json += ",\"status\":" + String((int)WiFi.status());
  json += ",\"statusName\":" + jsonQuote(statusName(WiFi.status()));
  json += ",\"credentialSource\":" + jsonQuote(credentialSource_);
  json += ",\"saved\":" + String(savedCredentialsAvailable_ ? "true" : "false");
  json += ",\"mdnsName\":" + jsonQuote(mdnsName_);
  json += ",\"lastScanSummary\":" + jsonQuote(lastScanSummary_);
  json += ",\"lastStatusSummary\":" + jsonQuote(lastStatusSummary_);
  if (extra.length()) json += extra;
  json += "}";
  return json;
}

String RobotWifiManager::statusJson() const {
  return statusJsonWithExtra("", true);
}

String RobotWifiManager::firmwareAddress() const {
  if (WiFi.status() == WL_CONNECTED) return String("http://") + WiFi.localIP().toString();
  if (setupPortalActive_) return String("http://") + WiFi.softAPIP().toString();
  return String("http://") + mdnsName_ + ".local";
}

String RobotWifiManager::wifiStatusLabel() const {
  return WiFi.status() == WL_CONNECTED ? "CONNECTED" : "NOT CONNECTED";
}

String RobotWifiManager::currentSsid() const {
  return WiFi.status() == WL_CONNECTED ? WiFi.SSID() : "";
}

String RobotWifiManager::configuredSsid() const {
  return activeSsid_;
}

String RobotWifiManager::errorJson(const String& error, const String& extra) const {
  return statusJsonWithExtra(",\"error\":" + jsonQuote(error) + extra, false);
}

String RobotWifiManager::scanJson() {
  int networkCount = WiFi.scanNetworks(false, true);
  String currentSsid = WiFi.status() == WL_CONNECTED ? WiFi.SSID() : activeSsid_;
  String json = "{\"ok\":true,\"networks\":[";
  for (int i = 0; i < networkCount; ++i) {
    if (i) json += ",";
    int auth = (int)WiFi.encryptionType(i);
    json += "{\"ssid\":" + jsonQuote(WiFi.SSID(i));
    json += ",\"rssi\":" + String(WiFi.RSSI(i));
    json += ",\"channel\":" + String(WiFi.channel(i));
    json += ",\"auth\":" + String(auth);
    json += ",\"encrypted\":" + String(auth == WIFI_AUTH_OPEN ? "false" : "true");
    json += ",\"current\":" + String(WiFi.SSID(i) == currentSsid ? "true" : "false");
    json += "}";
  }
  json += "],\"count\":" + String(networkCount) + "}";
  lastScanSummary_ = "scanned=" + String(networkCount);
  WiFi.scanDelete();
  bumpDisplayRevision();
  return json;
}

String RobotWifiManager::connectJson(const String& ssid, const String& password, uint32_t timeoutMs, int& httpStatus) {
  httpStatus = 200;
  String cleanSsid = ssid;
  String cleanPass = password;
  cleanSsid.trim();
  String validationError;
  if (!validateCredentials(cleanSsid, cleanPass, validationError)) {
    httpStatus = 400;
    return errorJson(validationError);
  }

  String previousSource = credentialSource_;
  String previousSsid = activeSsid_.length() ? activeSsid_ : WiFi.SSID();
  String previousPass = fallbackPassword_;
  if (previousSource == "saved") {
    String savedSsid;
    String savedPass;
    if (loadSavedCredentials(savedSsid, savedPass)) {
      previousSsid = savedSsid;
      previousPass = savedPass;
    }
  }
  String previousIp = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "";

  activeSsid_ = cleanSsid;
  lastScanSummary_ = "SSID entered manually";
  bool connected = connectToNetwork(cleanSsid, cleanPass, timeoutMs);
  if (!connected) {
    if (previousSsid.length()) {
      credentialSource_ = previousSource;
      activeSsid_ = previousSsid;
      connectToNetwork(previousSsid, previousPass, WIFI_RESTORE_TIMEOUT_MS);
    }
    if (!setupPortalActive_) openSetupPortal("connect-failed");
    return errorJson("connect failed", ",\"requestedSsid\":" + jsonQuote(cleanSsid));
  }

  saveCredentials(cleanSsid, cleanPass);
  credentialSource_ = "saved";
  bool changedNetwork = cleanSsid != previousSsid || WiFi.localIP().toString() != previousIp;
  String extra = ",\"changedNetwork\":";
  extra += changedNetwork ? "true" : "false";
  extra += ",\"requestedSsid\":" + jsonQuote(cleanSsid);
  // Keep the setup hotspot alive so the HTTP response reaches the phone and
  // the user can read the confirmed station address before leaving setup.
  setupReason_ = "connected";
  extra += ",\"setupPortalRetained\":" + String(setupPortalActive_ ? "true" : "false");
  bumpDisplayRevision();
  return statusJsonWithExtra(extra, true);
}

String RobotWifiManager::forgetJson() {
  clearSavedCredentials();
  credentialSource_ = "fallback";
  openSetupPortal("forgotten");
  return statusJsonWithExtra(",\"forgotten\":true", true);
}

String RobotWifiManager::closeJson() {
  requestSetupPortalClose();
  return statusJsonWithExtra(",\"closing\":true", true);
}

String RobotWifiManager::setupPageHtml() const {
  return R"HTML(<!doctype html>
<html lang="zh-CN">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Desktop Robot Wi-Fi</title>
<style>
*{box-sizing:border-box}body{margin:0;font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",sans-serif;background:#091318;color:#e9fbff}.wrap{max-width:620px;margin:0 auto;padding:20px}.panel{border:1px solid #244553;background:#0d1c24;border-radius:8px;padding:14px;margin-top:12px}h1{font-size:24px;margin:0 0 6px}.hint{display:none}.setup-intro{color:#9cc5d1;font-size:14px;line-height:1.45}.grid{display:grid;gap:10px}label{display:grid;gap:5px;font-size:13px;color:#b8d8e0}input,button{width:100%;min-width:0;border:1px solid #2d5968;border-radius:6px;background:#071017;color:#f1fdff;padding:10px;font:inherit}button{cursor:pointer;font-weight:700}.primary{background:#0f3d35;border-color:#35cdbb}.danger{background:#2b1318;border-color:#894a58}.notice{border:1px solid #2d5968;border-radius:6px;padding:10px;font-size:14px;line-height:1.4;color:#e9fbff}.row{display:grid;grid-template-columns:1fr 1fr;gap:8px}.status{display:grid;grid-template-columns:110px 1fr;gap:6px;font-size:14px}.status b{color:#8fb8c4}
</style>
</head>
<body>
<main class="wrap">
<h1>Desktop Robot Wi-Fi</h1>
<div class="setup-intro">Enter the Wi-Fi name and password you want Desktop Robot to use. The robot does not scan nearby networks.</div>
<div class="hint">SSID: Deskbot-Eiiii<br>PASS: 12345678<br>Open: http://192.168.4.1<br>Then choose the home Wi-Fi for the robot.</div>
<section class="panel grid">
<button onclick="refreshStatus()">Refresh status</button>
<label>SSID<input id="ssid" maxlength="32" placeholder="Wi-Fi name"></label>
<label>Password<input id="password" maxlength="64" type="password" placeholder="Leave blank for open Wi-Fi"></label>
<button class="primary" onclick="connectWifi()">Connect and save</button>
<div id="connectionNotice" class="notice" role="status">Checking connection…</div>
<button onclick="closeSetup()">Return to standby</button>
<button class="danger" onclick="forgetWifi()">Forget saved Wi-Fi</button>
</section>
<section class="panel">
<div class="status"><b>Device</b><span id="deviceName">Desktop Robot</span><b>Wi-Fi</b><span id="wifi">-</span><b>SSID</b><span id="currentSsid">-</span><b>Firmware</b><span id="firmware">-</span><b>Setup AP</b><span id="ap">-</span><b>Saved</b><span id="saved">-</span></div>
</section>
</main>
<script>
const $=id=>document.getElementById(id);
async function api(path,body){try{const res=await fetch(path,{method:body?'POST':'GET',headers:{'Content-Type':'application/json'},body:body?JSON.stringify(body):undefined});return await res.json()}catch{return{ok:false,error:'No response'}}}
function showNotice(message){$('connectionNotice').textContent=message}
function renderStatus(s){const connected=!!s.connected;const address=s.firmwareUrl||s.mdnsUrl||'-';$('deviceName').textContent=s.deviceName||'Desktop Robot';$('wifi').textContent=connected?'Connected':(s.setupPortalActive?'Setup hotspot active':'Not connected');$('currentSsid').textContent=s.ssid||'-';$('firmware').textContent=address;$('ap').textContent=s.setupPortalActive?`${s.apSsid} ${s.apIp}`:'closed';$('saved').textContent=s.saved?'yes':'no';showNotice(connected?'Connected successfully.':`Connection failed: ${s.statusName||s.error||'Unknown'}.`)}
async function refreshStatus(){renderStatus(await api('/api/setup/status'))}
async function connectWifi(){const ssid=$('ssid').value.trim();if(!ssid){showNotice('Connection failed: enter Wi-Fi name.');return}showNotice('Connecting…');renderStatus(await api('/api/setup/connect',{ssid,password:$('password').value}))}
async function forgetWifi(){renderStatus(await api('/api/setup/forget',{}))}
async function closeSetup(){renderStatus(await api('/api/setup/close',{}))}
refreshStatus();
</script>
</body>
</html>)HTML";
}
