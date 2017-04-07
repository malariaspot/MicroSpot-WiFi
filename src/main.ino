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
#include "Mechanical.h"
#include "Server.h"

///////////////////////////////////////////////
// GPIO definitions
//
///////////////////////////////////////////////
#define LEDPIN 14 //GPIO for the LED
#define SERIALEN 4 //GPIO to activate Arduino-esp8266 serial bridge (LOW == ON)

/**
 * @brief mDNS and OTA Constants
 * @{
 */
#define HOSTNAME "MicroSpot-" ///< Hostename. The setup function adds the Chip ID at the end.
//Also used for AP name.
/// @}

Server server();

/**
 * @brief Default WiFi connection information.
 * @{
 */
const char* ap_default_psk = "microspot"; ///< Default PSK.
/// @}

///////////////////////////////////////////////
// LED ticker and functions to make a Blink
//
///////////////////////////////////////////////

Ticker ledBlink;

void ledFlick(){
  digitalWrite(LEDPIN,!digitalRead(LEDPIN));
}

/**
 * @brief Arduino setup function.
 */
void setup() { 
  String station_ssid = "";
  String station_psk = "";

  pinMode(LEDPIN,OUTPUT);
  digitalWrite(LEDPIN,LOW);
  pinMode(SERIALEN,OUTPUT);
  digitalWrite(SERIALEN,HIGH);

  delay(100);

  // Set Hostname.
  String hostname(HOSTNAME);
  hostname += String(ESP.getChipId(), HEX);

  String APname(HOSTNAME);
  APname += String(ESP.getChipId(),HEX);
  // Initialize file system.
  if (!SPIFFS.begin()) { return; }
  // Load wifi connection information.
  if (! loadConfig(&station_ssid, &station_psk)) {
    station_ssid = "";
    station_psk = "";
  }



  ledBlink.attach(1,ledFlick);

  // Start OTA server.
  ArduinoOTA.setHostname((const char *)hostname.c_str());
  ArduinoOTA.begin();

  server.setUp();
}

/**
 * @brief Arduino loop function.
 */
void loop() {
  //serverLoop();
  server.run();
  yield();
  // Handle OTA server.
  ArduinoOTA.handle();
  yield();
}