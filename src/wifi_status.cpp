#include "wifi_status.hpp"
#include <pico/cyw43_arch.h>
#include <pico/stdlib.h>
#include <stdio.h>

WifiStatus::WifiStatus(const char *ssid, const char *psk)
    : ssid_(ssid), psk_(psk), is_online_(false), last_check_ms_(0),
      last_retry_ms_(0) {}

void WifiStatus::init() {
  if (cyw43_arch_init() == 0) {
    cyw43_arch_enable_sta_mode();
    cyw43_arch_wifi_connect_async(ssid_, psk_, CYW43_AUTH_WPA2_AES_PSK);
    printf("Wi-Fi connecting (async)...\n");
  } else {
    printf("cyw43_arch_init failed\n");
  }
}

bool WifiStatus::update(bool &status_changed) {
  status_changed = false;
  uint32_t now_ms = to_ms_since_boot(get_absolute_time());

  // Throttle status checks to once per second
  if (now_ms - last_check_ms_ < 1000)
    return is_online_;
  last_check_ms_ = now_ms;

  extern cyw43_t cyw43_state;
  bool new_online =
      (cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA) == CYW43_LINK_UP);

  // If not connected, retry async connect every 5 s.
  // This handles both: AP not available at boot, and AP disappearing mid-use.
  if (!new_online && (now_ms - last_retry_ms_ >= 5000)) {
    last_retry_ms_ = now_ms;
    printf("Wi-Fi: retrying connection...\n");
    cyw43_arch_wifi_connect_async(ssid_, psk_, CYW43_AUTH_WPA2_AES_PSK);
  }

  if (new_online != is_online_) {
    is_online_ = new_online;
    status_changed = true;
    printf("WiFi: %s\n", is_online_ ? "online" : "offline");
  }

  return is_online_;
}
