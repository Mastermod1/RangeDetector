#include <ArduinoJson.h>
#include <EEPROM.h>
#include <WebServer.h>
#include <WiFi.h>

#include "SPIFFS.h"
#include "esp32-hal-gpio.h"

const int G_EPROM_SIZE = 1;

const char* G_NETWORK_SSID = "your_ssid";
const char* G_NETWORK_PASSWORD = "your_password";

const char* G_AP_SSID = "ESP32-AP";
const char* G_AP_PASSWORD = "123456789";

const int G_BUTTON_PIN = 3;

int G_BUTTON_CURRENT_STATE = LOW;
int G_BUTTON_LAST_STATE = LOW;

int G_START_PRESS = 0;
int G_STOP_PRESS = 0;

String G_MAIN_PAGE = "";
WebServer G_SERVER;

enum class ButtonState
{
    NotPressed,
    Pressed,
    ShortPress,
    LongPress
};

void handleData()
{
    G_SERVER.sendHeader("Content-Type", "application/json");
    G_SERVER.send(200, "application/json", "{\"msg\":\"Hello World " + String(0) + "\"}");
}

void handleWiFiSetupPage()
{
    G_SERVER.sendHeader("Content-Type", "text/html");
    G_SERVER.send(200, "text/html", G_MAIN_PAGE);
}

void handleSetup()
{
    JsonDocument doc;
    G_SERVER.sendHeader("Content-Type", "application/json");
    if (G_SERVER.hasArg("plain"))
    {
        String json = G_SERVER.arg("plain");
        DeserializationError error = deserializeJson(doc, json);
        if (error)
        {
            Serial.print("JSON parse error: ");
            Serial.println(error.f_str());
            G_SERVER.send(200, "application/json", R"({"status": "not_correct"})");
            return;
        }
        if (doc.containsKey("ssid") and doc.containsKey("password"))
        {
            const char* ssid = doc["ssid"];
            Serial.println("Received message: " + String(ssid));
        }
        EEPROM.write(0, 69);
        EEPROM.commit();
        G_SERVER.send(200, "application/json", R"({"status": "correct"})");
        delay(200);
        WiFi.softAPdisconnect(true);
    }
}

bool isWiFiSsidAndPasswordSet() { return EEPROM.read(0) == 69; }

void initMainWebPage()
{
    File file2 = SPIFFS.open("/main.html");
    if (!file2)
    {
        Serial.println("Failed to open file for reading");
        return;
    }

    Serial.println("Saving main.html to cache");
    while (file2.available())
    {
        G_MAIN_PAGE += static_cast<char>(file2.read());
    }

    file2.close();
}

void mountSpiffs()
{
    if (SPIFFS.begin(true))
    {
        initMainWebPage();
    }
    else
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
    }
}

void connectToWiFi()
{
    WiFi.begin(G_NETWORK_SSID, G_NETWORK_PASSWORD);
    Serial.print("Connecting To WiFi");
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi Connected!");

    Serial.print("ESP32 Ip Address: ");
    Serial.println(WiFi.localIP());

    G_SERVER.on("/data", HTTP_GET, handleData);
    G_SERVER.begin();
}

void startSetupAp()
{
    WiFi.softAP(G_AP_SSID, G_AP_PASSWORD);

    delay(100);
    IPAddress Ip(192, 168, 1, 1);
    IPAddress NMask(255, 255, 255, 0);
    WiFi.softAPConfig(Ip, Ip, NMask);

    G_SERVER.on("/", HTTP_GET, handleWiFiSetupPage);
    G_SERVER.on("/setup", HTTP_POST, handleSetup);
    G_SERVER.begin();
}

void setup()
{
    Serial.begin(9600);
    EEPROM.begin(G_EPROM_SIZE);
    mountSpiffs();

    pinMode(G_BUTTON_PIN, INPUT);

    if (isWiFiSsidAndPasswordSet())
    {
        connectToWiFi();
    }
    else
    {
        Serial.println("Startin Access Point with Config Page");
        startSetupAp();
    }
}

ButtonState check()
{
    if (G_BUTTON_CURRENT_STATE == HIGH)
    {
        G_START_PRESS = millis();
        return ButtonState::Pressed;
    }
    else
    {
        G_STOP_PRESS = millis();
        int holdTime = G_STOP_PRESS - G_START_PRESS;

        if (holdTime > 1000)
        {
            return ButtonState::LongPress;
        }
        else if (holdTime > 10)
        {
            return ButtonState::ShortPress;
        }
    }
    return ButtonState::NotPressed;
}

void enterSetupStateOnButtonClick()
{
    G_BUTTON_CURRENT_STATE = digitalRead(G_BUTTON_PIN);

    if (G_BUTTON_CURRENT_STATE != G_BUTTON_LAST_STATE)
    {
        const auto buttonState = check();
        if (G_BUTTON_CURRENT_STATE == LOW)  // on release
        {
            if (buttonState == ButtonState::ShortPress)
            {
                Serial.println("To do");
            }
            else if (buttonState == ButtonState::LongPress)
            {
                EEPROM.write(0, 69);
                EEPROM.commit();
                startSetupAp();
            }
        }
    }

    G_BUTTON_LAST_STATE = G_BUTTON_CURRENT_STATE;
}

void loop()
{
    G_SERVER.handleClient();
    enterSetupStateOnButtonClick();
}
