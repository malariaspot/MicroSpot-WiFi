#include "MicroServer.h"
#include "Mechanical.h"
#include <Ticker.h>

#define LEDPIN 14 //GPIO for the LED
#define REQUESTBUFFERSIZE 512

Ticker ledBlink; // LED ticker and functions to make a Blink

void ledFlick() { digitalWrite(LEDPIN,!digitalRead(LEDPIN)); }

WiFiServer serverWifi(80);
char requestBuffer[REQUESTBUFFERSIZE];
char urlBuffer[32];
int bufferIndex;

/* INIT */

MicroServer::MicroServer(Mechanical *m) {
  mechanical = m;
  mechanical->addObserver(this);
}

/* PUBLIC */

void MicroServer::setup(String hostname) {
  //Set the hostname of the server
  WiFi.hostname(hostname);
  //Check of there has been a change in WiFi configuration.
  String station_ssid, station_psk;
  // Load wifi connection information.
  if (! fileManager.loadWifiConfig(&station_ssid, &station_psk)) {
    station_ssid = "";
    station_psk = "";
  }
  // Check WiFi connection
  if (WiFi.getMode() != WIFI_STA) {
    WiFi.mode(WIFI_STA);
    delay(10);
  }
  // ... Compare file config with sdk config.
  if (WiFi.SSID() != station_ssid || WiFi.psk() != station_psk) {
    WiFi.begin(station_ssid.c_str(), station_psk.c_str());
  }else{
    // ... Begin with sdk config.
    WiFi.begin();
  }
  // ... Give ESP 10 seconds to connect to station.
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) { delay(500); }
  // Check connection
  if(WiFi.status() != WL_CONNECTED) {
    // Go into software AP mode.
    pinMode(LEDPIN,OUTPUT);
    digitalWrite(LEDPIN,LOW);
    delay(100);
    ledBlink.attach(1,ledFlick);

    WiFi.mode(WIFI_AP);

    delay(10);

    WiFi.softAP((const char *)hostname.c_str(), this->ap_default_psk);
  }

  serverWifi.begin();
  mechanical->toggle(true);
}

void MicroServer::run() {
  WiFiClient newClient = serverWifi.available();

  if (newClient) {

    while(!newClient.available()) { delay(1); }

    while(newClient.available()) {
      requestBuffer[bufferIndex] =  newClient.read();
      bufferIndex++;
    }

    int questionMarkIndex = getCharIndex(requestBuffer, "?");
    int httpIndex = getCharIndex(requestBuffer, " HTTP/");
    requestBuffer[httpIndex] = '\0'; //crop the end of the request.
    if (questionMarkIndex > -1) strncpy(urlBuffer, requestBuffer+4, questionMarkIndex-3);
    else strncpy(urlBuffer, requestBuffer+4, httpIndex-3);
    
    newClient.flush();

    if (getCharIndex(urlBuffer, "/pan") > -1){

      int x = arg("x=");
      int y = arg("y=");
      int f = arg("f=");
      if (x > -1 && y > -1 && f > -1) {
        if (mechanical->panAxis(requestBuffer,x,y,f)) currentClient = newClient; 
        else send(200, "Busy", &newClient); 
      }else send(404, "Error: One or more position arguments are missing in pan!", &newClient); 

    }else if (getCharIndex(urlBuffer, "/uniJog") > -1){

      int c = arg("c=");
      int f = arg("f=");
      if (c > -1 && f > -1) {
        if (mechanical->uniJog(requestBuffer,c,f)) currentClient = newClient; 
        else send(200, "Busy", &newClient); 
      }else send(404, "Error: One or more position arguments are missing in uniJog!", &newClient); 

    }else if (getCharIndex(urlBuffer, "/ayy/lmao") > -1) {

      send(200, "Ayy Lmao", &newClient);
    }else if (getCharIndex(urlBuffer,"/stop") > -1) {

      if (mechanical->stopJog()) currentClient = newClient;
      else send(200, "Busy", &newClient);

    }else if (getCharIndex(urlBuffer, "/home") > -1) {

      if (mechanical->homeAxis()) currentClient = newClient; 
      else send(200, "Busy for home", &newClient);

    }else if (getCharIndex(urlBuffer, "/move") > -1) {

      int x = arg("x=");
      int y = arg("y=");
      int f = arg("f=");
      if (x > -1 && y > -1 && f > -1) {
        if (mechanical->moveAxis(requestBuffer,x,y,f)) currentClient = newClient; 
        else send(200, "Busy for move", &newClient); 
      }else send(404, "Error: One or more position arguments are missing in move!", &newClient); 

    }else if (getCharIndex(urlBuffer,"/jog") > -1) {

      int x = arg("x=");
      int y = arg("y=");
      int f = arg("f=");
      int r = arg("r=");
      int s = arg("s=");
      if (x > -1 && y > -1 && f > -1 && r > -1 && s > -1) {
        if (mechanical->jogAxis(requestBuffer,x,y,f,r,s)) currentClient = newClient; 
        else send(200, "Busy", &newClient); 
      }else send(404, "Error: One or more position arguments are missing in jog!", &newClient); 
      
    }else if (getCharIndex(urlBuffer, "/position") > -1) {

      mechanical->getPos(newClient);

    }else if(getCharIndex(urlBuffer, "/light") > -1){

      int l = arg("l=");
      if (l > -1){
        currentClient = newClient;
        mechanical->toggleLight(requestBuffer,l);
      }else{
        send(404, "Error: Wrong argument in light!", &newClient); 
      }
    }else if(getCharIndex(urlBuffer, "/toggle") > -1){
      
      int o = arg("o=");
      if (o > -1){
        currentClient = newClient;
        if(getCharIndex(o, requestBuffer, "1") > -1) mechanical->toggle(true);
        else mechanical->toggle(false);
      }
    }else if(getCharIndex(urlBuffer, "/unlock") > -1){
      if(mechanical->unlockAxis()) send(200, "Axis unlocked", &newClient);
      else send(200, "error unlocking axis", &newClient);
    }else send(404, "Not found!", &newClient); 
  }

  bufferIndex = 0;
  memset(requestBuffer, 0, sizeof requestBuffer);
  memset(urlBuffer, 0, sizeof urlBuffer);
}

void MicroServer::update(String msg) { send(200, msg, &currentClient); }

/* PRIVATE */

int MicroServer::arg(const char * arg) {
  return getCharIndex(requestBuffer, arg);
}

void MicroServer::send(int code, String msg, WiFiClient * client) { 
  String s = "HTTP/1.1 " 
    + String(code) + " OK\r\nContent-Type: application/json\r\n\r\n"
    + msg;

  client->flush();
  client->print(s);
  delay(1);
  client->stop();
}
