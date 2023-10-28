#include "CameraHandler.h"

void CameraHandler::begin()
{
    // Inicialização da câmera
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
    camSettings.xclk_freq_hz = 20000000;
    camSettings.pixel_format = PIXFORMAT_JPEG;

    if (psramFound())
    {
        camSettings.frame_size = FRAMESIZE_XGA;
        camSettings.jpeg_quality = 10;
        camSettings.fb_count = 2;
    }
    else
    {
        camSettings.frame_size = FRAMESIZE_XGA;
        camSettings.jpeg_quality = 12;
        camSettings.fb_count = 1;
    }

    esp_err_t err = esp_camera_init(&camSettings);
    if (err != ESP_OK)
    {
        return;
    }
}

camera_fb_t *CameraHandler::captureSnapshot()
{
    camera_fb_t *fb = esp_camera_fb_get();
    return fb;
}
