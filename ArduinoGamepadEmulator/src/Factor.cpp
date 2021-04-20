#include "Factor.hpp"

Button::Button(uint8_t _pin, bool _state) :
    Factor(),
    state(_state),
    pin(_pin),
    emulatedKeyCode(0x00)
{}

void Button::updateFactor(bool _state, void* joyPosition)
{
    state = _state;
}

void Button::updateEmulatedStuff(void) const
{
    if (state == true)
        keybd_event(emulatedKeyCode, 0, 0, 0);
    else
        keybd_event(emulatedKeyCode, 0, KEYEVENTF_KEYUP, 0);
}

Joystick::Joystick(uint8_t _nr) :
    Factor(FactorType::JOYSTICK),
    mode(JoystickMode::MOUSE),
    nr(_nr),
    buttonEmulatedKeyCode(nullptr),
    buttonState(nullptr),
    mousePos(nullptr)
{}

Joystick::~Joystick()
{
    if (mode == JoystickMode::BUTTONS)
    {
        delete[] buttonEmulatedKeyCode;
        delete[] buttonState;
    }
    else if (mode == JoystickMode::MOUSE)
    {
        delete mousePos;
    }
}

void Joystick::setUpForMouseMode(void)
{
    mousePos = new POINT();

    mouseShift[0] = [](POINT* mousePosition, char axis)
    {
        if (axis == 'x')        mousePosition->y -= 40;
        else if (axis == 'y')    mousePosition->x += 40;
    };
    mouseShift[1] = [](POINT* mousePosition, char axis)
    {
        if (axis == 'x')        mousePosition->y -= 20;
        else if (axis == 'y')   mousePosition->x += 20;
    };
    mouseShift[2] = [](POINT* mousePosition, char axis)
    {
        if (axis == 'x')        mousePosition->y -= 10;
        else if (axis == 'y')   mousePosition->x += 10;
    };
    mouseShift[3] = [](POINT* mousePosition, char axis)
    {
        if (axis == 'x')        mousePosition->y -= 5;
        else if (axis == 'y')   mousePosition->x += 5;
    };
    mouseShift[4] = [](POINT* mousePosition, char axis)
    {
        if (axis == 'x')        mousePosition->y += 5;
        else if (axis == 'y')   mousePosition->x -= 5;
    };
    mouseShift[5] = [](POINT* mousePosition, char axis)
    {
        if (axis == 'x')        mousePosition->y += 10;
        else if (axis == 'y')   mousePosition->x -= 10;
    };
    mouseShift[6] = [](POINT* mousePosition, char axis)
    {
        if (axis == 'x')        mousePosition->y += 20;
        else if (axis == 'y')   mousePosition->x -= 20;
    };
    mouseShift[7] = [](POINT* mousePosition, char axis)
    {
        if (axis == 'x')        mousePosition->y += 40;
        else if (axis == 'y')   mousePosition->x -= 40;
    };
}

void Joystick::setUpForButtonsMode(void)
{
    buttonEmulatedKeyCode = new int[4];
    buttonState = new bool[4];
}

void Joystick::updateFactor(bool _state, void* joyPosition)
{
    JoyPosition* newPos = (JoyPosition*)joyPosition;
    position = *newPos;

    /*
        1..3x - UP [0]
        5..8x - DOWN [1]
        x5..8 - LEFT [2]
        x1..3 - RIGHT [3]
    */

    if (mode == JoystickMode::BUTTONS)
    {
        if (position.x < 4)         // up
            buttonState[0] = true;
        else if (position.x > 4)    // down
            buttonState[1] = true;
        else                        // neutral on x axis
        {
            buttonState[0] = false;
            buttonState[1] = false;
        }

        if (position.y < 4)         // right
            buttonState[3] = true;
        else if (position.y > 4)    // left
            buttonState[2] = true;
        else                        // neutral on y axis
        {
            buttonState[3] = false;
            buttonState[2] = false;
        }
    }
}

void Joystick::updateEmulatedStuff(void) const
{
    if (mode == JoystickMode::BUTTONS)
    {
        for (uint8_t i = 0; i < 4; i++)
        {
            if (buttonState[i] == true)
                keybd_event(buttonEmulatedKeyCode[i], 0, 0, 0);
            else
                keybd_event(buttonEmulatedKeyCode[i], 0, KEYEVENTF_KEYUP, 0);
        }
    }
    else if (mode == JoystickMode::MOUSE)
    {
        GetCursorPos(mousePos);
        POINT temp = *mousePos;

        if (position.x < 4)
            mouseShift[position.x](mousePos, 'x');
        else if (position.x > 4)
            mouseShift[position.x - 1](mousePos, 'x');
        if (position.y < 4)
            mouseShift[position.y](mousePos, 'y');
        if (position.y > 4)
            mouseShift[position.y - 1](mousePos, 'y');

        if ((temp.x != mousePos->x) || (temp.y != mousePos->y))
            SetCursorPos(mousePos->x, mousePos->y);
    }
}