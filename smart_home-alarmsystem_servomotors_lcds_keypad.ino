#include <Wire.h>
#include <Servo.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

#define PCAADDR 0x70
#define GAS_SENSOR_PIN A1
#define FLAME_SENSOR_PIN A2
#define VIBRATION_SENSOR_PIN A3
#define BUZZER_PIN 12
#define PIR_SENSOR A0

Servo garageservo, doorservo;
String correctCode = "1234";
String enteredCode = "";
static int lastMotionState = LOW;

// LCD Setup
LiquidCrystal_I2C lcd0(0x3F, 16, 2);
LiquidCrystal_I2C lcd1(0x3F, 16, 2); 

char keys[4][3] = {  // Fixed 3 columns
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};
byte rowPins[4] = {2, 3, 4, 5};
byte colPins[3] = {6, 7, 8};  // Fixed size to 3
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, 4, 3);

// Timing Variables
unsigned long previousKeypadMillis = 0;
unsigned long previousMotionMillis = 0;
unsigned long previousAlarmMillis = 0;
const long intervalKeypad = 1;  // Run keypad check every 100ms
const long intervalMotion = 300;  // Run motion check every 300ms
const long intervalAlarm = 500;   // Run alarm check every 500ms

void setup() {
  Serial.begin(9600);
  Wire.begin();
  pinMode(PIR_SENSOR, INPUT);
  pinMode(GAS_SENSOR_PIN, INPUT);
  pinMode(FLAME_SENSOR_PIN, INPUT);
  pinMode(VIBRATION_SENSOR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  doorservo.attach(9);
  garageservo.attach(10);
  garageservo.write(0);
  doorservo.write(0);

  // Initialize LCDs
  pcaselect(0);
  lcd0.init();
  lcd0.backlight();
  lcd0.clear();
  lcd0.setCursor(0, 0);
  lcd0.print("Door is closed");

  pcaselect(1);
  lcd1.init();
  lcd1.backlight();
  lcd1.clear();
  lcd1.setCursor(0, 0);
  lcd1.print("Garage is closed");
}

void loop() {
  unsigned long currentMillis = millis();

  // Run keypad handling every 100ms
  if (currentMillis - previousKeypadMillis >= intervalKeypad) {
    previousKeypadMillis = currentMillis;
    handleKeypadInput();
  }

  // Run motion detection every 300ms
  if (currentMillis - previousMotionMillis >= intervalMotion) {
    previousMotionMillis = currentMillis;
    handleMotionDetection();
  }

  // Run alarm every 500ms
  if (currentMillis - previousAlarmMillis >= intervalAlarm) {
    previousAlarmMillis = currentMillis;
    alarm();
  }handleSerialInput();
}

void handleKeypadInput() {
  char key = keypad.getKey();
  if (key) {
    Serial.print("Key Pressed: ");
    Serial.println(key);

    if (key == '#') { 
      if (enteredCode == correctCode) {
        garageservo.write(180);
        delay(1500);
        garageservo.write(0);
        displayResult("Access Granted");
      } else {
        displayResult("Wrong Code!");
      }
      enteredCode = "";  // Reset input
    } else if (key == '*') {
      enteredCode = "";
      displayResult("Enter Code:");
    } else {
      enteredCode += key;
      displayResult("Code: " + enteredCode);
    }
  }
}

void handleMotionDetection() {
  int motion = digitalRead(PIR_SENSOR);
  if (motion != lastMotionState) {  
    pcaselect(0);
    lcd0.clear();  // Clear only when state changes
    lcd0.setCursor(0, 0);

    if (motion == HIGH) {
      doorservo.write(180);
      lcd0.print("Motion Detected");
    } else {
      doorservo.write(0);
      lcd0.print("No Motion");      
    }
    lastMotionState = motion;
  }
}

void alarm() {
  int gasState = digitalRead(GAS_SENSOR_PIN);
  int flameState = digitalRead(FLAME_SENSOR_PIN);
  int vibrationState = digitalRead(VIBRATION_SENSOR_PIN);

  if (gasState == HIGH || flameState == HIGH || vibrationState == HIGH) {
    digitalWrite(BUZZER_PIN, HIGH);  // Buzzer ON
  } else {
    digitalWrite(BUZZER_PIN, LOW);   // Buzzer OFF
  }
}

void displayResult(String message) {
  pcaselect(1);
  lcd1.clear();  // Ensure no flickering
  lcd1.setCursor(0, 0);
  lcd1.print(message);
}

void pcaselect(uint8_t i) {
  if (i > 7) return;
  Wire.beginTransmission(PCAADDR);
  Wire.write(1 << i);
  Wire.endTransmission();
}
void handleSerialInput() {
    if (Serial.available() > 0) {
        String command = Serial.readStringUntil('\n');  // Read full input until newline
        command.trim();  // Remove whitespace

        if (command.length() > 0) {
            int angle = command.toInt();  // Convert to integer
            if (angle >= 0 && angle <= 180) {
                garageservo.write(angle);
                Serial.print("Door angle set to: ");
                Serial.println(angle);
            } else {
                Serial.println("Invalid angle. Enter value between 0-180.");
            }
        }
    }
}