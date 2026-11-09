#pragma once

#include <Arduino.h>
#include <WiFi.h>

class RobotWifiManager {
 public:
  void begin(const char* fallbackSsid, const char* fallbackPassword, const char* mdnsName);
  bool connectBoot(uint32_t timeoutMs = 15000);

  bool openSetupPortal(const String& reason);
  void closeSetupPortal();
  void requestSetupPortalClose(uint32_t delayMs = 1000);
  void update(uint32_t nowMs);
  bool setupPortalActive() const { return setupPortalActive_; }
  uint32_t displayRevision() const { return displayRevision_; }

  String statusJson() const;
  String scanJson();
  String connectJson(const String& ssid, const String& password, uint32_t timeoutMs, int& httpStatus);
  String forgetJson();
  String closeJson();
  String setupPageHtml() const;
  String firmwareAddress() const;
  String wifiStatusLabel() const;
  String currentSsid() const;
  String configuredSsid() const;

  const String& lastScanSummary() const { return lastScanSummary_; }
  const String& lastStatusSummary() const { return lastStatusSummary_; }
  const char* statusName(wl_status_t status) const;

 private:
  void configureStation();
  bool loadSavedCredentials(String& ssid, String& pass);
  void saveCredentials(const String& ssid, const String& pass);
  void clearSavedCredentials();
  bool validateCredentials(const String& ssid, const String& pass, String& error) const;
  bool connectToNetwork(const String& ssid, const String& pass, uint32_t timeoutMs);
  bool startMdns();
  void scanForSsid(const String& ssid);
  void bumpDisplayRevision();
  String statusJsonWithExtra(const String& extra, bool ok = true) const;
  String errorJson(const String& error, const String& extra = "") const;
  String jsonQuote(const String& value) const;

  String fallbackSsid_;
  String fallbackPassword_;
  String mdnsName_ = "desktop-robot";
  String activeSsid_;
  String credentialSource_ = "fallback";
  String setupReason_ = "";
  String lastScanSummary_ = "SSID entered manually";
  String lastStatusSummary_ = "idle";
  int lastAuth_ = -1;
  bool savedCredentialsAvailable_ = false;
  bool setupPortalActive_ = false;
  bool setupPortalClosePending_ = false;
  uint32_t setupPortalCloseAtMs_ = 0;
  uint32_t displayRevision_ = 0;
};
