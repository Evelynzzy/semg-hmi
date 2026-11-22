// Surface-EMG HMI controller: reads the analogue muscle envelope on A0 and drives
// a 10-LED bar, a servo and a buzzer in proportion to muscle activity.
#include <Servo.h>

// --- Settings ---
const int emgPin = A0;
const int buzzerPin = 12;
const int servoPin = 11;  // must be a PWM pin
const int ledPins[] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 13};

Servo myMotor;
int sensorMin = 1023;
int sensorMax = 0;
const int threshold = 1;   // trigger margin above the resting minimum (ADC counts)
const int windowSize = 8;  // moving-average length
int readings[windowSize];
int readIndex = 0;
long total = 0;

void setup() {
Serial.begin(9600);
for (int i = 0; i < 10; i++) pinMode(ledPins[i], OUTPUT);
pinMode(buzzerPin, OUTPUT);
myMotor.attach(servoPin);
myMotor.write(0);
// LED and servo self-test
for (int i = 0; i < 10; i++) digitalWrite(ledPins[i], HIGH);
delay(500);
for (int i = 0; i < 10; i++) digitalWrite(ledPins[i], LOW);
Serial.println(F(">>> Step 1/3: servo self-test..."));
myMotor.write(90); delay(600);
myMotor.write(0); delay(600);
// 5 s resting calibration sets this user's baseline range
Serial.println(F(">>> Step 2/3: 5 s calibration, please relax the muscle..."));
unsigned long startTime = millis();
while (millis() - startTime < 5000) {
int val = analogRead(emgPin);
if (val < sensorMin) sensorMin = val;
if (val > sensorMax) sensorMax = val;
int progress = map(millis() - startTime, 0, 5000, 0, 10);
for (int i = 0; i < 10; i++) {
digitalWrite(ledPins[i], i < progress ? HIGH : LOW);
}
delay(10);
}
// Fall back to a 4-20 range if the calibration range is too narrow
if (sensorMax - sensorMin < 3) {
Serial.println(F("Note: weak signal, using default range 4-20."));
sensorMin = 4;
sensorMax = 20;
}
for (int i = 0; i < 10; i++) digitalWrite(ledPins[i], LOW);
Serial.println(F(">>> Step 3/3: calibration done, monitoring started."));
}

void loop() {
// Moving-average filter
total = total - readings[readIndex];
readings[readIndex] = analogRead(emgPin);
total = total + readings[readIndex];
readIndex = (readIndex + 1) % windowSize;
int avgValue = total / windowSize;
// LED bar level (0-10)
int currentLevel = map(avgValue, sensorMin, sensorMax, 0, 10);
currentLevel = constrain(currentLevel, 0, 10);
for (int i = 0; i < 10; i++) {
digitalWrite(ledPins[i], i < currentLevel ? HIGH : LOW);
}
// Servo angle (0-180) and buzzer pitch (400-1500 Hz) follow the envelope
if (avgValue > (sensorMin + threshold)) {
int targetVal = map(avgValue, sensorMin, sensorMax, 0, 180);
targetVal = constrain(targetVal, 0, 180);
myMotor.write(targetVal);
int pitch = map(avgValue, sensorMin, sensorMax, 400, 1500);
tone(buzzerPin, pitch);
Serial.print(F("Contraction - ADC: ")); Serial.print(avgValue);
Serial.print(F(" | servo: ")); Serial.println(targetVal);
} else {
myMotor.write(0);
noTone(buzzerPin);
}
delay(20);
}
