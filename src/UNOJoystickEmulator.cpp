#define _CRT_SECURE_NO_WARNINGS

#undef DEBUG

#include "json.hpp"
#include "SerialPort.hpp"
#include <string>
#include <sstream>
#include <unordered_map>
#include <functional>
#include <utility>
#include <list>
#include <memory>
#include <conio.h>

#define LMB 0x01
#define RMB 0x02
#define KEY0 0x30
#define KEY1 0x31
#define KEY2 0x32
#define KEY3 0x33
#define KEY4 0x34
#define KEY5 0x35
#define KEY6 0x36
#define KEY7 0x37
#define KEY8 0x38
#define KEY9 0x39

enum class FactorType { BUTTON, JOYSTICK };

struct Factor
{
    unsigned ID;
    FactorType type;
    Factor(FactorType factorType = FactorType::BUTTON) : type(factorType)
    {
        static unsigned id = 0;
        ID = id;
        id++;
    }
    virtual void update(bool state) {}
    virtual void updateState() const {}
    virtual ~Factor() {}
};

struct Button : public Factor
{
    int pin;
    int emulatedKeyCode;
    bool state;
    //std::function<void(Button*)> callback;

    Button(int _pin = -1, bool _state = 0) :
        Factor(),
        state(_state),
        pin(_pin),
        emulatedKeyCode(0x00)
    {}

    virtual void update(bool _state)
    {
        state = _state;
    }

    virtual void updateState(void) const
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
    DataSerializer mDataSerializer;

    int showMainMenu(void) const
    {
        int option;
        std::cout << "\n<--------------------------->\n";
        std::cout << "1. Load controller settings\n";
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
        std::cout << "/ or configure your joystick.                                  /\n";
        std::cout << "/*************************************************************/\n";
    }
    
    void showSavedFactors(const std::list<std::unique_ptr<Factor>>& factors) const
    {
        for (auto& factor : factors)
        {
            std::cout << "ID : "<<factor->ID;
            if (factor->type == FactorType::BUTTON)
            {
                std::cout << " Type: Button ";
                Factor* tempFactor = factor.get();
                Button* tempButton = static_cast<Button*>(tempFactor);
                std::cout << " PIN : "<<tempButton->pin << " STATE: "<<tempButton->state<<"\n";
            }
            else
            {

            }
        } 
    }
};


class Emulator : public Gui
{
    unsigned mMsgSize;
    const char* mportId;
    char* mMsgGet;
    SerialPort* mArduino;
    std::list<std::unique_ptr<Factor>> mFactors;

public:
    
    explicit Emulator(const char* portId)
        :   mportId(portId),
            mMsgSize(4),
            mArduino(nullptr)
    {
        std::cout << "Connecting...\n";
        char* portName;
        mMsgGet = new char[mMsgSize];
        portName = new char[15];   
        strcpy(portName,"\\\\.\\COM");
        strcat(portName,mportId);

        mArduino = new SerialPort(portName);
        delete[] portName;
    }

    ~Emulator()
    {
        delete mArduino;
    }

    void getSerialMessageAndSaveExistingFactors(void)
    {
        bool readEverything = false;
        int numDeclaredFactors;
        int configuredFactors = 0;

        std::cout << "\nEnter the number of buttons you want to config : ";
        std::cin >> numDeclaredFactors;
        //std::cout << "\nEnter the number of buttons you want to config : ";
        //std::cin >> numDeclaredJoysticks;

        while (!readEverything && mArduino->isConnected())
        {
            if (numDeclaredFactors == configuredFactors)
                readEverything = true;

            // reading factors data one by one (data is reperented by 4 bytes - b/j | 0..13 | 0..1 - for example b100
            // meaning that button is connected to pin 10 and it is currently in off state 
            if (mArduino->readSerialPort(mMsgGet, mMsgSize) != 0)
            {
                static int temp = 0;
                if (temp % 2 == 1)
                {
                    temp++;
                    break;
                }
                char bData[5] = { ' ',' ',' ',' ','\0' };
                memcpy(bData, mMsgGet, sizeof(char) * 4);

                if (bData[0] == 'b') // button detected 
                {
                    char* pin = nullptr;
                    if (bData[1] == '0')
                    {
                        pin = new char[2];
                        pin[0] = bData[2];
                        pin[1] = '\0';
                    }
                    else
                    {
                        pin = new char[3];
                        pin[0] = bData[1];
                        pin[1] = bData[2];
                        pin[2] = '\0';
                    }
                    std::cout << bData << "\n";
                    mFactors.emplace_back(new Button(atoi(pin), (bData[3] == '0') ? 0 : 1));
                    delete [] pin;
                }
                else if (mMsgGet[0] == 'j') // joystick detected
                {
                    std::cout << "joystick";
                }
                configuredFactors++;
                temp++;
            }
        }
        std::cout << "\n .. detected ..\n";
        system("pause");
    }

    void configureFactors(void)
    {
        std::cout << "\n";
        for (auto& factor : mFactors)
        {
            std::cout << "ID : " << factor->ID;
            if (factor->type == FactorType::BUTTON)
            {
                std::cout << " | Type: Button | Enter hex VirtualKeyCode to set a key to be emulated : ";
                Factor* tempFactor = factor.get();
                Button* tempButton = static_cast<Button*>(tempFactor);
                int vKeyCode;
                std::cin >> std::hex >> vKeyCode;
                tempButton->emulatedKeyCode = vKeyCode;
                //std::cout << std::hex << tempButton->emulatedKeyCode << "\n";
                //std::cout << " PIN : " << tempButton->pin << " STATE: " << tempButton->state << "\n";

            }
            else
            {

            }
        }
        /*
        int c;
        while (true)
        {
            std::cin.get();
            c = getchar();
            uint8_t f;
            SHORT zm = VkKeyScanA(c);
            memcpy(&f, &zm, sizeof(uint8_t));
            std::cout << std::hex << (int)f << "\n";
        }
        */
    }
    void setUpNewJoystick(void)
    {
        showSetUpMenu();
        getSerialMessageAndSaveExistingFactors();
        showSavedFactors(mFactors);
        configureFactors();
    }

    void run(void)
    {
        int option = showMainMenu();
        switch (option)
        {
            case 1:
                
            break;
            case 2: 
                setUpNewJoystick();
                 
            break;
            default: std::cout << "ASD"; exit(EXIT_FAILURE); break;
        }

        mMsgSize = 4;
        bool state = 0;

        while (mArduino->isConnected())
        {  
            Sleep(10);
            for (auto& factor : mFactors)
            {
                char bData[5] = { ' ',' ',' ',' ','\0' };
                //memcpy()
                if (mArduino->readSerialPort(mMsgGet, mMsgSize) == mMsgSize)
                {
                    memcpy(bData, mMsgGet, sizeof(char) * 4);
                    std::cout << bData<< "\n";
                    state= (bData[3] == '0') ? false : true;
                    //std::cout << "STATE - " << state << "\n";
                }
                
                factor->update(state);
                factor->updateState();
                /*
                if (factor->type == FactorType::BUTTON)
                {

                    Factor* tempFactor = factor.get();
                    Button* tempButton = static_cast<Button*>(tempFactor);
                }
                else
                {

                }
                */
                
            }
            //mArduino->readSerialPort(mMsgGet, mMsgSize);

        }

        delete [] mMsgGet;
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
