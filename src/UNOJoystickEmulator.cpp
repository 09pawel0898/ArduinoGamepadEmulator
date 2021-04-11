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

    Button(int _pin = -1, bool _state = false) :
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
    
    void showSavedFactors(const std::unordered_map<std::string,std::unique_ptr<Factor>>& factors) const
    {
        for (auto& factor : factors)
        {
            std::cout << "ID : "<<static_cast<unsigned>(factor.second->ID);
            if (factor.second->type == FactorType::BUTTON) // is a button
            {
                std::cout << " Type: Button ";
                Factor* tempFactor = factor.second.get();
                Button* tempButton = static_cast<Button*>(tempFactor);
                std::cout << " PIN : "<<tempButton->pin <<"\n";
            }
            else // is a joystick
            {

            }
        } 
    }
};

/*
    Digital pins
    0..13

    Analog pins
    A0..A5

*/

class Emulator : public Gui
{
    unsigned mMsgSize;
    const char* mportId;
    char* mMsgGet;
    SerialPort* mArduino;
    //std::list<std::unique_ptr<Factor>> mFactors;
    std::unordered_map<std::string, std::unique_ptr<Factor>> mMap;

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

    char* getPinFromMsg(const char* msg) const
    {
        char* pin = nullptr;
        if (msg[1] == '0')
        {
            pin = new char[2];
            pin[0] = msg[2];
            pin[1] = '\0';
        }
        else
        {
            pin = new char[3];
            pin[0] = msg[1];
            pin[1] = msg[2];
            pin[2] = '\0';
        }
        return pin;
    }

    void getSerialMessageAndSaveExistingFactors(void)
    {
        bool readEverything = false;
        int numDeclaredFactors = 0;
        int configuredFactors = 0;

        std::cout << "\nEnter the number of buttons you want to config : ";
        std::cin >> numDeclaredFactors;
        

        while (!readEverything && mArduino->isConnected())
        {
            

            // reading factors data one by one (data is reperented by 4 bytes - b/j | 0..13 | 0..1 - for example b100
            // meaning that button is connected to pin 10 and it is currently in off state 
            if (mArduino->readSerialPort(mMsgGet, mMsgSize) != 0)
            {
                char bData[5] = { ' ',' ',' ',' ','\0' };
                memcpy(bData, mMsgGet, sizeof(char) * 4);

                if (bData[0] == 'b') // button detected 
                {
                    char* pin = getPinFromMsg(mMsgGet);
                    std::cout << bData << "\n";

                    configuredFactors++;
                    while (mArduino->readSerialPort(mMsgGet, mMsgSize) == 0) 
                    {}

                    mMap.insert(std::make_pair(std::string(pin), new Button(atoi(pin), false)));
                    //mFactors.emplace_back(new Button(atoi(pin), (bData[3] == '0') ? 0 : 1));
                    delete [] pin;
                }
                else if (mMsgGet[0] == 'j') // joystick detected
                {
                    // to do
                    std::cout << "joystick";
                }
                

            }
            if (numDeclaredFactors == configuredFactors)
                readEverything = true;
        }
        std::cout << "\n .. detected ..\n";
        system("pause");
    }

    void configureFactors(void)
    {
        std::cout << "\n";
        for (auto& factor : mMap)
        {
           
            std::cout << "ID : " << static_cast<unsigned>(factor.second->ID);
            if (factor.second->type == FactorType::BUTTON)
            {
                std::cout << " | Type: Button | Enter hex VirtualKeyCode to set a key to be emulated : ";
                Factor* tempFactor = factor.second.get();
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
        showSavedFactors(mMap);
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
        bool state = false;
        char bData[5] = { ' ',' ',' ',' ','\0' };

        while (mArduino->isConnected())
        {  
            Sleep(10);

            if (mArduino->readSerialPort(mMsgGet, mMsgSize) == mMsgSize)
            {
                memcpy(bData, mMsgGet, sizeof(char) * 4);
                std::cout << bData<< "\n";
                char* pin = getPinFromMsg(mMsgGet);

                auto pair = mMap.find(std::string(pin));
                if (pair == mMap.end())
                {
                    std::cerr << "Error! Undefined data occured on serial port.\n";
                }

                mMap[std::string(pin)]->update((bData[3] == '0') ? false : true);
                
                delete[] pin;
            }
            for( auto& factor : mMap)
                factor.second->updateState();
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
