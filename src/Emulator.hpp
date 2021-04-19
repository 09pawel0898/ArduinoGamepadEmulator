#ifndef EMULATOR_H
#define EMULATOR_H

#include "Factor.hpp"
#include "DataSerializer.hpp"

class Gui
{
protected:
    int showMainMenu(const char* portName) const;
    void showSetUpMenu(void) const;
    void showSavedFactors(const std::unordered_map<std::string, std::unique_ptr<Factor>>& factors) const;
};

class Emulator : public Gui
{
private:
    const unsigned mMsgSize;
    const char* mPortId;
    char* mMsg;
    std::unique_ptr<SerialPort> mArduinoPort;
    std::unordered_map<std::string, std::unique_ptr<Factor>> mMap;
    DataSerializer mDataSerializer;

    char* getPinFromMsg(const char* msg, FactorType type) const;
    void saveNewFactors(void);
    void configureFactors(void);
    void setUpNewGamepad(void);
    void listenForArduinoEvents(void);

public:

    explicit Emulator(const char* portId);
    void run(void);
};

#endif
