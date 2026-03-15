#pragma once

#include <stdint.h>
#include <string.h>

/**
 * AppState — pure data model for the button box application.
 *
 * No GPIO, no display, no SDK calls. Only stores the current
 * selection/counter values and exposes helper methods.
 *
 * Layout:
 *   Categories  → 12 buttons, cols 0-3, row-major index: row*4 + col
 *   Counters    → 3 buttons, col 4, index = row (0-2)
 */
struct AppState {
  static constexpr int NUM_CATS = 12;
  static constexpr int NUM_COUNTERS = 3;
  static constexpr uint8_t COUNTER_MAX = 127;

  int active_cat = -1;                 ///< -1 = no selection; 0-11 otherwise
  uint8_t counters[NUM_COUNTERS] = {}; ///< raw count per counter

  /** True when we have a category AND at least one counter > 0. */
  bool canSend() const {
    if (active_cat < 0)
      return false;
    for (int i = 0; i < NUM_COUNTERS; i++)
      if (counters[i] > 0)
        return true;
    return false;
  }

  /** Clear selection and all counters. */
  void reset() {
    active_cat = -1;
    memset(counters, 0, sizeof(counters));
  }

  // ── Helpers for button ↔ LED index mapping ──────────────────────────────

  /** Convert button-matrix (row, col) to a category index. */
  static int catIndex(int row, int col) { return row * 4 + col; }

  /** Convert category index to LED-matrix (row, col). */
  static void catToLed(int cat, int &row, int &col) {
    row = cat / 4;
    col = cat % 4;
  }
};
