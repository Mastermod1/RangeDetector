#pragma once

#include <functional>

#include "esp32-hal-gpio.h"

class Button
{
  public:
    Button(int gpio_pin, int press_state = LOW) : gpio_pin(gpio_pin), press_state(press_state) { initPin(); }

    void initPin() { pinMode(gpio_pin, INPUT); }
    void handleButtonPress(const std::function<void(const int)>& onPress,
                           const std::function<void(const int)>& onRelease);

  private:
    int getTime();

    int gpio_pin;
    int press_state;
    int current_state = LOW;
    int last_state = LOW;
    int start_press = 0;
    int stop_press = 0;
};
