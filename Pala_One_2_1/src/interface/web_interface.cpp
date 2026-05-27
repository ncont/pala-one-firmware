#include "src/interface/web_interface.h"

#include "src/config.h"
#include "src/hal/wifi_provisioning.h"
#include "src/state.h"               // server
#include "src/storage/wifi_creds.h"  // WifiCreds::ssid() — pre-populate for sessionInfo()

// Definition of the singleton used by WifiScreen and web routes.
WebInterface g_webInterface;

// How long to wait for an STA association before falling back to AP.
static constexpr uint32_t kStaTimeoutMs = 5000;

// ---- ExternalInterface overrides --------------------------------------------

void WebInterface::begin() {
  // Tell WifiProvisioning to keep its hands off the radio for the duration of
  // the session — set BEFORE wifiStaBegin so a same-tick
  // WifiProvisioning::loop() can't race the WiFi.begin() we're about to fire.
  WifiProvisioning::notifyUploadSession(true);

  if (wifiStaBegin()) {
    phase_        = Phase::ConnectingSta;
    staStartedMs_ = millis();
    net_.staSsid  = WifiCreds::ssid();  // pre-populate so sessionInfo() has a name to show
  } else {
    // No stored creds — straight to AP.
    net_ = wifiBeginAccessPoint();
    enterReady();
  }
}

void WebInterface::end() {
  server.stop();
  wifiEnd();
  WifiProvisioning::notifyUploadSession(false);
  phase_ = Phase::Ready;  // reset so the next begin() starts clean
}

void WebInterface::poll() {
  if (phase_ == Phase::ConnectingSta) {
    WifiStaResult r = wifiStaPoll(net_);
    if (r == WifiStaResult::Connected) {
      enterReady();
      return;
    }
    if (r == WifiStaResult::Failed ||
        (uint32_t)(millis() - staStartedMs_) > kStaTimeoutMs) {
      fallbackToAp();
    }
    return;
  }

  server.handleClient();
}

ExternalInterface::State WebInterface::state() const {
  switch (phase_) {
    case Phase::ConnectingSta: return State::Provisioning;
    case Phase::Ready:         return State::Ready;
  }
  return State::Idle;
}

ExternalInterface::SessionInfo WebInterface::sessionInfo() const {
  SessionInfo info;
  info.networkName      = net_.staSsid.length() > 0 ? net_.staSsid
                                                     : String(net_.apSsid);
  info.networkCredential = net_.mode == WifiMode::Station ? String()
                                                           : String(net_.apPass);
  info.primaryAddress   = net_.primaryUrl;
  info.secondaryAddress = net_.fallbackUrl;
  return info;
}

bool WebInterface::idleTimedOut() const {
  return phase_ == Phase::Ready &&
         (uint32_t)(millis() - startedMs_) > UPLOAD_AUTO_EXIT_MS;
}

void WebInterface::forceReady() {
  if (phase_ == Phase::ConnectingSta) {
    fallbackToAp();
  }
}

// ---- Private helpers --------------------------------------------------------

void WebInterface::enterReady() {
  phase_     = Phase::Ready;
  startedMs_ = millis();
  server.begin();
}

void WebInterface::fallbackToAp() {
  wifiStaAbort();
  net_ = wifiBeginAccessPoint();
  enterReady();
}
