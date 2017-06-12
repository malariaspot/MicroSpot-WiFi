#include "MicroServer.h"
#include "Mechanical.h"
#include <Ticker.h>

#define LEDPIN 14 //GPIO for the LED
#define REQUESTBUFFERSIZE 1024
#define URLBUFFERSIZE 32

//const char *ssid = "hotspotlab";
//const char *password = "spotlabwifi";

Ticker ledBlink; // LED ticker and functions to make a Blink

void ledFlick() { digitalWrite(LEDPIN,!digitalRead(LEDPIN)); }

WiFiServer serverWifi(80);
char requestBuffer[REQUESTBUFFERSIZE];
char urlBuffer[URLBUFFERSIZE];
int bufferIndex;

String _hostname;

/* INIT */

MicroServer::MicroServer(Mechanical *m) {
  mechanical = m;
  mechanical->addObserver(this);
}

/* PUBLIC */

void MicroServer::setup(String hostname) {

  _hostname = hostname;
  WiFi.mode(WIFI_AP_STA);
  WiFi.setAutoReconnect(false);
  delay(10);

  pinMode(LEDPIN,OUTPUT);
  digitalWrite(LEDPIN,LOW);
  delay(100);
  ledBlink.attach(1,ledFlick);

  WiFi.softAP((const char *)hostname.c_str(), this->ap_default_psk);

  serverWifi.begin();
  mechanical->toggle(true);
}

void MicroServer::run() {
  WiFiClient newClient = serverWifi.available();

  if (newClient) {

    while(!newClient.available()) { delay(1); }

    while(newClient.available()) {
      requestBuffer[bufferIndex] =  newClient.read();
      bufferIndex++;
    }

    //Filter requests that don't come from the app.
    int user = getCharIndex(requestBuffer, "User-Agent");
    if(user > -1) user = getCharIndex(requestBuffer, "MicroSpotApp");
    if(user == -1) {
      send(200, "Please connect from the App", &newClient);
      return;
    }

    int questionMarkIndex = getCharIndex(requestBuffer, "?");
    int httpIndex = getCharIndex(requestBuffer, " HTTP/");
    requestBuffer[httpIndex] = '\0'; //crop the end of the request.
    if (questionMarkIndex > -1) strncpy(urlBuffer, requestBuffer+4, questionMarkIndex-3);
    else strncpy(urlBuffer, requestBuffer+4, httpIndex-3);
    
    newClient.flush();

    if (getCharIndex(urlBuffer, "/pan") > -1){

      int x = arg("x=");
      int y = arg("y=");
      int f = arg("f=");
      if (x > -1 && y > -1 && f > -1) {
        if (mechanical->panAxis(requestBuffer,x,y,f)) currentClient = newClient; 
        else send(200, "{\"msg\":\"Busy\",\"status\":" + mechanical->getStatus() + "}", &newClient); 
      }else send(404, "{\"msg\":\"One or more position arguments are missing in pan!\"}", &newClient); 

    }else if (getCharIndex(urlBuffer, "/uniJog") > -1){

      int c = arg("c=");
      int f = arg("f=");
      if (c > -1 && f > -1) {
        if (mechanical->uniJog(requestBuffer,c,f)) currentClient = newClient; 
        else send(200,"{\"msg\":\"Busy\",\"status\":" + mechanical->getStatus() + "}", &newClient); 
      }else send(404, "{\"msg\":\"One or more position arguments are missing in uniJog!\"}", &newClient); 

    }else if (getCharIndex(urlBuffer,"/stop") > -1) {

      if (mechanical->stopJog()) currentClient = newClient;
      else send(200, "{\"msg\":\"Busy\",\"status\":" + mechanical->getStatus() + "}", &newClient);

    }else if (getCharIndex(urlBuffer, "/home") > -1) {

      if (mechanical->homeAxis()) currentClient = newClient; 
      else send(200, "{\"msg\":\"Busy\",\"status\":" + mechanical->getStatus() + "}", &newClient);

    }else if (getCharIndex(urlBuffer, "/move") > -1) {

      int x = arg("x=");
      int y = arg("y=");
      int f = arg("f=");
      if (x > -1 && y > -1 && f > -1) {
        if (mechanical->moveAxis(requestBuffer,x,y,f)) currentClient = newClient; 
        else send(200, "{\"msg\":\"Busy\",\"status\":" + mechanical->getStatus() + "}", &newClient);
      }else send(404, "{\"msg\":\"One or more position arguments are missing in move!\"}", &newClient);

    }else if (getCharIndex(urlBuffer,"/jog") > -1) {

      int x = arg("x=");
      int y = arg("y=");
      int f = arg("f=");
      int r = arg("r=");
      int s = arg("s=");
      if (x > -1 && y > -1 && f > -1 && r > -1 && s > -1) {
        if (mechanical->jogAxis(requestBuffer,x,y,f,r,s)) currentClient = newClient; 
        else send(200, "{\"msg\":\"Busy\",\"status\":" + mechanical->getStatus() + "}", &newClient);
      }else send(404, "{\"msg\":\"One or more position arguments are missing in jog!\"}", &newClient); 
      
    }else if (getCharIndex(urlBuffer, "/position") > -1) {

      mechanical->getPos(newClient);

    }else if (getCharIndex(urlBuffer, "/networks") > -1) {
      int n = WiFi.scanNetworks();
      if (n != 0) {
        String res = "{\"msg\":\"Networks\",\"networks\":[";
        for (int i = 0; i < n; ++i) {
          if(i == n - 1) {
            res = res + "{\"SSID\":\"" + WiFi.SSID(i) + "\",\"RSSI\":\"" + WiFi.RSSI(i);
            if (WiFi.encryptionType(i) == ENC_TYPE_NONE) res = res + "\"}";
            else res = res + "\", \"crypt\":true}";
          }else{
            res = res + "{\"SSID\":\"" + WiFi.SSID(i) + "\",\"RSSI\":\"" + WiFi.RSSI(i);
            if (WiFi.encryptionType(i) == ENC_TYPE_NONE) res = res + "\"},";
            else res = res + "\", \"crypt\":true},";
          }
        }
        res = res + "]}";
        send(200, res, &newClient);
      }else{
        send(200, "{\"msg\":\"No networks found\"}", &newClient);
      }
    }else if (getCharIndex(urlBuffer, "/connect") > -1) {
      int id = arg("ssid");
      int pass = arg("pass");
      if (id > -1 && pass > -1) {

        requestBuffer[id-1] = '\0';
        requestBuffer[pass-1] = '\0';

        WiFi.begin(requestBuffer+id+5, requestBuffer+pass+5);

        unsigned long startTime = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) { delay(500); }

        if(WiFi.status() != WL_CONNECTED) {
          send(200, "{\"msg\":\"Couldn't connect!\"}", &newClient); 
        }

        String res = "{\"msg\":\"Connected\",\"ip\":\""
          + WiFi.localIP().toString() + "\",\"SSID\":\""
          + WiFi.SSID() +"\"}";

        send(200, res, &newClient);
      }else{
        send(404, "{\"msg\":\"ssid & pass missing!\"}", &newClient);
      }
    }else if(getCharIndex(urlBuffer, "/light") > -1){

      int l = arg("l=");
      if (l > -1){
        currentClient = newClient;
        mechanical->toggleLight(requestBuffer,l);
      }else{
        send(404, "{\"msg\":\"Wrong argument in light!\"}", &newClient); 
      }
    }else if(getCharIndex(urlBuffer, "/toggle") > -1){
      
      int o = arg("o=");
      if (o > -1){
        currentClient = newClient;
        if(getCharIndex(o, requestBuffer, "1") > -1) mechanical->toggle(true);
        else mechanical->toggle(false);
      }
    }else if(getCharIndex(urlBuffer, "/unlock") > -1){
      if(mechanical->unlockAxis()) send(200, "{\"msg\":\"Axis unlocked\"}", &newClient);
      else send(200, "{\"msg\":\"Busy\",\"status\":" + mechanical->getStatus() + "}", &newClient);
    
    }else if(getCharIndex(urlBuffer, "/disconnect") > -1){

        WiFi.disconnect();
        send(200, "{\"msg\":\"Disconnected\",\"ssid\":\"" + _hostname + "\",\"ip\":\""
          + WiFi.softAPIP().toString()+"\"}", &newClient);

    }else if(getCharIndex(urlBuffer, "/info") > -1){
      String info = "{\"msg\":\"Info\",\"AP\":{\"ip\":\""
        +WiFi.softAPIP().toString()+"\", \"SSID\":\""
        +_hostname+"\"}";

      if(WiFi.SSID() != "") {
        info = info + ",\"STA\":{\"ip\":\"" + WiFi.localIP() + "\", \"SSID\":\"" + WiFi.SSID() + "\"}}";
      }else{
        info = info + "}";
      }

      send(200, info, &newClient);
    }else send(404, "{\"msg\":\"Not found\"}", &newClient); 
  }

  bufferIndex = 0;
  memset(requestBuffer, 0, sizeof requestBuffer);
  memset(urlBuffer, 0, sizeof urlBuffer);
}

void MicroServer::send(int code, String msg, WiFiClient * client) { 
  String s = "HTTP/1.1 " 
    + String(code) + " OK\r\nContent-Type: application/json\r\n\r\n"
    + msg;

  client->flush();
  client->print(s);
  delay(1);
  client->stop();
}

void MicroServer::update(String msg) { send(200, msg, &currentClient); }

/* PRIVATE */

int MicroServer::arg(const char * arg) {
  return getCharIndex(requestBuffer, arg);
}
