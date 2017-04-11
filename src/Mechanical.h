#include "Arduino.h"

struct Position{
  float x;
  float y;
};

struct Line{
  Line *prev;
  String content;
  Line *next;
};

enum Status{
  OFF, //Enable pin is off.
  OFFLINE, //Enable pin is on, but serial is not activated.
  ERROR, //Connection has ben stablished, but GRBL is in an ERROR state
  LOCK, //GRBL is in an ALARM/LOCK state, such as limit hit or not homed.
  MOVING, //The motors are expected to be moving.
  OUTDATED, //The motors may be stopped, but the position reported may not be correct.
  IDLE //GRBL is sitting idle, and the position has been reported by the machine.
};

class Mechanical
{

  private:
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
};