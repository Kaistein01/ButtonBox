#pragma once

#include <stdint.h>

/**
 * ApiClient — sends the current session data to a remote endpoint.
 *
 * Current implementation is a stub: it logs the data over serial.
 *
 * Future implementation:
 *   Replace the stub body in api_client.cpp with an HTTP GET (or POST)
 *   using the Pico W's lwIP stack. The interface intentionally keeps the
 *   caller decoupled from the transport layer.
 *
 * Example future URL:
 *   http://192.168.1.100/api/count?cat=Alpha&Bier=2&Shot=1
 */
class ApiClient {
public:
  /**
   * Send a session's data.
   *
   * @param category      Active category name string.
   * @param ctr_names     Array of counter name strings (length = n).
   * @param ctr_values    Array of counter values (length = n).
   * @param n             Number of counters.
   * @return              true on success, false on failure.
   */
  bool send(const char *category, const char *const *ctr_names,
            const uint8_t *ctr_values, int n);
};
