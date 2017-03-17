#include <ESP8266WiFi.h>

void setup()
{
  pinMode(14,OUTPUT);
  digitalWrite(14,LOW);
}

void loop(){
  delay(1000);
  digitalWrite(14,HIGH);
  delay(1000);
  digitalWrite(14,LOW);
}
