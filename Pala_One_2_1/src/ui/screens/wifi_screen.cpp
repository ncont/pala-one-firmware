#include "src/ui/screens/wifi_screen.h"

#include "src/config.h"
#include "src/hal/display.h"
#include "src/hal/input.h"        // resetInputFrontend() — used on exit only
#include "src/interface/web_interface.h"
#include "src/state.h"
#include "src/storage/library.h"
#include "src/ui/font.h"
#include "src/ui/screens/library_screen.h"
#include "src/ui/widgets.h"
#include "src/web/apps_upload.h"  // resetAppUpload()
#include "src/web/upload.h"       // resetBookUpload() / resetSleepUpload()

// ---- Drawing --------------------------------------------------------------

void WifiScreen::draw() {
  prepareMenuFrame();
  int y = drawSectionHeader(D_UPLOAD_HEADER);

  const auto  ifState = g_webInterface.state();
  const auto  info    = g_webInterface.sessionInfo();

  if (ifState == ExternalInterface::State::Provisioning) {
    Font::useBold();
    u8g2.setCursor(MARGIN_X, y);
    u8g2.print(D_UPLOAD_CONNECTING);
    y += 14;

    Font::useBody();
    u8g2.setCursor(MARGIN_X, y);
    u8g2.print(info.networkName.c_str());
    y += 18;

    Font::useBody();
    u8g2.setCursor(MARGIN_X, y);
    u8g2.print(D_UPLOAD_HOTSPOT_HINT_L1);
    y += 14;
    u8g2.setCursor(MARGIN_X, y);
    u8g2.print(D_UPLOAD_HOTSPOT_HINT_L2);
  } else if (info.networkCredential.isEmpty()) {
    // Ready + no credential needed — connected via STA (user is already on the network)
    Font::useBold();
    u8g2.setCursor(MARGIN_X, y);
    u8g2.print(D_UPLOAD_CONNECTED);
    y += 14;

    Font::useBody();
    u8g2.setCursor(MARGIN_X, y);
    u8g2.print(info.networkName.c_str());
    y += 18;

    Font::useBold();
    u8g2.setCursor(MARGIN_X, y);
    u8g2.print(D_UPLOAD_OPEN);
    y += 14;

    Font::useBody();
    u8g2.setCursor(MARGIN_X, y);
    u8g2.print(info.primaryAddress.c_str());
    y += 14;

    if (info.secondaryAddress.length() > 0) {
      u8g2.setCursor(MARGIN_X, y);
      u8g2.print(info.secondaryAddress.c_str());
      y += 14;
    }
  } else {
    // Ready + credential required — user must join our network first (AP mode)
    Font::useBold();
    u8g2.setCursor(MARGIN_X, y);
    u8g2.print(D_UPLOAD_WIFI);
    y += 14;

    Font::useBody();
    u8g2.setCursor(MARGIN_X, y);
    u8g2.print(info.networkName.c_str());
    y += 16;

    Font::useBold();
    u8g2.setCursor(MARGIN_X, y);
    u8g2.print(D_UPLOAD_PASSWORD);
    y += 14;

    Font::useBody();
    u8g2.setCursor(MARGIN_X, y);
    u8g2.print(info.networkCredential.c_str());
    y += 16;

    Font::useBold();
    u8g2.setCursor(MARGIN_X, y);
    u8g2.print(D_UPLOAD_OPEN);
    y += 14;

    Font::useBody();
    u8g2.setCursor(MARGIN_X, y);
    u8g2.print(info.primaryAddress.c_str());
    y += 18;
  }

  display.update();
}

// ---- Lifecycle ------------------------------------------------------------

void WifiScreen::onEnter() {
  resetBookUpload();
  resetSleepUpload();
  resetAppUpload();

  g_webInterface.begin();
  lastState_ = g_webInterface.state();
  draw();
}

void WifiScreen::stopSessionToLibrary() {
  g_webInterface.end();

  resetBookUpload();
  resetSleepUpload();
  resetAppUpload();

  loadBooks();
  // Discard the exit-click so it doesn't leak into the library screen as a
  // menu selection.
  resetInputFrontend();
  nextScreen = &g_libraryScreen;
}

// ---- Per-iteration input + tick -------------------------------------------

void WifiScreen::onButton(const ButtonEvent& e) {
  if (!e.any()) return;

  // Any tap during the connecting splash means "use the hotspot instead".
  if (g_webInterface.state() == ExternalInterface::State::Provisioning) {
    g_webInterface.forceReady();
    lastState_ = g_webInterface.state();  // sync so onIdleTick doesn't re-draw
    draw();
    return;
  }

  if (e.kind == ButtonEvent::Short || e.kind == ButtonEvent::Triple) {
    stopSessionToLibrary();
  }
}

void WifiScreen::onIdleTick() {
  g_webInterface.poll();

  if (g_webInterface.idleTimedOut()) {
    stopSessionToLibrary();
    return;
  }

  // Redraw whenever the interface transitions between states (e.g. STA
  // connecting → ready with URL, or AP fallback).
  auto newState = g_webInterface.state();
  if (newState != lastState_) {
    lastState_ = newState;
    draw();
  }
}
