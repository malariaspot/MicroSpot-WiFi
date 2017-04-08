///////////////////////////////////////////////////////////////////
//
// MicroSpot WiFi firmware for esp8266.
//
// @Authors: Jaime Garcia <garciavillena.jaime@gmail.com>
//           Alexander Bakardyev <alexander.vladimirov3@gmail.com>
//
//
//
//
// Based on Pascal Gollor OTA-mDNS-SPIFFS example
//
///////////////////////////////////////////////////////////////////


#include <ArduinoOTA.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>

#include "Mechanical.h"
#include "MicroServer.h"

#define HOSTNAME "MicroSpot-" //Hostname and AP name root.


///////////////////////////////////////////////
// Server declaration.
//
///////////////////////////////////////////////

MicroServer microServer;


///////////////////////////////////////////////
// Arduino Setup
//
///////////////////////////////////////////////
void setup() { 



  String hostname(HOSTNAME);
  hostname += String(ESP.getChipId(), HEX);

  microServer.setUp(hostname);

  // Start OTA server.
  ArduinoOTA.setHostname((const char *)hostname.c_str());
  ArduinoOTA.begin();
}

///////////////////////////////////////////////
// Arduino Loop
//
///////////////////////////////////////////////
void loop() {
  //serverLoop();
  microServer.run();
  yield();
  // Handle OTA server.
  ArduinoOTA.handle();
  yield();
}
