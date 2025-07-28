const int EXCLUDED_PINS[] = {3, 13, 14, 16, 17, 18, 20, 41};
const int NUM_EXCLUDED = sizeof(EXCLUDED_PINS) / sizeof(EXCLUDED_PINS[0]);

bool isExcluded(int pin) {
  for (int i = 0; i < NUM_EXCLUDED; i++) {
    if (pin == EXCLUDED_PINS[i]) {
      return true;
    }
  }
  return false;
}

void setup() {
  Serial.begin(115200);

  // Set semua pin digital dari 2 sampai 53 sebagai INPUT_PULLUP, kecuali yang dikecualikan
  for (int pin = 2; pin <= 53; pin++) {
    if (!isExcluded(pin)) {
      pinMode(pin, INPUT_PULLUP);
    }
  }

  Serial.println("Siap mendeteksi tombol pada semua pin digital (2–53), kecuali pin yang dikecualikan.");
}

void loop() {
  for (int pin = 2; pin <= 53; pin++) {
    if (isExcluded(pin)) continue;

    if (digitalRead(pin) == LOW) {
      Serial.print("Tombol ditekan di pin: ");
      Serial.println(pin);
      delay(200);  // debounce sederhana agar tidak spam
    }
  }
}
