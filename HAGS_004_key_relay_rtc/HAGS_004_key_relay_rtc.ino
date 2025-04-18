/*
Author :sanjyot kadu
Date : 15-04-2025

rev_001 :
1. adding relay ON OFF Code,its working fine

rev_002 (17-04-2025):
1. RTC working fine
2. Timer added to keep the relay on for set duration is also working fine

rev_003
3. keypad added and working fine
*

rev_004 (18-04-2025):

1. Alarm triggers relay ON for configured time
✔ checkAlarm() checks for time match and triggers motor ON
✔ Sets motorOnStartTime and alarmTriggered = true

✅ 2. After repeat interval, relay turns ON again
✔ handleRepeatTrigger() checks if repeat is allowed
✔ It only runs after initial alarm (alarmTriggered == true)
✔ Uses repeatIntervalMs and lastTriggerTime to manage interval
✔ Each time it turns ON the relay, updates time + count

✅ 3. Repeat continues until max repeat count
✔ Controlled by repeatCounter < repeatCount
✔ Once count is reached, repeatEnabled is set to false

✅ 4. After max repeats, no more ON until next alarm
✔ Since repeatEnabled = false, handleRepeatTrigger() does nothing
✔ resetRepeatCycle() only called after alarm triggers again

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

//---------------------repeat--------------
unsigned long repeatIntervalMs = 2 * 60 * 1000UL; // Default: 15 minutes
unsigned long lastTriggerTime = 0;
int repeatCount = 3;                   // Total times to repeat (default)
int repeatCounter = 0;                 // Tracks how many times done
bool repeatEnabled = true;            // Allows turning off repeat mode



// ------------------------ Setup ------------------------
void setup() {
  pinMode(MOTOR1_EN_PIN, OUTPUT);
  makeMotor1OnOFF(false); // Ensure relay is OFF initially

  Serial.begin(9600);
  Wire.begin();
  initializeRTC();
  keypad.begin();

  // Example: Set alarm for 9:40 AM and default duration
  setAlarm(14, 14);
  setMotorOnDuration(0, 30); // 1 min 30 sec
}

// ------------------------ Loop ------------------------
void loop() {
  handleKeypad();

  if (!isSettingDuration) {
    DateTime now = readRTC();
    handleRepeatTrigger(); 
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



//-----------------------timer -------------------------

void handleRepeatTrigger() {
  if (!alarmTriggered || !repeatEnabled) return; // Only start after alarm

  if (repeatCounter >= repeatCount) {
    repeatEnabled = false;
    Serial.println("Repeat completed all cycles.");
    return;
  }

  if (!makeMotorIsOn() && (millis() - lastTriggerTime >= repeatIntervalMs)) {
    makeMotor1OnOFF(true);
    motorOnStartTime = millis();
    lastTriggerTime = millis();
    repeatCounter++;
    Serial.print("Repeat #"); Serial.println(repeatCounter);
  }
}



void resetRepeatCycle() {
  repeatCounter = 0;
  repeatEnabled = true;
  lastTriggerTime = millis(); // optional reset time
  Serial.println("Repeat cycle reset.");
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

    resetRepeatCycle(); // Start repeat cycle after alarm
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
