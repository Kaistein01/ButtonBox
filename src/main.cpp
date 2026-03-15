#include <hardware/watchdog.h>
#include <pico/stdlib.h>
#include <pico/time.h>
#include <stdio.h>

#include "api_client.hpp"
#include "app_controller.hpp"
#include "button_led_matrix.hpp"
#include "button_matrix.hpp"
#include "config.hpp"
#include "config_store.hpp"
#include "hw_config.h"
#include "standalone_button.hpp"
#include "tft_display.hpp"
#include "webserver.hpp"
#include "wifi_status.hpp"

int main() {
  stdio_init_all();

  // ── Boot-time button check for config mode ────────────────────────────────
  // Check if counter button 1 (row 0, col 4, bit index 4) is held for 3 seconds
  ButtonMatrix bmatrix;
  bmatrix.init();

  uint32_t t0 = to_ms_since_boot(get_absolute_time());
  uint32_t t_now;
  uint16_t mask;
  bool enter_config = false;

  // Read button once to check if it's initially pressed
  mask = bmatrix.read();
  if (mask & (1u << 4)) {
    // Button is held; wait up to 3 seconds to see if still held
    printf("Counter 1 button held, checking for 3s...\n");
    while ((t_now = to_ms_since_boot(get_absolute_time())) - t0 < 3000) {
      mask = bmatrix.read();
      if (!(mask & (1u << 4))) {
        // Button released before 3s
        break;
      }
      sleep_ms(50);
    }

    // Check if we reached 3s and button is still held
    t_now = to_ms_since_boot(get_absolute_time());
    if (t_now - t0 >= 3000) {
      mask = bmatrix.read();
      if (mask & (1u << 4)) {
        enter_config = true;
      }
    }
  }

  // If config mode requested, start webserver and never return
  if (enter_config) {
    TftDisplay display;
    display.init();
    display.showSetupMode();

    // Load current config (or use defaults)
    char ssid[64] = "AP1";
    char psk[64] = "1154542275109433";
    char uuid[64] = "";
    configLoad(ssid, psk, uuid);

    // Run webserver (blocks until config saved + reboot)
    webserverRun(ssid, psk, uuid);
    // Never reaches here
  }

  // ── Display: init first so we can show status messages during boot ────────
  TftDisplay display;
  display.init();

  // ── Load WiFi credentials from flash or use defaults ──────────────────────
  char ssid[64] = "AP1";
  char psk[64] = "1154542275109433";
  char uuid[64] = "";

  if (!configLoad(ssid, psk, uuid)) {
    printf("No config in flash, using defaults\n");
  } else {
    printf("Loaded config from flash: ssid=%s\n", ssid);
  }

  // ── Wi-Fi: show connecting message and block until online ─────────────────
  display.showConnecting();
  WifiStatus wifi(ssid, psk);
  wifi.init();

  bool wifi_online = false;
  while (!wifi_online) {
    bool changed;
    wifi_online = wifi.update(changed);
    sleep_ms(10);
  }

  // ── Hardware: init all matrix, buttons ────────────────────────────────────
  ButtonLedMatrix matrix;
  matrix.init();

  StandaloneButton accept_btn(PIN_BUT_ACCEPT, PIN_LED_ACCEPT);
  StandaloneButton cancel_btn(PIN_BUT_CANCEL, PIN_LED_CANCEL);
  accept_btn.init();
  cancel_btn.init();

  // ── Application layer ─────────────────────────────────────────────────────
  ApiClient api;
  AppController controller(matrix, display, accept_btn, cancel_btn, api,
                           CAT_NAMES, CTR_NAMES);

  // Draw the initial UI now that Wi-Fi is confirmed
  display.drawAppUI(CAT_NAMES, CTR_NAMES);
  printf("Button box ready\n");

  // ── Main loop ─────────────────────────────────────────────────────────────
  while (true) {

    // Check Wi-Fi; block + redraw if connection drops
    bool wifi_changed = false;
    wifi_online = wifi.update(wifi_changed);

    if (wifi_changed && !wifi_online) {
      display.showOffline();
      while (!wifi_online) {
        wifi_online = wifi.update(wifi_changed);
        sleep_ms(10);
      }
      // Reconnected: restore full UI from current state
      controller.redrawAll();
    }

    // Run one iteration of the application state machine
    controller.update();
  }
}
