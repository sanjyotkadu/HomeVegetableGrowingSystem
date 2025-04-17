/*
Author :sanjyot kadu
Date : 15-04-2025

rev_001 :
1. adding relay ON OFF Code,its working fine

rev_002 (17-04-2025):
1. RTC working fine
2. Timer added to keep the relay on for set duration is also working fine
*/

#include <Wire.h>
#include "RTClib.h"

// ------------------------ Definitions ------------------------
#define MOTOR1_EN_PIN 2   // Relay pin connected to D2

RTC_DS3231 rtc;

// ------------------------ Configurable Variables ------------------------
int alarmHour = -1;
int alarmMinute = -1;
unsigned long motorOnDurationMs = 1 * 60 * 1000UL; // default 1 minute
bool alarmTriggered = false;
unsigned long motorOnStartTime = 0;

// ------------------------ Setup ------------------------
void setup() {
  pinMode(MOTOR1_EN_PIN, OUTPUT);
  makeMotor1OnOFF(false); // Ensure relay is OFF initially

  Serial.begin(9600);
  initializeRTC();

  // Example: Set alarm for 8:08 AM and motor ON for 2 minutes 30 seconds
  setAlarm(8, 33);
  setMotorOnDuration(1, 30); // 2 minutes, 30 seconds
}

// ------------------------ Loop ------------------------
void loop() {
  DateTime now = readRTC();
  checkAlarm(now);
  checkMotorOff();
  delay(1000);
}

// ------------------------ API Functions ------------------------

// Set alarm time (24-hour format)
void setAlarm(int hour, int minute) {
  alarmHour = hour;
  alarmMinute = minute;
  alarmTriggered = false;
  Serial.print("Alarm set for: ");
  Serial.print(hour); Serial.print(":"); Serial.println(minute);
}

// Set how long motor should remain ON after alarm (min:sec)
void setMotorOnDuration(int minutes, int seconds) {
  motorOnDurationMs = (unsigned long)((minutes * 60UL + seconds) * 1000UL);
  Serial.print("Motor ON Duration set to ");
  Serial.print(minutes); Serial.print(" min ");
  Serial.print(seconds); Serial.println(" sec");
}

// Turn ON/OFF motor relay (true = ON, false = OFF)
void makeMotor1OnOFF(bool state) {
  digitalWrite(MOTOR1_EN_PIN, state ? HIGH : LOW); // HIGH = ON, LOW = OFF
  Serial.println(state ? "Motor ON (Relay = HIGH)" : "Motor OFF (Relay = LOW)");
}

// ------------------------ RTC Functions ------------------------

void initializeRTC() {
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }
  if (rtc.lostPower()) {
    Serial.println("RTC lost power. Setting time...");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // Sets to compile time
  }
}

DateTime readRTC() {
  DateTime now = rtc.now();
  Serial.print("Time: ");
  Serial.print(now.hour()); Serial.print(":");
  Serial.print(now.minute()); Serial.print(":");
  Serial.println(now.second());
  return now;
}

// ------------------------ Alarm Logic ------------------------

void checkAlarm(DateTime now) {
  if (!alarmTriggered &&
      now.hour() == alarmHour &&
      now.minute() == alarmMinute &&
      now.second() == 0) {

    Serial.println("Alarm triggered!");
    makeMotor1OnOFF(true);
    motorOnStartTime = millis();
    alarmTriggered = true;
  }
}

// ------------------------ Motor OFF Timer ------------------------

void checkMotorOff() {
  if (alarmTriggered && makeMotorIsOn() && (millis() - motorOnStartTime >= motorOnDurationMs)) {
    makeMotor1OnOFF(false);
    Serial.println("Motor OFF (timer complete)");
  }
}

bool makeMotorIsOn() {
  return digitalRead(MOTOR1_EN_PIN) == HIGH;
}
