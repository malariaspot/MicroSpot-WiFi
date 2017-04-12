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

Mechanical::Mechanical(int baud)
{
  this->baudios = baud;
  maxpos.x = MAX_X;
  maxpos.y = MAX_Y;
  pinMode(ENABLEPIN,OUTPUT);
  this->st = OFF;
}

//Activate and deactivate serial connection.
bool
Mechanical::toggle(bool button)
{
  String notice;
  if(button)
  {
    digitalWrite(ENABLEPIN,LOW);
    delay(TICK); //delay cautelar time before starting the communication.
    Serial.begin(this->baudios);
    timeStamp = millis();
    while(!Serial){
      if(millis()-timeStamp > TIMEOUT) {
        Serial.end();
        st = OFFLINE;
        notice = "Serial connection establishment timed out!";
        notifyObserver(notice);
        return false;
      }
    }
    st = LOCK;
    flush();
    notice = "Conection stablished";
    notifyObserver(notice);
    return true;
  }
  else
  {
    Serial.end();
    digitalWrite(ENABLEPIN,HIGH);
    st = OFF;
    notice = "Serial connection turned off";
    notifyObserver(notice);
    return false;
  }
}

//////////////////////////
// Motion control calls //
//                      //
//////////////////////////

//Home the axes
bool
Mechanical::homeAxis()
{
  bool result;
  String notice;
  result = sendCommand("$h",LOCK,IDLE,ERROR);
  if(result){
    setStatus(MOVING);
    waitForMove();
    pos.x = "0";
    pos.y = "0";
    setStatus(IDLE);
    notice = "Axes homed successfully";
  }
  else{
    setStatus(ERROR);
    notice = "Error when sending the command! (checkSanity error)";
  }
  notifyObserver(notice);
  return result;
}

//Uninterruptible movement
bool
Mechanical::moveAxis(String X, String Y, String F)
{
  bool result;
  String notice;
  result = sendCommand("G1 X" + X + " Y" + Y + " F" + F,
    MOVING,MOVING,ERROR);
  if(result){
    setStatus(MOVING);
    waitForMove();
    pos.x = X;
    pos.y = Y;
    setStatus(IDLE);
    notice = "Movement completed";
  }
  else{
    setStatus(ERROR);
    notice = "Error when sending the command! (checkSanity error)";
  }
  notifyObserver(notice);
  return result;
}

//Interruptible movement
bool
Mechanical::jogAxis(String X, String Y, String F)
{
  return sendCommand("$J=G90 X" + X + " Y" + Y + " F" + F,
    MOVING, OUTDATED, ERROR);
}

//stop jogging movement.
bool
Mechanical::stopJog()
{
  return sendCommand("\x85",MOVING,OUTDATED,ERROR);
}

void Mechanical::unlockAxis(){
  Serial.println("$X");
  setStatus(IDLE);
  notifyObserver("CAUTION: Axis unlocked!");
}

////////////////////
// Info reporting //
//                //
////////////////////

//Report position
bool
Mechanical::getPos()
{
  String notice;
  if(st == IDLE){
    notice = "X: "  + pos.x + " Y: " + pos.y;
  }
  else{
    if (askPos()){
      notice = "X: "  + pos.x + " Y: " + pos.y;
    }
    else{
      setStatus(ERROR);
      notice = "Position cannot be determined";
    }
  }
  notifyObserver(notice);
}

//Report config
bool
Mechanical::getConfig(String *config)
{
  return sendCommand("$$", ERROR, this->st, this->st, config);
}

//Returns the number of the current status
int
Mechanical::getStatus()
{
  return this->st;
}

//Ask GRBL for position, and update our local variables.
bool Mechanical::askPos(){
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
  }
  else{
    return false;
  }
  
}

//Change the status of the machine.
void Mechanical::setStatus(Status stat){
  st = stat;
  return;
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
bool
Mechanical::sendCommand(String command, Status atLeast, Status success, Status failure)
{
  return this->sendCommand(command,atLeast,success,failure,NULL);
}

bool
Mechanical::sendCommand(String command, Status atLeast,
  Status success, Status failure, String *response)
{
  if(st >= atLeast)
  {
    Serial.println(command);
    if(response != NULL){
      this->receiveLines(response);
//    if(this->checkSanity(message)){
        setStatus(success);
        return true;
//    }
//    else
//    {
//      st = failure;
//      return false;
//    }
    }
  }
  else
  {
    *response = "not enough status";
    return false;
  }
}


//Wait for GRBL to send a response.
void Mechanical::waitResponse(){
  while(Serial.available() == 0){
    delay(100);
  }
  return;
}
 
void Mechanical::flush(){
  while(Serial.available() > 0) Serial.read();//empty buffer.
  return;
}

void Mechanical::waitForMove(){
  flush(); //flush previous messages in buffer.
  Serial.println("G4P0"); //send improvised confirm token.
  waitResponse(); //wait for G4P0's confirm
  flush(); //flush the G4P0's confirm.
  waitResponse(); //wait for the actual confirm.
  return;
}

//After sending a command, check if GRBL understood well.
bool
Mechanical::checkSanity(String *message)
{
  int len = message->length();
  if(message->substring(len-3,len) == "ok\r\n" &&
      message->substring(len-8,len-4) == "ok\r\n") //if the two last lines are "ok"
  {
    *message = message->substring(0,len-8); //trim the last two "ok"
    return true;
  }
  else
  {
    return false;
  }
}

//Receive the lines and put them into a concatenated Line
//list.
bool
Mechanical::receiveLines(String *message)
{
  if(Serial.available() == 0) 
  {
    return false;
  }
  while(Serial.available() > 0)
  {
    *message += Serial.readStringUntil('\n');
  }
  return true;
}


/////////////////////////////////
// Server notifying tools      //
/////////////////////////////////


void Mechanical::addObserver(MicroServer * ms) {
  microServer = ms;
}

//TODO - reevaluate states
void Mechanical::notifyObserver(String notice) {
  switch (st) {
      case OFF:
      case OFFLINE:
      case ERROR:
        microServer->error("Error: " + notice);
        break;
      case LOCK:
      case MOVING:
      case OUTDATED:
      case IDLE:
        microServer->success(notice);
        break;

      default: 
        microServer->success(notice); 
        break;
  }
}
