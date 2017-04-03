
struct Position{
  float x;
  float y;
};

enum Status{
  OFF,
  OFFLINE,
  LOCK,
  MOVING,
  OUTDATED,
  IDLE
};

class Mechanical
{

  public:
    Mechanical(int baud);

    //Serial activation and release
    bool release();
    bool restart();

    //Movement
    bool homeAxis();
    bool moveAxis(float X,float Y);
    bool moveAxis(float X,float Y,float F);
    bool jogAxis(float X,float Y);
    bool jogAxis(float X,float Y,float F);
    bool stopJog();

    //Status reporting
    bool updatePos();
    void reportPos();
    void reportConfig();

  private:
    int baudios;
    double timeStamp;
    Status status;
    Position pos;
};
