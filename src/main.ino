/**
 * @file OTA-mDNS-SPIFFS.ino
 *
 * @author Pascal Gollor (http://www.pgollor.de/cms/)
 * @date 2015-09-18
 *
 * changelog:
 * 2015-10-22:
 * - Use new ArduinoOTA library.
 * - loadConfig function can handle different line endings
 * - remove mDNS studd. ArduinoOTA handle it.
 *
 */

#include <ArduinoOTA.h>
#include <Ticker.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>

#include "Mechanical.h"
#include "MicroServer.h"

#define HOSTNAME "MicroSpot-" //Hostname and AP name.


///////////////////////////////////////////////
// Server declaration.
//
///////////////////////////////////////////////

MicroServer microServer();


///////////////////////////////////////////////
// LED ticker and functions to make a Blink
//
///////////////////////////////////////////////

#define LEDPIN 14 //GPIO for the LED

Ticker ledBlink;

void ledFlick(){
  digitalWrite(LEDPIN,!digitalRead(LEDPIN));
}


///////////////////////////////////////////////
// Arduino Setup
//
///////////////////////////////////////////////
void setup() { 

  pinMode(LEDPIN,OUTPUT);
  digitalWrite(LEDPIN,LOW);

  delay(100);

  ledBlink.attach(1,ledFlick);

  String hostname(HOSTNAME);
  hostname += String(ESP.getChipId(), HEX);

  // Start OTA server.
  ArduinoOTA.setHostname((const char *)hostname.c_str());
  ArduinoOTA.begin();

  microServer.setUp(hostname);
}

/**
 * @brief Arduino loop function.
 */
void loop() {
  //serverLoop();
  microServer.run();
  yield();
  // Handle OTA server.
  ArduinoOTA.handle();
  yield();
}
