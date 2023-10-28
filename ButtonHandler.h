#ifndef ButtonHandler_h
#define ButtonHandler_h

#include <Arduino.h>

class ButtonHandler
{
public:
    ButtonHandler(uint8_t buttonPin);
    void begin();
    void update();
    bool isPressed();
    bool wasPressed();
    
private:
    uint8_t buttonPin;
    bool currentState;
    bool lastState;
    unsigned long lastDebounceTime;
    const unsigned long debounceDelay = 50;
};

#endif
