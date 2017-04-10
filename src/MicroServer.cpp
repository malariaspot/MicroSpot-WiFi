#include "MicroServer.h"

#include "Mechanical.h"
#include <Ticker.h>

WiFiServer serverWifi(80);
Mechanical mechanical(115200);


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

MicroServer::MicroServer() {

}

void MicroServer::setUp(String hostname) {

  pinMode(LEDPIN,OUTPUT);
  digitalWrite(LEDPIN,LOW);

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
    
    ledBlink.attach(1,ledFlick);
  }

  serverWifi.begin();
  
  if(!mechanical.toggle(true)){
    ledBlink.detach();
    ledBlink.attach(0.25,ledFlick);
  }
  

  
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
  String val = req.substring(4,req.indexOf("HTTP")-1);
  
  if(val.indexOf("/ayy/lmao") != -1)
  {
    val = req;
  }
  else if(val.indexOf("/get/config") != -1)
  {
    String config;
    if(!mechanical.getConfig(&config))
    {
      val = "get Config error";
    }
    else
    {
      if(config.length() == 0){
        val = "RECEIVED EMPTY CONFIG";
      }else{
        val = config;
      }
    }
  }
  else if(val.indexOf("/get/status") != -1)
  {

  }
  else
  {
    val = "404";
  }
  
  client.flush();
  // Prepare the response
  client.println(prepareHtmlPage(val));  
  client.stop();
  return;
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
