#include "Mechanical.h"
#include "MicroServer.h"
#include "charUtils.h"
#include "mathUtils.h"
#include <stdint.h>

#define TIMEOUT 4000
#define REQUESTLIMIT 200
#define TICK 100
#define WATCHDOG_LIMIT 20000

/////////////////////////////////////////
// Internal variables
/////////////////////////////////////////

#define ENABLEPIN 4
#define ENDLINE '\n'
#define BUFFERSIZE 512
#define REQSIZE 512
#define GRBLSIZE 256
#define SERIAL_FRAMERATE 25

int bufIndex, lastIndex;
char serialBuffer[BUFFERSIZE];
char reqBuffer[REQSIZE];
char GRBLcommand[GRBLSIZE];
int expected, infos;
double timeStamp;
double serialStamp;
double watchDogStamp;
bool dogWatching, dogTriggered;
bool posOutdated, answered;
Position afterPos;
WiFiClient askClient;
float max_x,max_y;


/////////////////////////////////////////
// Internal status management
/////////////////////////////////////////

void Mechanical::alarmHandler(int alarmNum){
  switch(alarmNum){
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
      reset();
      break;
    case 6:
    case 7:
    case 8:
    case 9:
    default:
      break;
  }
}

void Mechanical::errorHandler(int errNum){
  switch(errNum){
    case 1:
    case 2:
    case 20:
    case 16:
    case 3:
      answered = true;
      microServer->update("GRBL didn't understand: " + String(GRBLcommand));
      break;
    case 9:
      microServer->update("GRBL is locked. Home to release");
      st = LOCK;
      break;
    case 15:
      answered = true;
      microServer->update("Jog out of bounds");
      break;
    default:
      answered = true;
      microServer->update("Error " + String(errNum) + " ocurred");
      break;
  }
  return;
}

void Mechanical::restartAll(){
    dogWatching = false; //turn off watchdog
    longWait = false; //unlock MicroServer
    bufIndex = 0;
    lastIndex = 0;
    expected = 0;
    infos = 0;
    flush();
}


/////////////////////
//                 //
// Private methods //
//                 //
/////////////////////


//////////////////////////////////
// Serial input check utilities //
//////////////////////////////////


MsgType msgClassify(int from, char * msg){
  if(getCharIndex(from, msg, "ok") >= 0){
    return AFFIRMATIVE;
  }else if(getCharIndex(from, msg, "error") >= 0){
    return ERRONEOUS;
  }else if(getCharIndex(from, msg, "<") >= 0){
    return POSITION;
  }else if(getCharIndex(from, msg, "ALARM") >= 0){
    return ALARM;
  }else if(getCharIndex(from, msg, "Grbl") >= 0){
    return HANDSHAKE;
  }else if(getCharIndex(from, msg, "\r") == from){
    return EMPTYLINE;
  }else if(getCharIndex(from, msg, "[") >= 0){
    return NQMESSAGE;
  }else if(getCharIndex(from, msg, "$") >= 0){
    return MEMORY;
  }else if(getCharIndex(from, msg, ">") >= 0){
    return STARTUP;
  }else{
    return DIRTY;
  }
}



void Mechanical::serialListen(){
//transmit messages from serial.
  while(Serial.available() > 0){
    serialBuffer[bufIndex] = Serial.read();
    bufIndex++;
    if(serialBuffer[bufIndex - 1] == ENDLINE){
      serialBuffer[bufIndex] = '\0';
      switch(msgClassify(lastIndex, serialBuffer)){
        case AFFIRMATIVE:
          expected--;
          break;
        case ERRONEOUS:
          expected--;
          errorHandler(atoi(serialBuffer + lastIndex + 6));
          break;
        case POSITION:
          infos --;
          int a, b, c, d;
          a = getCharIndex(lastIndex, serialBuffer, ":");
          b = getCharIndex(a + 1, serialBuffer, ",");
          c = getCharIndex(b + 1, serialBuffer, ",");
          d = getCharIndex(c + 1, serialBuffer, ",");
          serialBuffer[b] = '\0';
          serialBuffer[c] = '\0';
          serialBuffer[d] = '\0';
          pos.x = String(serialBuffer + a + 1);
          pos.y = String(serialBuffer + b + 1);
          if(st == JOGGING){
            afterPos.x = pos.x;
            afterPos.y = pos.y;
          }
          microServer->send(200,"Position: X: " + pos.x + " Y: " + pos.y, &askClient);
          break;
        case ALARM:
          microServer->update("GRBL Alarm : " + String(serialBuffer[lastIndex + 6]));
          reset();
          answered = true;
          restartAll();
          st = LOCK;
          break;
        case HANDSHAKE:
        case NQMESSAGE:
        case EMPTYLINE:
        case MEMORY:
        case STARTUP:
          break;
        default:
          microServer->update("WRONG RESPONSE: " + String(serialBuffer));
          answered = true;
          restartAll();
          break;
      }
      lastIndex = bufIndex;
      if(expected <= 0){
        restartAll();
        pos.x = afterPos.x;
        pos.y = afterPos.y;
        setStatus(after.success);
      }
      break;
    }
  }
}

//Safely send a command, under certain conditions, with certain consequences,
//and expecting or not, a response that will be stored in a Line list.
bool Mechanical::sendCommand(Status atLeast, Status success, Status failure) {
  if(st >= atLeast) {
    if(dogTriggered){flush(); dogTriggered = false;}
    answered = false;
    this->after.success = success;
    this->after.failure = failure;
    watchDogStamp = millis();
    dogWatching = true;
    Serial.println(GRBLcommand);
    return true;
  }else{
    return false;
  }
}


void Mechanical::flush(){
  while(Serial.available() > 0) Serial.read();//empty buffer.
  return;
}


////////////////////
//                //
// Public methods //
//                //
////////////////////

/////////////////////////////////
// Instatiation and activation //
//                             //
/////////////////////////////////

Mechanical::Mechanical(int baud) {
  this->baudios = baud;
  maxpos.x = MAX_X;
  maxpos.y = MAX_Y;
  max_x = maxpos.x.toFloat();
  max_y = maxpos.y.toFloat();
  pos.x = "";
  pos.y = "";
  afterPos.x = "";
  afterPos.y = "";
  pinMode(ENABLEPIN,OUTPUT);
  expected = 0;
  bufIndex = 0;
  lastIndex = 0;
  infos = 0;
  answered = true;
  longWait = false;
  this->st = OFF;
  serialStamp = millis();
  dogWatching = false;
  dogTriggered = false;
}

//Activate and deactivate serial connection.
bool Mechanical::toggle(bool button) {
  answered = false;
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
    homeAxis();
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
  
  //compose the command
  strcpy(GRBLcommand, "$h");
  
  //check if the command can be sent, and send it.
  if(!sendCommand(LOCK,IDLE,ERROR)) return false;
  
  //update status
  st = HOMING;
  
  //update status expectations.
  expected += 2;
  //this command can take a while to confirm
  longWait = true;
  posOutdated = true; //temporary cautionary measure.
  afterPos.x = "0.000";
  afterPos.y = "0.000";
  return true;
}

//Uninterruptible movement
bool Mechanical::moveAxis(char * request, int x, int y, int f) {
  
  //Copy and adequate the request for parsing.
  strcpy(reqBuffer, request);
  reqBuffer[y - 1] = '\0';
  reqBuffer[f - 1] = '\0';
  reqBuffer[x - 1] = '\0';
  
  //Take numerical values for safety cautions.
  float xCoord = atof(reqBuffer + x + 1);
  float yCoord = atof(reqBuffer + y + 1);
  float fSpeed = atof(reqBuffer + f + 1);
  
  //out of bounds safelock.
  if(xCoord > max_x || yCoord > max_y) return false;
  
  //Too slow movement safelock.
  //using norm 1 for speed purposes.
  //That turns that 70000 into a magic number.
  if(70000.0*(xCoord + yCoord)/fSpeed > WATCHDOG_LIMIT)
    return false;
  
  //Compose the command into GRBLcommand.
  strcpy(GRBLcommand, "G1 X");
  strcat(GRBLcommand, reqBuffer + x + 2);
  strcat(GRBLcommand, " Y");
  strcat(GRBLcommand, reqBuffer + y + 2);
  strcat(GRBLcommand, " F");
  strcat(GRBLcommand, reqBuffer + f + 2);
  strcat(GRBLcommand, "\r\nG4p0");
  
  //check if the command can be sent, and send it
  if(!sendCommand(IDLE,IDLE,ERROR)) return false;
  
  //update status.
  st = MOVING;
  
  //update status expectations.
  expected += 4;
  //this command can take a while to confirm.
  longWait = true;
  posOutdated = true; //temporary cautionary measure.
  afterPos.x = String(reqBuffer + x + 2);
  afterPos.y = String(reqBuffer + y + 2);
  return true;
}

//Interruptible movement
bool Mechanical::jogAxis(char * request, int x, int y, int f, int r, int s) {
  
  //if previous jog has been unanswered, return a false
  if(!answered) return false;
  
  //Copy and adequate the request for parsing.
  strcpy(reqBuffer, request);
  reqBuffer[x - 1] = '\0';
  reqBuffer[y - 1] = '\0';
  reqBuffer[f - 1] = '\0';
  reqBuffer[r - 1] = '\0';
  reqBuffer[s - 1] = '\0';
  
  //compose the command into GRBLcommand.
  if(getCharIndex(s,reqBuffer,"true")){
    strcpy(GRBLcommand, "\x85\r\n");
    infos++;
  }else{
    GRBLcommand[0] = '\0'; //make strcat write from the beggining.
  }
  strcat(GRBLcommand, "$J=");
  if(getCharIndex(r,reqBuffer,"true")){
    strcat(GRBLcommand, "G91 ");
  }else{
    strcat(GRBLcommand, "G90 ");
  }
  strcat(GRBLcommand, "X");
  strcat(GRBLcommand, reqBuffer + x + 2);
  strcat(GRBLcommand, "Y");
  strcat(GRBLcommand, reqBuffer + y + 2);
  strcat(GRBLcommand, "F");
  strcat(GRBLcommand, reqBuffer + y + 2);
  
  //check if the command can be sent, and send it.
  if(!sendCommand(JOGGING, JOGGING, ERROR)) return false;
  
  //update the status expectations.
  expected += 4;
  posOutdated = true;
  return true;
}

//axis panning
bool Mechanical::panAxis(char * request, int x, int y, int f) {
  
  //if the previous command hasn't even been answered, return false.
  if(!answered) return false;
  
  //Copy and adequate the response for parsing.
  strcpy(reqBuffer, request);
  reqBuffer[x - 1] = '\0';
  reqBuffer[y - 1] = '\0';
  reqBuffer[f - 1] = '\0';
  
  //Compose the command.
  strcpy(GRBLcommand, "\x85\r\n$J=G91 X");
  strcat(GRBLcommand, reqBuffer + x + 2);
  strcat(GRBLcommand, " Y");
  strcat(GRBLcommand, reqBuffer + y + 2);
  strcat(GRBLcommand, " F");
  strcat(GRBLcommand, reqBuffer + f + 2);
  
  //check if the command can be sent, and send it.
  if(!sendCommand(JOGGING, JOGGING, ERROR)) return false;
  expected += 4;
  infos++;
  posOutdated = true;
  return true;
}

bool Mechanical::uniJog(char * request, int c, int f){
  
  //if the previous command hasn't even been answered, return false.
  if(!answered) return false;
  
  //Copy and adequate the response into GRBLcommmand.
  strcpy(reqBuffer, request);
  reqBuffer[c - 1] = '\0';
  reqBuffer[f - 1] = '\0';
  
  //Compose the command.
  strcpy(GRBLcommand, "$J=G90 ");
  if(reqBuffer[c + 3] == 'X'){
    strcat(GRBLcommand, "X");
    if(reqBuffer[c + 2] == '-') strcat(GRBLcommand, "0");
    else strcat(GRBLcommand, MAX_X);
  }else{
    strcat(GRBLcommand, "Y");
    if(reqBuffer[c + 2] == '-') strcat(GRBLcommand, "0");
    else strcat(GRBLcommand, MAX_Y);
  }
  strcat(GRBLcommand, " F");
  strcat(GRBLcommand, reqBuffer + f + 2);
  
  //Check if the command can be sent, and send it.
  if(!sendCommand(JOGGING, JOGGING, ERROR)) return false;
  
  //update status expectations for the future.
  expected += 2;
  posOutdated = true;
  return true;
}

//stop jogging movement.
bool Mechanical::stopJog() {
  
  //Compose command into GRBLcommand.
  strcpy(GRBLcommand, "\x85");
  
  //Check if the command can be sent, and send it.
  if(!sendCommand(JOGGING,IDLE,ERROR)) return false;
  
  //update status expectations for the future.
  expected += 2;
  infos++;
  posOutdated = true;
  return true;
}

bool Mechanical::unlockAxis() {
  
  //compose command into GRBLcommand
  strcpy(GRBLcommand, "$x");
  
  //Check if the command can be sent, and send it.
  if(!sendCommand(LOCK,IDLE,ERROR)) return false;
  
  //update expectations for the future.
  answered = true;
  expected += 2;
  return true;
}

bool Mechanical::toggleLight(char * request, int l){
  
  //Copy and adequate the request for parsing.
  strcpy(reqBuffer,request);
  reqBuffer[l - 1] = '\0';
  
  //saturate intensity between 0 and 255.
  int inputNum = atoi(reqBuffer + l + 2);
  inputNum = saturate(inputNum, 0, 255);
  
  //Compose the command into GRBLcommand
  strcpy(GRBLcommand, "M03 S");
  char number[8];
  sprintf(number, "%d", inputNum);
  strcat(GRBLcommand, number);
  
  //check if the command can be sent, and send it.
  if (!sendCommand(IDLE,st,st)) return false;
  
  //update expectations for the future
  expected += 2;
  
  //this command is answered right here.
  
  answered = true;
  microServer->update("Light set to " + String(number));
  return true;
}

bool Mechanical::reset(){
  Serial.println("\x18");
}

////////////////////
// Info reporting //
//                //
////////////////////

//Report position
bool Mechanical::getPos(WiFiClient client) {
  String notice;
  askClient = client;
  if(posOutdated){
    askPos();
  }
  else
  {
    notice = "X: "  + pos.x + " Y: " + pos.y;
    microServer->send(200,notice, &askClient);
  }
  return true;
}

//Returns the number of the current status
String Mechanical::getStatus() { return statusToString(this->st); }

//Ask GRBL for position, and update our local variables.
bool Mechanical::askPos() {
  expected += 2;
  infos += 1;
  Serial.println("?");
  return true;
}

//Change the status of the machine.
void Mechanical::setStatus(Status stat){
  st = stat;
  if(!answered) {
    microServer->update(getStatus());
    answered = true;
  }
}

void Mechanical::run(){
  if((expected != 0) && (millis() - serialStamp > SERIAL_FRAMERATE)){
    this->serialListen();
    serialStamp = millis();
  }
  if(dogWatching && (millis() - watchDogStamp > WATCHDOG_LIMIT)){
    microServer->update("WATCHDOG ERROR. Expected = " 
      + String(expected));
    restartAll();
    dogTriggered = true; //notify the next command to flush any possible serial leftover.
    reset();
    st = LOCK; //force the user to make a homing before continuing.
  }
}


/////////////////////////////////
// Server notifying tools      //
/////////////////////////////////

void Mechanical::addObserver(MicroServer * ms) { microServer = ms; }



