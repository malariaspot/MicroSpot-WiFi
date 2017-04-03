
struct MechStatus{
  float x;
  float y;
  bool updated = false;
  bool locked = true;
  bool released;
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
    void updatePos();
    void reportPos();
    void reportConfig();

  private:
    int baudios;
    double timeStamp;
    MechStatus status;
};
