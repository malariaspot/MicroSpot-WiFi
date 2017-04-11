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
  pinMode(ENABLEPIN,OUTPUT);
  this->st = OFF;
}

//Activate and deactivate serial connection.
bool
Mechanical::toggle(bool button)
{
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
        return false;
      }
    }
    st = LOCK;
    //Serial.flush();
    return true;
  }
  else
  {
    Serial.end();
    digitalWrite(ENABLEPIN,HIGH);
    st = OFF;
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
  result = sendCommand("$h",LOCK,IDLE,ERROR);
  if(result){
    pos.x = 0;
    pos.y = 0;
    return true;
  }
  else{
    return false;
  }
}

//Uninterruptible movement
bool
Mechanical::moveAxis(float X, float Y, float F)
{
  bool result;
  result = sendCommand("G1 X" + String(X, 3) + " Y" + String(Y, 3) + " F" + String(F, 3),
    MOVING,IDLE,ERROR);
  if(result){
    pos.x = X;
    pos.y = Y;
    return true;
  }
  else{
    return false;
  }
}

//Interruptible movement
bool
Mechanical::jogAxis(float X,float Y,float F)
{
  return sendCommand("$J=G1 X" + String(X , 3) + " Y" + String(Y , 3) + " F" + String(F , 3),
    MOVING,OUTDATED,ERROR);
}

//stop jogging movement.
bool
Mechanical::stopJog()
{
  return sendCommand("\x85",MOVING,OUTDATED,ERROR);
}

////////////////////
// Info reporting //
//                //
////////////////////

//Report position
bool
Mechanical::getPos(Position p)
{
  if(st == IDLE)
  {
    p = pos;
    return true;
  }else{
    if (updatePos()){
      p = pos;
      return true;
    }
    else{
      return false;
    }
  }
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
    this->receiveLines(response);
//    if(this->checkSanity(message))
//    {
      st = success;
      return true;
//    }
//    else
//    {
//      st = failure;
//      return false;
//    }
  }
  else
  {
    *response = "not enough status";
    return false;
  }
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
    *message = "Serial was empty";
    return false;
  }
  while(Serial.available() > 0)
  {
    *message += Serial.readStringUntil('\n');
  }
  return true;
}

/*
//Safely free all memory allocated by a list of lines.
void
Mechanical::eraseBuffer(Line * buf)
{
  Line *p = buf;
   //navigate to last line
  while(p->next != NULL) p = p->next;

  //Deletes whole buffer
  while(p->prev != NULL)
  {
    p = p->prev;
    delete(p->next);
  }
  delete(p);
  delete(buf);
  buf = NULL;
}
*/

/////////////////////////////////
// Status management utilities //
/////////////////////////////////

//Ask GRBL for position, and update our local variables.
//The machine needs to have it's axis stopped.
bool Mechanical::updatePos(){

  bool result;
  String response;
  result = sendCommand("?",OUTDATED,IDLE,ERROR,&response);
  if(result){
    int index;
    if(index = response.indexOf("MPos:") < 0){
      st = ERROR;
      return false;
    }else{
      pos.x = response.substring(index + 5, index + 10).toFloat();
      pos.y = response.substring(index + 12, index + 17).toFloat();
      return true;
    }
  }
  else{
    return false;
  }
}



void Mechanical::addObserver(MicroServer * ms) {
  microServer = ms;
}

//TODO - reevaluate states
void Mechanical::notifyObserver() {
  switch (st) {
      case IDLE:
        microServer->success();
        break;
      case ERROR:
        microServer->error();
        break;
      default: break;
  }
}
