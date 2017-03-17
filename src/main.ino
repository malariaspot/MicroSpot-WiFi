#define SECONDS 300

void setup()
{
  pinMode(14,OUTPUT);
  digitalWrite(14,LOW);
}

void loop(){
  delay(SECONDS);
  digitalWrite(14,HIGH);
  delay(SECONDS);
  digitalWrite(14,LOW);
}
