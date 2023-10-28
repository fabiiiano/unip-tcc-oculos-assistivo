#include "WiFiHandler.h"

bool WiFiHandler::connect(const char *ssid, const char *password)
{
    unsigned long startingTime = millis();
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        if ((millis() - startingTime) > timeout)
        {
            return false;
        }
    }
    return true;
}

void WiFiHandler::reconnect()
{
    Serial.println("Reconnecting to WiFi...");
    WiFi.disconnect();
    WiFi.reconnect();

    unsigned long startTime = millis();

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);

        if (millis() - startTime > timeout)
        {
            Serial.println("WiFi connection failed. Will retry...");
            startTime = millis();
            WiFi.reconnect();
        }
    }

    Serial.println("WiFi reconnected successfully.");
}

bool WiFiHandler::isConnected()
{
    return WiFi.status() == WL_CONNECTED;
}
