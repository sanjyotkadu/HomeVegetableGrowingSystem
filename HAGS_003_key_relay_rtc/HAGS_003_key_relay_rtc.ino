/*
Author :sanjyot kadu
Date : 15-04-2025

rev_001 :
1. adding relay ON OFF Code,its working fine

rev_002 (17-04-2025):
1. RTC working fine
2. Timer added to keep the relay on for set duration is also working fine
3. keypad added and working fine
*/

#include <Wire.h>
#include "RTClib.h"
#include <Keypad_I2C.h>

// ------------------------ Definitions ------------------------
#define MOTOR1_EN_PIN 2   // Relay pin connected to D2
#define I2C_KEYPAD_ADDR 0x20

RTC_DS3231 rtc;

// ------------------------ Keypad Setup ------------------------
char keys[4][3] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
byte rowPins[4] = {0,1,2,3};
byte colPins[3] = {4,5,6};
Keypad_I2C keypad(makeKeymap(keys), rowPins, colPins, 4, 3, I2C_KEYPAD_ADDR);

// ------------------------ Variables ------------------------
int alarmHour = -1;
int alarmMinute = -1;
unsigned long motorOnDurationMs = 1 * 60 * 1000UL; // default 1 minute
bool alarmTriggered = false;
unsigned long motorOnStartTime = 0;

bool isSettingDuration = false;
String inputBuffer = "";

// ------------------------ Setup ------------------------
void setup() {
  pinMode(MOTOR1_EN_PIN, OUTPUT);
  makeMotor1OnOFF(false); // Ensure relay is OFF initially

  Serial.begin(9600);
  Wire.begin();
  initializeRTC();
  keypad.begin();

  // Example: Set alarm for 9:40 AM and default duration
  setAlarm(10, 32);
  setMotorOnDuration(1, 30); // 1 min 30 sec
}

// ------------------------ Loop ------------------------
void loop() {
  handleKeypad();

  if (!isSettingDuration) {
    DateTime now = readRTC();
    checkAlarm(now);
    checkMotorOff();
  }

  delay(100);
}

// ------------------------ API Functions ------------------------
void setAlarm(int hour, int minute) {
  alarmHour = hour;
  alarmMinute = minute;
  alarmTriggered = false;
  Serial.print("Alarm set for: ");
  Serial.print(hour); Serial.print(":"); Serial.println(minute);
}

void setMotorOnDuration(int minutes, int seconds) {
  motorOnDurationMs = (unsigned long)((minutes * 60UL + seconds) * 1000UL);
  Serial.print("Motor ON Duration set to ");
  Serial.print(minutes); Serial.print(" min ");
  Serial.print(seconds); Serial.println(" sec");
}

void makeMotor1OnOFF(bool state) {
  digitalWrite(MOTOR1_EN_PIN, state ? HIGH : LOW);
  Serial.println(state ? "Motor ON (Relay = HIGH)" : "Motor OFF (Relay = LOW)");
}

// ------------------------ RTC ------------------------
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

// ------------------------ Motor OFF ------------------------
void checkMotorOff() {
  if (alarmTriggered && makeMotorIsOn() && (millis() - motorOnStartTime >= motorOnDurationMs)) {
    makeMotor1OnOFF(false);
    Serial.println("Motor OFF (timer complete)");
  }
}

bool makeMotorIsOn() {
  return digitalRead(MOTOR1_EN_PIN) == HIGH;
}

// ------------------------ Keypad Handler ------------------------
void handleKeypad() {
  char key = keypad.getKey();
  if (!key) return;

  if (key == '*') {
    Serial.println("Entering duration input mode... Enter seconds then press #");
    isSettingDuration = true;
    inputBuffer = "";
  } else if (key == '#' && isSettingDuration) {
    int totalSeconds = inputBuffer.toInt();
    int mins = totalSeconds / 60;
    int secs = totalSeconds % 60;
    setMotorOnDuration(mins, secs);
    Serial.println("Duration updated from keypad.");
    isSettingDuration = false;
  } else if (isSettingDuration && isDigit(key)) {
    inputBuffer += key;
    Serial.print("Input: "); Serial.println(inputBuffer);
  }
}
