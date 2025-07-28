import serial
import threading
import time

PORT = "COM3"
BAUD = 115200
EOL = "\r"  # ← kirim CR saja.  Bisa diganti "\r\n" jika mau.


def baca_serial(ser):
    while ser.is_open:
        try:
            if ser.in_waiting:
                line = ser.readline().decode("utf-8", errors="ignore").strip()
                if line:
                    print(f"[ARDUINO] {line}")
            time.sleep(0.05)
        except serial.SerialException:
            print("❌ Serial terputus.")
            break


def kirim_input(ser):
    while ser.is_open:
        try:
            user_input = input()  # blok menunggu stdin
            if user_input.lower() == "exit":
                print("🚪 Keluar dari serial monitor.")
                ser.close()
                break
            # ── KIRIM DENGAN <CR> ───────────────────────────────
            ser.write((user_input.strip() + EOL).encode("utf-8"))
            ser.flush()
        except Exception as e:
            print(f"❌ Error input: {e}")
            break


def tunggu_arduino_ready(ser):
    print("⏳ Menunggu Arduino siap...")
    while True:
        if ser.in_waiting:
            line = ser.readline().decode("utf-8", errors="ignore").strip()
            if line:
                print(f"[ARDUINO] {line}")
                if any(tag in line.upper() for tag in ("READY", "CALIBRATION", "ONLINE")):
                    print("✅ Arduino siap menerima perintah.")
                    break
        time.sleep(0.05)


def main():
    try:
        with serial.Serial(PORT, BAUD, timeout=1) as ser:
            time.sleep(2)  # tunggu port stabil
            print(f"📡 Serial monitor aktif di {PORT} ({BAUD} baud)")

            tunggu_arduino_ready(ser)

            # mulai thread baca
            threading.Thread(target=baca_serial, args=(ser,), daemon=True).start()

            print("✏️  Ketik perintah (G28, M114, dsb).  'exit' untuk keluar.\n")
            kirim_input(ser)

    except serial.SerialException as e:
        print(f"❌ Gagal membuka {PORT}:\n{e}")


if __name__ == "__main__":
    main()
