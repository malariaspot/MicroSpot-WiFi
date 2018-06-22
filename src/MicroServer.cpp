#include "MicroServer.h"
#include "Mechanical.h"
#include <Ticker.h>

#define LEDPIN 5 //GPIO for the LED
#define REQUESTBUFFERSIZE 1024
#define URLBUFFERSIZE 32

#define SOFTAP_IP 0x0104A8C0 //192.168.4.1
#define SOFTAP_SUBNET 0x0104A8C0 //192.168.4.1
#define SOFTAP_MASK 0x00FFFFFF //255.255.255.0

#define FIRST_CHANNEL 7

#define RECONNECT_TIME 10000

#ifdef DEBUG_ESP_PORT
  long debugStamp;
#endif

Ticker ledBlink; // LED ticker and functions to make a Blink

void ledFlick() { digitalWrite(LEDPIN,!digitalRead(LEDPIN)); }

WiFiServer serverWifi(80);
char requestBuffer[REQUESTBUFFERSIZE];
char urlBuffer[URLBUFFERSIZE];
int bufferIndex;
int currentChannel;

String _hostname;

bool desist;

/* INIT */

MicroServer::MicroServer(Mechanical *m) {
  mechanical = m;
  mechanical->addObserver(this);
}

/* PUBLIC */

void MicroServer::setup(String hostname) {

  desist = false;
  _hostname = hostname;
  currentChannel = FIRST_CHANNEL;

  WiFi.setAutoConnect(false);
  WiFi.setAutoReconnect(false);
  WiFi.mode(WIFI_AP_STA);
  delay(10);

  WiFi.softAP((const char *)hostname.c_str(), this->ap_default_psk, currentChannel);
  WiFi.softAPConfig(SOFTAP_IP, SOFTAP_SUBNET, SOFTAP_MASK);

  serverWifi.begin();
  
  pinMode(LEDPIN,OUTPUT);
  digitalWrite(LEDPIN,LOW);
  delay(100);
  ledBlink.attach(1,ledFlick);
}

void MicroServer::run() {
  bufferIndex = 0;
  memset(requestBuffer, 0, sizeof requestBuffer);
  memset(urlBuffer, 0, sizeof urlBuffer);
  WiFiClient newClient = serverWifi.available();

  if(connectClient){
    #ifdef DEBUG_ESP_PORT
      if(millis() - debugStamp > 2000){
        Serial.println("TRYING TO CONNECT");
        debugStamp = millis();
      }
    #endif
    if(WiFi.status() == WL_CONNECTED){
      WiFi.setAutoReconnect(false);
      String res = "{\"msg\":\"Connected to " 
      + WiFi.SSID() + " \",\"ip\":\""
      + WiFi.localIP().toString() + "\",\"SSID\":\""
      + WiFi.SSID() +"\"}";
     send(200, res, &connectClient);
    }
    if(desist){
      WiFi.setAutoReconnect(false);
      send(200, "{\"msg\":\"Desisted by User\"}", &connectClient);
      WiFi.setOutputPower(0);
    }
  }

  if (newClient) {

    while(!newClient.available()) { delay(1); }

    while(newClient.available()) {
      requestBuffer[bufferIndex] =  newClient.read();
      bufferIndex++;
    }

    requestBuffer[bufferIndex] = '\0';

    //Filter requests that don't come from the app.
    int user = getCharIndex(requestBuffer, "User-Agent");
    if(user > -1) user = getCharIndex(requestBuffer, "MicroSpotApp");
    if(user == -1) {
      send(200, "Please connect from the App", &newClient);
    }else{

      //newClient.flush();

      if(getCharIndex(requestBuffer, "GET") > -1){

        int questionMarkIndex = getCharIndex(requestBuffer, "?");
        int httpIndex = getCharIndex(requestBuffer, " HTTP/");
        requestBuffer[httpIndex] = '\0'; //crop the end of the request.
        if (questionMarkIndex > -1) strncpy(urlBuffer, requestBuffer+4, questionMarkIndex-3);
        else strncpy(urlBuffer, requestBuffer+4, httpIndex-3);
        
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
            WiFi.setAutoReconnect(false);
            WiFi.disconnect();
            send(200, "{\"msg\":\"Disconnected\",\"ssid\":\"" + _hostname + "\",\"ip\":\""
              + WiFi.softAPIP().toString()+"\"}", &newClient);

        }else if(getCharIndex(urlBuffer, "/info") > -1){
          String info = "{\"msg\":\"Info\",\"AP\":{\"ip\":\""
            +WiFi.softAPIP().toString()+"\", \"SSID\":\""
            +_hostname+"\"}";

          if(WiFi.status() == WL_CONNECTED) {
            info = info + ",\"STA\":{\"ip\":\"" + WiFi.localIP().toString() + "\", \"SSID\":\"" + WiFi.SSID() + "\"}}";
          }else{
            info = info + "}";
          }

          send(200, info, &newClient);
        }else if(getCharIndex(urlBuffer, "/desist") > -1){
          desist = true;
          send(200, "{\"msg\":\"Desisted\"}", &newClient);
        }else send(404, "{\"msg\":\"Get method not found\"}", &newClient); 
      }else if(getCharIndex(requestBuffer, "POST") > -1){
        if (getCharIndex(requestBuffer, "/connect") > -1) {
          #ifdef DEBUG_ESP_PORT
            Serial.println(String(requestBuffer));
            Serial.println("END REQUEST");
          #endif

          if(connectClient) send(200, "{\"msg\":\"Desisted by connect\"}", &connectClient);


          int bodyIndex = getCharIndex(requestBuffer, "\r\n\r\n");
          int id = getCharIndex(bodyIndex, requestBuffer, "ssid");
          int pass = getCharIndex(bodyIndex, requestBuffer, "pass");

          //Make another try at rescueing the html body.
          if(id > -1){
            while(newClient.available()) {
              requestBuffer[bufferIndex] =  newClient.read();
              bufferIndex++;
            }
          }
          
          if (id > -1 && bodyIndex > -1) {

            connectClient = newClient;
            desist = false;

            ESP.eraseConfig();

            requestBuffer[id-1] = '\0';
            if(pass > -1) requestBuffer[pass-1] = '\0';

            WiFi.setOutputPower(20.5);
            WiFi.setAutoReconnect(true);
            delay(10);


            //Change the softAP to the new channel to avoid channel 
            //switching errors
            WiFi.scanNetworks();
            int nextChannel = -1;

            #ifdef DEBUG_ESP_PORT
              Serial.println(WiFi.SSID(0) + "WIFI");
            #endif

            for(int i = 0; WiFi.SSID(i) != "" && nextChannel == -1; i++)
            {
                #ifdef DEBUG_ESP_PORT
                  Serial.printf("TRIED %d\r\n",i);
                  Serial.println(WiFi.SSID(i+1));
                #endif
              if(WiFi.SSID(i) == String(requestBuffer + id + 5)){
                #ifdef DEBUG_ESP_PORT
                  Serial.println("GOT IT");
                #endif
                nextChannel = WiFi.channel(i);
              }
            }

            if(nextChannel == -1) send(404, "{\"msg\":\"Network not found\"}", &connectClient);
            else{
              
              if(nextChannel != currentChannel){ 
                #ifdef DEBUG_ESP_PORT
                  Serial.println("Changing to channel: " + String(nextChannel));
                #endif
                WiFi.softAPdisconnect(true);
                delay(100);
                currentChannel = nextChannel;
                WiFi.softAP((const char *)_hostname.c_str(), this->ap_default_psk, nextChannel);
                WiFi.softAPConfig(SOFTAP_IP, SOFTAP_SUBNET, SOFTAP_MASK);
                delay(100);

                long timeStamp = millis();
                //while(WiFi.softAPgetStationNum() < 1 && millis() - timeStamp < RECONNECT_TIME){
                while(WiFi.softAPgetStationNum() < 1){
                  delay(1000);
                }
                
                //if(WiFi.softAPgetStationNum() < 1) 
              }
              if(pass > -1){
                WiFi.begin(requestBuffer+id+5, requestBuffer+pass+5);
              }else WiFi.begin(requestBuffer+id+5);
            }
          }else{
            send(404, "{\"msg\":\"ssid missing!\",\"buffer\":\"" + String(requestBuffer) + "\"}", &newClient);
            WiFi.setOutputPower(0);
          }
        }else send(404, "{\"msg\":\"Post method not found\"}", &newClient); 
      }else{
        send(404, "{\"msg\":\"Not found\"}", &newClient); 
      }
    }
  }

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
