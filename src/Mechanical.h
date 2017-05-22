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
    Status st = OFF;
    Position pos;
    Position maxpos;

    
    struct AfterStatus{
      Status success, failure;
    }after;
    

    bool checkSanity(char * buffer);
    //Asks GRBL its position with "?".
    bool askPos();
    //Safely send a command, and expect a response or not.
    bool sendCommand(String command, Status atLeast, Status success, Status failure);
    //flush input serial stream
    void flush();
    //Change the status
    void setStatus(Status stat);
    //loop for checking the serial
    void serialListen();
    //Restart serial communication.
    void restartAll();
    //Analyze the error and act in consequence.
    void errorHandler(int errNum);
    //Analyze the alarm and act in consequence.
    void alarmHandler(int alarmNum);

    String statusToString(Status status) {
        switch (status) {
            case OFF: return "Serial connection turned off";
            case OFFLINE: return "Serial connection establishment timed out!";
            case ERROR: return "Error when sending the command! (checkSanity error)";
            case LOCK: return "Axis locked";
            case HOMING: return "Now homing...";
            case MOVING: return "Movement in progress";
            //TODO update. This was set for compatibility purposes
            case JOGGING: return "Movement in progress";
            case IDLE: return "Action completed X: " + pos.x + " Y: " + pos.y;
            default: return "";
        }
    }

  public:
    //Instantiation
    Mechanical(int baud); //Instantiate the object and choose baudrate.

    //when true, the mechanical stage is making a movement that may
    //take a while to confirm. (homing and moves)
    bool longWait;

    //Serial activation and release
    bool toggle(bool state); //Turn on or off the serial interface
    
    //Reset GRBL
    bool reset();

    //Movement
    bool homeAxis(); //Take axis to home position.
    bool moveAxis(String X, String Y, String F); //Ininterruptible move to (X,Y) at speed F.
    bool jogAxis(String X,String Y,String F, String R, String s); //Interruptible move to (X,Y) at speed F.
    bool stopJog();  //Stop an interruptible movement.
    bool unlockAxis(); //Send and unlock token to GRBL. Breaks stability. Devs only.
    bool toggleLight(int intensity); //turn on or off the lights.

    //Status reporting
    bool getPos(); //Reports current position.
    int getStatus(); //Returns a number corresponding the status.

    void addObserver(MicroServer * ms); //ADDED - for the observation pattern
    void run(); //loop function to access scheduling features.
};

#endif //MECHANICAL_H
