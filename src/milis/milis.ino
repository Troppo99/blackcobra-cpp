const int buttonPins[4] = {4, 27, 29, 31};
const unsigned long intervals[4] = {
  100,   // Pin 25
  300,   // Pin 27
  600,  // Pin 29
  1000   // Pin 31
};

const char* serialMessages[4] = {
  "NWR1",
  "NWR2",
  "NWR3",
  "NWR4"
};

bool lastButtonState[4] = {HIGH, HIGH, HIGH, HIGH};  // Simpan status sebelumnya

unsigned long currentInterval = 2000;
unsigned long prevMillis = 0;
bool ledState = false;

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);

  for (int i = 0; i < 4; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
  }
}

void loop() {
  // Tombol: cek perubahan dan kirim string jika baru ditekan
  for (int i = 3; i >= 0; i--) {
    int state = digitalRead(buttonPins[i]);
    // Serial.print(state);
    // Serial.print(" ");

    // Jika tombol baru saja ditekan
    if (state == LOW && lastButtonState[i] == HIGH) {
      Serial.println(serialMessages[i]);  // Kirim "NWRx"
      currentInterval = intervals[i];     // Set interval blink
    }

    lastButtonState[i] = state;  // Simpan status untuk perbandingan
  }

  Serial.println();

  // Blink LED
  unsigned long now = millis();
  if (now - prevMillis >= currentInterval) {
    prevMillis = now;
    ledState = !ledState;
    digitalWrite(LED_BUILTIN, ledState);
  }

  delay(50);
}
