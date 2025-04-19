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

rev_005 19-04-2025
1. OLED working with messages and RTC Time display

rev_006 19-04-2025
1. Dashboard page is workings

*/


// Integrated version of RTC + Relay Timer + OLED Clock + Message + Icon

#include <Wire.h>
#include "RTClib.h"
#include <Keypad_I2C.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ---------------- OLED Setup ----------------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ---------------- Relay & RTC ----------------
#define MOTOR1_EN_PIN 2
RTC_DS3231 rtc;

// ---------------- Keypad Setup ----------------
#define I2C_KEYPAD_ADDR 0x20
char keys[4][3] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
byte rowPins[4] = {0,1,2,3};
byte colPins[3] = {4,5,6};
Keypad_I2C keypad(makeKeymap(keys), rowPins, colPins, 4, 3, I2C_KEYPAD_ADDR);

// ---------------- Time Control ----------------
int alarmHour = -1, alarmMinute = -1;
unsigned long motorOnDurationMs = 60000UL;
bool alarmTriggered = false;
unsigned long motorOnStartTime = 0;

// ---------------- Repeat Control ----------------
unsigned long repeatIntervalMs = 120000UL;
unsigned long lastTriggerTime = 0;
int repeatCount = 3, repeatCounter = 0;
bool repeatEnabled = true;

//------------SCREEN----------------------
// ---------------- Alarm Days ----------------
bool alarmDays[7] = {true, true, true, true, true, true, true}; // S M T W T F S
const char* dayLabels[7] = {"S", "M", "T", "W", "T", "F", "S"};


// ---------------- Keypad ----------------
bool isSettingDuration = false;
String inputBuffer = "";

// ---------------- Setup ----------------
void setup() {
  pinMode(MOTOR1_EN_PIN, OUTPUT);
  makeMotor1OnOFF(false);
  Serial.begin(9600);
  Wire.begin();

  // Init OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 failed")); while (1);
  }
  display.clearDisplay(); display.display();

  if (!rtc.begin()) {
    Serial.println("RTC missing"); while (1);
  }
  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  keypad.begin();

  setAlarm(15, 36);
  setMotorOnDuration(0, 30);
}

void loop() {
  handleKeypad();
  if (!isSettingDuration) {
    DateTime now = readRTC();
    handleRepeatTrigger();
    checkAlarm(now);
    checkMotorOff();
    display.clearDisplay();
    //displayCurrentTime(now);
    //displayMessage("123456789");
    //displayIcon();
    displayCurrentTime(now);
displayDashboard(now);
    display.display();
  }
  delay(1000);
}

// ---------------- Display APIs ----------------
void displayCurrentTime(DateTime now) {
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  char buffer[9];
  sprintf(buffer, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
  display.println(buffer);
}


void displayMessage(String msg) {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 40); // Move further down to avoid overlap
  display.cp437(true);      // Optional: Ensure font encoding
  display.print(msg);       // Use print instead of println to avoid newline gap
}



void displayIcon() {
  static const unsigned char PROGMEM icon_bmp[] = {
    0b00000000, 0b01100110, 0b11111111, 0b11111111,
    0b11111111, 0b01111110, 0b00111100, 0b00011000
  };
  display.drawBitmap(100, 50, icon_bmp, 8, 8, SSD1306_WHITE);
}

void displayDashboard(DateTime now) {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 20);

  int min = motorOnDurationMs / 60000UL;
int sec = (motorOnDurationMs % 60000UL) / 1000UL;
char durStr[10];
sprintf(durStr, "%02d:%02d", min, sec);
  display.print("ALM 1 EN for ");
  display.print(durStr);

  display.setCursor(0, 30);
  display.print("DAY - ");
  for (int i = 0; i < 7; i++) {
    if (alarmDays[i]) display.print(dayLabels[i]);
    else display.print("-");
    if (i < 6) display.print("|");
  }

  display.setCursor(0, 40);
  display.print("T-");
  char timeStr[6];
  sprintf(timeStr, "%02d:%02d", alarmHour, alarmMinute);
  display.print(timeStr);
  display.print("| R-");
  display.print(repeatCount);
}


// ---------------- Motor + Alarm ----------------
void makeMotor1OnOFF(bool state) {
  digitalWrite(MOTOR1_EN_PIN, state ? HIGH : LOW);
  Serial.println(state ? "Motor ON" : "Motor OFF");
}

void setAlarm(int hour, int minute) {
  alarmHour = hour;
  alarmMinute = minute;
  alarmTriggered = false;
}

void setMotorOnDuration(int minutes, int seconds) {
  motorOnDurationMs = (unsigned long)((minutes * 60UL + seconds) * 1000UL);
  Serial.print("Set duration: ");
  Serial.print(minutes); Serial.print("m ");
  Serial.print(seconds); Serial.println("s");
}

DateTime readRTC() {
  return rtc.now();
}

/*void checkAlarm(DateTime now) {
  if (!alarmTriggered && now.hour() == alarmHour && now.minute() == alarmMinute && now.second() == 0) {
    makeMotor1OnOFF(true);
    motorOnStartTime = millis();
    alarmTriggered = true;
    resetRepeatCycle();
  }
}*/

void checkAlarm(DateTime now) {
  if (!alarmTriggered && now.hour() == alarmHour && now.minute() == alarmMinute && now.second() == 0) {
    int day = now.dayOfTheWeek(); // 0 = Sunday, 6 = Saturday
    if (alarmDays[day]) {
      makeMotor1OnOFF(true);
      motorOnStartTime = millis();
      alarmTriggered = true;
      resetRepeatCycle();
    }
  }
}


void checkMotorOff() {
  if (alarmTriggered && makeMotorIsOn() && (millis() - motorOnStartTime >= motorOnDurationMs)) {
    makeMotor1OnOFF(false);
  }
}

void handleRepeatTrigger() {
  if (!alarmTriggered || !repeatEnabled) return;
  if (repeatCounter >= repeatCount) {
    repeatEnabled = false;
    return;
  }
  if (!makeMotorIsOn() && (millis() - lastTriggerTime >= repeatIntervalMs)) {
    makeMotor1OnOFF(true);
    motorOnStartTime = millis();
    lastTriggerTime = millis();
    repeatCounter++;
  }
}

void resetRepeatCycle() {
  repeatCounter = 0;
  repeatEnabled = true;
  lastTriggerTime = millis();
}

bool makeMotorIsOn() {
  return digitalRead(MOTOR1_EN_PIN) == HIGH;
}

// ---------------- Keypad ----------------
void handleKeypad() {
  char key = keypad.getKey();
  if (!key) return;

  if (key == '*') {
    Serial.println("Enter duration (sec), then #");
    isSettingDuration = true;
    inputBuffer = "";
  } else if (key == '#' && isSettingDuration) {
    int totalSeconds = inputBuffer.toInt();
    setMotorOnDuration(totalSeconds / 60, totalSeconds % 60);
    isSettingDuration = false;
  } else if (isSettingDuration && isDigit(key)) {
    inputBuffer += key;
  }
}