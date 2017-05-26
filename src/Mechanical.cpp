#include "Mechanical.h"
#include "MicroServer.h"
#include "charUtils.h"
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
  strcpy(GRBLcommand, "$h");
  bool result = sendCommand(LOCK,IDLE,ERROR);
  if(!result) return result;
  st = HOMING;
  expected += 2;
  //this command can take a while to confirm
  longWait = true;
  posOutdated = true; //temporary cautionary measure.
  afterPos.x = "0.000";
  afterPos.y = "0.000";
  return result;
}

//Uninterruptible movement
bool Mechanical::moveAxis(char * request, int x, int y, int f) {
  //using norm 1 for speed purposes.
  //That turns that 70000 into a magic number.
  strcpy(reqBuffer, request);
  reqBuffer[y - 1] = '\0';
  reqBuffer[f - 1] = '\0';
  reqBuffer[x - 1] = '\0';
  float xCoord = atof(reqBuffer + x + 1);
  float yCoord = atof(reqBuffer + y + 1);
  float fSpeed = atof(reqBuffer + f + 1);
  if(xCoord > max_x || yCoord > max_y) return false;
  if(70000.0*(xCoord + yCoord)/fSpeed > WATCHDOG_LIMIT){
    return false;
  }
  strcpy(GRBLcommand, "G1 X");
  strcat(GRBLcommand, reqBuffer + x + 2);
  strcat(GRBLcommand, " Y");
  strcat(GRBLcommand, reqBuffer + y + 2);
  strcat(GRBLcommand, " F");
  strcat(GRBLcommand, reqBuffer + f + 2);
  strcat(GRBLcommand, "\r\nG4p0");
  bool result = sendCommand(IDLE,IDLE,ERROR);
  if(!result) return result;
  st = MOVING;
  expected += 4;
  //this command can take a while to confirm.
  longWait = true;
  posOutdated = true; //temporary cautionary measure.
  afterPos.x = String(reqBuffer + x + 2);
  afterPos.y = String(reqBuffer + y + 2);
  return result;
}

//Interruptible movement
bool Mechanical::jogAxis(char * request, int x, int y, int f, int r, int s) {
  if(!answered) return false;
  strcpy(reqBuffer, request);
  String mode;
  if(getCharIndex(r,reqBuffer,"true")){
    mode = "G91";
  }else{
    mode = "G90";
  }
  String  stopping = "\x85\r\n";
  reqBuffer[x - 1] = '\0';
  reqBuffer[y - 1] = '\0';
  reqBuffer[f - 1] = '\0';
  reqBuffer[r - 1] = '\0';
  reqBuffer[s - 1] = '\0';
  /*
  stopping + "$J=" + mode 
  + " X" + String(reqBuffer + x + 2)
  + " Y" + String(reqBuffer + y + 2)
  + " F" + String(reqBuffer + f + 2)
  * */
  bool result = sendCommand(JOGGING, JOGGING, ERROR);
  
  if(!result) return result;
  expected += 4;
  posOutdated = true;
  return result;
}

//axis panning
bool Mechanical::panAxis(char * request, int x, int y, int f) {
  if(!answered) return false;
  strcpy(reqBuffer, request);
  reqBuffer[x - 1] = '\0';
  reqBuffer[y - 1] = '\0';
  reqBuffer[f - 1] = '\0';
  /*
  "\x85\r\n$J=G91 X" 
  + String(reqBuffer + x + 2)
  + " Y" + String(reqBuffer + y + 2) 
  + " F" + String(reqBuffer + f + 2)
   * */
  bool result = sendCommand(JOGGING, JOGGING, ERROR);
  if(!result) return result;
  expected += 4;
  posOutdated = true;
  return result;
}

bool Mechanical::uniJog(char * request, int c, int f){
  if(!answered) return false;
  strcpy(reqBuffer, request);
  reqBuffer[c - 1] = '\0';
  reqBuffer[f - 1] = '\0';
  String Coord = String(reqBuffer + c + 2);
  String F = String(reqBuffer + f + 2);
  String destination = (Coord[0] == '-') ? "0" : 
                        (Coord[1] == 'X') ? maxpos.x : maxpos.y;
  /*
  "$J=G90 " + String(Coord[1]) + destination +
  " F" + F
  */
  bool result = sendCommand(JOGGING, JOGGING, ERROR);
  if(!result) return result;
  expected += 2;
  posOutdated = true;
  return result;
}

//stop jogging movement.
bool Mechanical::stopJog() {
  strcpy(GRBLcommand, "\x85");
  bool result = sendCommand(JOGGING,IDLE,ERROR);
  if(!result) return result;
  expected += 2;
  posOutdated = true;
  return result;
}

bool Mechanical::unlockAxis() {
  strcpy(GRBLcommand, "$x");
  bool result = sendCommand(LOCK,IDLE,ERROR);
  if(!result) return result;
  expected += 2;
  infos += 1;
  return result;
}

bool Mechanical::toggleLight(char * request, int l){
  int inputNum;
  strcpy(reqBuffer,request);
  String input = String(reqBuffer + l + 2);
  inputNum = input.toInt();
  //saturate intensity between 0 and 255.
  //Not using just min() and max() seems uncanny, 
  //but issue#398 of the framework reveals that they just don' work.
  //Until there is a fix, this is what we got to do.
  if(inputNum >= 255){
    inputNum = 255;
  }else if(inputNum <= 0) inputNum = 0;
    //"M03 S" + input
  bool result = sendCommand(IDLE,st,st);
  if (!result) return result;
  answered = true;
  expected += 2;
  microServer->update("Light set to " + input);
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
int Mechanical::getStatus() { return this->st; }

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
    microServer->update(statusToString(st));
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



