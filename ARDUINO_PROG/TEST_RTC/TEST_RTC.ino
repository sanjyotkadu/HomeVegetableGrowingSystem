#include <Wire.h>
#include "RTClib.h"

#define MOTOR1_EN_PIN 2

RTC_DS3231 rtc;

int alarmHour = -1;
int alarmMinute = -1;
bool alarmTriggered = false;

void setup() {
  pinMode(MOTOR1_EN_PIN, OUTPUT);
  Serial.begin(9600);
  initializeRTC();
  setAlarm(23, 12); // Example alarm time
}

void loop() {
  DateTime now = readRTC();
  checkAlarm(now);
  delay(1000);
}

// ------------------------ RTC Functions ------------------------

void initializeRTC() {
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }
  if (rtc.lostPower()) {
    Serial.println("RTC lost power. Setting time...");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
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

void setAlarm(int hour, int minute) {
  alarmHour = hour;
  alarmMinute = minute;
  alarmTriggered = false;
}

void checkAlarm(DateTime now) {
  if (!alarmTriggered &&
      now.hour() == alarmHour &&
      now.minute() == alarmMinute &&
      now.second() == 0) {
        Serial.println("Alarm triggered!");
    makeMotor1OnOFF(true);
    alarmTriggered = true;
  }
}

// ------------------------ Motor Function ------------------------

void makeMotor1OnOFF(bool State) {
  digitalWrite(MOTOR1_EN_PIN, State ? LOW : HIGH);
}
