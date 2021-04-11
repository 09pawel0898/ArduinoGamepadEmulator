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
        std::cout << "/*****************************************************/\n";
        std::cout << "/ Listening for actions on your device...             /\n";
        std::cout << "/ Push button to let the program detect it and        /\n";
        std::cout << "/ choose a key to be emulated by pressing that button /\n";
        std::cout << "/*****************************************************/\n";
        system("pause");
    }
    
};

enum class FactorType { BUTTON, JOYSTICK };

struct Factor
{
    FactorType type;
    std::function<void(void)> callback;
    int id;

    Factor() : type(FactorType::BUTTON)
    {
        static int ID = 0;
        id = ID;
        ID++;
    }
};

class Emulator : public Gui
{
    const unsigned mMsgSize;
    const char* mportId;
    char* mMsgGet;
    SerialPort* mArduino;
    std::list<Factor> mFactors;

public:
    explicit Emulator(const char* portId)
        :   mportId(portId),
            mMsgSize(1),
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

    void listenForActions(void)
    {

    }

    void setUpNewJoystick(void)
    {
        showSetUpMenu();
        std::cout << "Press a button to let me get it..!";

        while (mArduino->isConnected())
        {
            //mArduino->readSerialPort(mMsgGet, mMsgSize);
            //std::string msg = mMsgGet;
        }
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

        

       
        while (mArduino->isConnected())
        {  
            mArduino->readSerialPort(mMsgGet,mMsgSize);
            std::string msg = mMsgGet;
        }
        showMainMenu();

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
