#pragma once

#include "api_client.hpp"
#include "app_state.hpp"
#include "button_led_matrix.hpp"
#include "standalone_button.hpp"
#include "tft_display.hpp"

/**
 * AppController — the application state machine.
 *
 * Owns an AppState (the data model) and orchestrates all hardware/display
 * reactions to user inputs. Call update() once per main-loop iteration.
 *
 * The caller (main.cpp) is responsible for:
 *   - Ensuring Wi-Fi is connected before constructing this object.
 *   - Calling redrawAfterReconnect() when Wi-Fi is restored.
 */
class AppController {
public:
  /**
   * @param matrix   Combined button+LED matrix (categories + counters)
   * @param display  TFT display driver
   * @param accept   The Accept standalone button
   * @param cancel   The Cancel standalone button
   * @param api      ApiClient used to send data on Accept
   * @param cat_names   Array of 12 category name strings (from config.hpp)
   * @param ctr_names   Array of 3 counter name strings (from config.hpp)
   */
  AppController(ButtonLedMatrix &matrix, TftDisplay &display,
                StandaloneButton &accept, StandaloneButton &cancel,
                ApiClient &api, const char *const *cat_names,
                const char *const *ctr_names);

  /**
   * Run one iteration of the application state machine.
   * Call this once per main-loop iteration.
   */
  void update();

  /**
   * Redraw the entire UI from the current AppState.
   * Call this after the display was cleared (e.g. after Wi-Fi reconnect).
   */
  void redrawAll();

private:
  // ── Event handlers ───────────────────────────────────────────────────────
  void onCategoryPressed(int row, int col);
  void onCounterPressed(int row);
  void handleAccept();
  void handleCancel();

  // ── Hardware references ──────────────────────────────────────────────────
  ButtonLedMatrix &matrix_;
  TftDisplay &display_;
  StandaloneButton &accept_;
  StandaloneButton &cancel_;
  ApiClient &api_;

  const char *const *cat_names_;
  const char *const *ctr_names_;

  // ── Application data model ───────────────────────────────────────────────
  AppState state_;

  // ── Button matrix edge tracking ──────────────────────────────────────────
  uint16_t buttons_old_ = 0;

  // ── Blink state for active category LED ──────────────────────────────────
  static constexpr uint32_t BLINK_HALF_MS = 200;
  uint32_t last_blink_ms_ = 0;
  bool blink_lit_ = false;
};
