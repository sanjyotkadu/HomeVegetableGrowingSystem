#include <ArduinoJson.h>

void setup() {
  Serial.begin(9600);
  Serial.println("Send JSON like: {\"pin\":5,\"state\":\"on\"}");
}

void loop() {
  static String input = "";

  // Read Serial until newline
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (input.length() > 0) {
        handleJSON(input);
        input = "";
      }
    } else {
      input += c;
    }
  }
}

void handleJSON(String jsonString) {
  StaticJsonDocument<128> doc;
  DeserializationError error = deserializeJson(doc, jsonString);
  
  if (error) {
    Serial.print("JSON Error: ");
    Serial.println(error.c_str());
    return;
  }

  int pin = doc["pin"];
  String state = doc["state"];

  pinMode(pin, OUTPUT);

  if (state == "on") {
    digitalWrite(pin, HIGH);
    Serial.println("Relay ON");
  } else if (state == "off") {
    digitalWrite(pin, LOW);
    Serial.println("Relay OFF");
  } else {
    Serial.println("Invalid state. Use \"on\" or \"off\"");
  }
}
