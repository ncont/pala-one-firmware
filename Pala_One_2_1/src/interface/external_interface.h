#ifndef PALA_INTERFACE_EXTERNAL_INTERFACE_H
#define PALA_INTERFACE_EXTERNAL_INTERFACE_H

#include <Arduino.h>

// Abstract contract for any external communication channel (WiFi/HTTP today,
// BLE or others in the future). A concrete implementation owns its transport
// lifecycle and exposes a uniform surface to the UI layer.
class ExternalInterface {
public:
  enum class State {
    Idle,         // not running
    Provisioning, // transport starting (STA connecting, BLE advertising, …)
    Ready,        // clients can connect and issue requests
  };

  // Transport-agnostic session data for display. All fields are optional:
  // empty strings mean "not applicable for this transport / mode".
  //
  //   WiFi STA:  networkName=SSID, credential="",       primaryAddress=URL
  //   WiFi AP:   networkName=SSID, credential=password, primaryAddress=URL
  //   BLE:       networkName=name, credential=PIN,       primaryAddress=""
  struct SessionInfo {
    String networkName;       // network / device to connect to
    String networkCredential; // password or pairing code — empty if not needed
    String primaryAddress;    // where to reach the interface (URL, …)
    String secondaryAddress;  // optional fallback — empty if none
  };

  virtual void        begin()                   = 0;
  virtual void        end()                     = 0;
  virtual void        poll()                    = 0;
  virtual State       state()       const       = 0;
  virtual SessionInfo sessionInfo() const       = 0;
  virtual bool        idleTimedOut() const      = 0;

  // Skip the Provisioning phase and enter Ready immediately using a fallback
  // mode (e.g. AP for WiFi). No-op if already Ready.
  virtual void        forceReady()              = 0;

  virtual ~ExternalInterface() = default;
};

#endif  // PALA_INTERFACE_EXTERNAL_INTERFACE_H
