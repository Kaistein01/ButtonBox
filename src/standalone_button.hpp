#pragma once

/**
 * StandaloneButton — a single active-LOW button with a dedicated LED.
 *
 * Used for the Accept and Cancel buttons which are independent of the
 * button/LED matrix.
 *
 * Wiring assumptions:
 *   - Button: active-LOW with internal pull-up (pressed = gpio_get() == 0).
 *   - LED:    active-HIGH (gpio_put(pin_led, 1) turns it on).
 */
class StandaloneButton {
public:
  /**
   * @param pin_btn  GPIO pin of the button
   * @param pin_led  GPIO pin of the button's LED
   */
  StandaloneButton(unsigned int pin_btn, unsigned int pin_led);

  /** Initialise GPIO directions and pull-up. */
  void init();

  /** Returns true while the button is physically held down. */
  bool held() const;

  /**
   * Returns true on the first call after the button was pressed.
   * Must be called every loop iteration for correct edge detection.
   */
  bool justPressed();

  /** Mirror the held() state on the LED: on while pressed, off otherwise. */
  void updateLed() const;

  /** Directly set the LED on or off (ignores hold state). */
  void setLed(bool on) const;

private:
  unsigned int pin_btn_;
  unsigned int pin_led_;
  bool prev_; ///< button state from previous call to justPressed()
};
