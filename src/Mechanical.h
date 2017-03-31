

class Mechanical
{

  public:
    Mechanical(int baud);
    bool homeAxis();
    bool moveAxis(float X,float Y);
    bool moveAxis(float X,float Y,float F);
    bool jogAxis(float X,float Y);
    bool jogAxis(float X,float Y,float F);
    void reportPos();
    void reportConfig();
};
