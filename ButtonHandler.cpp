#include "ButtonHandler.h"

ButtonHandler::ButtonHandler(uint8_t pin)
    : buttonPin(pin), currentState(LOW), lastState(LOW), lastDebounceTime(0) {}

void ButtonHandler::begin()
{
    pinMode(buttonPin, INPUT_PULLUP);
}

void ButtonHandler::update()
{
    int reading = digitalRead(buttonPin);

    if (reading != lastState)
    {
        lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > debounceDelay)
    {
        if (reading != currentState)
        {
            currentState = reading;
        }
    }

    lastState = reading;
}

bool ButtonHandler::isPressed()
{
    return currentState == LOW;
}

bool ButtonHandler::wasPressed()
{
    return (currentState == LOW) && (lastState == HIGH);
}
