#include "button_led_matrix.hpp"
#include <pico/time.h>

// ── Constructor ──────────────────────────────────────────────────────────────

ButtonLedMatrix::ButtonLedMatrix(uint32_t blink_period_ms)
    : blink_half_ms_(blink_period_ms / 2), active_idx_(-1), led_on_(false),
      last_toggle_ms_(0), prev_mask_(0) {}

// ── Public methods ───────────────────────────────────────────────────────────

void ButtonLedMatrix::init() {
  leds_.init();
  buttons_.init();
}

void ButtonLedMatrix::update() {
  // ── 1) Scan buttons, detect rising edges only ─────────────────────────────
  // Without edge detection, holding a button would call setActive() every loop
  // iteration, continuously resetting last_toggle_ms_ and preventing blinking.
  uint16_t current_mask = buttons_.read();
  uint16_t just_pressed = current_mask & ~prev_mask_; // bits that just went 0→1
  prev_mask_ = current_mask;

  // Find the lowest-index button that was just pressed this iteration
  int new_idx = -1;
  for (int i = 0; i < ButtonMatrix::ROWS * ButtonMatrix::COLS; i++) {
    if (just_pressed & (1u << i)) {
      new_idx = i;
      break;
    }
  }

  // ── 2) Update selection ───────────────────────────────────────────────────
  // Deselection is handled externally (Cancel button) — any new press selects.
  if (new_idx >= 0)
    setActive(new_idx);

  // ── 3) Blink active LED ───────────────────────────────────────────────────
  if (active_idx_ >= 0) {
    uint32_t now = to_ms_since_boot(get_absolute_time());
    if (now - last_toggle_ms_ >= blink_half_ms_) {
      last_toggle_ms_ = now;
      led_on_ = !led_on_;

      int r = active_idx_ / ButtonMatrix::COLS;
      int c = active_idx_ % ButtonMatrix::COLS;
      leds_.setLed(r, c, led_on_);
    }
  }
}

void ButtonLedMatrix::refresh() { leds_.refresh(); }

void ButtonLedMatrix::deselect() { setActive(-1); }

void ButtonLedMatrix::flashAll(uint32_t ms) {
  leds_.setAll();
  uint32_t end = to_ms_since_boot(get_absolute_time()) + ms;
  while (to_ms_since_boot(get_absolute_time()) < end)
    leds_.refresh();
  // Restore: clear matrix, then re-light the active LED if any
  leds_.clearAll();
  if (active_idx_ >= 0)
    leds_.setLed(active_idx_ / ButtonMatrix::COLS,
                 active_idx_ % ButtonMatrix::COLS, led_on_);
  // Reset blink timer so the LED doesn't immediately toggle away
  last_toggle_ms_ = to_ms_since_boot(get_absolute_time());
}

// ── Private methods ──────────────────────────────────────────────────────────

void ButtonLedMatrix::setActive(int idx) {
  // Turn off the old LED immediately
  if (active_idx_ >= 0) {
    int r = active_idx_ / ButtonMatrix::COLS;
    int c = active_idx_ % ButtonMatrix::COLS;
    leds_.setLed(r, c, false);
  }

  active_idx_ = idx;
  led_on_ = false;
  last_toggle_ms_ = to_ms_since_boot(get_absolute_time());

  // Start with LED on for the new selection
  if (active_idx_ >= 0) {
    int r = active_idx_ / ButtonMatrix::COLS;
    int c = active_idx_ % ButtonMatrix::COLS;
    led_on_ = true;
    leds_.setLed(r, c, true);
  }
}
