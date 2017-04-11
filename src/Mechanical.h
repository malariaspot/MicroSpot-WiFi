#ifndef MECHANICAL_H
#define MECHANICAL_H

#include "Arduino.h"
#include "Position.h"
#include "Line.h"
#include "Status.h"

class MicroServer;

class Mechanical
{

  private:
    MicroServer * microServer; //ADDED - for the observation pattern
    int baudios;
    double timeStamp;
    Status st = OFF;
    Position pos;

    bool updatePos(); //Asks GRBL its position.

    //Safely send a command, and expect a response or not.
    bool sendCommand(String command, Status atLeast, Status success, Status failure);
    bool sendCommand(String command, Status atLeast, Status success, Status failure, Line * response);
    bool receiveLines(Line *message); //Receive lines from GRBL.
    bool checkSanity(Line *message); //Check if the response from GRBL is ok.
    //checkSanity deletes the Line list if it yields a FALSE.


  public:
    //Instantiation
    Mechanical(int baud); //Instantiate the object and choose baudrate.

    //Serial activation and release
    bool toggle(bool state); //Turn on or off the serial interface

    //Movement
    bool homeAxis(); //Take axis to home position.
    bool moveAxis(float X,float Y,float F); //Ininterruptible move to (X,Y) at speed F.
    bool jogAxis(float X,float Y,float F); //Interruptible move to (X,Y) at speed F.
    bool stopJog();  //Stop an interruptible movement.

    //Status reporting
    bool getPos(Position p); //Stores the position in the argument "p".
    bool getConfig(Line * config); //Stores the config lines into Line list "config".
    //After using getConfig, the Line should be erased with eraseBuffer.
    int getStatus(); //Returns a number corresponding the status.

    static void eraseBuffer(Line * buf); //deletes whole list of lines.

    void addObserver(MicroServer * ms); //ADDED - for the observation pattern
    void notifyObserver(); //ADDED - for the observation pattern
};

#endif //MECHANICAL_H