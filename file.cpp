#include "Arduino.h"
#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <esp_http_server.h>

#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5

#define BTN_OCR 2
#define BTN_ENVIRONMENT 15
#define BTN_OBJECT 14

#define FLASH_PIN 4

const char *ssid = "CASAB";
const char *password = "ksbw2020";

const unsigned long timeout = 10000; // 10 seconds in milliseconds
const unsigned long debounceDelay = 50; // Tempo de debounce em ms


unsigned long lastEnvironmentPress = 0;
unsigned long lastOcrPress = 0;
unsigned long lastObjectPress = 0;


String logBuffer = ""; // This will hold the log messages
String capturedResponse;


class Debouncer  {
  private:
    uint8_t currentState;              // Estado atual do botão
    uint8_t lastState;                 // Último estado do botão
    unsigned long lastDebounceTime;    // A última vez que o sinal do botão foi debounced
    const unsigned long debounceDelay; // Atraso de debounce em milissegundos

  public:
    Debouncer(unsigned long delay)
      : debounceDelay(delay), lastState(LOW), lastDebounceTime(0) {}

    bool update(uint8_t pinReading)
    {
      bool changed = false;

      // Se a leitura do botão é diferente do seu estado anterior, resetamos o contador de debounce
      if (pinReading != lastState)
      {
        lastDebounceTime = millis();
      }

      // Se o botão foi pressionado por mais tempo que o atraso de debounce, consideramos que o botão mudou de estado
      if ((millis() - lastDebounceTime) > debounceDelay)
      {
        if (pinReading != currentState)
        {
          currentState = pinReading;
          changed = true;
        }
      }

      // Salvamos a leitura do botão
      lastState = pinReading;

      return changed;
    }

    uint8_t getState()
    {
      return currentState;
    }
};

Debouncer ocrButtonDebouncer(debounceDelay);
Debouncer environmentButtonDebouncer(debounceDelay);
Debouncer objectButtonDebouncer(debounceDelay);

void addToLog(const String &message) {
  logBuffer += message + "<br>";
}

esp_err_t log_get_handler(httpd_req_t *req) {
  httpd_resp_send(req, logBuffer.c_str(), -1); // -1 = use string length as content length
  return ESP_OK;
}

httpd_uri_t log_uri = {
  .uri = "/logs",
  .method = HTTP_GET,
  .handler = log_get_handler,
  .user_ctx = NULL
};


httpd_uri_t response_uri = {
    .uri = "/response",
    .method = HTTP_GET,
    .handler = response_get_handler,
    .user_ctx = NULL
  };

httpd_config_t config = HTTPD_DEFAULT_CONFIG();

esp_err_t root_get_handler(httpd_req_t *req) {
  httpd_resp_send(req, "Hello from ESP32-CAM!", -1); // -1 = use string length as content length
  return ESP_OK;
}

esp_err_t response_get_handler(httpd_req_t *req)
{
    // Verifique se há uma resposta capturada
    if (capturedResponse.length() > 0)
    {
        // Envie a resposta capturada como resposta HTTP
        httpd_resp_send(req, capturedResponse.c_str(), -1);
    }
    else
    {
        // Caso não haja resposta capturada, retorne uma resposta vazia ou uma mensagem de erro
        httpd_resp_send(req, "No captured response available", -1);
    }
    return ESP_OK;
}

httpd_uri_t root = {
  .uri = "/",
  .method = HTTP_GET,
  .handler = root_get_handler,
  .user_ctx = NULL
};

void start_webserver() {
  httpd_handle_t server = NULL;
  if (httpd_start(&server, &config) == ESP_OK)
  {
    httpd_register_uri_handler(server, &root);
    httpd_register_uri_handler(server, &log_uri);
    httpd_register_uri_handler(server, &response_uri);
  }

  httpd_uri_t capture_uri = {
    .uri = "/capture",
    .method = HTTP_GET,
    .handler = capture_image_handler,
    .user_ctx = NULL
  };

  httpd_register_uri_handler(server, &capture_uri);
}

bool wifiConnect() {
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

void reconnectWiFi() {
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
      startTime = millis(); // Reset the timer for the next retry
      WiFi.reconnect();
    }
  }

  Serial.println("WiFi reconnected successfully.");
}

void turnOnFlash() {
  digitalWrite(FLASH_PIN, HIGH);
}

void turnOffFlash() {
  digitalWrite(FLASH_PIN, LOW);
}

void handleButtonPress(int buttonPin) {
  Debouncer *currentDebouncer;
  unsigned long *lastPress;
  String action = "";

  // Assign the current debouncer and lastPress variables based on the button
  switch (buttonPin) {
    case BTN_OCR:
      currentDebouncer = &ocrButtonDebouncer;
      lastPress = &lastOcrPress;
      action = "ocr";
      break;
      return;
    case BTN_ENVIRONMENT:
      currentDebouncer = &environmentButtonDebouncer;
      lastPress = &lastEnvironmentPress;
      action = "envioronment";
      break;
      return;
    case BTN_OBJECT:
      currentDebouncer = &objectButtonDebouncer;
      lastPress = &lastObjectPress;
      action = "object";
      break;
      return;
  }

  int reading = digitalRead(buttonPin);

  if (currentDebouncer->update(reading) && currentDebouncer->getState() == LOW) {
    addToLog("Button pressed");
    turnOnFlash();
    processImage(action);
    turnOffFlash();
    *lastPress = millis();
  }
}

void processImage(String action) {
  handleAction(action);
}

camera_fb_t *captureSnapshot() {
  camera_fb_t *fb = NULL;
  esp_err_t res = ESP_OK;
  fb = esp_camera_fb_get();
  return fb;
}

esp_err_t capture_image_handler(httpd_req_t *req) {
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

String randomNumber() {
  int randomValue;
    String randomValueString;
    randomValue = random(0, 101);

    return randomValueString = String(randomValue);
}

void handleAction(const String &action) {
    String random = randomNumber();
    capturedResponse = action + random;
}

void setup() {

  camera_config_t camSettings;
  camSettings.ledc_channel = LEDC_CHANNEL_0;
  camSettings.ledc_timer = LEDC_TIMER_0;
  camSettings.pin_d0 = Y2_GPIO_NUM;
  camSettings.pin_d1 = Y3_GPIO_NUM;
  camSettings.pin_d2 = Y4_GPIO_NUM;
  camSettings.pin_d3 = Y5_GPIO_NUM;
  camSettings.pin_d4 = Y6_GPIO_NUM;
  camSettings.pin_d5 = Y7_GPIO_NUM;
  camSettings.pin_d6 = Y8_GPIO_NUM;
  camSettings.pin_d7 = Y9_GPIO_NUM;
  camSettings.pin_xclk = XCLK_GPIO_NUM;
  camSettings.pin_pclk = PCLK_GPIO_NUM;
  camSettings.pin_vsync = VSYNC_GPIO_NUM;
  camSettings.pin_href = HREF_GPIO_NUM;
  camSettings.pin_sscb_sda = SIOD_GPIO_NUM;
  camSettings.pin_sscb_scl = SIOC_GPIO_NUM;
  camSettings.pin_pwdn = PWDN_GPIO_NUM;
  camSettings.pin_reset = RESET_GPIO_NUM;
  camSettings.xclk_freq_hz = 10000000;
  camSettings.pixel_format = PIXFORMAT_JPEG; // init with high specs to pre-allocate larger buffers


  if (psramFound()) {
    camSettings.frame_size = FRAMESIZE_QVGA; // 320x240
    camSettings.jpeg_quality = 10;
    camSettings.fb_count = 2;
  }
  else {
    camSettings.frame_size = FRAMESIZE_SVGA;
    camSettings.jpeg_quality = 12;
    camSettings.fb_count = 1;
  }
  // camera init
  esp_err_t err = esp_camera_init(&camSettings);
  if (err != ESP_OK) {
    addToLog("Camera init failed with error 0x%x");
    return;
  }

  // start_webserver();
  if (!wifiConnect()) {
    // WiFi connection failed, retry
    reconnectWiFi();
  }

  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
  }

  Serial.begin(115200);
  Serial.print("Camera pronta! Acesse: http://");
  Serial.println(WiFi.localIP());
  Serial.println("Para ver os botões, acesse /index após o IP.");

  start_webserver();

}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection lost. Attempting to reconnect...");
    reconnectWiFi();
  }
  else {
    handleButtonPress(BTN_OCR);
    handleButtonPress(BTN_ENVIRONMENT);
    handleButtonPress(BTN_OBJECT);
  }
  delay(100); // Small delay to avoid excessive CPU usage
}