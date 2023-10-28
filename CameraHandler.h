#ifndef CameraHandler_h
#define CameraHandler_h

#include <Arduino.h>
#include "esp_camera.h"

class CameraHandler
{
public:
    void begin();
    camera_fb_t *captureSnapshot();
};

#endif
