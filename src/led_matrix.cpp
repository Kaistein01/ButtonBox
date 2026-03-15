#include "led_matrix.hpp"
#include "hw_config.h"
#include "pico/stdlib.h"
#include <cstdint>

// ── Static pin arrays ────────────────────────────────────────────────────────

const unsigned int LedMatrix::rows_[LedMatrix::ROWS] = {
    PIN_LEDM_ROW_1,
    PIN_LEDM_ROW_2,
    PIN_LEDM_ROW_3,
};

const unsigned int LedMatrix::cols_[LedMatrix::COLS] = {
    PIN_LEDM_COL_1, PIN_LEDM_COL_2, PIN_LEDM_COL_3,
    PIN_LEDM_COL_4, PIN_LEDM_COL_5,
};

// ── Public methods ───────────────────────────────────────────────────────────

void LedMatrix::init() {
  // Rows: cathodes — idle HIGH (no sink → all LEDs off)
  for (int r = 0; r < ROWS; r++) {
    gpio_init(rows_[r]);
    gpio_set_dir(rows_[r], GPIO_OUT);
    gpio_put(rows_[r], 1);
  }

  // Cols: anodes — idle LOW (no forward voltage → all LEDs off)
  for (int c = 0; c < COLS; c++) {
    gpio_init(cols_[c]);
    gpio_set_dir(cols_[c], GPIO_OUT);
    gpio_put(cols_[c], 0);
  }

  clearAll();
}

void LedMatrix::refresh() {
  for (int r = 0; r < ROWS; r++) {
    // 1) Drive all rows HIGH and all cols LOW → safe blank state
    for (int rr = 0; rr < ROWS; rr++)
      gpio_put(rows_[rr], 1);
    for (int c = 0; c < COLS; c++)
      gpio_put(cols_[c], 0);

    // 2) Activate current row as sink
    gpio_put(rows_[r], 0);

    // 3) Apply column pattern for this row
    for (int c = 0; c < COLS; c++) {
      gpio_put(cols_[c], frame_[r][c] ? 1 : 0);
    }

    // 4) Hold the row on for the dwell time
    sleep_us(300);
  }

  // 5) Blank everything after the last row
  for (int rr = 0; rr < ROWS; rr++)
    gpio_put(rows_[rr], 1);
  for (int c = 0; c < COLS; c++)
    gpio_put(cols_[c], 0);
}

void LedMatrix::setLed(int row, int col, bool on) {
  if (row >= 0 && row < ROWS && col >= 0 && col < COLS)
    frame_[row][col] = on ? 1 : 0;
}

void LedMatrix::toggleLed(int row, int col) {
  if (row >= 0 && row < ROWS && col >= 0 && col < COLS)
    frame_[row][col] ^= 1;
}

bool LedMatrix::getLed(int row, int col) const {
  if (row >= 0 && row < ROWS && col >= 0 && col < COLS)
    return frame_[row][col] != 0;
  return false;
}

void LedMatrix::setAll() {
  for (int r = 0; r < ROWS; r++)
    for (int c = 0; c < COLS; c++)
      frame_[r][c] = 1;
}

void LedMatrix::clearAll() {
  for (int r = 0; r < ROWS; r++)
    for (int c = 0; c < COLS; c++)
      frame_[r][c] = 0;
}

void LedMatrix::setFromMask(uint16_t mask) {
  for (int r = 0; r < ROWS; r++)
    for (int c = 0; c < COLS; c++)
      frame_[r][c] = (mask >> (r * COLS + c)) & 1u;
}

uint16_t LedMatrix::getMask() const {
  uint16_t mask = 0;
  for (int r = 0; r < ROWS; r++)
    for (int c = 0; c < COLS; c++)
      if (frame_[r][c])
        mask |= 1u << (r * COLS + c);
  return mask;
}
