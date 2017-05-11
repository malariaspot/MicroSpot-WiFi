#include "MicroServer.h"
#include "Mechanical.h"
#include <Ticker.h>

WiFiServer serverWifi(80);
WiFiClient currentClient;

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

  /////////////////////
  // Server commands //
  /////////////////////

  serverWifi.begin();

  mechanical->toggle(true);
}

void MicroServer::run() {
  //if(!mechanical->longWait) serverWifi.handleClient();
  mechanical->run();

  WiFiClient client = serverWifi.available();

  // Check if a client has connected
  if (client) {
    currentClient = client;
  // Wait until the client sends some data
    while(!client.available()){
      delay(1);
    }
    
    // Read the first line of the request
    String req = client.readStringUntil('\r');
    currentClient.flush();
    
    // Match the request
    if (req.indexOf("/ayy/lmao") != -1)
      handleAyyLmao();
    else if (req.indexOf("/homeAxis") != -1){
      handleHomeAxis();
    }else if (req.indexOf("/who") != -1){
      update("Current: "+currentClient.remoteIP().toString() + "\n" + currentClient.remotePort() +", NEW: " +client.remoteIP().toString() + "\n" + client.remotePort() );
      return;
    }

  }
}

//////////////////////
// Server responses //
//                  //
//////////////////////

void MicroServer::update(String msg) { 
  //serverWifi.send(200, "application/json", "Success: " + msg); 
  currentClient.flush();

  // Prepare the response
  String s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\nSuccess: "+msg+"\r\n";

  // Send the response to the client
  currentClient.print(s);
  delay(1);
  currentClient.stop();
}

//////////////////////
// Command Handlers //
//////////////////////

void MicroServer::handleWhomst() {}
void MicroServer::handleAyyLmao() { update("Ayy LMAO"); }
void MicroServer::handleUnlockAxis() {}
void MicroServer::handleHomeAxis() { 
  mechanical->homeAxis();
}
void MicroServer::handleStopJog() { mechanical->stopJog(); }
void MicroServer::handleGetPos() { mechanical->getPos(); }
void MicroServer::handleMoveAxis() {}
void MicroServer::handleJogAxis() {}
void MicroServer::handleToggle() {}
void MicroServer::handleToggleLight(){}