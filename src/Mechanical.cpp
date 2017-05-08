#include "Mechanical.h"
#include "MicroServer.h"
#define TIMEOUT 4000
#define REQUESTLIMIT 200
#define TICK 100

#define ENABLEPIN 4

////////////////////
// Public methods //
////////////////////

/////////////////////////////////
// Instatiation and activation //
//                             //
/////////////////////////////////

Mechanical::Mechanical(int baud) {
  this->baudios = baud;
  maxpos.x = MAX_X;
  maxpos.y = MAX_Y;
  pinMode(ENABLEPIN,OUTPUT);
  this->st = OFF;
}

//Activate and deactivate serial connection.
bool Mechanical::toggle(bool button) {
  if(button) {
    digitalWrite(ENABLEPIN,LOW);
    delay(TICK); //delay cautelar time before starting the communication.
    Serial.begin(this->baudios);
    timeStamp = millis();
    while(!Serial){
      if(millis()-timeStamp > TIMEOUT) {
        Serial.end();
        setStatus(OFFLINE);
        return false;
      }
    }
    setStatus(LOCK);
    flush();
    return true;
  }else{
    Serial.end();
    digitalWrite(ENABLEPIN,HIGH);
    setStatus(OFF);
    return false;
  }
}

//////////////////////////
// Motion control calls //
//                      //
//////////////////////////

//Home the axes
bool Mechanical::homeAxis() {
  bool result;
  result = sendCommand("$h",LOCK,IDLE,ERROR);
  return result;
}

//Uninterruptible movement
bool Mechanical::moveAxis(String X, String Y, String F) {
  bool result;
  //String notice;
  result = sendCommand("G1 X" + X + " Y" + Y + " F" + F,
    MOVING,MOVING,ERROR);
  
  return result;
}

//Interruptible movement
bool Mechanical::jogAxis(String X, String Y, String F, String R, String S) {
  String mode;
  if(R == "true"){
    mode = "G91";
  }else{
    mode = "G90";
  }
  String stopping = "";
  if(S == "true"){
    stopping = "\x85\r\n";
  }
  setStatus(OUTDATED);
  return sendCommand(stopping + "$J=" + mode + " X" + X + " Y" + Y + " F" + F, MOVING, OUTDATED, ERROR);
}

//stop jogging movement.
bool Mechanical::stopJog() {
  setStatus(OUTDATED);
  return sendCommand("\x85",MOVING,OUTDATED,ERROR);
}

void Mechanical::unlockAxis() {
  Serial.println("$X");
  setStatus(IDLE);
}

void Mechanical::toggleLight(int intensity){
  int inputNum;
  //saturate intensity between 0 and 255.
  //Not using just min() and max() seems uncanny, 
  //but issue#398 of the framework reveals that they just don' work.
  //Until there is a fix, this is what we got to do.
  if(intensity >= 255){
    inputNum = 255;
  }else if(intensity <= 0){
    inputNum = 0;
  }else{
    inputNum = intensity;
  }
  String input = String(inputNum);
  Serial.println("M03 S" + input);
  microServer->update("Light set to " + input);
}

////////////////////
// Info reporting //
//                //
////////////////////

//Report position
bool Mechanical::getPos() {
  String notice;
  if(st == IDLE) { notice = "X: "  + pos.x + " Y: " + pos.y; }
  else if (askPos()){ notice = "X: "  + pos.x + " Y: " + pos.y; }
  else{ setStatus(ERROR); }
  microServer->update(notice);
}

//Returns the number of the current status
int Mechanical::getStatus() { return this->st; }

//Ask GRBL for position, and update our local variables.
bool Mechanical::askPos() {
  bool result;
  String response;
  result = sendCommand("?", MOVING, st, ERROR);
  if(result){
    int index;
    if(index = response.indexOf("MPos:") < 0){
      setStatus(ERROR);
      return false;
    }else{
      pos.x = response.substring(index + 5, index + 10);
      pos.y = response.substring(index + 12, index + 17);
      return true;
    }
  }else{ return false; }
}

//Change the status of the machine.
void Mechanical::setStatus(Status stat){
  st = stat;
  microServer->update(statusToString(st));
}

/////////////////////
//                 //
// Private methods //
//                 //
/////////////////////


//////////////////////////////////
// Serial input check utilities //
//////////////////////////////////

//Safely send a command, under certain conditions, with certain consequences,
//and expecting or not, a response that will be stored in a Line list.
bool Mechanical::sendCommand(String command, Status atLeast, Status success, Status failure) {
  if(st >= atLeast) {
    Serial.println(command);
    setStatus(success);
    return true;
  }else{
    return false;
  }
}


void Mechanical::flush(){
  while(Serial.available() > 0) Serial.read();//empty buffer.
  return;
}

/////////////////////////////////
// Server notifying tools      //
/////////////////////////////////

void Mechanical::addObserver(MicroServer * ms) { microServer = ms; }
