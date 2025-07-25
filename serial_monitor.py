import serial
import time

PORT = "COM3"
BAUD = 9600

try:
    with serial.Serial(PORT, BAUD, timeout=1) as ser:
        print(f"📡 Serial monitor aktif di {PORT} ({BAUD} baud)")
        while True:
            if ser.in_waiting:
                line = ser.readline().decode("utf-8", errors="ignore").strip()
                if line:
                    print(line)
            time.sleep(0.1)
except serial.SerialException as e:
    print(f"❌ Gagal membuka {PORT}:\n{e}")
