#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>

// Initialize the LCD (adjust the address to 0x27 or 0x3F based on your module)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Button pins
const int buttonOnOffPin = 2;
const int buttonScrollPin = 3;
const int buttonSelectPin = 4;

// Sensor pins
const int soilMoisturePin = A0;
const int waterDrumLevelPin = A1;

// Output pins
const int pumpRelayPin = 8;
const int buzzerPin = 9;

// Variables
bool systemOn = false;
int selectedVegetableIndex = 0;
unsigned long startTime = 0;
unsigned long lastPesticideTime = 0;

// Structures
struct Vegetable {
  String name;
  int growthPeriod; // in days
  int pesticideFrequency; // in days
  int spare1;
  int spare2;
  int spare3;
};

Vegetable vegetables[5] = {
  {"Potato", 90, 15, 0, 0, 0},
  {"Cauliflower", 75, 12, 0, 0, 0},
  {"Spinach", 45, 10, 0, 0, 0},
  {"Tomato", 80, 14, 0, 0, 0},
  {"Carrot", 70, 13, 0, 0, 0}
};

void setup() {
  // Initialize serial communication
  Serial.begin(9600);

  // Initialize LCD
  lcd.init();
  lcd.backlight();

  // Initialize pins
  pinMode(buttonOnOffPin, INPUT_PULLUP);
  pinMode(buttonScrollPin, INPUT_PULLUP);
  pinMode(buttonSelectPin, INPUT_PULLUP);
  pinMode(pumpRelayPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(soilMoisturePin, INPUT);
  pinMode(waterDrumLevelPin, INPUT);

  // Read saved vegetable selection and start time from EEPROM
  EEPROM.get(0, selectedVegetableIndex);
  EEPROM.get(4, startTime);
  lastPesticideTime = startTime;

  // Display initial message
  lcd.clear();
  lcd.print("Press ON to Start");
}

void loop() {
  // Read button states
  bool onOffButton = !digitalRead(buttonOnOffPin);
  bool scrollButton = !digitalRead(buttonScrollPin);
  bool selectButton = !digitalRead(buttonSelectPin);

  // Handle ON/OFF button
  if (onOffButton) {
    systemOn = !systemOn;
    delay(300); // Debounce delay
    if (systemOn) {
      lcd.clear();
      lcd.print("System ON");
      delay(1000);
      displayVegetable(vegetables[selectedVegetableIndex].name);
    } else {
      lcd.clear();
      lcd.print("System OFF");
      digitalWrite(pumpRelayPin, LOW);
      digitalWrite(buzzerPin, LOW);
    }
  }

  if (systemOn) {
    // Handle Scroll button
    if (scrollButton) {
      selectedVegetableIndex = (selectedVegetableIndex + 1) % 5;
      displayVegetable(vegetables[selectedVegetableIndex].name);
      delay(300); // Debounce delay
    }

    // Handle Select button
    if (selectButton) {
      EEPROM.put(0, selectedVegetableIndex);
      startTime = millis();
      EEPROM.put(4, startTime);
      lastPesticideTime = startTime;
      lcd.clear();
      lcd.print("Selected:");
      lcd.setCursor(0, 1);
      lcd.print(vegetables[selectedVegetableIndex].name);
      delay(2000);
      lcd.clear();
      displayVegetable(vegetables[selectedVegetableIndex].name);
      delay(300); // Debounce delay
    }

    // Daily check for watering
    checkSoilMoisture();

    // Check for pesticide spraying time
    checkPesticideSchedule();

    // Check water drum level
    checkWaterDrumLevel();
  }
}

void displayVegetable(String name) {
  lcd.clear();
  lcd.print("Select Veg:");
  lcd.setCursor(0, 1);
  lcd.print(name);
}

void checkSoilMoisture() {
  int moistureValue = analogRead(soilMoisturePin);
  // Adjust threshold as per sensor calibration
  if (moistureValue < 500) {
    // Soil is dry, start pump
    digitalWrite(pumpRelayPin, HIGH);
    lcd.setCursor(0, 1);
    lcd.print("Watering Plants ");
  } else {
    // Soil is moist, stop pump
    digitalWrite(pumpRelayPin, LOW);
    lcd.setCursor(0, 1);
    lcd.print("Soil Moist OK   ");
  }
}

void checkPesticideSchedule() {
  unsigned long currentTime = millis();
  unsigned long elapsedDays = (currentTime - lastPesticideTime) / 86400000UL; // Convert ms to days

  if (elapsedDays >= vegetables[selectedVegetableIndex].pesticideFrequency) {
    // Time to spray pesticide
    lcd.clear();
    lcd.print("Spray Pesticide!");
    digitalWrite(buzzerPin, HIGH);
    delay(5000); // Alert duration
    digitalWrite(buzzerPin, LOW);
    lastPesticideTime = currentTime;
  }
}

void checkWaterDrumLevel() {
  int drumLevel = analogRead(waterDrumLevelPin);
  // Adjust threshold based on sensor calibration
  if (drumLevel < 500) {
    // Water level is low
    lcd.clear();
    lcd.print("Fill Water Drum!");
    digitalWrite(buzzerPin, HIGH);
    delay(5000); // Alert duration
    digitalWrite(buzzerPin, LOW);
    // Wait until water drum is refilled
    while (analogRead(waterDrumLevelPin) < 500) {
      // Keep checking
    }
    lcd.clear();
    displayVegetable(vegetables[selectedVegetableIndex].name);
  }
}
