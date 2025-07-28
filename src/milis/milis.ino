const unsigned long INTERVAL = 1000;
unsigned long prevMillis     = 0;
bool           ledState      = false;

void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  Serial.println("Ini adalah milis src");
  unsigned long curMillis = millis();
  if (curMillis - prevMillis >= INTERVAL) {
    prevMillis = curMillis;
    ledState   = !ledState;
    digitalWrite(LED_BUILTIN, ledState);
  }
}
