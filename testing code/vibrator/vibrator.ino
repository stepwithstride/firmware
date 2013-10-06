int vibrPin = 9;
int distance = 1;

void setup()
{
  pinMode(vibrPin, OUTPUT);
  analogWrite(vibrPin, (distance * 4 + 150));
  distance+= 5;
}
void loop() 
{
  delay(1);
}
