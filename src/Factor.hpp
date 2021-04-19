#ifndef FACTOR_H
#define FACTOR_H

#define _CRT_SECURE_NO_WARNINGS

#include "SerialPort.hpp"

#include <string>
#include <unordered_map>
#include <functional>
#include <utility>
#include <list>
#include <memory>
#include <stdio.h>
#include <assert.h>
#include <algorithm>

enum class FactorType   { BUTTON, JOYSTICK };
enum class JoystickMode { MOUSE, BUTTONS };

struct JoyPosition
{
    uint8_t x;
    uint8_t y;
};

struct Factor
{
    uint8_t ID;
    FactorType type;

    Factor(FactorType factorType = FactorType::BUTTON) : type(factorType)
    {
        static uint8_t id = 0;
        ID = id;
        id++;
    }
    virtual void updateFactor(bool state, void* joyPosition) = 0;
    virtual void updateEmulatedStuff(void) const = 0;
    virtual ~Factor() {}
};

struct Button : public Factor
{
    uint8_t pin;
    int emulatedKeyCode;
    bool state;

    Button(uint8_t _pin = 0, bool _state = false);
    virtual void updateFactor(bool _state, void* joyPosition) override;
    virtual void updateEmulatedStuff(void) const override;
};

struct Joystick : public Factor
{
    /*
    *   0 - up, 1 - down, 2- left, 3 -right
    */

    uint8_t nr;
    JoystickMode mode;
    JoyPosition position = { 4,4 };

    int* buttonEmulatedKeyCode;
    bool* buttonState;

    POINT* mousePos;
    void (*mouseShift[8])(POINT*, char) = { nullptr };

    Joystick(uint8_t _nr = 0);
    ~Joystick();

    void setUpForMouseMode(void);
    void setUpForButtonsMode(void);
    virtual void updateFactor(bool _state, void* joyPosition) override;
    virtual void updateEmulatedStuff(void) const override;
};
#endif
