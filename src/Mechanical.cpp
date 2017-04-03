#include "Mechanical.h"

#include "Arduino.h"

#define TIMEOUT 4000
#define REQUESTLIMIT 200

///////////////////////////////////////////////
// Motion control calls with GRBL GCODE
//
///////////////////////////////////////////////

Mechanical::Mechanical(int baud){
  baudios = baud;
  Serial.begin(baud);
  status.released = false;
}


bool Mechanical::restart(){
  Serial.begin(baudios);
  status.released = false;
}

bool Mechanical::release(){
  status.locked = true;
  status.updated = false;
  status.released = true;
  Serial.end();
}

//Home the axis
bool Mechanical::homeAxis(){
  if (status.released) return false;
  Serial.println("$h");
  status.locked = false;
  return true;
}

//Uninterruptible move
//without speed
bool Mechanical::moveAxis(float X,float Y){
  if (status.released || status.locked) return false;
  Serial.println("G0 X" + (String) X + " Y" + (String) Y);
  status.x = X;
  status.y = Y;
  status.updated = true;
  return true;
}

//with speed
bool Mechanical::moveAxis(float X,float Y,float F){
  if (status.released || status.locked) return false;
  Serial.println("G0 X" + (String) X + " Y" + (String) Y + " F" + (String) F);
  return true;
}

//Interruptible move (jog)
bool Mechanical::jogAxis(float X,float Y){
  if (status.released || status.locked) return false;
  Serial.println("G0 X" + (String) X + " Y" + (String) Y);
  return true;
}

//with speed
bool Mechanical::jogAxis(float X,float Y,float F){
  if (status.released || status.locked) return false;
  Serial.println("$J=X" + (String) X + " Y" + (String) Y + " F" + (String) F);
  return true;
}

//stop jogging movement.
bool Mechanical::stopJog(){
  if (status.released || status.locked) return false;
  this->status.updated = false;
  Serial.write(0x85);
  return this->updatePos();
}

//Ask GRBL for position
bool Mechanical::updatePos(){
  if (status.released || status.locked) return false;
  Serial.println("?");
  timeStamp = millis();

  //yield() needs to be used not to lock the entire processor.
  while(Serial.available = 0){
    if(millis() - timestamp > TIMEOUT) return false;
  }

  String message;
  int index;
  message = Serial.readString()
  Serial
  if(index = message.indexOf("MPos:") < 0){
    return false;
  }else{
    status.x = message.substring(index + 5,index + 10 ).toFloat();
    status.y = message.substring(index + 12, index + 17).toFloat();
    status.updated = true;
    return true;
  }
}

//Report position
void Mechanical::reportPos(){
  if (status.released) return;
  //TODO
}

//Report config
void Mechanical::reportConfig(){
  if (status.released) return false;
  Serial.println("$$");
  //TODO
}
