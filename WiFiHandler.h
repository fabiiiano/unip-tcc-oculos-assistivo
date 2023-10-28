#ifndef WiFiHandler_h
#define WiFiHandler_h

#include <Arduino.h>
#include <WiFi.h>

class WiFiHandler
{
public:
    bool connect(const char *ssid, const char *password);
    void reconnect();
    bool isConnected();
};

#endif
