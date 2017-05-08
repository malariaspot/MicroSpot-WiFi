#include "Mechanical.h"
#include "MicroServer.h"

#define TIMEOUT 4000
#define REQUESTLIMIT 200
#define TICK 100
#define WATCHDOG_LIMIT 20000

/////////////////////////////////////////
// Internal variables
/////////////////////////////////////////

#define ENABLEPIN 4
#define ENDLINE '\r'
#define BUFFERSIZE 512
#define SERIAL_FRAMERATE 50

int bufferIndex;
char serialBuffer[BUFFERSIZE];
char inputChar;
int expected, infos;
double timeStamp;
double serialStamp;
double watchDogStamp;
bool dogWatching;
bool posOutdated;
char xBuffer[7];
char yBuffer[7];

enum MsgType{
  POSITION,
  AFFIRMATIVE,
  ERRONEOUS,
  ALARM
};

/////////////////////
//                 //
// Private methods //
//                 //
/////////////////////


//////////////////////////////////
// Serial input check utilities //
//////////////////////////////////

MsgType msgClassify(String msg){
  if(msg.indexOf("ok") != -1){
    return AFFIRMATIVE;
  }else if(msg.indexOf("error") != -1){
    return ERRONEOUS;
  }else if(msg.indexOf("<") != -1){
    return POSITION;
  }else{
    return ALARM;
  }
}

int getCharIndex(int from, char * buffer,char control){
  if(buffer[from] == control){
    return from;
  }else{
    return getCharIndex(from + 1, buffer, control);
  }
}

int getCharIndex(char * buffer, char control){
  return getCharIndex(0,buffer,control);
}


void Mechanical::serialListen(){
//transmit messages from serial.
  while(Serial.available() > 0){
    inputChar = Serial.read();
    serialBuffer[bufferIndex] = inputChar;
    bufferIndex++;
    if(inputChar == ENDLINE){
      char message[bufferIndex + 1];
      strncpy(message,serialBuffer,bufferIndex);
      String msg = String(message);
      switch(msgClassify(msg)){
        case AFFIRMATIVE:
          expected--;
          break;
        case ERRONEOUS:
          expected--;
          //Do something about the error.
          break;
        case POSITION:
          infos --;
          int b,c;
          b = 0;
          c = 0;
          b = getCharIndex(11,message,','); //msg.indexOf(',',11);
          c = getCharIndex(b + 1,message,',');// msg.indexOf(',',b + 1);
          //strncpy(xBuffer,message + 6,b - 6); GUILTY OF SIGSEGV
          this->pos.x = String(b);
          //strncpy(yBuffer,message + b + 1, c - b); GUILTY OF SIGSEGV
          this->pos.y = String(c);
          posOutdated = false;
          break;
        default:
          infos--;
          break;
      }
      if(expected == 0){
        dogWatching = false;
        microServer->longWait = false;
        infos = 0;
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
  pinMode(ENABLEPIN,OUTPUT);
  expected = 0;
  infos = 0;
  this->st = OFF;
  serialStamp = millis();
  dogWatching = false;
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
  expected += 2;
  //this command can take a while to confirm
  microServer->longWait = true;
  posOutdated = true; //temporary cautionary measure.
  return sendCommand("$h",LOCK,IDLE,ERROR);
}

//Uninterruptible movement
bool Mechanical::moveAxis(String X, String Y, String F) {
  expected += 4;
  //this command can take a while to confirm.
  microServer->longWait = true;
  posOutdated = true; //temporary cautionary measure.
  return sendCommand("G1 X" + X + " Y" + Y + " F" + F + "\r\nG4P0",
   MOVING,IDLE,ERROR);
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
  expected += 2;
  posOutdated = true;
  return sendCommand(stopping + "$J=" + mode + " X" + X + " Y" + Y + 
    " F" + F, MOVING, JOGGING, ERROR);
}

//stop jogging movement.
bool Mechanical::stopJog() {
  expected += 2;
  posOutdated = true;
  return sendCommand("\x85",MOVING,IDLE,ERROR);
}

void Mechanical::unlockAxis() {
  expected += 2;
  infos += 1;
  sendCommand("$x",LOCK,IDLE,ERROR);
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
  flush();
  microServer->update("Light set to " + input);
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
  microServer->update(statusToString(st));
}

void Mechanical::run(){
  if((expected != 0) && (millis() - serialStamp > SERIAL_FRAMERATE)){
    this->serialListen();
    serialStamp = millis();
  }
  if(dogWatching && (millis() - watchDogStamp > WATCHDOG_LIMIT)){
    microServer->update("WATCHDOG ERROR. Expected = " 
      + String(expected));
    dogWatching = false; //turn off watchdog
    microServer->longWait = false; //unlock MicroServer
    expected = 0;
    infos = 0;
  }
}


/////////////////////////////////
// Server notifying tools      //
/////////////////////////////////

void Mechanical::addObserver(MicroServer * ms) { microServer = ms; }



