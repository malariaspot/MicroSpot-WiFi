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
}


bool Mechanical::restart(){
  Serial.begin(baudios);
}

bool Mechanical::release(){

  Serial.end();
}

//Home the axis
bool Mechanical::homeAxis(){

}

//Uninterruptible move
//without speed
bool Mechanical::moveAxis(float X,float Y){

}

//with speed
bool Mechanical::moveAxis(float X,float Y,float F){

}

//Interruptible move (jog)
bool Mechanical::jogAxis(float X,float Y){

}

//with speed
bool Mechanical::jogAxis(float X,float Y,float F){

}

//stop jogging movement.
bool Mechanical::stopJog(){

}

//Ask GRBL for position
bool Mechanical::updatePos(){
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
    return true;
  }
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
