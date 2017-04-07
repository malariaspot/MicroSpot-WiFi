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
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <FS.h>
#include <ArduinoOTA.h>
#include <Ticker.h>
#include "Mechanical.h"

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


WiFiServer server(80);

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
void setup()
{
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
  WiFi.hostname(hostname);




  // Initialize file system.
  if (!SPIFFS.begin())
  {
    //html message
    //Serial.println("Failed to mount file system");
    return;
  }

  // Load wifi connection information.
  if (! loadConfig(&station_ssid, &station_psk))
  {
    station_ssid = "";
    station_psk = "";
    //Serial.println("No WiFi connection information available.");
  }

  // Check WiFi connection
  // ... check mode
  if (WiFi.getMode() != WIFI_STA)
  {
    WiFi.mode(WIFI_STA);
    delay(10);
  }

  // ... Compare file config with sdk config.
  if (WiFi.SSID() != station_ssid || WiFi.psk() != station_psk)
  {

    WiFi.begin(station_ssid.c_str(), station_psk.c_str());

  }
  else
  {
    // ... Begin with sdk config.
    WiFi.begin();
  }

  //Serial.println("Wait for WiFi connection.");

  // ... Give ESP 10 seconds to connect to station.
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000)
  {
    //Serial.write('.');
    ////Serial.print(WiFi.status());
    delay(500);
  }
  //Serial.println();

  // Check connection
  if(WiFi.status() == WL_CONNECTED)
  {
    // ... print IP Address
    //Serial.print("IP address: ");
    //Serial.println(WiFi.localIP());
  }
  else
  {
    //Serial.println("Can not connect to WiFi station. Go into AP mode.");

    // Go into software AP mode.
    WiFi.mode(WIFI_AP);

    delay(10);

    WiFi.softAP((const char *)APname.c_str(), ap_default_psk);

    //Serial.print("IP address: ");
    //Serial.println(WiFi.softAPIP());
  }



  ledBlink.attach(1,ledFlick);

  // Start OTA server.
  ArduinoOTA.setHostname((const char *)hostname.c_str());
  ArduinoOTA.begin();
  server.begin();
}


/**
 * @brief Arduino loop function.
 */
void loop()
{
  serverLoop();
  yield();
  // Handle OTA server.
  ArduinoOTA.handle();
  yield();
}


// prepare a web page to be send to a client (web browser)
String prepareHtmlPage(String response) {
  String htmlPage = String("HTTP/1.1 200 OK\r\n") +
            "Content-Type: text/html\r\n" +
            "Connection: close\r\n" +  // the connection will be closed after completion of the response
            "Refresh: 5\r\n" +  // refresh the page automatically every 5 sec
            "\r\n" +
            "<!DOCTYPE HTML>" +
            "<html>" +
            response +
            "</html>" +
            "\r\n";

  return htmlPage;
}

void serverLoop() {
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) { return; }
  // Wait until the client sends some data
  while(!client.available()){ delay(1); }
  // Read the first line of the request
  String req = client.readStringUntil('\r');
  client.flush();
  // Match the request
  String val;
  if (req.indexOf("/ayy/lmao") != -1) val = req;
  else {
    client.stop();
    return;
  }
  client.flush();
  // Prepare the response
  client.println(prepareHtmlPage(val));
}
