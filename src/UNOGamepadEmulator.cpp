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
    virtual void updateFactor(bool state, JoyPosition* joyPosition) = 0;
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

    inline virtual void updateFactor(bool _state, JoyPosition* joyPosition) override
    {
        state = _state;
    }

    virtual void updateEmulatedStuff(void) const override
    {
        if (state == true) 
        { 
            keybd_event(emulatedKeyCode, 0, 0, 0);
        }
        else 
        { 
            keybd_event(emulatedKeyCode, 0, KEYEVENTF_KEYUP, 0);
        }
    }
};

struct Joystick : public Factor
{
    int nr;

    Joystick(int _nr = -1) :
        Factor(FactorType::JOYSTICK),
        nr(_nr)
    {}

    inline virtual void updateFactor(bool _state, JoyPosition* joyPosition) override
    {
       
    }

    virtual void updateEmulatedStuff(void) const override
    {
        
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
                    std::cout << msgStr << " - button detected on pin "<<pin<<"\n";

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
                std::cout << " | Type: Joystick | Choose either 1 for emulating mouse or 2 for emulating 4 buttons : ";
                Factor* tempFactor = factor.second.get();
                Joystick* tempJoystick = static_cast<Joystick*>(tempFactor);
                
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

                    auto pair = mMap.find("b" + std::string(pin));
                    if (pair == mMap.end())
                    {
                        std::cerr << "Error! Undefined data occured on serial port. Unknown pin - not assigned to any factor.\n";
                    }
                    else
                    {
                        mMap["b" + std::string(pin)]->updateFactor((msgStr[3] == '0') ? false : true, nullptr);
                        mMap["b" + std::string(pin)]->updateEmulatedStuff();
                    }
                    delete[] pin;
                }
                else if (msgStr[0] == 'j') // joystick msg arrived
                {

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
