#include <ESP8266WiFi.h>

void setup(){
  pinMode(4,OUTPUT);
  digitalWrite(4,LOW);
  Serial.begin(9600);
  delay(2000);
  Serial.print("Serial test 1\r\n");
  delay(2000);
  Serial.print("Serial test 2\r\n");
  delay(2000);
  digitalWrite(4,HIGH);
}

void loop(){

}
