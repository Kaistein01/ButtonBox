#include "standalone_button.hpp"
#include <pico/stdlib.h>

StandaloneButton::StandaloneButton(unsigned int pin_btn, unsigned int pin_led)
    : pin_btn_(pin_btn), pin_led_(pin_led), prev_(false) {}

void StandaloneButton::init() {
  gpio_init(pin_btn_);
  gpio_set_dir(pin_btn_, GPIO_IN);
  gpio_pull_up(pin_btn_);

  gpio_init(pin_led_);
  gpio_set_dir(pin_led_, GPIO_OUT);
  gpio_put(pin_led_, 0);
}

bool StandaloneButton::held() const {
  return !gpio_get(pin_btn_); // active-LOW
}

bool StandaloneButton::justPressed() {
  bool h = held();
  bool edge = h && !prev_;
  prev_ = h;
  return edge;
}

void StandaloneButton::updateLed() const { gpio_put(pin_led_, held() ? 1 : 0); }

void StandaloneButton::setLed(bool on) const { gpio_put(pin_led_, on ? 1 : 0); }
