#include "button_matrix.hpp"
#include "hw_config.h"
#include "pico/stdlib.h"

// ── Static pin arrays ────────────────────────────────────────────────────────

const unsigned int ButtonMatrix::rows_[ButtonMatrix::ROWS] = {
    PIN_BUTM_ROW_1,
    PIN_BUTM_ROW_2,
    PIN_BUTM_ROW_3,
};

const unsigned int ButtonMatrix::cols_[ButtonMatrix::COLS] = {
    PIN_BUTM_COL_1, PIN_BUTM_COL_2, PIN_BUTM_COL_3,
    PIN_BUTM_COL_4, PIN_BUTM_COL_5,
};

// ── Public methods ───────────────────────────────────────────────────────────

void ButtonMatrix::init() {
  // Rows: Hi-Z inputs (driven HIGH only during scan)
  for (int r = 0; r < ROWS; r++) {
    gpio_init(rows_[r]);
    gpio_set_dir(rows_[r], GPIO_IN);
  }
  // Cols: inputs with pull-down
  for (int c = 0; c < COLS; c++) {
    gpio_init(cols_[c]);
    gpio_set_dir(cols_[c], GPIO_IN);
    gpio_pull_down(cols_[c]);
  }
}

uint16_t ButtonMatrix::read() const {
  uint16_t state = 0;

  for (int r = 0; r < ROWS; r++) {
    // 1) Discharge columns briefly to prevent ghost presses
    for (int c = 0; c < COLS; c++) {
      gpio_set_dir(cols_[c], GPIO_OUT);
      gpio_put(cols_[c], 0);
    }
    sleep_us(5);
    for (int c = 0; c < COLS; c++) {
      gpio_set_dir(cols_[c], GPIO_IN);
      gpio_pull_down(cols_[c]);
    }

    // 2) All rows → Hi-Z
    for (int rr = 0; rr < ROWS; rr++)
      gpio_set_dir(rows_[rr], GPIO_IN);

    // 3) Drive current row HIGH
    gpio_set_dir(rows_[r], GPIO_OUT);
    gpio_put(rows_[r], 1);
    sleep_us(50); // let signals settle

    // 4) Sample columns
    for (int c = 0; c < COLS; c++) {
      if (gpio_get(cols_[c]))
        state |= 1u << (r * COLS + c);
    }

    // 5) Return row to Hi-Z
    gpio_set_dir(rows_[r], GPIO_IN);
  }

  // Ensure all rows end as Hi-Z
  for (int rr = 0; rr < ROWS; rr++)
    gpio_set_dir(rows_[rr], GPIO_IN);

  return state;
}
