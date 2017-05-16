#include "MicroServer.h"
#include "Mechanical.h"
#include <Ticker.h>

WiFiServer serverWifi(80);

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

  /////////////////////
  // Server commands //
  /////////////////////

  serverWifi.begin();

  mechanical->toggle(true);
}

void MicroServer::handleClients() {

  WiFiClient client = serverWifi.available();

  if (client) {

    while(!client.available()) { delay(1); }
    
    String req = client.readStringUntil('\r');
    client.flush();
    if (req.indexOf("/ayy/lmao") != -1) {
      update("Ayy Lmao", &client);
      return;
    }else if (req.indexOf("/homeAxis") != -1) {
      if (mechanical->homeAxis()) {
        currentClient = client;
      }else{
        update("Busy", &client);
      }
    }else if (req.indexOf("/who") != -1) {
      update(client.remoteIP().toString() + " " + client.remotePort(),
        &client);
      return;
    }else if (req.indexOf("/stopJog") != -1) {
      if (mechanical->stopJog()) {
        currentClient = client;
      }else{
        update("Busy", &client);
      }
    }else if (req.indexOf("/getPos") != -1) {
      currentClient = client;
      mechanical->getPos();
      return;
    }else if (req.indexOf("/moveAxis") != -1) {
      int x,y,f,e;
      x = req.indexOf("x=");
      y = req.indexOf("y=");
      f = req.indexOf("f=");
      e = req.indexOf(" ",f);
      if (x > 0 && y > 0 && f > 0) { 
        if (mechanical->moveAxis(req.substring(x+2, y-1), req.substring(y+2, f - 1), req.substring(f+2,e))) {
          currentClient = client;
        }else{
          update("Busy", &client);
        }
      }else{ update("Error: One or more position arguments are missing!", &client); }
    }else if (req.indexOf("/jogAxis") != -1) {
      int x,y,f,s,r,e;
      x = req.indexOf("x=");
      y = req.indexOf("y=");
      f = req.indexOf("f=");
      r = req.indexOf("r=");
      s = req.indexOf("s=");
      e = req.indexOf(" ",s);
      if (x > 0 && y > 0 && f > 0) { 
        if (mechanical->jogAxis(req.substring(x+2, y - 1),req.substring(y+2,f - 1),req.substring(f+2,r - 1),req.substring(r+2,s - 1),req.substring(s+2, e))) {
          currentClient = client;
        }else{
          update("Busy", &client);
        }
      }else{ update("Error: One or more position arguments are missing!", &client); }
    }else{
      update("Not found!", &client);
      return;
    }
  }
}

//////////////////////
// Server responses //
//////////////////////

void MicroServer::update(String msg, WiFiClient * client) { 
  client->flush();
  String s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\nSuccess: "+msg+"\r\n" 
   + client->remoteIP().toString() + ":" + client->remotePort() + " " 
   + client->localIP().toString() + ":" + client->localPort();
  client->print(s);
  delay(1);
  client->stop();
}

void MicroServer::update(String msg) { 
  update(msg, &currentClient);
}
