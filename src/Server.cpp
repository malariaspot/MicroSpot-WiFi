/**
 * Project Untitled
 */

#include "Server.h"
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>

/**
 * Server implementation
 */

Server::Server() {}

void Server::setUp(String hostname) {
 	WiFi.hostname(hostname);

  // Check WiFi connection
  // ... check mode
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
    WiFi.mode(WIFI_AP);

    delay(10);

    WiFi.softAP((const char *)APname.c_str(), ap_default_psk);
  }

  serverWifi.begin();
}

void Server::run() {
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

void Server::success() { client.println(prepareHtmlPage("Done")); }
void Server::error() { client.println(prepareHtmlPage("Error")); }

String Server::prepareHtmlPage(String response) {
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