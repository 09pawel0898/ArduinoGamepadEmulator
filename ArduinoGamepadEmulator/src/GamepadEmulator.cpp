/*
* 
* A program for emulating keyboard or mouse by interacting with componnents connected to Arduino.
* It tracks the messages sent via serial port from arduino, recognizes and handle them properly.
* Its very easily to set up a bunch of components connected to your arduino like buttons, switches, joystick
* and then to assign actions to them.
* 
* Author: Paweł Młynarz 
* LICENSE: MIT
*/

#include "Emulator.hpp"

int main(void)
{
    char port[3];
    std::cout << "Enter COM port that your device is connected to : ";
    std::cin >> port;
    Emulator emulator(port);
    emulator.run();
}
