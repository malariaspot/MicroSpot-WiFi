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

// includes
//#include <ESP8266WiFi.h>
//#include <ESP8266mDNS.h>
//#include <WiFiUdp.h>
#include <FS.h>
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

//WiFiServer server(80);
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
 * @brief Read WiFi connection information from file system.
 * @param ssid String pointer for storing SSID.
 * @param pass String pointer for storing PSK.
 * @return True or False.
 *
 * The config file have to containt the WiFi SSID in the first line
 * and the WiFi PSK in the second line.
 * Line seperator can be \r\n (CR LF) \r or \n.
 */
bool loadConfig(String *ssid, String *pass)
{
  // open file for reading.
  File configFile = SPIFFS.open("/cl_conf.txt", "r");
  if (!configFile)
  {
    //Replace for an html message.
    ////Serial.println("Failed to open cl_conf.txt.");
    return false;
  }

  // Read content from config file.
  String content = configFile.readString();
  configFile.close();

  content.trim();

  // Check if ther is a second line available.
  int8_t pos = content.indexOf("\r\n");
  uint8_t le = 2;
  // check for linux and mac line ending.
  if (pos == -1) {
    le = 1;
    pos = content.indexOf("\n");
    if (pos == -1) {
      pos = content.indexOf("\r");
    }
  }

  // If there is no second line: Some information is missing.
  if (pos == -1) {
    //Serial.println("Invalid content.");
    //Serial.println(content);
    return false;
  }

  // Store SSID and PSK into string vars.
  *ssid = content.substring(0, pos);
  *pass = content.substring(pos + le);

  ssid->trim();
  pass->trim();

  return true;
}

/**
 * @brief Save WiFi SSID and PSK to configuration file.
 * @param ssid SSID as string pointer.
 * @param pass PSK as string pointer,
 * @return True or False.
 */
bool saveConfig(String *ssid, String *pass) {
  // Open config file for writing.
  File configFile = SPIFFS.open("/cl_conf.txt", "w");
  if (!configFile) { return false; }
  // Save SSID and PSK.
  configFile.println(*ssid);
  configFile.println(*pass);
  configFile.close();
  return true;
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

  server.setUp(hos);
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