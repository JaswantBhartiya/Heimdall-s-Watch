How to Verify the Hardware
Build and upload the code in PlatformIO, then open the Serial Monitor (115200 baud).

Resting Test (Flat on Table): Total should read approximately 1.00G (Earth's gravity pulling along the Z-axis) with status -> [STABLE / RESTING].

Rotation / Shake Test: Flick or turn the sensor in your hand. The Gyro and Total values will rise to 1.5G – 2.0G with status -> [WRIST MOVEMENT / SHAKE].

Impact Test: Tap the table near the sensor or bump the sensor gently. Total G-force will spike above 3.0G, triggering -> [ALERT: HIGH-IMPACT COLLISION / FALL].
