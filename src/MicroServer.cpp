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

/*serverWifi.on("/client", [this](){ handleWhomst();});
  serverWifi.on("/homeAxis", [this](){ handleHomeAxis();});
  serverWifi.on("/moveAxis", [this](){ handleMoveAxis();});
  serverWifi.on("/jogAxis", [this](){ handleJogAxis();});
  serverWifi.on("/stopJog", [this](){ handleStopJog();});
  serverWifi.on("/ayy/lmao", [this](){ handleAyyLmao();});
  serverWifi.on("/unlockAxis", [this](){handleUnlockAxis();});
  serverWifi.on("/toggle", [this](){handleToggle();});
  serverWifi.on("/getPos", [this](){handleGetPos();});
  serverWifi.on("/toggleLight",[this](){handleToggleLight();});*/

  serverWifi.begin();

  mechanical->toggle(true);
}

void MicroServer::run() {
  //if(!mechanical->longWait) serverWifi.handleClient();

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
    }else{
      update("Not Found!");
      return;
    }

    mechanical->run();
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
//                  //
//////////////////////

void MicroServer::handleWhomst() { /*update(serverWifi.client().remoteIP().toString());*/ }
void MicroServer::handleAyyLmao() { update("Ayy LMAO"); }
void MicroServer::handleUnlockAxis() {/*mechanical->unlockAxis();*/}
void MicroServer::handleHomeAxis() { 
  //if(mechanical->getStatus() != MOVING) {
    mechanical->homeAxis(); 
  /*}else{
    update("BUSY");
  }*/
}
void MicroServer::handleStopJog() { mechanical->stopJog(); }
void MicroServer::handleGetPos() { mechanical->getPos(); }

void MicroServer::handleMoveAxis() {
  /*if (serverWifi.arg("x") != "" && serverWifi.arg("y") != "" && serverWifi.arg("f") != "") {
    mechanical->moveAxis((String)serverWifi.arg("x"), (String)serverWifi.arg("y"), (String)serverWifi.arg("f"));
  }else{ update("Error: One or more position arguments are missing!"); }*/
}

void MicroServer::handleJogAxis() {
  /*if (serverWifi.arg("x") != "" && serverWifi.arg("y") != "" && serverWifi.arg("f") != "") {
    mechanical->jogAxis((String)serverWifi.arg("x"), (String)serverWifi.arg("y"), (String)serverWifi.arg("f"), 
      (String)serverWifi.arg("r"), (String)serverWifi.arg("s"));
  }else{ update("Error: One or more position arguments are missing!"); }*/
}

void MicroServer::handleToggle() {
  /*if (serverWifi.arg("option") !=  "") {
    if(serverWifi.arg("option") == "true") mechanical->toggle(true);
    else if(serverWifi.arg("option") == "false") mechanical->toggle(false);
    else update("Error: Invalid 'option' value!");
  }else{ update("Error: No 'option' value provided!"); }*/
}

void MicroServer::handleToggleLight(){
  /*if (serverWifi.arg("l") != "") {
    mechanical->toggleLight(serverWifi.arg("l").toInt());
  }else{
    update("Error: No intensity value provided!");
  }*/
}
