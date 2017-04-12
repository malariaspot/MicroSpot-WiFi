#include "MicroServer.h"
#include "Mechanical.h"
#include <Ticker.h>

//WiFiServer serverWifi(80);
ESP8266WebServer serverWifi(80);  

///////////////////////////////////////////////
// LED ticker and functions to make a Blink
//
///////////////////////////////////////////////

#define LEDPIN 14 //GPIO for the LED

Ticker ledBlink;

void ledFlick(){
  digitalWrite(LEDPIN,!digitalRead(LEDPIN));
}


/**
 * Server implementation
 */

MicroServer::MicroServer(Mechanical *m) { 
  mechanical = m; 
  mechanical->addObserver(this);
}

void MicroServer::setUp(String hostname) {

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

  serverWifi.on("/homeAxis", [this](){ handleHomeAxis();}); 
  serverWifi.on("/moveAxis", [this](){ handleMoveAxis();}); 
  serverWifi.on("/jogAxis", [this](){ handleJogAxis();}); 
  serverWifi.on("/stopJog", [this](){ handleStopJog();});
  serverWifi.on("/ayy/lmao", [this](){ handleAyyLmao();}); //TODO - remove
  serverWifi.on("/unlockAxis", [this](){handleUnlockAxis();});
  serverWifi.on("/toggle", [this](){handleToggle();});

  serverWifi.begin();

  mechanical->toggle(true);
  
}

void MicroServer::run() { serverWifi.handleClient(); }

void MicroServer::success() { serverWifi.send(200, "text/plain", "Success: Done!"); }
void MicroServer::error(String msg) { serverWifi.send(200, "text/plain", msg); }

void MicroServer::handleHomeAxis() { mechanical->homeAxis(); }
void MicroServer::handleStopJog() { mechanical->stopJog(); }

void MicroServer::handleMoveAxis() { 
  if (serverWifi.arg("x") != "" && 
      serverWifi.arg("y") != "" && 
      serverWifi.arg("f") != "") { 
    mechanical->moveAxis((String)serverWifi.arg("x"), (String)serverWifi.arg("y"), (String)serverWifi.arg("f"));
  }else{ error("Error: One or more position arguments are missing!"); }
}

void MicroServer::handleJogAxis() { 
  if (serverWifi.arg("x") != "" && 
      serverWifi.arg("y") != "" && 
      serverWifi.arg("f") != "") { 
    mechanical->jogAxis((String)serverWifi.arg("x"), (String)serverWifi.arg("y"), (String)serverWifi.arg("f"));
  }else{ error("Error: One or more position arguments are missing!"); }
}

void MicroServer::handleAyyLmao() { success(); }

void MicroServer::handleUnlockAxis() {mechanical->unlockAxis();}

void MicroServer::handleToggle() {
  if (serverWifi.arg("option") !=  "") { 
    if(serverWifi.arg("option") == "true") mechanical->toggle(true);
    else if(serverWifi.arg("option") == "false") mechanical->toggle(false);
    else error("Error: Invalid 'option' value!");
  }else{ error("Error: No 'option' value provided!"); }
}