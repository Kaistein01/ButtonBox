#pragma once

#include "button_matrix.hpp"
#include "led_matrix.hpp"
#include <stdint.h>

/**
 * ButtonLedMatrix — combines a 3×5 button matrix with a 3×5 LED matrix.
 *
 * Each button at (row, col) controls the LED at the same position.
 *
 * Behaviour:
 *   - Pressing a button makes its LED blink (toggling every blink_period_ms).
 *   - Only ONE button/LED can be active at a time (single-select).
 *   - Pressing a new button deactivates the previous one.
 *   - Call deselect() to clear the selection (e.g. from Cancel button).
 *   - Call flashAll() to briefly illuminate all matrix LEDs (e.g. on Accept).
 *
 * Usage pattern in main loop:
 *   1. Call update() once per loop iteration — scans buttons, manages blink.
 *   2. Call refresh() once per loop iteration — drives LED matrix multiplexing.
 *
 * Both calls together take < 2 ms, so 500 Hz loop rate is comfortable.
 */
class ButtonLedMatrix {
public:
  /**
   * @param blink_period_ms  Full blink period in ms (LED on for half, off for
   * half). Default: 400 ms → 2.5 Hz blink.
   */
  explicit ButtonLedMatrix(uint32_t blink_period_ms = 400);

  /** Initialise both LED and button GPIO hardware. */
  void init();

  /**
   * Call this once per main-loop iteration.
   * Scans buttons, updates the active selection, and toggles blink state.
   */
  void update();

  /**
   * Call this once per main-loop iteration to drive the LED matrix.
   * Delegates directly to LedMatrix::refresh().
   */
  void refresh();

  /** Clear the current selection and turn off its LED immediately. */
  void deselect();

  /**
   * Flash ALL matrix LEDs on for @p ms milliseconds, then restore state.
   * Blocks the caller for the duration (keeps calling refresh() internally).
   * The accept/cancel LEDs are NOT managed here — control them from the caller.
   */
  void flashAll(uint32_t ms);

  // ── Query ─────────────────────────────────────────────────────────

  /** Returns the index (r*5+c) of the currently selected button, or -1 if none.
   */
  int activeIndex() const { return active_idx_; }

  /** Returns true if any button is currently selected. */
  bool hasActive() const { return active_idx_ >= 0; }

  /** Direct access to the underlying LED matrix (for custom patterns etc.). */
  LedMatrix &leds() { return leds_; }
  const LedMatrix &leds() const { return leds_; }

  /** Direct access to the underlying button matrix. */
  ButtonMatrix &buttons() { return buttons_; }
  const ButtonMatrix &buttons() const { return buttons_; }

private:
  LedMatrix leds_;
  ButtonMatrix buttons_;

  uint32_t blink_half_ms_;  // half the blink period
  int active_idx_;          // bit index r*5+c, -1 = none
  bool led_on_;             // current blink phase
  uint32_t last_toggle_ms_; // timestamp of last blink toggle
  uint16_t prev_mask_; // button scan from previous call (for edge detection)

  /** Set active button by index (-1 clears selection). */
  void setActive(int idx);
};
