#pragma once

#include <stdint.h>

/**
 * WifiStatus - Manages Cyw43 Wi-Fi connection and status polling.
 */
class WifiStatus {
public:
  /**
   * @param ssid WiFi SSID
   * @param psk  WiFi Password
   */
  WifiStatus(const char *ssid, const char *psk);

  /**
   * Initialize hardware and start asynchronous connection.
   * Does not block.
   */
  void init();

  /**
   * Poll the Wi-Fi link status (internal check happens at most once per
   * second).
   * @param status_changed  Set to true if the online status has changed since
   * the last check.
   * @return true if currently online, false if offline.
   */
  bool update(bool &status_changed);

private:
  const char *ssid_;
  const char *psk_;
  bool is_online_;
  uint32_t last_check_ms_;
  uint32_t last_retry_ms_; ///< timestamp of last reconnect attempt
};
