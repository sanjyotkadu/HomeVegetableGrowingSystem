#include <Wire.h>
#include <SoftwareWire.h>
#include <RtcDS3231.h>
#include <Adafruit_SSD1306.h>
#include <Keypad.h>
#include <EEPROM.h>

// ==== DEBUG MACRO ====
#define DEBUG_ENABLED 0
#if DEBUG_ENABLED
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_PRINTLN(x) Serial.println(x)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
#endif

// ==== EEPROM Addresses ====
#define EEPROM_ADDR_HOUR         0
#define EEPROM_ADDR_MINUTE       1
#define EEPROM_ADDR_MAGIC_BYTE   5
#define EEPROM_MAGIC_VALUE       0xAB

// ==== OLED Settings ====
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ==== RTC on SoftwareWire (D8 = SDA, D9 = SCL) ====
SoftwareWire myWire(8, 9);
RtcDS3231<SoftwareWire> rtc(myWire);

// ==== Keypad Settings ====
const byte ROWS = 4;
const byte COLS = 3;
char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
byte rowPins[ROWS] = {2, 3, 4, 5};
byte colPins[COLS] = {6, 7, 10};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// ==== Output Pins ====
const int digitalOut[] = {11, 12, 13, 14};

// ==== Variables ====
unsigned long lastUpdate = 0;
const unsigned long updateInterval = 500;

bool editMode = false;
unsigned long blinkTimer = 0;
bool blinkState = true;
int editField = 0;
int editedValue = 0;
int hourTens = 0, hourOnes = 0, minuteTens = 0, minuteOnes = 0;

// ==== Alarm Structure ====
struct Alarm {
  int hour;
  int minute;
  int durationSec;
  int repeatMin;
  int repeatCount;
  int triggeredCount;
  unsigned long lastTriggerTime;
  bool active;
  bool outputOn;
  unsigned long outputStartMillis;
};
Alarm alarm = {19, 11, 30, 1, 3, 0, 0, false, false, 0};

// ==== Setup ====
void setup() {
  Serial.begin(9600);
  initOLED();
  initRTC();
  initKeypad();
  initDigitalOutputs();
  readEEPROM();
}

// ==== EEPROM Read ====
void readEEPROM(void) {
  if (EEPROM.read(EEPROM_ADDR_MAGIC_BYTE) == EEPROM_MAGIC_VALUE) {
    alarm.hour = EEPROM.read(EEPROM_ADDR_HOUR);
    alarm.minute = EEPROM.read(EEPROM_ADDR_MINUTE);
  } else {
    alarm.hour = 8;
    alarm.minute = 30;
  }

  hourTens = alarm.hour / 10;
  hourOnes = alarm.hour % 10;
  minuteTens = alarm.minute / 10;
  minuteOnes = alarm.minute % 10;
}

// ==== Loop ====
void loop() {
  handleKeypadInput();
  if (millis() - lastUpdate >= updateInterval) {
    lastUpdate = millis();
    updateOLED();
  }
  checkAlarms();
}

// ==== Init Functions ====
void initOLED() {
  Wire.begin();
  int attempts = 3;
  while (attempts > 0) {
    if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
      DEBUG_PRINTLN(F("OLED initialized successfully"));
      return;
    } else {
      DEBUG_PRINTLN(F("OLED init failed! Retrying..."));
      attempts--;
      delay(1000);
    }
  }
  DEBUG_PRINTLN(F("OLED init failed after multiple attempts!"));
  while (1);
}

void initRTC() {
  myWire.begin();
  rtc.Begin();
  if (!rtc.IsDateTimeValid()) {
    rtc.SetDateTime(RtcDateTime(__DATE__, __TIME__));
  }
}

void initKeypad() {
  // already initialized in global scope
}

void initDigitalOutputs() {
  for (int i = 0; i < 4; i++) pinMode(digitalOut[i], OUTPUT);
}

// ==== Handle Keypad ====
void handleKeypadInput() {
  char key = keypad.getKey();

  if (key == '*') {
    if (!editMode) {
      editMode = true;
      editField = 0;
      blinkTimer = millis();
      blinkState = true;
    } else {
      alarm.hour = hourTens * 10 + hourOnes;
      alarm.minute = minuteTens * 10 + minuteOnes;
      editMode = false;
      EEPROM.write(EEPROM_ADDR_HOUR, alarm.hour);
      EEPROM.write(EEPROM_ADDR_MINUTE, alarm.minute);
      EEPROM.write(EEPROM_ADDR_MAGIC_BYTE, EEPROM_MAGIC_VALUE);
    }
  }

  if (editMode && key >= '0' && key <= '9') {
    int val = key - '0';
    switch (editField) {
      case 0:
        if (val <= 2) {
          hourTens = val;
          editField++;
        }
        break;
      case 1:
        if (hourTens == 2 && val > 3) val = 3;
        hourOnes = val;
        editField++;
        break;
      case 2:
        if (val <= 5) {
          minuteTens = val;
          editField++;
        }
        break;
      case 3:
        minuteOnes = val;
        break;
    }
  }
}

// ==== OLED Display ====
void updateOLED() {
  RtcDateTime now = rtc.GetDateTime();
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print(F("Time: "));
  display.print(now.Hour());
  display.print(F(":"));
  if (now.Minute() < 10) display.print("0");
  display.print(now.Minute());
  display.print(F(":"));
  if (now.Second() < 10) display.print("0");
  display.print(now.Second());

  display.setCursor(0, 10);
  display.print(F("Alarm "));

  if (editMode) {
    if (millis() - blinkTimer >= 500) {
      blinkTimer = millis();
      blinkState = !blinkState;
    }

    if (blinkState) {
      display.print(hourTens);
      display.print(hourOnes);
      display.print(F(":"));
      display.print(minuteTens);
      display.print(minuteOnes);
    } else {
      display.print(F("  :  "));
    }
  } else {
    display.print(hourTens);
    display.print(hourOnes);
    display.print(F(":"));
    display.print(minuteTens);
    display.print(minuteOnes);
  }

  display.print(F("|D:"));
  display.print(alarm.durationSec);
  display.setCursor(0, 20);
  display.print(F("R:"));
  display.print(alarm.repeatMin);
  display.print(F(" |C:"));
  display.print(alarm.repeatCount);
  display.display();
}

// ==== Alarm Logic ====
void checkAlarms() {
  RtcDateTime now = rtc.GetDateTime();
  unsigned long currentMillis = millis();

  if (!alarm.active && now.Hour() == alarm.hour && now.Minute() == alarm.minute && now.Second() == 0) {
    alarm.active = true;
    alarm.triggeredCount = 0;
    alarm.lastTriggerTime = currentMillis - alarm.repeatMin * 60000UL;
    DEBUG_PRINT(F("Alarm Activated at "));
    DEBUG_PRINT(now.Hour()); DEBUG_PRINT(":"); DEBUG_PRINT(now.Minute()); DEBUG_PRINT(":"); DEBUG_PRINTLN(now.Second());
  }

  if (alarm.active && alarm.triggeredCount < alarm.repeatCount) {
    if ((currentMillis - alarm.lastTriggerTime) >= (alarm.repeatMin * 60000UL)) {
      digitalWrite(digitalOut[0], HIGH);
      alarm.outputOn = true;
      alarm.outputStartMillis = currentMillis;
      alarm.lastTriggerTime = currentMillis;
      alarm.triggeredCount++;
      DEBUG_PRINT(F("Alarm Triggered #"));
      DEBUG_PRINT(alarm.triggeredCount);
      DEBUG_PRINT(F(" at "));
      DEBUG_PRINT(now.Hour()); DEBUG_PRINT(":"); DEBUG_PRINT(now.Minute()); DEBUG_PRINT(":"); DEBUG_PRINTLN(now.Second());
    }
  }

  if (alarm.outputOn && (currentMillis - alarm.outputStartMillis >= alarm.durationSec * 1000UL)) {
    digitalWrite(digitalOut[0], LOW);
    alarm.outputOn = false;
    DEBUG_PRINT(F("Output OFF at "));
    DEBUG_PRINT(now.Hour()); DEBUG_PRINT(":"); DEBUG_PRINT(now.Minute()); DEBUG_PRINT(":"); DEBUG_PRINTLN(now.Second());
  }

  if (alarm.triggeredCount >= alarm.repeatCount && !alarm.outputOn && alarm.active) {
    alarm.active = false;
    DEBUG_PRINTLN(F("Alarm cycle complete."));
  }
}
