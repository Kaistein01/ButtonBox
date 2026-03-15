#include "app_controller.hpp"
#include <hardware/watchdog.h>
#include <pico/stdlib.h>
#include <pico/time.h>
#include <stdio.h>

// ── Constructor
// ───────────────────────────────────────────────────────────────

AppController::AppController(ButtonLedMatrix &matrix, TftDisplay &display,
                             StandaloneButton &accept, StandaloneButton &cancel,
                             ApiClient &api, const char *const *cat_names,
                             const char *const *ctr_names)
    : matrix_(matrix), display_(display), accept_(accept), cancel_(cancel),
      api_(api), cat_names_(cat_names), ctr_names_(ctr_names), buttons_old_(0) {
}

// ── Public: update
// ────────────────────────────────────────────────────────────

void AppController::update() {

  // ── 1) Read button matrix (edge detection) ────────────────────────────────
  uint16_t buttons_now = matrix_.buttons().read();
  uint16_t just_pressed = buttons_now & ~buttons_old_;
  buttons_old_ = buttons_now;

  for (int bit = 0; bit < 15; bit++) {
    if (!(just_pressed & (1u << bit)))
      continue;
    int row = bit / 5;
    int col = bit % 5;
    if (col < 4)
      onCategoryPressed(row, col);
    else
      onCounterPressed(row);
  }

  // ── 2) Counter button LEDs: on while held ─────────────────────────────────
  for (int r = 0; r < AppState::NUM_COUNTERS; r++)
    matrix_.leds().setLed(r, 4, (buttons_now >> (r * 5 + 4)) & 1u);

  // ── 3) Blink active category LED ──────────────────────────────────────────
  if (state_.active_cat >= 0) {
    uint32_t now = to_ms_since_boot(get_absolute_time());
    if (now - last_blink_ms_ >= BLINK_HALF_MS) {
      last_blink_ms_ = now;
      blink_lit_ = !blink_lit_;
      int br, bc;
      AppState::catToLed(state_.active_cat, br, bc);
      matrix_.leds().setLed(br, bc, blink_lit_);
    }
  }
  matrix_.leds().refresh();

  // ── 4) Accept LED readiness indicator ────────────────────────────────────
  accept_.setLed(state_.canSend());

  // ── 5) Cancel LED: on while pressed ──────────────────────────────────────
  cancel_.updateLed();

  // ── 6) Hardware reset: hold both Accept AND Cancel ────────────────────────
  if (accept_.held() && cancel_.held()) {
    printf("Hardware reset triggered!\n");
    display_.showAlert("RESET", nullptr);
    sleep_ms(500);
    watchdog_enable(1, 1);
    while (1)
      ;
  }

  // ── 7) Accept / Cancel edge events ───────────────────────────────────────
  if (accept_.justPressed())
    handleAccept();
  if (cancel_.justPressed())
    handleCancel();
}

// ── Public: redrawAll
// ─────────────────────────────────────────────────────────

void AppController::redrawAll() {
  display_.drawAppUI(cat_names_, ctr_names_);

  if (state_.active_cat >= 0) {
    display_.updateActiveCategory(cat_names_[state_.active_cat], true);
  }
  for (int p = 0; p < AppState::NUM_COUNTERS; p++) {
    if (state_.counters[p] > 0)
      display_.updateCounter(p, ctr_names_[p], state_.counters[p]);
  }
}

// ── Private: onCategoryPressed
// ────────────────────────────────────────────────

void AppController::onCategoryPressed(int row, int col) {
  int new_cat = AppState::catIndex(row, col);
  if (new_cat == state_.active_cat)
    return; // same button — do nothing

  // Turn off old LED
  if (state_.active_cat >= 0) {
    int or_, oc;
    AppState::catToLed(state_.active_cat, or_, oc);
    matrix_.leds().setLed(or_, oc, false);
  }

  state_.active_cat = new_cat;
  matrix_.leds().setLed(row, col, true);
  display_.updateActiveCategory(cat_names_[new_cat], true);
  printf("[Category] %s\n", cat_names_[new_cat]);
}

// ── Private: onCounterPressed
// ─────────────────────────────────────────────────

void AppController::onCounterPressed(int row) {
  uint8_t &cnt = state_.counters[row];
  if (cnt < AppState::COUNTER_MAX) {
    cnt++;
    display_.updateCounter(row, ctr_names_[row], cnt);
    printf("[Counter] %s = %d\n", ctr_names_[row], cnt);
  }
}

// ── Private: handleAccept
// ─────────────────────────────────────────────────────

void AppController::handleAccept() {
  if (!state_.canSend())
    return; // guard — should not happen (LED is off)

  bool ok = api_.send(cat_names_[state_.active_cat], ctr_names_,
                      state_.counters, AppState::NUM_COUNTERS);

  if (ok) {
    // Flash all category LEDs (col 0-3 only, not counters)
    for (int r = 0; r < 3; r++)
      for (int c = 0; c < 4; c++)
        matrix_.leds().setLed(r, c, true);

    display_.showSent();
    accept_.setLed(true);

    uint32_t end = to_ms_since_boot(get_absolute_time()) + 600;
    while (to_ms_since_boot(get_absolute_time()) < end)
      matrix_.leds().refresh();

    accept_.setLed(false);
  } else {
    display_.showAlert("Send failed!", nullptr);
    sleep_ms(1500);
  }

  // Reset state and restore UI
  state_.reset();
  matrix_.leds().clearAll();
  display_.drawAppUI(cat_names_, ctr_names_);
}

// ── Private: handleCancel
// ─────────────────────────────────────────────────────

void AppController::handleCancel() {
  printf("[Cancel] resetting\n");
  state_.reset();
  matrix_.leds().clearAll();
  display_.drawAppUI(cat_names_, ctr_names_);
}
