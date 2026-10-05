import serial
import time
from datetime import datetime

PORT = '/dev/ttyUSB1'  # Update to your COM port (e.g., /dev/ttyUSB0 on Linux)
BAUD = 921600

def start_passive_listener():
    print(f"Connecting to {PORT} at {BAUD} baud...")
    ser = serial.Serial(PORT, BAUD, timeout=1)
    time.sleep(2)
    print("\n[LISTENER ACTIVE] Waiting for triggers (Button press or Scream/Clap)...\n")

    while True:
        try:
            line = ser.readline()
            if not line:
                continue

            decoded = line.decode('utf-8', errors='ignore').strip()
            if decoded:
                print(f"[MCU]: {decoded}")

            # Automatically catch transfer when Button or Acoustic DSP triggers on ESP32
            if "<<<START_WAV_TRANSFER>>>" in decoded:
                timestamp_str = datetime.now().strftime("%Y%m%d_%H%M%S")
                filename = f"auto_evidence_{timestamp_str}.wav"
                print(f"\n==========================================")
                print(f"[TRIGGER DETECTED] Receiving stream -> '{filename}'")
                print(f"==========================================")

                expected_bytes = 44 + (16000 * 10 * 2)  # 44-byte header + 10s audio
                wav_bytes = bytearray()

                while len(wav_bytes) < expected_bytes:
                    chunk = ser.read(expected_bytes - len(wav_bytes))
                    if not chunk:
                        break
                    wav_bytes.extend(chunk)

                if len(wav_bytes) > 0:
                    with open(filename, "wb") as f:
                        f.write(wav_bytes)
                    print(f"SUCCESS: Saved file to '{filename}' on PC!\n")
                    print("[LISTENER ACTIVE] Resumed monitoring for next trigger...\n")

        except KeyboardInterrupt:
            print("\nListener stopped by user.")
            break
        except Exception as e:
            print("Serial communication error:", e)

    ser.close()

if __name__ == "__main__":
    start_passive_listener()
