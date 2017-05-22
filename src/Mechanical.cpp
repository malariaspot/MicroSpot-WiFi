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
#define SERIAL_FRAMERATE 25

int bufferIndex, lastIndex;
char serialBuffer[BUFFERSIZE];
char inputChar;
int expected, infos;
double timeStamp;
double serialStamp;
double watchDogStamp;
bool dogWatching, dogTriggered;
bool posOutdated, answered;
char xBuffer[7];
char yBuffer[7];
Position afterPos;
String lastCommand;


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
      microServer->update("GRBL didn't understand: " + lastCommand);
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
    bufferIndex = 0;
    lastIndex = 0;
    expected = 0;
    infos = 0;
    flush();
}



enum MsgType{
  POSITION, //status and position report
  AFFIRMATIVE, //this is an "ok"
  ERRONEOUS, //this is an error report
  ALARM, //ALARM report.
  HANDSHAKE, //Initial handshake, after restart.
  MEMORY, //information fomr EEPROM
  NQMESSAGE, //Non queried message
  STARTUP, // startup routine stored in GRBL
  EMPTYLINE, //GRBL sent an empty line.
  DIRTY // dirty message, unparseable.
};

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
    inputChar = Serial.read();
    serialBuffer[bufferIndex] = inputChar;
    bufferIndex++;
    if(inputChar == ENDLINE){
      serialBuffer[bufferIndex] = '\0';
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
          strncpy(xBuffer, serialBuffer + a + 1, b - a -1);
          strncpy(yBuffer, serialBuffer + b + 1, c - b -1);
          pos.x = String(xBuffer);
          pos.y = String(yBuffer);
          microServer->update("Position: X: " + pos.x + " Y: " + pos.y);
          answered = true;
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
      lastIndex = bufferIndex;
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
bool Mechanical::sendCommand(String command, Status atLeast, Status success, Status failure) {
  if(st >= atLeast) {
    if(dogTriggered){flush(); dogTriggered = false;}
    lastCommand = command;
    answered = false;
    this->after.success = success;
    this->after.failure = failure;
    watchDogStamp = millis();
    dogWatching = true;
    Serial.println(command);
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
  pos.x = "";
  pos.y = "";
  afterPos.x = "";
  afterPos.y = "";
  pinMode(ENABLEPIN,OUTPUT);
  expected = 0;
  bufferIndex = 0;
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
    infos += 2;
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
  bool result = sendCommand("$h",LOCK,IDLE,ERROR);
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
bool Mechanical::moveAxis(String X, String Y, String F) {
  //using norm 1 for speed purposes.
  //That turns that 70000 into a magic number.
  if(70000.0*(X.toFloat() + Y.toFloat())/F.toFloat() > WATCHDOG_LIMIT){
    return false;
  }
  bool result = sendCommand("G1 X" + X + " Y" + Y + " F" + F + "\r\nG4P0",
  IDLE,IDLE,ERROR);
  if(!result) return result;
  st == MOVING;
  expected += 4;
  //this command can take a while to confirm.
  longWait = true;
  posOutdated = true; //temporary cautionary measure.
  afterPos.x = X;
  afterPos.y = Y;
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
  String  stopping = "\x85\r\n";
  bool result = sendCommand(stopping + "$J=" + mode + " X" + X + " Y" + Y + 
  " F" + F, JOGGING, JOGGING, ERROR);
  if(!result) return result;
  expected += 4;
  posOutdated = true;
  return result;
}

//stop jogging movement.
bool Mechanical::stopJog() {
  bool result = sendCommand("\x85",MOVING,IDLE,ERROR);
  if(!result) return result;
  expected += 2;
  posOutdated = true;
  return result;
}

bool Mechanical::unlockAxis() {
  bool result = sendCommand("$x",LOCK,IDLE,ERROR);
  if(!result) return result;
  expected += 2;
  infos += 1;
  return result;
}

bool Mechanical::toggleLight(int intensity){
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
  bool result = sendCommand("M03 S" + inputNum, IDLE,st,st);
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
bool Mechanical::getPos() {
  String notice;
  if(posOutdated){
    askPos();
  }
  else
  {
    notice = "X: "  + pos.x + " Y: " + pos.y;
    microServer->update(notice);
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
  if(!answered) microServer->update(statusToString(st));
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
    st = LOCK; //force the user to make a homing before continuing.
  }
}


/////////////////////////////////
// Server notifying tools      //
/////////////////////////////////

void Mechanical::addObserver(MicroServer * ms) { microServer = ms; }



