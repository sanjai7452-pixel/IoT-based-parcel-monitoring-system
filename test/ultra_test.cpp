/* Ultrasonic test sketch
   TRIG -> GPIO33
   ECHO -> GPIO32 (use a voltage divider or level shifter if sensor is 5V)
   VCC  -> 5V, GND -> GND
*/
#include <Arduino.h>

#define TRIG_PIN 33
#define ECHO_PIN 32

void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);
  delay(200);
  Serial.println("Ultrasonic test started");
}

long readCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 30000); // 30 ms timeout
  if (duration == 0) return -1;
  long cm = duration / 29 / 2;
  return cm;
}

void loop() {
  long d = readCm();
  if (d < 0) Serial.println("Distance: N/A (no echo)");
  else Serial.print("Distance: "), Serial.print(d), Serial.println(" cm");
  delay(1000);
}
