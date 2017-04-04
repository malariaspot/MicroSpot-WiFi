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
        st = OFF;
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
  if(st >= LOCK)
  {
    Serial.println("$h");
    Line * message;
    this->receiveLines(message);
    if(this->checkSanity(message))
    {
      st = IDLE;
      pos.x = 0;
      pos.y = 0;
      this->eraseBuffer(message);
      return true;
    }
    else
    {
      st = ERROR;
      return false;
    }
  }
  else
  {
    return false;
  }
}

//Uninterruptible movement
bool
Mechanical::moveAxis(float X,float Y,float F)
{
  if(st > LOCK)
  {
    Serial.println("G1 X" + String(X , 3) + " Y" + String(Y , 3) + " F" + String(F , 3));
    Line * message;
    this->receiveLines(message);
    if(this->checkSanity(message))
    {
      st = IDLE;
      pos.x = X;
      pos.y = Y;
      return true;
    }
    else
    {
      st = ERROR;
      return false;
    }
  }
  else
  {
    return false;
  }
}

//Interruptible movement
//with speed
bool
Mechanical::jogAxis(float X,float Y,float F)
{
  //TODO
  return false;
}

//stop jogging movement.
bool
Mechanical::stopJog()
{
  //TODO
  return false;
}

////////////////////
// Info reporting //
//                //
////////////////////

//Report position
void
Mechanical::getPos()
{
  //TODO
}

//Report config
void
Mechanical::getConfig()
{
  Serial.println("$$");
  //TODO
}

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
    delete(p->prev);
  }
  delete(buf);
  buf = NULL;
}

/////////////////////////////////
// Status management utilities //
/////////////////////////////////

//Ask GRBL for position
bool Mechanical::updatePos(){

  if(st > LOCK)
  {
    Serial.println("?");
    Line * message;
    this->receiveLines(message);
    if(this->checkSanity(message))
    {
      int index;
      if(index = message->content.indexOf("MPos:") < 0){
        this->eraseBuffer(message);
        return false;
      }else{
        pos.x = message->content.substring(index + 5, index + 10).toFloat();
        pos.y = message->content.substring(index + 12, index + 17).toFloat();
        this->eraseBuffer(message);
        return true;
      }
    }
    else
    {
      st = ERROR;
      this->eraseBuffer(message);
      return false;
    }
  }
  else
  {
    return false;
  }

}
