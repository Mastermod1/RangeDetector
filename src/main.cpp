#include <EEPROM.h>

#include "HardwareSerial.h"
#include "button.hpp"
#include "network_utils.hpp"
#include "button_handlers.hpp"
#include "esp32-hal-gpio.h"

const int G_BUTTON_PIN = 3;

Button G_WIFI_BUTTON(G_BUTTON_PIN);

void handleButtonRelease(const int time);
void handleButton(const int time);

void setup()
{
    Serial.begin(9600);
    EEPROM.begin(G_EPROM_SIZE);
    mountSpiffs();

    pinMode(G_BUTTON_PIN, INPUT);

    initialzieInternetConnection();
}

void loop()
{
    G_SERVER.handleClient();
    G_WIFI_BUTTON.handleButtonPress(emptyOnPress, setupAccessPointOnRelease);
}
