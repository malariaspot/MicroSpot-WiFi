#include <ESP8266WiFi.h>
#include "Arduino.h"

void setup()
{
  Serial.begin(9600);
  Serial.println();
  Serial.println("Initializing...");
  Serial.println();
}

void loop(){
  delay(1000);
  Serial.println("Test");
}
