#include <ESP8266WiFi.h>

void setup(){
  pinMode(14,OUTPUT);
}

void loop(){
  digitalWrite(14,HIGH);
  delay(1000);
  digitalWrite(14,LOW);
  delay(1000);
}
