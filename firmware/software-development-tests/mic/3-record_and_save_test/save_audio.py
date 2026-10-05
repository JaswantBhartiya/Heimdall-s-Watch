import serial
import time

# UPDATE THIS TO YOUR SERIAL PORT (e.g., 'COM3' on Windows or '/dev/ttyUSB0' on Linux/Mac)
PORT = '/dev/ttyUSB0'  
BAUD = 921600
OUTPUT_FILENAME = "esp32_audio.wav"

def capture_audio():
    print(f"Connecting to {PORT} at {BAUD} baud...")
    ser = serial.Serial(PORT, BAUD, timeout=10)
    time.sleep(2)

    # Trigger recording on ESP32
    ser.write(b'r')
    print("Triggered recording! Speak into the INMP441 microphone now...")

    recording_started = False
    wav_bytes = bytearray()

    while True:
        line = ser.readline()
        try:
            decoded = line.decode('utf-8', errors='ignore').strip()
            print(f"[ESP32]: {decoded}")
            
            if "<<<START_WAV_TRANSFER>>>" in decoded:
                print("\nDownloading WAV binary data from ESP32...")
                # Total expected bytes: 44-byte header + (16000 samples * 5 sec * 2 bytes) = 160,044 bytes
                expected_bytes = 160044
                
                while len(wav_bytes) < expected_bytes:
                    chunk = ser.read(expected_bytes - len(wav_bytes))
                    if not chunk:
                        break
                    wav_bytes.extend(chunk)
                
                print(f"Successfully received {len(wav_bytes)} bytes!")
                break
        except Exception as e:
            print("Error reading serial:", e)

    ser.close()

    # Save to file
    if len(wav_bytes) > 0:
        with open(OUTPUT_FILENAME, "wb") as f:
            f.write(wav_bytes)
        print(f"\nSUCCESS: Audio saved to '{OUTPUT_FILENAME}'!")
        print("You can now open and play this file on your PC.")
    else:
        print("\nFAILED: No binary audio data received.")

if __name__ == "__main__":
    capture_audio()
