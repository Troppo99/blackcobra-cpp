void setup() {
  Serial.begin(9600);
  pinMode(13, OUTPUT);

}

void loop() {
  digitalWrite(13, HIGH);
  delay(100);
  digitalWrite(13, LOW);
  delay(100);
  Serial.println("Ini adalah blink");
}
