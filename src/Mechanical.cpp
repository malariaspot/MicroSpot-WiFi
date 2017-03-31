#include "Mechanical.h"

#include "Arduino.h"

///////////////////////////////////////////////
// Motion control calls with GRBL GCODE
//
///////////////////////////////////////////////

Mechanical::Mechanical(int baud){
  Serial.begin(baud);
}


//Home the axis
bool Mechanical::homeAxis(){
  Serial.println("$h");
  return true;
}

//Uninterruptible move
//without speed
bool Mechanical::moveAxis(float X,float Y){
  Serial.println("G0 X" + (String) X + " Y" + (String) Y);
  return true;
}

//with speed
bool Mechanical::moveAxis(float X,float Y,float F){
  Serial.println("G0 X" + (String) X + " Y" + (String) Y + " F" + (String) F);
  return true;
}

//Interruptible move (jog)
bool Mechanical::jogAxis(float X,float Y){
  Serial.println("G0 X" + (String) X + " Y" + (String) Y);
  return true;
}

//with speed
bool Mechanical::jogAxis(float X,float Y,float F){
  Serial.println("$J=X" + (String) X + " Y" + (String) Y + " F" + (String) F);
  return true;
}

//Report position
void Mechanical::reportPos(){
  //TODO
}

//Report config
void Mechanical::reportConfig(){
  Serial.println("$$");
  //TODO
}
