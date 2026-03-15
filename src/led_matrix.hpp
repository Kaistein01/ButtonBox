#pragma once

#include <stdint.h> // uint8_t, uint16_t — no Pico SDK chain pulled in here

/**
 * LedMatrix — multiplexed 3×5 LED matrix driver.
 *
 * Wiring convention (matches hw_config.h):
 *   Rows  = cathodes  (active LOW  — sink current)
 *   Cols  = anodes    (active HIGH — source current)
 *
 * LED at (row, col) is ON when:  row pin LOW  AND  col pin HIGH
 *
 * Call refresh() frequently from the main loop (or a timer ISR) to
 * multiplex through all rows.  A call interval of ≤ 1 ms gives
 * flicker-free output.
 */
class LedMatrix {
public:
  static constexpr int ROWS = 3;
  static constexpr int COLS = 5;

  /** Initialise GPIOs and clear the frame buffer. */
  void init();

  /**
   * Scan one full refresh cycle through all rows.
   * Each row stays on for ~300 µs.  Total call time ≈ 0.9 ms.
   */
  void refresh();

  // ── Per-LED control ──────────────────────────────────────────────────

  /** Turn a single LED on (row 0-2, col 0-4). */
  void setLed(int row, int col, bool on);

  /** Toggle a single LED. */
  void toggleLed(int row, int col);

  /** Return the current state of a single LED. */
  bool getLed(int row, int col) const;

  // ── Bulk control ─────────────────────────────────────────────────────

  /** Turn every LED on. */
  void setAll();

  /** Turn every LED off. */
  void clearAll();

  /**
   * Load the frame buffer from a 15-bit mask.
   * Bit (r*5 + c) == 1  →  LED[r][c] ON.
   * Compatible with the raw key-scan bitmask from the button matrix.
   */
  void setFromMask(uint16_t mask);

  /** Return the current frame buffer as a 15-bit mask. */
  uint16_t getMask() const;

private:
  static const unsigned int
      rows_[ROWS]; // same underlying type as Pico SDK `uint`
  static const unsigned int cols_[COLS];

  uint8_t frame_[ROWS][COLS] = {}; // 1 = on, 0 = off
};
