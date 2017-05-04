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
  if(result){
    st = MOVING;
    flush();
    waitResponse(); //this locks the whole unit!!
    pos.x = "0";
    pos.y = "0";
    setStatus(IDLE);
  }else{ setStatus(ERROR); }
  return result;
}

//Uninterruptible movement
bool Mechanical::moveAxis(String X, String Y, String F) {
  bool result;
  //String notice;
  result = sendCommand("G1 X" + X + " Y" + Y + " F" + F,
    MOVING,MOVING,ERROR);
  if(result){
    st = MOVING;
    waitForMove();
    pos.x = X;
    pos.y = Y;
    setStatus(IDLE);
  } else { setStatus(ERROR); }
  return result;
}

//Interruptible movement
bool Mechanical::jogAxis(String X, String Y, String F, String R) {
  String mode;
  if(R == "true"){
    mode = "G91";
  }else{
    mode = "G90";
  }
  setStatus(OUTDATED);
  return sendCommand("$J=" + mode + " X" + X + " Y" + Y + " F" + F, MOVING, OUTDATED, ERROR);
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

//Report config
bool Mechanical::getConfig(String *config) {
  return sendCommand("$$", ERROR, this->st, this->st, config);
}

//Returns the number of the current status
int Mechanical::getStatus() { return this->st; }

//Ask GRBL for position, and update our local variables.
bool Mechanical::askPos() {
  bool result;
  String response;
  result = sendCommand("?", MOVING, st, ERROR, &response);
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
  return this->sendCommand(command,atLeast,success,failure,NULL);
}

bool Mechanical::sendCommand(String command, Status atLeast, Status success, Status failure, String *response) {
  if(st >= atLeast) {
    Serial.println(command);
    if(response != NULL) {
      this->receiveLines(response);
      setStatus(success);
      return true;
    }
  }else{
    *response = "not enough status";
    return false;
  }
}

//Wait for GRBL to send a response.
void Mechanical::waitResponse() {
  while(Serial.available() == 0) { delay(100); }
  return;
}

void Mechanical::flush(){
  while(Serial.available() > 0) Serial.read();//empty buffer.
  return;
}

void Mechanical::waitForMove() {
  flush(); //flush previous messages in buffer.
  Serial.println("G4P0"); //send improvised confirm token.
  waitResponse(); //wait for G4P0's confirm
  flush(); //flush the G4P0's confirm.
  waitResponse(); //wait for the actual confirm.
  return;
}

//After sending a command, check if GRBL understood well.
bool Mechanical::checkSanity(String *message) {
  int len = message->length();
  if(message->substring(len-3,len) == "ok\r\n" && message->substring(len-8,len-4) == "ok\r\n") { //if the two last lines are "ok"
    *message = message->substring(0,len-8); //trim the last two "ok"
    return true;
  } else { return false; }
}

//Receive the lines and put them into a concatenated Line
//list.
bool Mechanical::receiveLines(String *message) {
  if(Serial.available() == 0) { return false; }
  while(Serial.available() > 0) {*message += Serial.readStringUntil('\n'); }
  return true;
}

/////////////////////////////////
// Server notifying tools      //
/////////////////////////////////

void Mechanical::addObserver(MicroServer * ms) { microServer = ms; }
