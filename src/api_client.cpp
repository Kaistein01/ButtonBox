#include "api_client.hpp"
#include <stdio.h>

/**
 * Stub implementation.
 *
 * TODO (future): Replace this body with an lwIP HTTP GET call, e.g.:
 *   http://<server>/api/count?cat=<category>&<ctr_name>=<value>&...
 *
 * The function signature is already designed for this:
 *   - category, ctr_names and ctr_values carry all payload data.
 *   - Return false on network/HTTP error so the caller can handle it.
 */
bool ApiClient::send(const char *category, const char *const *ctr_names,
                     const uint8_t *ctr_values, int n) {
  printf("[ApiClient] SEND category=%s", category);
  for (int i = 0; i < n; i++) {
    if (ctr_values[i] > 0)
      printf(", %s=%d", ctr_names[i], ctr_values[i]);
  }
  printf("\n");

  // Stub always returns success
  return true;
}
