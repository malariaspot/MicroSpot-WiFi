#include "Mechanical.h"


#define TIMEOUT 4000
#define REQUESTLIMIT 200
#define TICK 100

#define ENABLEPIN 14



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
    return true;
  }
  else
  {
    Serial.end();
    digitalWrite(ENABLEPIN,HIGH);
    st = OFF;
    return true;
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
Mechanical::getConfig(Line * config)
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

//After sending a command, check if GRBL understood well.
bool
Mechanical::checkSanity(Line *message)
{
  Line *p = message;
  while(p->next != NULL) p = p->next; //navigate to last line
  if(p->content.equals("ok") && p->prev->content.equals("ok")) //if the two last lines are "ok"
  {
    //delete the last two "ok" lines.
    p->prev->prev->next = NULL; //Is this necessary? -> YES
    delete(p->prev);
    delete(p);
    return true;
  }
  else
  {
    //Deletes whole buffer
    eraseBuffer(message);
    return false;
  }
}

//Receive the lines and put them into a concatenated Line
//list.
bool
Mechanical::receiveLines(Line *message)
{
  Line *p = message;
  if(Serial.available() == 0) return false;
  while(Serial.available() > 0)
  {
    p->content = Serial.readStringUntil('\n');
    p->next = new(Line);
    p = p->next;
  }
  //link the previous pointers
  while(p->next != NULL)
  {
    p->next->prev = p;
    p = p->next;
  }
  return true;
}

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

/////////////////////////////////
// Status management utilities //
/////////////////////////////////

//Ask GRBL for position, and update our local variables.
//The machine needs to have it's axis stopped.
bool Mechanical::updatePos(){

  bool result;
  Line * response;
  result = sendCommand("$h",OUTDATED,IDLE,ERROR,response);
  if(result){
    int index;
    if(index = response->content.indexOf("MPos:") < 0){
      this->eraseBuffer(response);
      st = ERROR;
      return false;
    }else{
      pos.x = response->content.substring(index + 5, index + 10).toFloat();
      pos.y = response->content.substring(index + 12, index + 17).toFloat();
      this->eraseBuffer(response);
      return true;
    }
  }
  else{
    return false;
  }
}


//Safely send a command, under certain conditions, with certain consequences,
//and expecting or not, a response that will be stored in a Line list.
bool
Mechanical::sendCommand(String command, Status atLeast, Status success, Status failure)
{
  return this->sendCommand(command,atLeast,success,failure,NULL);
}

bool
Mechanical::sendCommand(String command, Status atLeast,
  Status success, Status failure, Line * response)
{
  if(st >= atLeast)
  {
    Serial.println(command);
    Line * message;
    this->receiveLines(message);
    if(this->checkSanity(message))
    {
      st = success;
      if(response)
        response = message;
      else
        this->eraseBuffer(message);
      return true;
    }
    else
    {
      st = failure;
      return false;
    }
  }
  else
  {
    return false;
  }
}
