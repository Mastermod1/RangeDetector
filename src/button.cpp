#include "button.hpp"

void Button::handleButtonPress(const std::function<void(const int)>& onPress,
                               const std::function<void(const int)>& onRelease)
{
    current_state = digitalRead(gpio_pin);

    if (current_state != last_state)
    {
        const auto time = getTime();
        if (current_state == press_state)
        {
            onRelease(time);
        }
        else
        {
            onPress(time);
        }
    }

    last_state = current_state;
}

int Button::getTime()
{
    if (current_state == press_state)
    {
        stop_press = millis();
        return stop_press - start_press;
    }
    else
    {
        start_press = millis();
        return start_press - stop_press;
    }
    return 0;
}
