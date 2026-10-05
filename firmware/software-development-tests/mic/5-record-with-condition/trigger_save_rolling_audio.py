import serial
import time
from datetime import datetime

PORT = '/dev/ttyUSB1'  # Update to your COM port (/dev/ttyUSB0 on Linux)
BAUD = 921600

def capture_rolling_audio():
    ser = serial.Serial(PORT, BAUD, timeout=10)
    time.sleep(2)

    # Generate ISO timestamp filename
    timestamp_str = datetime.now().strftime("%Y%m%d_%H%M%S")
    output_filename = f"evidence_{timestamp_str}.wav"

    print(f"Triggering buffer export on MCU... File will be saved as: {output_filename}")
    ser.write(b's')

    wav_bytes = bytearray()
    expected_bytes = 44 + (16000 * 10 * 2) # Header + 10s audio @ 16kHz 16-bit

    while True:
        line = ser.readline()
        try:
            decoded = line.decode('utf-8', errors='ignore').strip()
            print(f"[MCU]: {decoded}")

            if "<<<START_WAV_TRANSFER>>>" in decoded:
                print("Receiving rolling WAV buffer...")
                while len(wav_bytes) < expected_bytes:
                    chunk = ser.read(expected_bytes - len(wav_bytes))
                    if not chunk:
                        break
                    wav_bytes.extend(chunk)
                break
        except Exception as e:
            print("Serial error:", e)

    ser.close()

    if len(wav_bytes) > 0:
        with open(output_filename, "wb") as f:
            f.write(wav_bytes)
        print(f"\nSUCCESS: 10-second pre-trigger audio saved to '{output_filename}'!")
    else:
        print("\nFAILED: No data received.")

if __name__ == "__main__":
    capture_rolling_audio()
