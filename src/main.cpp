#include <ArduinoJson.h>
#include <EEPROM.h>
#include <WebServer.h>
#include <WiFi.h>

#include <functional>

#include "HardwareSerial.h"
#include "SPIFFS.h"
#include "esp32-hal-gpio.h"

const int G_EPROM_SIZE = 1;

const char* G_AP_SSID = "ESP32-AP";
const char* G_AP_PASSWORD = "123456789";

const int G_BUTTON_PIN = 3;

int G_BUTTON_CURRENT_STATE = LOW;
int G_BUTTON_LAST_STATE = LOW;

int G_START_PRESS = 0;
int G_STOP_PRESS = 0;

String G_MAIN_PAGE = "";
WebServer G_SERVER;

constexpr std::uint8_t WIFI_CONFIGURED_FLAG = 69;

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
        if (doc.containsKey("ssid") and doc.containsKey("passwd"))
        {
            const char* ssid = doc["ssid"];
            Serial.println("Received message: " + String(ssid));
            const char* passwd = doc["passwd"];
            Serial.println("Received message: " + String(passwd));
            EEPROM.write(0, 69);
            EEPROM.commit();
            File file = SPIFFS.open("/wifi.txt", FILE_WRITE);
            file.println(ssid);
            file.println(passwd);
            file.close();

            G_SERVER.send(200, "application/json", R"({"status": "correct"})");
            delay(200);
            WiFi.softAPdisconnect(true);
        }
        else
        {
            G_SERVER.send(200, "application/json", R"({"status": "not_correct"})");
            Serial.println("Failure setting up");
        }
    }
}

bool isWiFiSsidAndPasswordSet() { return EEPROM.read(0) == WIFI_CONFIGURED_FLAG; }

void initMainWebPage()
{
    File file = SPIFFS.open("/main.html");
    if (!file)
    {
        Serial.println("Failed to open file for reading");
        return;
    }

    Serial.println("Saving main.html to cache");
    while (file.available())
    {
        G_MAIN_PAGE += static_cast<char>(file.read());
    }

    file.close();
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

enum class Status
{
    Success,
    Failure
};

Status connectToWiFi()
{
    File file = SPIFFS.open("/wifi.txt");
    if (!file)
    {
        Serial.println("Failed to open wifi.txt file for reading");
        return Status::Failure;
    }

    String ssid = file.readStringUntil('\n');
    String passwd = file.readStringUntil('\n');
    ssid.trim();
    passwd.trim();
    file.close();

    WiFi.begin(ssid, passwd);
    Serial.print("Connecting To WiFi");
    int timeCounter = 0;
    while (WiFi.status() != WL_CONNECTED)
    {
        if (timeCounter > 15)
        {
            Serial.println("Couldn't connect to WiFi with saved credentials.");
            WiFi.disconnect();
            return Status::Failure;
        }
        delay(1000);
        Serial.print(".");
        timeCounter++;
    }
    Serial.println("");
    Serial.println("WiFi Connected!");

    Serial.print("ESP32 Ip Address: ");
    Serial.println(WiFi.localIP());

    G_SERVER.on("/data", HTTP_GET, handleData);
    G_SERVER.begin();

    return Status::Success;
}

void sendBigFile(const String& fileName, const String& type = "text/plain")
{
    File file = SPIFFS.open(fileName, FILE_READ);
    if (!file)
    {
        G_SERVER.send(404, "text/plain", "File not found");
        return;
    }

    G_SERVER.setContentLength(file.size());
    G_SERVER.send(200, type, "");

    const size_t chunkSize = 1024;
    std::uint8_t buffer[chunkSize];
    size_t bytesRead = 0;
    while ((bytesRead = file.read(buffer, chunkSize)) > 0){
        G_SERVER.client().write(buffer, bytesRead);
    }
    file.close();
}

void handleJsRequest()
{
    sendBigFile("/jquery-3.7.1.min.js", "text/javascript");
}

void handleCssRequest()
{
    sendBigFile("/output.css", "text/css");
}

void startSetupAp()
{
    Serial.println("Start Access Point");
    WiFi.softAP(G_AP_SSID, G_AP_PASSWORD);

    delay(100);
    IPAddress Ip(192, 168, 1, 1);
    IPAddress NMask(255, 255, 255, 0);
    WiFi.softAPConfig(Ip, Ip, NMask);

    G_SERVER.enableCORS();
    G_SERVER.on("/", HTTP_GET, handleWiFiSetupPage);
    G_SERVER.on("/setup", HTTP_POST, handleSetup);
    G_SERVER.on("/jquery-3.7.1.min.js", HTTP_GET, handleJsRequest);
    G_SERVER.on("/output.css", HTTP_GET, handleCssRequest);
    G_SERVER.begin();
}

void initialzieInternetConnection()
{
    auto wifi_connection = Status::Failure;
    if (isWiFiSsidAndPasswordSet())
    {
        wifi_connection = connectToWiFi();
    }

    if (wifi_connection == Status::Failure)
    {
        startSetupAp();
    }
}

void setup()
{
    Serial.begin(9600);
    EEPROM.begin(G_EPROM_SIZE);
    mountSpiffs();

    pinMode(G_BUTTON_PIN, INPUT);

    initialzieInternetConnection();
}

ButtonState getButtonState()
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

void commitEEPROMFlag(int addr, int val)
{
    EEPROM.write(addr, val);
    EEPROM.commit();
}

void handleButtonRelease(const ButtonState buttonState)
{
    switch (buttonState)
    {
        case ButtonState::ShortPress:
            Serial.println("To do");
            break;
        case ButtonState::LongPress:
            commitEEPROMFlag(0, WIFI_CONFIGURED_FLAG);
            startSetupAp();
            break;
        default:
            Serial.print("Not handled state: ");
            Serial.println(static_cast<int>(buttonState));
            break;
    }
}

void handleButtonPress(const std::function<void(const ButtonState)>& onPress,
                       const std::function<void(const ButtonState)>& onRelease)
{
    G_BUTTON_CURRENT_STATE = digitalRead(G_BUTTON_PIN);

    if (G_BUTTON_CURRENT_STATE != G_BUTTON_LAST_STATE)
    {
        const auto buttonState = getButtonState();
        if (G_BUTTON_CURRENT_STATE == LOW)  // on release
        {
            onRelease(buttonState);
        }
        else
        {
            onPress(buttonState);
        }
    }

    G_BUTTON_LAST_STATE = G_BUTTON_CURRENT_STATE;
}

void loop()
{
    G_SERVER.handleClient();
    handleButtonPress([](const ButtonState) {}, handleButtonRelease);
}
