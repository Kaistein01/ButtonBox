#pragma once

#include <stdint.h> // uint16_t — no Pico SDK chain in the header

/**
 * ButtonMatrix — 3×5 diode matrix scanner.
 *
 * Wiring convention (matches hw_config.h):
 *   Rows = driven HIGH one at a time (output)
 *   Cols = read with pull-down (input)
 *
 * Bit layout of the returned mask:  bit (r*5 + c) = 1 if pressed
 * — identical to the LedMatrix frame buffer layout so both can share
 *   the same bitmask.
 */
class ButtonMatrix {
public:
  static constexpr int ROWS = 3;
  static constexpr int COLS = 5;

  /** Initialise GPIOs (rows Hi-Z, cols input with pull-down). */
  void init();

  /**
   * Perform one full scan of the matrix.
   * Returns a 15-bit mask: bit (r*5 + c) == 1  →  button pressed.
   * Takes ~165 µs (3 rows × 55 µs each).
   */
  uint16_t read() const;

private:
  static const unsigned int rows_[ROWS];
  static const unsigned int cols_[COLS];
};
