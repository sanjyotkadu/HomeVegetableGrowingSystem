/*
Author :sanjyot kadu
Date : 15-04-2025

rev_001 :
1. adding relay ON OFF Code
*/

#define MOTOR1_EN_PIN 2  // D2 Is 12 v relay of motor 1

void setup() {
  pinMode(MOTOR1_EN_PIN, OUTPUT);
}

void loop() {
  makeMotor1OnOFF(true);   // Turn ON motor (Enable)
  delay(5000);             // Wait 5 seconds
  makeMotor1OnOFF(false);  // Turn OFF motor (Disable)
  delay(5000);             // Wait 5 seconds
}

void makeMotor1OnOFF(bool State) {
  digitalWrite(MOTOR1_EN_PIN, State ? LOW : HIGH); // LOW = Enable, HIGH = Disable
}
