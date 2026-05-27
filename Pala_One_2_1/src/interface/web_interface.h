#ifndef PALA_INTERFACE_WEB_INTERFACE_H
#define PALA_INTERFACE_WEB_INTERFACE_H

#include "src/hal/wifi.h"
#include "src/interface/external_interface.h"

// WiFi + HTTP implementation of ExternalInterface.
//
// Owns the full WiFi session lifecycle that previously lived in WifiScreen:
//   - STA connection attempt with timeout and AP fallback
//   - HTTP server start / stop
//   - Idle-timeout tracking
//   - WifiProvisioning coordination (notifyUploadSession)
//
// WifiScreen calls begin()/poll()/end() and reads sessionInfo()/idleTimedOut()
// for its display — it no longer touches WiFi or the server directly.
class WebInterface : public ExternalInterface {
public:
  void        begin()                override;
  void        end()                  override;
  void        poll()                 override;
  State       state()      const     override;
  SessionInfo sessionInfo() const    override;
  bool        idleTimedOut() const   override;

  // Skip STA connecting — fall back to AP and enter Ready immediately.
  void        forceReady()           override;

private:
  enum class Phase { ConnectingSta, Ready };

  Phase    phase_        = Phase::Ready;
  uint32_t startedMs_   = 0;   // set when entering Ready (for auto-exit timer)
  uint32_t staStartedMs_ = 0;  // set when STA attempt begins (for 5s timeout)
  WifiSession net_;             // internal WiFi session state

  void enterReady();
  void fallbackToAp();
};

extern WebInterface g_webInterface;

#endif  // PALA_INTERFACE_WEB_INTERFACE_H
