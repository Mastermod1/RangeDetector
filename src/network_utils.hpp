#pragma once

#include <WebServer.h>

extern const int G_EPROM_SIZE;
extern WebServer G_SERVER;
constexpr std::uint8_t WIFI_CONFIGURED_FLAG = 69;

enum class Status
{
    Success,
    Failure
};

void handleData();
void handleWiFiSetupPage();
void handleJsRequest();
void handleCssRequest();
void handleSetup();
void sendBigFile(const String& fileName, const String& type = "text/plain");

void mountSpiffs();
void initialzieInternetConnection();
void commitEEPROMFlag(int addr, int val);

bool isWiFiSsidAndPasswordSet();
Status connectToWiFi();
void startSetupAp();
