#include "Arduino.h"
#include "ButtonHandler.h"
#include "CameraHandler.h"
#include "WiFiHandler.h"

ButtonHandler ocrButton(BTN_OCR);
ButtonHandler environmentButton(BTN_ENVIRONMENT);
ButtonHandler objectButton(BTN_OBJECT);
CameraHandler camera;
WiFiHandler wifi;

void setup()
{
    ocrButton.begin();
    environmentButton.begin();
    objectButton.begin();
    camera.begin();

    const char *ssid = "CASA";
    const char *password = "ksbw2020";

    // Inicialização da conexão Wi-Fi
    if (!wifi.connect(ssid, password))
    {
        // WiFi connection failed, retry
        wifi.reconnect();
    }

    pinMode(FLASH_PIN, OUTPUT);
    pinMode(BTN_OCR, INPUT_PULLUP);
    pinMode(BTN_ENVIRONMENT, INPUT_PULLUP);
    pinMode(BTN_OBJECT, INPUT_PULLUP);

    Serial.begin(115200);
    Serial.print("Camera pronta! Acesse: http://");
    Serial.println(WiFi.localIP());
    Serial.println("Para ver os botões, acesse /index após o IP.");
}

void turnOnFlash()
{
    digitalWrite(FLASH_PIN, HIGH);
}

void turnOffFlash()
{
    digitalWrite(FLASH_PIN, LOW);
}

void handleButtonPress(int buttonPin, String pin)
{
    Debouncer *currentDebouncer;
    unsigned long *lastPress;

    switch (buttonPin)
    {
    case BTN_OCR:
        currentDebouncer = &ocrButtonDebouncer;
        lastPress = &lastOcrPress;
        break;

    case BTN_ENVIRONMENT:
        currentDebouncer = &environmentButtonDebouncer;
        lastPress = &lastEnvironmentPress;
        break;

    case BTN_OBJECT:
        currentDebouncer = &objectButtonDebouncer;
        lastPress = &lastObjectPress;
        break;
    }

    int reading = digitalRead(buttonPin);
    if (currentDebouncer->update(reading) && currentDebouncer->getState() == LOW)
    {
        turnOnFlash();
        turnOffFlash();
        handleBtnPress(pin);
        *lastPress = millis();
    }
}

void handleBtnPress(String pin)
{
    capturedResponse = pin;
}

void processImage(const char *endpoint, void (*responseHandler)(const String &))
{
    camera_fb_t *fb = captureSnapshot();
    if (fb)
    {
        esp_camera_fb_return(fb);
    }
}

camera_fb_t *captureSnapshot()
{
    camera_fb_t *fb = NULL;
    esp_err_t res = ESP_OK;
    fb = esp_camera_fb_get();
    return fb;
}

#include "base64.h"

#include <ArduinoJson.h>

esp_err_t capture_image_handler(httpd_req_t *req)
{
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb)
    {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "image/jpeg");
    httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=captured.jpg");
    httpd_resp_send(req, (const char *)fb->buf, fb->len);
    esp_camera_fb_return(fb);

    return ESP_OK;
}

void loop()
{
    static unsigned long lastButtonCheckTime = 0;
    unsigned long currentTime = millis();

    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("WiFi connection lost. Attempting to reconnect...");
        wifi.reconnect();
    }
    else
    {
        String buttonOcrValue = "2";
        String buttonEnvironmentValue = "15";
        String buttonObjectValue = "14";

        handleButtonPress(BTN_OCR, buttonOcrValue);
        handleButtonPress(BTN_ENVIRONMENT, buttonEnvironmentValue);
        handleButtonPress(BTN_OBJECT, buttonObjectValue);

        int randomValue;
        String randomValueString;
        randomValue = random(0, 101);

        randomValueString = String(randomValue);

        if (currentTime - lastButtonCheckTime >= 100)
        {
            int ocrButtonState = digitalRead(BTN_OCR);
            int environmentButtonState = digitalRead(BTN_ENVIRONMENT);
            int objectButtonState = digitalRead(BTN_OBJECT);

            if (ocrButtonState == LOW)
            {
                capturedResponse = "ocr" + randomValueString;
            }
            else if (environmentButtonState == LOW)
            {
                capturedResponse = "caption" + randomValueString;
            }
            else if (objectButtonState == LOW)
            {
                capturedResponse = "object" + randomValueString;
            }

            lastButtonCheckTime = currentTime;
        }
    }

    delay(100); // Pequeno atraso para evitar uso excessivo da CPU
}
