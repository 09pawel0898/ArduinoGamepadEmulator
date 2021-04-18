#include "Emulator.hpp"

int Gui::showMainMenu(const char* portName) const
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

void Gui::showSetUpMenu(void) const
{
    std::cout << "/*************************************************************/\n";
    std::cout << "/ Wait for program to detect all factors in your device...    /\n";
    std::cout << "/ Then choose a key to be emulated by pressing each button    /\n";
    std::cout << "/ or configure your joystick.                                 /\n";
    std::cout << "/*************************************************************/\n";
}

void Gui::showSavedFactors(const std::unordered_map<std::string, std::unique_ptr<Factor>>& factors) const
{
    system("cls");
    std::cout << "-----------Factor-list-----------\n";
    for (auto& factor : factors)
    {
        std::cout << "ID : " << static_cast<unsigned>(factor.second->ID);
        if (factor.second->type == FactorType::BUTTON) // is a button
        {
            std::cout << " Type: Button ";
            Factor* tempFactor = factor.second.get();
            Button* tempButton = static_cast<Button*>(tempFactor);
            std::cout << "  PIN : " << static_cast<unsigned>(tempButton->pin) << "\n";
        }
        else // is a joystick
        {
            std::cout << " Type: Joystick ";
            Factor* tempFactor = factor.second.get();
            Joystick* tempJoystick = static_cast<Joystick*>(tempFactor);
            std::cout << " Nr : " << static_cast<unsigned>(tempJoystick->nr) << "\n";
        }
    }
}

Emulator::Emulator(const char* portId)
    : mPortId(portId),
    mMsgSize(4),
    mArduinoPort(nullptr)
{
    char* portName;
    mMsg = new char[mMsgSize];
    portName = new char[15];
    strcpy(portName, "\\\\.\\COM");
    strcat(portName, mPortId);
    std::cout << "Connecting...\n";
    mArduinoPort = std::make_unique<SerialPort>(portName);
    delete[] portName;
}

char* Emulator::getPinFromMsg(const char* msg, FactorType type) const
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

void Emulator::saveNewFactors(void)
{
    bool readEverything = false;
    int numDeclaredFactors = 0;
    int configuredFactors = 0;

    std::cout << "\nEnter the number of factors you want to config : ";
    std::cin >> numDeclaredFactors;
    std::cout << "OK.. Now interact with each of them - this will let me detect them all! \n\n";
    while (!readEverything && mArduinoPort->isConnected())
    {
        if (mArduinoPort->readSerialPort(mMsg, mMsgSize) != 0)
        {
            char msgStr[5] = { ' ',' ',' ',' ','\0' };
            memcpy(msgStr, mMsg, sizeof(char) * 4);

            if (msgStr[0] == 'b') // button detected 
            {
                char* pin = getPinFromMsg(mMsg, FactorType::BUTTON);

                auto existing = mMap.find("b" + std::string(pin));
                if (existing != mMap.end())
                    continue;

                std::cout << msgStr << " - button detected on pin " << pin << "\n";

                configuredFactors++;
                while (mArduinoPort->readSerialPort(mMsg, mMsgSize) == 0)
                {
                }

                mMap.insert(std::make_pair("b" + std::string(pin), new Button(atoi(pin), false)));
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
                } while (strcmp(temp, "44"));

                mMap.insert(std::make_pair("j" + std::string(jNr), new Joystick(atoi(jNr))));
                delete[] jNr;
            }

        }
        if (numDeclaredFactors == configuredFactors)
            readEverything = true;
    }
}

void Emulator::configureFactors(void)
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

void Emulator::setUpNewGamepad(void)
{
    showSetUpMenu();
    saveNewFactors();
    showSavedFactors(mMap);
    configureFactors();
}

void Emulator::run(void)
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
    listenForArduinoEvents();
}

void Emulator::listenForArduinoEvents(void)
{
    char msgStr[5] = { ' ',' ',' ',' ','\0' };
    std::cout << "Running..\n";
    while (mArduinoPort->isConnected())
    {
        Sleep(10);

        if (mArduinoPort->readSerialPort(mMsg, mMsgSize) == mMsgSize)
        {
            memcpy(msgStr, mMsg, sizeof(char) * 4);
            std::cout << msgStr << "\n";

            if (msgStr[0] == 'b') // button msg arrived
            {
                char* pin = getPinFromMsg(mMsg, FactorType::BUTTON);

                auto existing = mMap.find("b" + std::string(pin));
                if (existing == mMap.end())
                    std::cerr << "Error! Undefined data occured on serial port. Unknown pin - not assigned to any factor.\n";
                else
                    mMap["b" + std::string(pin)]->updateFactor((msgStr[3] == '0') ? false : true, nullptr);
                delete[] pin;
            }
            else if (msgStr[0] == 'j') // joystick msg arrived
            {
                char* jNr = getPinFromMsg(mMsg, FactorType::JOYSTICK);

                auto existing = mMap.find("j" + std::string(jNr));
                if (existing == mMap.end())
                    std::cerr << "Error! Undefined data occured on serial port. Unknown pin - not assigned to any factor.\n";
                else
                {
                    JoyPosition pos;
                    pos.x = static_cast<uint8_t>(msgStr[2]) - 48;
                    pos.y = static_cast<uint8_t>(msgStr[3]) - 48;

                    mMap["j" + std::string(jNr)]->updateFactor(false, &pos);
                }
                delete[] jNr;
            }

        }
        for (auto& factor : mMap)
            factor.second->updateEmulatedStuff();
    }
    delete[] mMsg;
}
