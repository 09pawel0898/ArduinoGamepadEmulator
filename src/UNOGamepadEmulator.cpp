#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <assert.h>

#include "json.hpp"
#include "SerialPort.hpp"

#include <string>
#include <unordered_map>
#include <functional>
#include <utility>
#include <list>
#include <memory>

enum class FactorType { BUTTON, JOYSTICK };
enum class JoystickMode { MOUSE, BUTTONS };

//typedef void (*mouseShift)(uint8_t, uint8_t);

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
    int pin;
    int emulatedKeyCode;
    bool state;

    Button(int _pin = -1, bool _state = false) :
        Factor(),
        state(_state),
        pin(_pin),
        emulatedKeyCode(0x00)
    {}

    inline virtual void updateFactor(bool _state, void* joyPosition) override
    {
        state = _state;
    }

    virtual void updateEmulatedStuff(void) const override
    {
        if (state == true) 
            keybd_event(emulatedKeyCode, 0, 0, 0);
        else 
            keybd_event(emulatedKeyCode, 0, KEYEVENTF_KEYUP, 0);
    }
};

struct Joystick : public Factor
{
    /*
    *   0 - up, 1 - down, 2- left, 3 -right
    */

    int nr;
    JoystickMode mode;
    JoyPosition position = { 4,4 };
    
    int* buttonEmulatedKeyCode;         
    bool* buttonState;                  

    POINT* mousePos;
    void (*mouseShift[8])(POINT*, char) = { nullptr };

    Joystick(int _nr = -1) :
        Factor(FactorType::JOYSTICK),
        mode(JoystickMode::MOUSE),
        nr(_nr),
        buttonEmulatedKeyCode(nullptr),
        buttonState(nullptr),
        mousePos(nullptr)
    {}

    ~Joystick()
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

    void setUpForMouseMode(void)
    {
        mousePos = new POINT();

        mouseShift[0] = [](POINT* mousePosition, char axis) 
        {
            if (axis == 'x')        mousePosition->y -= 40;
            else if(axis == 'y')    mousePosition->x += 40;
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

    void setUpForButtonsMode(void)
    {
        buttonEmulatedKeyCode = new int[4];
        buttonState = new bool[4];
    }

    inline virtual void updateFactor(bool _state, void* joyPosition) override
    {
        JoyPosition* newPos= (JoyPosition*)joyPosition;
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

    virtual void updateEmulatedStuff(void) const override
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

            if((temp.x !=mousePos->x) || (temp.y != mousePos->y))
                SetCursorPos(mousePos->x, mousePos->y);
        }
    }
};

class DataSerializer
{
public:
    /*
    /
    /   To do
    /
    */
};

class Gui
{
protected:
    int showMainMenu(const char* portName) const
    {
        int option;
        system("cls");
        std::cout << "Successfully connected with your arduino on port COM" << portName;
        std::cout << "\n<--------------------------------------------------->\n";
        std::cout << "1. Load controller settings from *.json file\n";
        std::cout << "2. Set up new controller\n";
        std::cin >> option;
        system("cls");
        return option;
    }

    void showSetUpMenu(void) const
    {
        std::cout << "/*************************************************************/\n";
        std::cout << "/ Wait for program to detect all factors in your device...    /\n";
        std::cout << "/ Then choose a key to be emulated by pressing each button    /\n";
        std::cout << "/ or configure your joystick.                                 /\n";
        std::cout << "/*************************************************************/\n";
    }
    
    void showSavedFactors(const std::unordered_map<std::string,std::unique_ptr<Factor>>& factors) const
    {
        system("cls");
        std::cout << "-----------Factor-list-----------\n";
        for (auto& factor : factors)
        {
            std::cout << "ID : "<<static_cast<unsigned>(factor.second->ID);
            if (factor.second->type == FactorType::BUTTON) // is a button
            {
                std::cout << " Type: Button ";
                Factor* tempFactor = factor.second.get();
                Button* tempButton = static_cast<Button*>(tempFactor);
                std::cout << "  PIN : "<<tempButton->pin <<"\n";
            }
            else // is a joystick
            {
                std::cout << " Type: Joystick ";
                Factor* tempFactor = factor.second.get();
                Joystick* tempJoystick = static_cast<Joystick*>(tempFactor);
                std::cout << " Nr : " << tempJoystick->nr << "\n";
            }
        } 
    }
};

class Emulator : public Gui
{
    const unsigned mMsgSize;
    const char* mPortId;
    char* mMsg;
    std::unique_ptr<SerialPort> mArduinoPort;
    std::unordered_map<std::string, std::unique_ptr<Factor>> mMap;
    DataSerializer mDataSerializer;

public:
    
    explicit Emulator(const char* portId)
        :   mPortId(portId),
            mMsgSize(4),
            mArduinoPort(nullptr)
    {
        char* portName;
        mMsg = new char[mMsgSize];
        portName = new char[15];   
        strcpy(portName,"\\\\.\\COM");
        strcat(portName,mPortId);
        std::cout << "Connecting...\n";
        mArduinoPort = std::make_unique<SerialPort>(portName);
        delete[] portName;
    }

    char* getPinFromMsg(const char* msg, FactorType type) const
    {
        char* pin = nullptr;
        if (type == FactorType::BUTTON) // button msg
        {
            if (msg[1] == '0')
            {
                pin = new char[2];
                memcpy(pin, msg + 2, 1);
                pin[1] = '\0';
            }
            else
            {
                pin = new char[3];
                memcpy(pin, msg + 1, 2);
                pin[2] = '\0';
            }
        }
        else // joystick msg [ actually not getting a pin in this case but a joystick ID ]
        {
            pin = new char[2];
            memcpy(pin, msg + 1, 1);
            pin[1] = '\0';
        }
        return pin;
    }

    void saveNewFactors(void)
    {
        bool readEverything = false;
        int numDeclaredFactors = 0;
        int configuredFactors = 0;
        
        std::cout << "\nEnter the number of factors you want to config : ";
        std::cin >> numDeclaredFactors;
        
        while (!readEverything && mArduinoPort->isConnected())
        {
            // reading factors data one by one (data is reperented by 4 bytes - b/j | 0..13 | 0..1 - for example b100
            // meaning that button is connected to pin 10 and it is currently in off state 
            if (mArduinoPort->readSerialPort(mMsg, mMsgSize) != 0)
            {
                char msgStr[5] = { ' ',' ',' ',' ','\0' };
                memcpy(msgStr, mMsg, sizeof(char) * 4);

                if (msgStr[0] == 'b') // button detected 
                {
                    char* pin = getPinFromMsg(mMsg,FactorType::BUTTON);

                    auto existing = mMap.find("b" + std::string(pin));
                    if (existing != mMap.end())
                        continue;

                    std::cout << msgStr << " - button detected on pin " << pin << "\n";

                    configuredFactors++;
                    while (mArduinoPort->readSerialPort(mMsg, mMsgSize) == 0) 
                    {}

                    mMap.insert(std::make_pair("b"+std::string(pin), new Button(atoi(pin), false)));
                    delete[] pin;
                }
                else if (mMsg[0] == 'j') // joystick detected
                {
                    char* jNr = getPinFromMsg(mMsg, FactorType::JOYSTICK);
                    
                    auto existing = mMap.find("j" + std::string(jNr));
                    if (existing != mMap.end())
                        continue;

                    std::cout << msgStr << " - joystick nr " << jNr << " detected\n";

                    configuredFactors++;
                    char temp[3] = { '0','0','\0' };
                    do
                    {
                        mArduinoPort->readSerialPort(mMsg, mMsgSize);
                        memcpy(temp, mMsg + 2, 2);
                    } while (strcmp(temp,"44"));

                    mMap.insert(std::make_pair("j" + std::string(jNr), new Joystick(atoi(jNr))));
                    delete[] jNr;
                }
                
            }
            if (numDeclaredFactors == configuredFactors)
                readEverything = true;
        }
    }

    void configureFactors(void)
    {
        std::cout << "\nNow enter hex virtual key codes to set a key to be emulated by button. \n[ For example 0x31 for \"1\" key or 0x70 for \"F1\" key. ]\n";
        std::cout << "Or when configuring a joystick choose either emulating mouse or 4 buttons.\nIn the second case enter virtual key codes for every 4 swing directions.\n";
        std::cout << "[ When asked for mode - type \"buttons\" when you want your joystick to emulate buttons or \"mouse\" otherwise. ]\n\n";

        for (auto& factor : mMap)
        {
            std::cout << "ID : " << static_cast<unsigned>(factor.second->ID);
            if (factor.second->type == FactorType::BUTTON) // button config
            {
                std::cout << " | Type: Button | Code : ";
                Factor* tempFactor = factor.second.get();
                Button* tempButton = static_cast<Button*>(tempFactor);
                int vKeyCode;
                std::cin >> std::hex >> vKeyCode;
                tempButton->emulatedKeyCode = vKeyCode;
            }
            else // joystick config
            {
                std::cout << " | Type: Joystick | Mode : ";
                Factor* tempFactor = factor.second.get();
                Joystick* tempJoystick = static_cast<Joystick*>(tempFactor);
                std::string mode;
                std::cin >> mode;
                std::transform(mode.begin(), mode.end(), mode.begin(), ::tolower);
                
                if (mode == "mouse")
                {
                    tempJoystick->mode = JoystickMode::MOUSE;
                    tempJoystick->setUpForMouseMode();
                }
                else if (mode == "buttons")
                {
                    tempJoystick->mode = JoystickMode::BUTTONS;
                    tempJoystick->setUpForButtonsMode();
                    int vKeyCode;
                    std::cout << "Up swing : ";
                    std::cin >> std::hex >> vKeyCode;
                    tempJoystick->buttonEmulatedKeyCode[0] = vKeyCode;
                    std::cout << "Down swing : ";
                    std::cin >> std::hex >> vKeyCode;
                    tempJoystick->buttonEmulatedKeyCode[1] = vKeyCode;
                    std::cout << "Left swing : ";
                    std::cin >> std::hex >> vKeyCode;
                    tempJoystick->buttonEmulatedKeyCode[2] = vKeyCode;
                    std::cout << "Right swing : ";
                    std::cin >> std::hex >> vKeyCode;
                    tempJoystick->buttonEmulatedKeyCode[3] = vKeyCode;
                }
            }
        }
    }
    
    void setUpNewGamepad(void)
    {
        showSetUpMenu();
        saveNewFactors();
        showSavedFactors(mMap);
        configureFactors();
    }

    void run(void)
    {
        
        int option = showMainMenu(mPortId);
        switch (option)
        {
            case 1:
                
            break;
            case 2: 
                setUpNewGamepad();
                 
            break;
            default: exit(EXIT_FAILURE); break;
        }
        
        char msgStr[5] = { ' ',' ',' ',' ','\0' };

        while (mArduinoPort->isConnected())
        {  
            Sleep(10);

            if (mArduinoPort->readSerialPort(mMsg, mMsgSize) == mMsgSize)
            {
                memcpy(msgStr, mMsg, sizeof(char) * 4);
                std::cout << msgStr<< "\n";

                if (msgStr[0] == 'b') // button msg arrived
                {
                    char* pin = getPinFromMsg(mMsg, FactorType::BUTTON);

                    auto existing = mMap.find("b" + std::string(pin));
                    if (existing == mMap.end())
                    {
                        std::cerr << "Error! Undefined data occured on serial port. Unknown pin - not assigned to any factor.\n";
                    }
                    else
                    {
                        mMap["b" + std::string(pin)]->updateFactor((msgStr[3] == '0') ? false : true, nullptr);
                    }
                    delete [] pin;
                }
                else if (msgStr[0] == 'j') // joystick msg arrived
                {
                    char* jNr = getPinFromMsg(mMsg, FactorType::JOYSTICK);

                    auto existing = mMap.find("j" + std::string(jNr));
                    if (existing == mMap.end())
                    {
                        std::cerr << "Error! Undefined data occured on serial port. Unknown pin - not assigned to any factor.\n";
                    }
                    else
                    {
                        JoyPosition pos;
                        pos.x = static_cast<uint8_t>(msgStr[2]) - 48;
                        pos.y = static_cast<uint8_t>(msgStr[3]) - 48;

                        //std::cout << +pos.x << " " << +pos.y << "\n";
                        mMap["j" + std::string(jNr)]->updateFactor(false, &pos);
                        //mMap["b" + std::string(pin)]->updateEmulatedStuff();
                    }
                    delete[] jNr;
                }
                
            }
            for(auto& factor : mMap)
                factor.second->updateEmulatedStuff();
        }

        delete [] mMsg;
    }

};

int main(void)
{
    char port[3];
    std::cout << "Enter COM port that your device is connected to : ";
    std::cin >> port;
    Emulator emulator(port);
    emulator.run();
}
