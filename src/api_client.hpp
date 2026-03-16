#pragma once

#include <stdint.h>

/**
 * ApiClient — sends the current session data to a remote HTTP endpoint.
 *
 * URL format: http://<host>:<port>/log?counter1=1&counter2=2&counter3=3&category=alpha
 *
 * Call init() once after loading config from flash, then send() for each session.
 */
class ApiClient {
public:
  /**
   * Configure the API server address.
   * Must be called before send().
   *
   * @param host  IP address string, e.g. "192.168.1.100"
   * @param port  Port number, e.g. 3000
   */
  void init(const char *host, uint16_t port);

  /**
   * Send a session's data via HTTP GET.
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
