#include "MicroServer.h"
#include "Mechanical.h"

//WiFiServer serverWifi(80);
ESP8266WebServer serverWifi(80);  

/**
 * Server implementation
 */

MicroServer::MicroServer(Mechanical *m) {
  mechanical = m;
}

void MicroServer::setUp(String hostname) {

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
    WiFi.mode(WIFI_AP);

    delay(10);

    WiFi.softAP((const char *)hostname.c_str(), this->ap_default_psk);
  }

  serverWifi.on("/homeAxis", [this](){ handleHomeAxis();}); 
  serverWifi.on("/moveAxis", [this](){ handleMoveAxis();}); 
  serverWifi.on("/jogAxis", [this](){ handleJogAxis();}); 
  serverWifi.on("/stopJog", [this](){ handleStopJog();});

  serverWifi.begin();
  
}

void MicroServer::run() { serverWifi.handleClient(); }
void MicroServer::success() { serverWifi.send(200, "text/plain", "Done"); }
void MicroServer::error() { serverWifi.send(404, "text/plain", "Error"); }

void MicroServer::handleHomeAxis() { 
  mechanical->homeAxis(); 
  //success();
}

void MicroServer::handleStopJog() { 
  mechanical->stopJog();
  //success();
}

void MicroServer::handleMoveAxis() { 
  if (serverWifi.arg("x") != "" && 
      serverWifi.arg("y") != "" && 
      serverWifi.arg("f") != "") { 
  
    std::tuple<float, float, float> positionTuple = strongToFloat(serverWifi.arg("x"), serverWifi.arg("y"), serverWifi.arg("f"));
    mechanical->moveAxis(std::get<0>(positionTuple), std::get<1>(positionTuple), std::get<2>(positionTuple));
    //success();
  }else{ error(); }
}

void MicroServer::handleJogAxis() { 
  if (serverWifi.arg("x") != "" && 
      serverWifi.arg("y") != "" && 
      serverWifi.arg("f") != "") { 

    std::tuple<float, float, float> positionTuple = strongToFloat(serverWifi.arg("x"), serverWifi.arg("y"), serverWifi.arg("f"));
    mechanical->jogAxis(std::get<0>(positionTuple), std::get<1>(positionTuple), std::get<2>(positionTuple));
    //success();
  }else{ error(); }
}