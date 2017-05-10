#ifndef MECHANICAL_H
#define MECHANICAL_H


#include "Arduino.h"
#include "Position.h"
#include "Status.h"

#define MAX_X "50"
#define MAX_Y "15"

class MicroServer;

class Mechanical {

  private:
    MicroServer * microServer; //ADDED - for the observation pattern
    int baudios;
    double timeStamp;
    Status st = OFF;
    Position pos;
    Position maxpos;

    String inputString;  // a string to hold incoming data
    bool stringComplete;  // whether the string is complete

    //bool askPos(); //Asks GRBL its position with "?".
    //Safely send a command, and expect a response or not.
    //bool sendCommand(String command, Status atLeast, Status success, Status failure);
    //bool sendCommand(String command, Status atLeast, Status success, Status failure, String *response);
    //Receive lines from GRBL.
    //bool receiveLines(String *message);
    //Check if the response from GRBL is ok.
    //bool checkSanity(String *message);
    //Wait for a response from GRBL
    //void waitResponse();
    //flush input serial stream
    //void flush();
    //wait for a movement to finish before continuing execution.
    //void waitForMove();
    //Change the status
    void setStatus(Status stat);

    String statusToString(Status status) {
        switch (status) {
            case OFF: return "Serial connection turned off";
            case OFFLINE: return "Serial connection establishment timed out!";
            case ERROR: return "Error when sending the command! (checkSanity error)";
            case LOCK: return "Axis locked";
            case MOVING: return "Movement in progress";
            case OUTDATED: return "Status outdated";
            case IDLE: return "Action completed";
            default: return "";
        }
    }

  public:
    //Instantiation
    Mechanical(int baud); //Instantiate the object and choose baudrate.
    void setUp();

    void handleSerial();

    //Serial activation and release
    void toggle(bool state); //Turn on or off the serial interface

    //Movement
    void homeAxis(); //Take axis to home position.
    void moveAxis(String X, String Y, String F); //Ininterruptible move to (X,Y) at speed F.
    void jogAxis(String X,String Y,String F, String R, String s); //Interruptible move to (X,Y) at speed F.
    void stopJog();  //Stop an interruptible movement.
    void unlockAxis(); //Send and unlock token to GRBL. Breaks stability. Devs only.
    void toggleLight(int intensity); //turn on or off the lights.

    //Status reporting
    //bool getPos(); //Reports current position.
    //bool getConfig(String *config); //Stores the config lines into Line list "config".
    int getStatus(); //Returns a number corresponding the status.

    void addObserver(MicroServer * ms); //ADDED - for the observation pattern
};

#endif //MECHANICAL_H
