#ifndef PALA_UI_SCREENS_WIFI_SCREEN_H
#define PALA_UI_SCREENS_WIFI_SCREEN_H

#include "src/interface/web_interface.h"
#include "src/ui/screen.h"

class WifiScreen : public Screen {
public:
  void onEnter() override;
  void onButton(const ButtonEvent& e) override;
  void draw() override;
  void onIdleTick() override;

  // The Wi-Fi session (AP or STA) can't keep running while the device
  // deep-sleeps.
  bool allowSleep() const override { return false; }

private:
  ExternalInterface::State lastState_ = ExternalInterface::State::Idle;

  void stopSessionToLibrary();
};

extern WifiScreen g_wifiScreen;

#endif  // PALA_UI_SCREENS_WIFI_SCREEN_H
