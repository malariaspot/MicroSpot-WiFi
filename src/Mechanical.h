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
  OFF,
  OFFLINE,
  ERROR,
  LOCK,
  MOVING,
  OUTDATED,
  IDLE
};

class Mechanical
{

  private:
    int baudios;
    double timeStamp;
    Status st = OFF;
    Position pos;

    bool updatePos();
    bool checkSanity(Line *message);
    bool receiveLines(Line *message);
    void eraseBuffer(Line * buf);

  public:
    Mechanical(int baud);

    //Serial activation and release
    bool toggle(bool state);

    //Movement
    bool homeAxis();
    bool moveAxis(float X,float Y,float F);
    bool jogAxis(float X,float Y,float F);
    bool stopJog();

    //Status reporting
    void getPos();
    void getConfig();
    int getStatus();


};
