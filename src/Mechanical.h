#ifndef MECHANICAL_H
#define MECHANICAL_H


#include "Arduino.h"
#include "Position.h"
#include "Status.h"

#define MAX_X "50"
#define MAX_Y "15"

class MicroServer;

class Mechanical
{

  private:
    MicroServer * microServer; //ADDED - for the observation pattern
    int baudios;
    double timeStamp;
    Status st = OFF;
    Position pos;
    Position maxpos;

    bool askPos(); //Asks GRBL its position with "?".

    //Safely send a command, and expect a response or not.
    bool sendCommand(String command, Status atLeast, Status success, Status failure);
    bool sendCommand(String command, Status atLeast, Status success, Status failure, String *response);
    //Receive lines from GRBL.
    bool receiveLines(String *message);
    //Check if the response from GRBL is ok.
    bool checkSanity(String *message);
    
    //Wait for a response from GRBL
    void waitResponse(); 
    //flush input serial stream
    void flush();
    //wait for a movement to finish before continuing execution.
    void waitForMove();
    
    //Change the status
    void setStatus(Status stat);


  public:
    //Instantiation
    Mechanical(int baud); //Instantiate the object and choose baudrate.

    //Serial activation and release
    bool toggle(bool state); //Turn on or off the serial interface

    //Movement
    bool homeAxis(); //Take axis to home position.
    bool moveAxis(String X, String Y, String F); //Ininterruptible move to (X,Y) at speed F.
    bool jogAxis(String X,String Y,String F); //Interruptible move to (X,Y) at speed F.
    bool stopJog();  //Stop an interruptible movement.
    void unlockAxis(); //Send and unlock token to GRBL. Breaks stability. Devs only.

    //Status reporting
    bool getPos(); //Reports current position.
    bool getConfig(String *config); //Stores the config lines into Line list "config".
    int getStatus(); //Returns a number corresponding the status.

    void addObserver(MicroServer * ms); //ADDED - for the observation pattern
    void notifyObserver(String notice); //ADDED - for the observation pattern

};

#endif //MECHANICAL_H
