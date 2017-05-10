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

#include "MicroServer.h"
#include "Mechanical.h"

#define HOSTNAME "MicroSpot-" //Hostname and AP name root.

///////////////////////////////////////////////
// Server declaration.
//
///////////////////////////////////////////////

Mechanical mechanical(115200);
MicroServer microServer(&mechanical);

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
  microServer.run();
  yield();
  // Handle OTA server.
  ArduinoOTA.handle();
  yield();
}
