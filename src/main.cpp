#include <Arduino.h>

#define RGB_PINS 4

void setup()
{
    pinMode(RGB_PINS, OUTPUT);
    Serial.begin(9600);
    Serial.println("Hello");
}

void loop()
{
    digitalWrite(RGB_PINS, HIGH);
    delay(1000);
    Serial.println("Hello loop");
    digitalWrite(RGB_PINS, LOW);
    delay(1000);
}
