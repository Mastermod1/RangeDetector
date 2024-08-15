#include "network_utils.hpp"

#include <ArduinoJson.h>
#include <EEPROM.h>
#include "SPIFFS.h"
#include <WiFi.h>
#include <WebServer.h>

const int G_EPROM_SIZE = 1;
WebServer G_SERVER;

const char* G_AP_SSID = "ESP32-AP";
const char* G_AP_PASSWORD = "123456789";

void handleData()
{
    G_SERVER.sendHeader("Content-Type", "application/json");
    G_SERVER.send(200, "application/json", "{\"msg\":\"Hello World " + String(0) + "\"}");
}

void handleWiFiSetupPage()
{
    sendBigFile("/main.html", "text/html");
}

void handleJsRequest() { sendBigFile("/jquery-3.7.1.min.js", "text/javascript"); }

void handleCssRequest() { sendBigFile("/output.css", "text/css"); }

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
            G_SERVER.send(404, "application/json", R"({"status": "not_correct"})");
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
            G_SERVER.send(404, "application/json", R"({"status": "not_correct"})");
            Serial.println("Failure setting up");
        }
    }
}

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

bool isWiFiSsidAndPasswordSet() { return EEPROM.read(0) == WIFI_CONFIGURED_FLAG; }

void mountSpiffs()
{
    if (not SPIFFS.begin(true))
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
    }
}

void sendBigFile(const String& fileName, const String& type)
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
    while ((bytesRead = file.read(buffer, chunkSize)) > 0)
    {
        G_SERVER.client().write(buffer, bytesRead);
    }
    file.close();
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

void commitEEPROMFlag(int addr, int val)
{
    EEPROM.write(addr, val);
    EEPROM.commit();
}
