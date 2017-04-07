#include "MicroServer.h"





/**
 * Server implementation
 */

MicroServer::MicroServer() {
  this->serverWifi(80);
  this->fileManager();
}

void MicroServer::setUp(String hostname) {

  
  hostname += String(ESP.getChipId(), HEX);

  String APname = hostname;

  //Set the hostname of the server
 	WiFi.hostname(hostname);
  
  //Check of there has been a change in WiFi configuration.
  String station_ssid, station_psk;

    // Load wifi connection information.
  if (! fileManager.loadWifiConfig(&station_ssid, &station_psk))
  {
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
    WiFi.mode(WIFI_AP);

    delay(10);

    WiFi.softAP((const char *)APname.c_str(), this->ap_default_psk);
  }

  serverWifi.begin();
}

void MicroServer::run() {
	// Check if a client has connected
  client = serverWifi.available();
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

void 
MicroServer::success() 
{ 
  client.println(prepareHtmlPage("Done")); 
}

void 
MicroServer::error() 
{
  client.println(prepareHtmlPage("Error")); 
}

String MicroServer::prepareHtmlPage(String response) {
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
