#include "MicroServer.h"
#include "Mechanical.h"
#include <Ticker.h>

#define LEDPIN 14 //GPIO for the LED

Ticker ledBlink; // LED ticker and functions to make a Blink

void ledFlick() { digitalWrite(LEDPIN,!digitalRead(LEDPIN)); }

WiFiServer serverWifi(80);

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
    request = newClient.readStringUntil('\r');
    
    if (request.indexOf('?') != -1) {
      url = request.substring(request.indexOf("GET ")+4, request.indexOf("?"));
    }else{
      url = request.substring(request.indexOf("GET ")+4, request.indexOf(" HTTP/"));      
    }
    
    newClient.flush();

    if (url == "/pan"){
      
      if (hasArg("x") && hasArg("y") && hasArg("f")) {

        if (mechanical->panAxis(arg("x"),arg("y"),arg("f"))) { 
          currentClient = newClient; 
        }else send(200, "Busy", &newClient); 

      }else send(404, "Error: One or more position arguments are missing!", &newClient); 
    }else if (url == "/uniJog"){
      if (hasArg("c") && hasArg("f")) {

        if (mechanical->uniJog(arg("c"),arg("f"))) { 
          currentClient = newClient; 
        }else send(200, "Busy", &newClient); 

      }else send(404, "Error: One or more position arguments are missing!", &newClient); 
    }else if (url == "/ayy/lmao") {
      send(200, "Ayy Lmao", &newClient);
    }else if (url == "/stop") {

      if (mechanical->stopJog()) currentClient = newClient;
      else send(200, "Busy", &newClient);

    }else if (url == "/home") {

      if (mechanical->homeAxis()) currentClient = newClient; 
      else send(200, "Busy for home", &newClient);

    }else if (url == "/move") {

      if (hasArg("x") && hasArg("y") && hasArg("f")) {

        if (mechanical->moveAxis(arg("x"),arg("y"),arg("f"))) currentClient = newClient; 
        else send(200, "Busy for move", &newClient); 

      }else send(404, "Error: One or more position arguments are missing!", &newClient); 
    }else if (url == "/jog") {

      if (hasArg("x") && hasArg("y") && hasArg("f") && hasArg("r") && hasArg("s")) {

        if (mechanical->jogAxis(arg("x"),arg("y"),arg("f"),arg("r"),arg("s"))) { 
          currentClient = newClient; 
        }else send(200, "Busy", &newClient); 

      }else send(404, "Error: One or more position arguments are missing!", &newClient); 
      
    }else if (url == "/position") {
      currentClient = newClient;
      mechanical->getPos();
    }else if(url == "/light"){
      
      if (hasArg("l")){
        currentClient = newClient;
        mechanical->toggleLight(arg("l").toInt());
      }
      
    }else if(url == "/toggle"){
      
      if (hasArg("o")){
        currentClient = newClient;
        if(arg("o") == "1") mechanical->toggle(true);
        else mechanical->toggle(false);
      }
      
    }else send(404,url + " not found!", &newClient); 
    //return;
  }
}

void MicroServer::update(String msg) { send(200, msg, &currentClient); }

/* PRIVATE */

String MicroServer::arg(String arg) {
  int beginning = request.indexOf(arg+"=")+2;
  int end = request.indexOf("&",beginning);
  if (end < 0) return request.substring(beginning, request.indexOf(" HTTP/"));
  return request.substring(beginning, end);
}

bool MicroServer::hasArg(String arg) {
  if (request.indexOf(arg+"=") != -1) return true;
  return false;
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
